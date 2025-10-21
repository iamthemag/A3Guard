#include "MonitoringEngine.h"
#include <QApplication>
#include <QClipboard>
#include <QProcess>
#include <QRegExp>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

MonitoringEngine::MonitoringEngine(std::shared_ptr<ConfigManager> configManager, QObject* parent)
    : QObject(parent)
    , m_monitoring(false)
    , m_appTimer(std::make_unique<QTimer>(this))
    , m_windowTimer(std::make_unique<QTimer>(this))
    , m_clipboardTimer(std::make_unique<QTimer>(this))
    , m_usbTimer(std::make_unique<QTimer>(this))
    , m_keystrokeTimer(std::make_unique<QTimer>(this))
    , m_keystrokeCount(0)
    , m_configManager(configManager)
{
    connect(m_appTimer.get(), &QTimer::timeout, this, &MonitoringEngine::checkApplications);
    connect(m_windowTimer.get(), &QTimer::timeout, this, &MonitoringEngine::checkWindows);
    connect(m_clipboardTimer.get(), &QTimer::timeout, this, &MonitoringEngine::checkClipboard);
    connect(m_usbTimer.get(), &QTimer::timeout, this, &MonitoringEngine::checkUSBDevices);
    connect(m_keystrokeTimer.get(), &QTimer::timeout, this, &MonitoringEngine::checkKeystrokes);
}

MonitoringEngine::~MonitoringEngine()
{
    stopMonitoring();
}

void MonitoringEngine::startMonitoring()
{
    if (m_monitoring) return;
    
    m_monitoring = true;
    m_appTimer->start(DEFAULT_APP_MONITOR_INTERVAL);
    m_windowTimer->start(1000);
    m_clipboardTimer->start(DEFAULT_CLIPBOARD_INTERVAL);
    m_usbTimer->start(5000);
    m_keystrokeTimer->start(2000); // Check keystrokes every 2 seconds
    
    LOG_INFO("Monitoring started");
}

void MonitoringEngine::stopMonitoring()
{
    if (!m_monitoring) return;
    
    m_monitoring = false;
    m_appTimer->stop();
    m_windowTimer->stop();
    m_clipboardTimer->stop();
    m_usbTimer->stop();
    m_keystrokeTimer->stop();
    
    LOG_INFO("Monitoring stopped");
}

bool MonitoringEngine::isMonitoring() const
{
    return m_monitoring;
}

void MonitoringEngine::checkApplications()
{
    // Get list of running processes (basic implementation)
    QProcess process;
    process.start("ps", QStringList() << "-eo" << "comm" << "--no-headers");
    process.waitForFinished(1000);
    
    QStringList runningApps;
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Check for suspicious applications
    QStringList suspiciousApps = {"firefox", "chrome", "chromium", "opera", "safari", 
                                 "telegram", "discord", "skype", "teams", "zoom"};
    
    for (const QString& line : lines) {
        QString appName = line.trimmed();
        
        // Skip if application is whitelisted
        if (isApplicationWhitelisted(appName)) {
            continue;
        }
        
        if (suspiciousApps.contains(appName, Qt::CaseInsensitive)) {
            emit suspiciousActivityDetected(QString("Suspicious application detected: %1").arg(appName));
            LOG_WARNING("Suspicious application running:" << appName);
        }
    }
}

void MonitoringEngine::checkWindows()
{
    // Get current active window title using X11
    QProcess process;
    process.start("xdotool", QStringList() << "getactivewindow" << "getwindowname");
    process.waitForFinished(500);
    
    QString currentWindow = process.readAllStandardOutput().trimmed();
    
    if (!currentWindow.isEmpty() && currentWindow != m_lastWindow) {
        m_lastWindow = currentWindow;
        emit windowChanged(currentWindow);
        LOG_DEBUG("Window changed to:" << currentWindow);
        
        // Skip if window is whitelisted (including localhost)
        if (isWindowWhitelisted(currentWindow)) {
            return;
        }
        
        // Check for suspicious window titles
        QStringList suspiciousKeywords = {"exam", "test", "quiz", "answer", "cheat", "solution"};
        for (const QString& keyword : suspiciousKeywords) {
            if (currentWindow.contains(keyword, Qt::CaseInsensitive)) {
                emit suspiciousActivityDetected(QString("Suspicious window title: %1").arg(currentWindow));
                break;
            }
        }
    }
}

void MonitoringEngine::checkClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard) return;
    
    QString currentClipboard = clipboard->text();
    
    // Only process if clipboard content actually changed
    if (!currentClipboard.isEmpty() && currentClipboard != m_lastClipboard) {
        m_lastClipboard = currentClipboard;
        emit clipboardChanged();
        
        LOG_DEBUG("Clipboard changed - length:" << currentClipboard.length());
        
        // Check for suspicious clipboard content
        if (currentClipboard.length() > 100) {
            emit suspiciousActivityDetected("Large clipboard content detected");
        }
        
        // Check for URLs (but allow whitelisted ones including localhost)
        if (currentClipboard.contains(QRegExp("https?://[^\\s]+"))) {
            if (!isUrlWhitelisted(currentClipboard)) {
                emit suspiciousActivityDetected("URL copied to clipboard");
            }
        }
    }
}

void MonitoringEngine::checkUSBDevices()
{
    // Check for removable USB storage devices using lsblk with detailed info
    QProcess process;
    process.start("lsblk", QStringList() << "-o" << "NAME,TRAN,TYPE,HOTPLUG,SIZE,VENDOR" << "-n");
    process.waitForFinished(1000);
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    static QStringList lastRemovableDevices;
    QStringList currentRemovableDevices;
    
    // Filter for removable USB storage devices only
    for (const QString& line : lines) {
        QStringList parts = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 4) {
            QString name = parts[0];
            QString transport = parts[1];
            QString type = parts[2];
            QString hotplug = parts[3];
            
            // Only check for removable USB storage devices (pendrives, external drives)
            if (transport == "usb" && type == "disk" && hotplug == "1") {
                currentRemovableDevices << name;
            }
        }
    }
    
    // Check for newly connected removable devices
    for (const QString& device : currentRemovableDevices) {
        if (!lastRemovableDevices.contains(device)) {
            // Get comprehensive device details for display
            QProcess detailProcess;
            detailProcess.start("lsblk", QStringList() << "-o" << "NAME,SIZE,VENDOR,MODEL" << "-n" << device);
            detailProcess.waitForFinished(1000);
            QString lsblkOutput = detailProcess.readAllStandardOutput().trimmed();
            
            // Also get device info from udevadm for more details
            QProcess udevProcess;
            udevProcess.start("udevadm", QStringList() << "info" << ("/dev/" + device));
            udevProcess.waitForFinished(1000);
            QString udevOutput = udevProcess.readAllStandardOutput();
            
            // Extract model and serial from udev output
            QString modelName = "Unknown";
            QString serial = "Unknown";
            QStringList udevLines = udevOutput.split('\n');
            for (const QString& line : udevLines) {
                if (line.contains("ID_MODEL=")) {
                    modelName = line.mid(line.indexOf("=") + 1).trimmed();
                } else if (line.contains("ID_SERIAL=")) {
                    serial = line.mid(line.indexOf("=") + 1).trimmed();
                }
            }
            
            // Format device details clearly
            QString deviceDetails = QString(
                "Device: /dev/%1\n"
                "Details: %2\n"
                "Model: %3\n"
                "Serial: %4"
            ).arg(device, lsblkOutput, modelName, serial);
            
            // New removable USB device detected
            emit usbDeviceDetected(deviceDetails);
            emit suspiciousActivityDetected(QString("Removable USB storage device connected: %1 (%2)").arg(device, modelName));
            LOG_WARNING("USB storage device connected:" << device << "Model:" << modelName << "Serial:" << serial);
            
            // Auto-unmount the USB device for security
            unmountUSBDevice(device);
        }
    }
    
    // Check for removed devices
    for (const QString& device : lastRemovableDevices) {
        if (!currentRemovableDevices.contains(device)) {
            LOG_INFO("USB storage device removed:" << device);
            emit suspiciousActivityDetected(QString("USB storage device removed: %1").arg(device));
        }
    }
    
    lastRemovableDevices = currentRemovableDevices;
}

void MonitoringEngine::unmountUSBDevice(const QString& device)
{
    // First, get all mounted partitions for this device
    QProcess findMountProcess;
    findMountProcess.start("lsblk", QStringList() << "-o" << "NAME,MOUNTPOINT" << "-n");
    findMountProcess.waitForFinished(2000);
    
    QString output = findMountProcess.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QStringList mountedPartitions;
    bool foundDevice = false;
    
    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith(device)) {
            foundDevice = true;
            QStringList parts = trimmedLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2 && !parts[1].isEmpty() && parts[1] != "-") {
                // This partition is mounted
                mountedPartitions << parts[1]; // Mount point
                LOG_INFO("Found mounted partition:" << parts[0] << "at" << parts[1]);
            }
        } else if (foundDevice && trimmedLine.startsWith(" ")) {
            // This is a partition of the device we're looking for
            QStringList parts = trimmedLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2 && !parts[1].isEmpty() && parts[1] != "-") {
                mountedPartitions << parts[1]; // Mount point
                LOG_INFO("Found mounted partition:" << parts[0] << "at" << parts[1]);
            }
        } else if (foundDevice) {
            // We've moved past this device
            break;
        }
    }
    
    // Unmount all found partitions
    if (!mountedPartitions.isEmpty()) {
        for (const QString& mountPoint : mountedPartitions) {
            // First try normal unmount
            QProcess unmountProcess;
            unmountProcess.start("umount", QStringList() << mountPoint);
            unmountProcess.waitForFinished(5000);
            
            if (unmountProcess.exitCode() == 0) {
                LOG_INFO("Successfully unmounted USB partition at:" << mountPoint);
                emit suspiciousActivityDetected(QString("Auto-unmounted USB partition: %1").arg(mountPoint));
            } else {
                LOG_WARNING("Failed to unmount USB partition at:" << mountPoint << "- trying lazy unmount");
                
                // Try lazy unmount as fallback
                QProcess lazyUnmountProcess;
                lazyUnmountProcess.start("umount", QStringList() << "-l" << mountPoint);
                lazyUnmountProcess.waitForFinished(5000);
                
                if (lazyUnmountProcess.exitCode() == 0) {
                    LOG_INFO("Lazy unmounted USB partition at:" << mountPoint);
                    emit suspiciousActivityDetected(QString("Lazy unmounted USB partition: %1").arg(mountPoint));
                } else {
                    LOG_ERROR("Failed to unmount USB partition at:" << mountPoint);
                    emit suspiciousActivityDetected(QString("CRITICAL: Unable to unmount USB partition: %1").arg(mountPoint));
                }
            }
        }
    } else {
        LOG_INFO("USB device" << device << "is not mounted, no action needed");
    }
}

bool MonitoringEngine::isApplicationWhitelisted(const QString& appName) const
{
    if (!m_configManager) {
        return false;
    }
    
    // Check general application whitelist
    QStringList whitelistedApps = m_configManager->getWhitelistedApplications();
    for (const QString& whitelistedApp : whitelistedApps) {
        if (appName.contains(whitelistedApp, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    // Check localhost-specific whitelist
    QStringList localhostApps = m_configManager->getWhitelistedLocalhostApps();
    for (const QString& localhostApp : localhostApps) {
        if (appName.contains(localhostApp, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

void MonitoringEngine::checkKeystrokes()
{
    // Check /proc/interrupts for keyboard activity
    static qint64 lastInputEvents = 0;
    static qint64 lastCheckTime = 0;
    
    QFile interruptsFile("/proc/interrupts");
    if (!interruptsFile.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QString content = interruptsFile.readAll();
    interruptsFile.close();
    
    // Look for keyboard-specific interrupt (i8042 is the standard PS/2 controller)
    QStringList lines = content.split('\n');
    qint64 currentInputEvents = 0;
    
    for (const QString& line : lines) {
        // Only look for i8042 (PS/2 keyboard) - most reliable indicator
        if (line.contains("i8042", Qt::CaseInsensitive)) {
            QStringList parts = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                bool ok;
                qint64 interrupts = parts[1].toLongLong(&ok);
                if (ok) {
                    currentInputEvents = interrupts; // Get the count for i8042
                    break;
                }
            }
        }
    }
    
    // Only report actual keystroke activity (significant increase from last check)
    if (lastInputEvents > 0 && currentInputEvents > lastInputEvents) {
        qint64 newEvents = currentInputEvents - lastInputEvents;
        
        // Only report if there's significant activity (more than 5 events = real keystroke)
        // and longer than 2 seconds has passed to avoid repeat triggers
        if (newEvents > 5) {
            m_keystrokeCount += static_cast<int>(newEvents);
            
            QString keyInfo = QString("Keyboard activity: %1 events (Session total: %2)")
                             .arg(newEvents)
                             .arg(m_keystrokeCount);
            
            emit keystrokeDetected(keyInfo);
            LOG_INFO("Keyboard activity detected:" << keyInfo);
            
            // Alert for very high activity (more than 100 keystrokes in 2 seconds is suspicious)
            if (newEvents > 100) {
                emit suspiciousActivityDetected(QString("Suspicious keyboard activity: %1 events in 2 seconds").arg(newEvents));
            }
            
            lastCheckTime = QDateTime::currentMSecsSinceEpoch();
        }
    }
    
    lastInputEvents = currentInputEvents;
}

bool MonitoringEngine::isWindowWhitelisted(const QString& windowTitle) const
{
    if (!m_configManager) {
        return false;
    }
    
    // Always allow localhost-related windows if localhost is allowed
    if (m_configManager->getAllowLocalhost()) {
        if (windowTitle.contains("localhost", Qt::CaseInsensitive) ||
            windowTitle.contains("127.0.0.1") ||
            windowTitle.contains("::1")) {
            return true;
        }
    }
    
    // Check window title whitelist
    QStringList whitelistedWindows = m_configManager->getWhitelistedWindows();
    for (const QString& whitelistedWindow : whitelistedWindows) {
        if (windowTitle.contains(whitelistedWindow, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

bool MonitoringEngine::isUrlWhitelisted(const QString& url) const
{
    if (!m_configManager) {
        return false;
    }
    
    // Always allow localhost URLs if localhost is allowed
    if (m_configManager->getAllowLocalhost()) {
        if (url.contains("localhost", Qt::CaseInsensitive) ||
            url.contains("127.0.0.1") ||
            url.contains("::1") ||
            url.contains("http://localhost", Qt::CaseInsensitive) ||
            url.contains("https://localhost", Qt::CaseInsensitive) ||
            url.contains("http://127.0.0.1") ||
            url.contains("https://127.0.0.1")) {
            return true;
        }
    }
    
    // Check URL whitelist
    QStringList whitelistedUrls = m_configManager->getWhitelistedUrls();
    for (const QString& whitelistedUrl : whitelistedUrls) {
        if (url.contains(whitelistedUrl, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}
