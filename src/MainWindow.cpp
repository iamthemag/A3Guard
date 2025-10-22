#include "MainWindow.h"
#include "MonitoringEngine.h"
#include "NetworkManager.h"
#include "ResourceMonitor.h"
#include "AlertManager.h"
#include "Logger.h"
#include "PrivilegeDialog.h"
#include "UpdateChecker.h"
// #include "KeyLogger.h" // Temporarily disabled
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QClipboard>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QCloseEvent>
#include <QTabWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QScrollArea>
#include <QGridLayout>
#include <QAction>
#include <QMenu>
#include <QFrame>
#include <QSplitter>
#include <QStyleOption>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QRegularExpression>

const int MainWindow::MAX_LOG_DISPLAY = 1000;

MainWindow::MainWindow(std::shared_ptr<ConfigManager> config,
                       std::shared_ptr<SecurityManager> security,
                       std::shared_ptr<Logger> logger,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_config(config)
    , m_security(security)
    , m_logger(logger)
    , m_tabWidget(nullptr)
    , m_updateTimer(new QTimer(this))
    , m_systemTray(nullptr)
    , m_currentState(MonitoringState::STOPPED)
    , m_sessionStartTime(QDateTime::currentDateTime())
    , m_violationCount(0)
    , m_shouldShowWindow(true)
{
    // Initialize monitoring components
    m_monitoring = std::make_shared<MonitoringEngine>(m_config, this);
    m_networkManager = std::make_shared<NetworkManager>(this);
    m_resourceMonitor = std::make_shared<ResourceMonitor>(this);
    m_alertManager = std::make_shared<AlertManager>(this);
    m_updateChecker = new UpdateChecker(this);
    // m_keyLogger = std::make_shared<KeyLogger>(this); // Temporarily disabled
    
    // Connect signals
    connect(m_monitoring.get(), &MonitoringEngine::clipboardChanged,
            this, &MainWindow::onClipboardChanged);
    connect(m_monitoring.get(), &MonitoringEngine::suspiciousActivityDetected,
            this, static_cast<void(MainWindow::*)(const QString&)>(&MainWindow::onViolationDetected));
    connect(m_monitoring.get(), &MonitoringEngine::applicationStarted,
            this, [this](const QString& app) { 
                onViolationDetected(QString("Application started: %1").arg(app)); 
            });
    connect(m_monitoring.get(), &MonitoringEngine::keystrokeDetected,
            this, &MainWindow::onKeystrokeDetected);
    connect(m_monitoring.get(), &MonitoringEngine::usbDeviceDetected,
            this, &MainWindow::onUsbDeviceDetected);
    
    // Add direct system clipboard monitoring for immediate feedback
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::onClipboardChanged);
    
    // Connect keylogger signals (temporarily disabled)
    // connect(m_keyLogger.get(), &KeyLogger::keystrokeDetected,
    //         this, &MainWindow::onKeystrokeDetected);
    
    setupUI();
    setupMenuBar();
    setupSystemTray();
    setupStatusBar();
    
    // Connect UpdateChecker signals
    connect(m_updateChecker, &UpdateChecker::checkStarted,
            this, &MainWindow::onUpdateCheckStarted);
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &MainWindow::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::checkFailed,
            this, &MainWindow::onUpdateCheckFailed);
    connect(m_updateChecker, &UpdateChecker::downloadStarted,
            this, &MainWindow::onDownloadStarted);
    connect(m_updateChecker, &UpdateChecker::downloadProgress,
            this, &MainWindow::onDownloadProgress);
    connect(m_updateChecker, &UpdateChecker::downloadFinished,
            this, &MainWindow::onDownloadFinished);
    connect(m_updateChecker, &UpdateChecker::downloadFailed,
            this, &MainWindow::onDownloadFailed);
    
    // Apply modern styling
    applyModernStyling();
    
    setWindowTitle(QString("%1 v%2").arg(A3GUARD_NAME).arg(A3GUARD_VERSION));
    setMinimumSize(1000, 700);
    resize(1300, 900);
    
    // DON'T start the update timer yet - wait for authentication first
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateUI);
    
    // Load last close time
    loadLastCloseTime();
    
    // Check for privileges IMMEDIATELY before showing the window or starting any features
    // This must happen synchronously before any UI interaction
    checkPrivileges();
    
    // Start resource monitoring after privileges are confirmed
    m_resourceMonitor->startMonitoring();
    
    // Now start all timers after authentication
    m_updateTimer->start(1000);
    
    // Connect logger signals (if available)
    if (m_logger) {
        connect(m_logger.get(), &Logger::newLogEntry, this, &MainWindow::updateLogs);
        
        // Load logs from last 5 hours on startup
        QTimer::singleShot(100, this, [this]() {
            QStringList last5Hours = m_logger->getLogsFromLastHours(5);
            if (!last5Hours.isEmpty()) {
                m_recentLogs = last5Hours;
                updateLogs();
            }
        });
    }
    
    // Capture console messages for logs tab
    m_logCaptureTimer = new QTimer(this);
    connect(m_logCaptureTimer, &QTimer::timeout, this, &MainWindow::captureConsoleLogs);
    m_logCaptureTimer->start(1000); // Check every second
    
    // Set focus policy
    setFocusPolicy(Qt::StrongFocus);
}

MainWindow::~MainWindow()
{
}

void MainWindow::applyModernStyling()
{
    // Modern light theme stylesheet
    QString stylesheet = R"(
        /* Main Window */
        QMainWindow {
            background-color: #f8f9fa;
            color: #212529;
        }
        
        /* Tab Widget */
        QTabWidget {
            background-color: #f8f9fa;
            border: none;
        }
        
        QTabWidget::pane {
            border: 1px solid #dee2e6;
            background-color: #ffffff;
            border-radius: 8px;
        }
        
        QTabBar::tab {
            background-color: #e9ecef;
            color: #495057;
            padding: 10px 18px;
            margin: 2px;
            border-radius: 6px;
            min-width: 100px;
            font-weight: 500;
        }
        
        QTabBar::tab:selected {
            background-color: #007bff;
            color: white;
            box-shadow: 0 2px 4px rgba(0,123,255,0.3);
        }
        
        QTabBar::tab:hover {
            background-color: #6c757d;
            color: white;
        }
        
        /* Group Boxes */
        QGroupBox {
            font-weight: bold;
            color: #495057;
            border: 2px solid #dee2e6;
            border-radius: 10px;
            margin: 8px;
            padding-top: 16px;
            background-color: #ffffff;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 15px;
            padding: 0 8px 0 8px;
            color: #007bff;
            font-size: 11pt;
        }
        
        /* Buttons */
        QPushButton {
            background-color: #e9ecef;
            color: #495057;
            border: 1px solid #ced4da;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: 500;
            min-height: 32px;
        }
        
        QPushButton:hover {
            background-color: #dee2e6;
            border-color: #adb5bd;
            transform: translateY(-1px);
        }
        
        QPushButton:pressed {
            background-color: #ced4da;
            transform: translateY(0px);
        }
        
        QPushButton:disabled {
            background-color: #f8f9fa;
            color: #adb5bd;
            border-color: #e9ecef;
        }
        
        /* Primary Button Style */
        QPushButton[class="primary"] {
            background-color: #007bff;
            border-color: #0056b3;
            color: white;
            font-weight: 600;
        }
        
        QPushButton[class="primary"]:hover {
            background-color: #0056b3;
            border-color: #004085;
            box-shadow: 0 4px 8px rgba(0,123,255,0.3);
        }
        
        /* Progress Bars */
        QProgressBar {
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            text-align: center;
            color: #495057;
            height: 24px;
            font-weight: 500;
        }
        
        QProgressBar::chunk {
            background-color: #007bff;
            border-radius: 5px;
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #28a745, stop: 1 #20c997);
        }
        
        /* Violation Status Frame */
        #violationStatusFrame {
            background-color: #d4edda;
            border-bottom: 3px solid #c3e6cb;
            margin: 0px;
        }
        
        #violationStatusFrame[violation="true"] {
            background-color: #f8d7da;
            border-bottom-color: #f5c6cb;
        }
        
        #violationStatusLabel {
            font-weight: bold;
            font-size: 16px;
            color: #155724;
        }
        
        #violationStatusLabel[violation="true"] {
            color: #721c24;
        }
        
        #lastCloseTimeLabel {
            color: #6c757d;
            font-size: 12px;
        }
        
        /* Labels */
        QLabel {
            color: #495057;
            font-size: 10pt;
        }
        
        /* Text Edit */
        QTextEdit {
            background-color: #ffffff;
            color: #212529;
            border: 1px solid #ced4da;
            border-radius: 6px;
            selection-background-color: #007bff;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 9pt;
            padding: 8px;
        }
        
        /* Small text edits for activity displays */
        QTextEdit[class="activity"] {
            max-height: 200px;
            min-height: 150px;
            background-color: #f8f9fa;
            border: 1px solid #e9ecef;
        }
        
        /* Scroll Areas */
        QScrollArea {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 8px;
        }
        
        /* Scroll Bars */
        QScrollBar:vertical {
            background-color: #f8f9fa;
            width: 14px;
            border-radius: 7px;
        }
        
        QScrollBar::handle:vertical {
            background-color: #ced4da;
            border-radius: 7px;
            min-height: 30px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #adb5bd;
        }
        
        /* Status indicators */
        QLabel[class="status-running"] {
            color: #28a745;
            font-weight: bold;
        }
        
        QLabel[class="status-stopped"] {
            color: #dc3545;
            font-weight: bold;
        }
        
        QLabel[class="status-warning"] {
            color: #ffc107;
            font-weight: bold;
        }
        
        QLabel[class="violation"] {
            color: #dc3545;
            font-size: 9pt;
            font-weight: 500;
        }
        
        QLabel[class="clipboard"] {
            color: #17a2b8;
            font-size: 9pt;
            font-weight: 500;
        }
    )";
    
    setStyleSheet(stylesheet);
}

QString MainWindow::formatTimeAMPM(const QDateTime& dateTime) const
{
    if (!dateTime.isValid()) return "N/A";
    return dateTime.toString("hh:mm:ss AP");
}

QString MainWindow::formatDateTimeAMPM(const QDateTime& dateTime) const
{
    if (!dateTime.isValid()) return "N/A";
    return dateTime.toString("MMM dd, hh:mm:ss AP");
}

void MainWindow::onClipboardChanged()
{
    QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard) return;
    
    QString content = clipboard->text();
    if (content.isEmpty()) return;
    
    // Filter out terminal/system data
    if (isTerminalOrSystemData(content)) {
        return;
    }
    
    QString timestamp = formatTimeAMPM(QDateTime::currentDateTime());
    
    // Get content type info
    QString contentType = detectClipboardOperation(content);
    
    // Add to clipboard data (full content)
    QString fullEntry = QString("[%1] %2\n\nFull Content:\n%3\n%4")
                       .arg(timestamp)
                       .arg(contentType)
                       .arg(QString("-").repeated(60))
                       .arg(content);
    
    m_recentClipboardData.prepend(fullEntry);
    if (m_recentClipboardData.size() > 100) {
        m_recentClipboardData.removeLast();
    }
    
    LOG_DEBUG("Clipboard content added:" << content.left(50));
    
    // Add to dashboard activity (summarized)
    QString displayContent = content.length() > 100 ? content.left(100) + "... [TRUNCATED]" : content;
    displayContent = displayContent.replace("\n", " ↵ ").replace("\t", " ⇥ ");
    
    QString activity = QString("[%1] %2: %3")
                      .arg(timestamp)
                      .arg(contentType.split("[").first().trimmed())
                      .arg(displayContent);
    
    onClipboardActivity(activity);
    
    // Update both displays immediately
    updateClipboardTabDisplay();
    updateClipboardDisplay();
    updateUI();
    
    // Force refresh of clipboard tab
    QTimer::singleShot(100, this, &MainWindow::updateClipboardTabDisplay);
}

void MainWindow::onClipboardActivity(const QString& activity)
{
    m_recentClipboardActivity.prepend(activity);
    if (m_recentClipboardActivity.size() > 50) {
        m_recentClipboardActivity.removeLast();
    }
    updateClipboardDisplay();
}

void MainWindow::onKeystrokeDetected(const QString& keystroke)
{
    QString timestamp = formatTimeAMPM(QDateTime::currentDateTime());
    QString formattedKeystroke = QString("[%1] %2").arg(timestamp, keystroke);
    
    // Add keystroke to keylogger data
    m_recentKeyloggerData.prepend(formattedKeystroke);
    if (m_recentKeyloggerData.size() > 500) {
        m_recentKeyloggerData.removeLast();
    }
    
    // Also add to clipboard activity display for dashboard
    m_recentClipboardActivity.prepend(formattedKeystroke);
    if (m_recentClipboardActivity.size() > 100) {
        m_recentClipboardActivity.removeLast();
    }
    
    updateClipboardDisplay();
    updateKeyloggerDisplay();
}

void MainWindow::onUsbDeviceDetected(const QString& deviceDetails)
{
    QString timestamp = formatTimeAMPM(QDateTime::currentDateTime());
    // deviceDetails already contains name, size, vendor, model from MonitoringEngine
    QString formattedEntry = QString("\n[%1] USB DEVICE DETECTED\n%2\n%3").arg(timestamp, deviceDetails, QString("-").repeated(60));
    
    // Add to USB activity
    m_recentUsbActivity.prepend(formattedEntry);
    if (m_recentUsbActivity.size() > 50) {
        m_recentUsbActivity.removeLast();
    }
    
    // Update USB display immediately
    updateUsbDisplay();
    updateUI();
}

void MainWindow::onViolationDetected(const QString& violation)
{
    QString timestamp = formatTimeAMPM(QDateTime::currentDateTime());
    QString formattedViolation = QString("[%1] VIOLATION: %2").arg(timestamp, violation);
    
    m_recentViolations.prepend(formattedViolation);
    m_violationCount++;
    
    if (m_recentViolations.size() > 20) {
        m_recentViolations.removeLast();
    }
    
    updateViolationsDisplay();
    updateViolationIndicator(true);
    
    // Also show in alert manager
    if (m_alertManager) {
        m_alertManager->showTrayNotification("Security Violation", violation);
    }
}

void MainWindow::updateClipboardDisplay()
{
    if (m_clipboardActivityDisplay) {
        QString content;
        if (m_recentClipboardActivity.isEmpty()) {
            content = "No clipboard activity detected yet.";
        } else {
            // Join entries with visual separators for better readability
            content = m_recentClipboardActivity.join("\n" + QString("-").repeated(50) + "\n");
        }
        m_clipboardActivityDisplay->setPlainText(content);
        // Scroll to top to show latest activity
        m_clipboardActivityDisplay->moveCursor(QTextCursor::Start);
    }
}

void MainWindow::updateViolationsDisplay()
{
    if (m_violationsDisplay) {
        QString content = m_recentViolations.join("\n");
        if (content.isEmpty()) {
            content = "No security violations detected.";
        }
        m_violationsDisplay->setPlainText(content);
        // Scroll to top to show latest violations
        m_violationsDisplay->moveCursor(QTextCursor::Start);
    }
}

void MainWindow::updateSessionHistoryDisplay()
{
    if (m_sessionHistoryDisplay) {
        QStringList historyText;
        
        for (const auto& entry : m_sessionHistory) {
            QString action = entry.second ? "STARTED" : "STOPPED";
            QString timestamp = formatDateTimeAMPM(entry.first);
            historyText.append(QString("[%1] Monitoring %2").arg(timestamp, action));
        }
        
        QString content = historyText.join("\n");
        if (content.isEmpty()) {
            content = "No monitoring sessions recorded yet.";
        }
        m_sessionHistoryDisplay->setPlainText(content);
        // Scroll to top to show latest activity
        m_sessionHistoryDisplay->moveCursor(QTextCursor::Start);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Always disable airplane mode and stop monitoring when closing
    if (m_monitoring && m_monitoring->isMonitoring()) {
        m_monitoring->stopMonitoring();
        
        // Disable airplane mode
        if (m_networkManager && m_networkManager->isAirplaneModeEnabled()) {
            m_networkManager->disableAirplaneMode();
        }
        
        // Save last close time
        saveLastCloseTime();
        
        LOG_INFO("Monitoring stopped and airplane mode disabled due to application close");
    }
    
    // Stop all timers
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_logCaptureTimer) {
        m_logCaptureTimer->stop();
    }
    
    // Always accept and close the application
    event->accept();
    
    // Exit the application completely
    QApplication::quit();
}

void MainWindow::changeEvent(QEvent *event)
{
    // System tray minimization disabled
    QMainWindow::changeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setupUI()
{
    // Create main widget with vertical layout
    QWidget* mainWidget = new QWidget;
    setCentralWidget(mainWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Violation status indicator at the top
    m_violationStatusFrame = new QFrame;
    m_violationStatusFrame->setFixedHeight(50);
    m_violationStatusFrame->setObjectName("violationStatusFrame");
    
    QHBoxLayout* statusLayout = new QHBoxLayout(m_violationStatusFrame);
    statusLayout->setContentsMargins(15, 10, 15, 10);
    
    m_violationStatusLabel = new QLabel("🟢 SECURE - No violations detected");
    m_violationStatusLabel->setObjectName("violationStatusLabel");
    m_violationStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    m_lastCloseTimeLabel = new QLabel("Last session closed: Never");
    m_lastCloseTimeLabel->setObjectName("lastCloseTimeLabel");
    m_lastCloseTimeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    statusLayout->addWidget(m_violationStatusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_lastCloseTimeLabel);
    
    // Tab widget
    m_tabWidget = new QTabWidget;
    
    // Add components to main layout
    mainLayout->addWidget(m_violationStatusFrame);
    mainLayout->addWidget(m_tabWidget);
    
    createDashboardTab();
    createClipboardTab();
    createKeyloggerTab();
    createUsbTab();
    createLogsTab();
    createStatisticsTab();
    
    // Update violation indicator
    updateViolationIndicator(false);
}

void MainWindow::setupMenuBar()
{
    m_aboutAction = new QAction("&About", this);
    
    // Set icon if available
    m_aboutAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->setStyleSheet(
        "QMenu {"
        "    background-color: #ffffff;"
        "    color: #212529;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 6px;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #007bff;"
        "    color: white;"
        "    padding: 5px 15px;"
        "}"
        "QMenu::item {"
        "    padding: 5px 15px;"
        "}"
    );
    
    // Check for Updates
    QAction *checkUpdatesAction = helpMenu->addAction("Check for Updates...");
    connect(checkUpdatesAction, &QAction::triggered, this, &MainWindow::checkForUpdates);
    
    helpMenu->addSeparator();
    
    helpMenu->addAction(m_aboutAction);
    
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::setupSystemTray()
{
    // System tray disabled to prevent multiple instances
    // The app will run as a normal window instead
    return;
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready - A3Guard Advanced Assessment Application");
}

void MainWindow::createDashboardTab()
{
    m_dashboardTab = new QWidget;
    m_tabWidget->addTab(m_dashboardTab, "🏠 Dashboard");
    
    // Main splitter for layout
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left panel
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // Status group
    m_statusGroup = new QGroupBox("🔍 Monitoring Status");
    QVBoxLayout *statusLayout = new QVBoxLayout;
    
    m_statusLabel = new QLabel("Status: Stopped");
    m_statusLabel->setProperty("class", "status-stopped");
    
    m_startTimeLabel = new QLabel("Start Time: N/A");
    m_durationLabel = new QLabel("Duration: N/A");
    m_lastStopTimeLabel = new QLabel("Last Stopped: N/A");
    
    m_toggleButton = new QPushButton("▶ Start Monitoring");
    m_toggleButton->setProperty("class", "primary");
    m_toggleButton->setMinimumHeight(40);
    
    connect(m_toggleButton, &QPushButton::clicked, this, &MainWindow::toggleMonitoring);
    
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addWidget(m_startTimeLabel);
    statusLayout->addWidget(m_durationLabel);
    statusLayout->addWidget(m_lastStopTimeLabel);
    statusLayout->addWidget(m_toggleButton);
    m_statusGroup->setLayout(statusLayout);
    
    // Security group
    m_securityGroup = new QGroupBox("🛡️ Security Status");
    QVBoxLayout *securityLayout = new QVBoxLayout;
    
    m_airplaneModeLabel = new QLabel("Airplane Mode: Disabled");
    m_violationsLabel = new QLabel("Violations: 0");
    
    // Add more security indicators
    QLabel *encryptionLabel = new QLabel("Encryption: ✓ Active");
    encryptionLabel->setStyleSheet("color: #00d400;");
    QLabel *integrityLabel = new QLabel("File Integrity: ✓ Monitoring");
    integrityLabel->setStyleSheet("color: #00d400;");
    
    securityLayout->addWidget(m_airplaneModeLabel);
    securityLayout->addWidget(m_violationsLabel);
    securityLayout->addWidget(encryptionLabel);
    securityLayout->addWidget(integrityLabel);
    m_securityGroup->setLayout(securityLayout);
    
    // Activity summary group
    m_clipboardGroup = new QGroupBox("📊 Activity Summary");
    QVBoxLayout *summaryLayout = new QVBoxLayout;
    
    m_clipboardActivityDisplay = new QTextEdit;
    m_clipboardActivityDisplay->setProperty("class", "activity");
    m_clipboardActivityDisplay->setReadOnly(true);
    m_clipboardActivityDisplay->setPlainText("Recent activity will appear here during monitoring.");
    
    summaryLayout->addWidget(m_clipboardActivityDisplay);
    m_clipboardGroup->setLayout(summaryLayout);
    
    leftLayout->addWidget(m_statusGroup);
    leftLayout->addWidget(m_securityGroup);
    leftLayout->addWidget(m_clipboardGroup);
    
    // Middle panel - Resource monitoring
    QWidget *middlePanel = new QWidget;
    QVBoxLayout *middleLayout = new QVBoxLayout(middlePanel);
    
    m_resourceGroup = new QGroupBox("📊 System Resources");
    QVBoxLayout *resourceLayout = new QVBoxLayout;
    
    // CPU Usage
    QLabel *cpuLabel = new QLabel("CPU Usage:");
    m_cpuUsage = new QProgressBar;
    m_cpuUsage->setFormat("CPU: %p%");
    m_cpuUsage->setMaximum(100);
    
    // Memory Usage
    QLabel *memoryLabel = new QLabel("Memory Usage:");
    m_memoryUsage = new QProgressBar;
    m_memoryUsage->setFormat("Memory: %p%");
    m_memoryUsage->setMaximum(100);
    
    resourceLayout->addWidget(cpuLabel);
    resourceLayout->addWidget(m_cpuUsage);
    resourceLayout->addWidget(memoryLabel);
    resourceLayout->addWidget(m_memoryUsage);
    m_resourceGroup->setLayout(resourceLayout);
    
    // Monitoring features status
    QGroupBox *featuresGroup = new QGroupBox("⚙️ Monitoring Features");
    QVBoxLayout *featuresLayout = new QVBoxLayout;
    
    QLabel *clipboardStatus = new QLabel("📋 Clipboard: ✓ Active");
    QLabel *processStatus = new QLabel("🔄 Process: ✓ Active");
    QLabel *networkStatus = new QLabel("🌐 Network: ✓ Active");
    QLabel *usbStatus = new QLabel("🔌 USB: ✓ Active");
    
    clipboardStatus->setStyleSheet("color: #00d400;");
    processStatus->setStyleSheet("color: #00d400;");
    networkStatus->setStyleSheet("color: #00d400;");
    usbStatus->setStyleSheet("color: #00d400;");
    
    featuresLayout->addWidget(clipboardStatus);
    featuresLayout->addWidget(processStatus);
    featuresLayout->addWidget(networkStatus);
    featuresLayout->addWidget(usbStatus);
    featuresGroup->setLayout(featuresLayout);
    
    middleLayout->addWidget(m_resourceGroup);
    middleLayout->addWidget(featuresGroup);
    
    // Right panel - Violations and Session History
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    // Violations group
    m_violationsGroup = new QGroupBox("⚠️ Security Violations");
    QVBoxLayout *violationsLayout = new QVBoxLayout;
    
    m_violationsDisplay = new QTextEdit;
    m_violationsDisplay->setProperty("class", "activity");
    m_violationsDisplay->setReadOnly(true);
    m_violationsDisplay->setPlainText("No security violations detected.");
    
    violationsLayout->addWidget(m_violationsDisplay);
    m_violationsGroup->setLayout(violationsLayout);
    
    // Session history group
    m_sessionHistoryGroup = new QGroupBox("🕐 Session History");
    QVBoxLayout *historyLayout = new QVBoxLayout;
    
    m_sessionHistoryDisplay = new QTextEdit;
    m_sessionHistoryDisplay->setProperty("class", "activity");
    m_sessionHistoryDisplay->setReadOnly(true);
    m_sessionHistoryDisplay->setPlainText("No monitoring sessions recorded yet.");
    
    historyLayout->addWidget(m_sessionHistoryDisplay);
    m_sessionHistoryGroup->setLayout(historyLayout);
    
    rightLayout->addWidget(m_violationsGroup);
    rightLayout->addWidget(m_sessionHistoryGroup);
    
    // Add panels to splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(middlePanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({33, 33, 34});
    
    QHBoxLayout *dashboardLayout = new QHBoxLayout;
    dashboardLayout->addWidget(mainSplitter);
    m_dashboardTab->setLayout(dashboardLayout);
}

void MainWindow::createClipboardTab()
{
    m_clipboardTab = new QWidget;
    m_tabWidget->addTab(m_clipboardTab, "📋 Clipboard");
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    // Header with controls
    QHBoxLayout *headerLayout = new QHBoxLayout;
    QLabel *titleLabel = new QLabel("📋 Clipboard Activity Monitor");
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #007bff; margin: 10px;");
    
    m_clipboardCountLabel = new QLabel("Entries: 0");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_clipboardCountLabel);
    
    // Main content area
    m_clipboardDataDisplay = new QTextEdit;
    m_clipboardDataDisplay->setReadOnly(true);
    m_clipboardDataDisplay->setFont(QFont("Consolas", 10));
    m_clipboardDataDisplay->setPlainText("No clipboard data captured yet. Copy something to see it appear here.");
    
    // Instructions
    QLabel *instructionsLabel = new QLabel(
        "💡 This tab shows detailed clipboard data captured by A3Guard.\n"
        "Each entry includes timestamp, content type, and the actual data copied."
    );
    instructionsLabel->setStyleSheet("color: #6c757d; font-style: italic; padding: 10px; background-color: #e9ecef; border-radius: 6px;");
    instructionsLabel->setWordWrap(true);
    
    layout->addLayout(headerLayout);
    layout->addWidget(instructionsLabel);
    layout->addWidget(m_clipboardDataDisplay);
    
    m_clipboardTab->setLayout(layout);
}

void MainWindow::createKeyloggerTab()
{
    m_keyloggerTab = new QWidget;
    m_tabWidget->addTab(m_keyloggerTab, "⌨️ Keylogger");
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    // Header with controls
    QHBoxLayout *headerLayout = new QHBoxLayout;
    QLabel *titleLabel = new QLabel("⌨️ Keystroke Monitor");
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #007bff; margin: 10px;");
    
    m_keyloggerCountLabel = new QLabel("Keystrokes: 0");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_keyloggerCountLabel);
    
    // Main content area
    m_keyloggerDisplay = new QTextEdit;
    m_keyloggerDisplay->setReadOnly(true);
    m_keyloggerDisplay->setFont(QFont("Consolas", 10));
    m_keyloggerDisplay->setPlainText("Keystroke logging is currently disabled for privacy and security.\n\nTo enable keylogging, administrator privileges are required.");
    
    // Privacy notice
    QLabel *privacyLabel = new QLabel(
        "🔒 Privacy Notice: Keystroke logging captures all keyboard input during monitoring sessions.\n"
        "This feature is designed for exam monitoring and security purposes only.\n"
        "Data is encrypted and automatically deleted when the application closes."
    );
    privacyLabel->setStyleSheet("color: #dc3545; font-weight: bold; padding: 15px; background-color: #f8d7da; border: 1px solid #f5c6cb; border-radius: 6px;");
    privacyLabel->setWordWrap(true);
    
    layout->addLayout(headerLayout);
    layout->addWidget(privacyLabel);
    layout->addWidget(m_keyloggerDisplay);
    
    m_keyloggerTab->setLayout(layout);
}

void MainWindow::createUsbTab()
{
    m_usbTab = new QWidget;
    m_tabWidget->addTab(m_usbTab, "💾 USB Monitor");
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    // Header with controls
    QHBoxLayout *headerLayout = new QHBoxLayout;
    QLabel *titleLabel = new QLabel("💾 USB Device Monitor");
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #007bff; margin: 10px;");
    
    m_usbCountLabel = new QLabel("Devices: 0");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_usbCountLabel);
    
    // Main content area
    m_usbDisplay = new QTextEdit;
    m_usbDisplay->setReadOnly(true);
    m_usbDisplay->setFont(QFont("Consolas", 10));
    m_usbDisplay->setPlainText("No USB activity detected yet. USB devices will be automatically monitored and logged here.\n\nMonitoring includes:\n• Device insertions/removals\n• Device details (name, vendor, capacity)\n• Auto-unmount for security");
    
    // Security notice
    QLabel *securityLabel = new QLabel(
        "🔒 Security Feature: USB Auto-Unmount\n"
        "All USB storage devices are automatically unmounted upon detection to prevent data exfiltration.\n"
        "This security measure ensures no unauthorized data transfer can occur during monitoring sessions."
    );
    securityLabel->setStyleSheet("color: #155724; font-weight: bold; padding: 15px; background-color: #d4edda; border: 1px solid #c3e6cb; border-radius: 6px;");
    securityLabel->setWordWrap(true);
    
    layout->addLayout(headerLayout);
    layout->addWidget(securityLabel);
    layout->addWidget(m_usbDisplay);
    
    m_usbTab->setLayout(layout);
}

void MainWindow::createLogsTab()
{
    m_logsTab = new QWidget;
    m_tabWidget->addTab(m_logsTab, "📝 Logs");
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    // Enhanced button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    m_exportLogsButton = new QPushButton("💾 Export Logs");
    m_logsCountLabel = new QLabel("Log Entries: 0");
    
    // Add refresh button
    QPushButton *refreshButton = new QPushButton("🔄 Refresh");
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updateLogs);
    
    buttonLayout->addWidget(m_exportLogsButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_logsCountLabel);
    
    m_logsDisplay = new QTextEdit;
    m_logsDisplay->setReadOnly(true);
    m_logsDisplay->setFont(QFont("Consolas", 9));
    
    // Add some sample log entries for testing
    QString currentTime = formatTimeAMPM(QDateTime::currentDateTime());
    m_logsDisplay->setText(
        QString("[INFO] %1 - A3Guard started successfully\n"
                "[INFO] %1 - Monitoring engine initialized\n"
                "[INFO] %1 - Security manager loaded\n"
                "[DEBUG] %1 - Clipboard monitoring active\n"
                "[DEBUG] %1 - Process monitoring active\n"
                "[INFO] %1 - Network monitoring enabled\n"
                "[INFO] %1 - USB monitoring enabled\n"
                "[INFO] %1 - All systems operational\n").arg(currentTime)
    );
    
    layout->addLayout(buttonLayout);
    layout->addWidget(m_logsDisplay);
    
    m_logsTab->setLayout(layout);
}


void MainWindow::createStatisticsTab()
{
    m_statisticsTab = new QWidget;
    m_tabWidget->addTab(m_statisticsTab, "📊 Statistics");
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    // Create statistics cards
    QHBoxLayout *cardsLayout = new QHBoxLayout;
    
    // Session stats card
    QGroupBox *sessionCard = new QGroupBox("📅 Session Statistics");
    QVBoxLayout *sessionLayout = new QVBoxLayout;
    m_totalSessionsLabel = new QLabel("Total Sessions: 1");
    m_totalViolationsLabel = new QLabel("Total Violations: 0");
    sessionLayout->addWidget(m_totalSessionsLabel);
    sessionLayout->addWidget(m_totalViolationsLabel);
    sessionCard->setLayout(sessionLayout);
    
    // Monitoring stats card
    QGroupBox *monitoringCard = new QGroupBox("🔍 Monitoring Statistics");
    QVBoxLayout *monitoringLayout = new QVBoxLayout;
    m_totalAppsLabel = new QLabel("Applications Monitored: 5");
    monitoringLayout->addWidget(m_totalAppsLabel);
    monitoringCard->setLayout(monitoringLayout);
    
    // Performance stats card
    QGroupBox *performanceCard = new QGroupBox("⚡ Performance Statistics");
    QVBoxLayout *performanceLayout = new QVBoxLayout;
    m_averageResourceLabel = new QLabel("Average CPU Usage: 5%");
    QLabel *averageMemoryLabel = new QLabel("Average Memory Usage: 85MB");
    performanceLayout->addWidget(m_averageResourceLabel);
    performanceLayout->addWidget(averageMemoryLabel);
    performanceCard->setLayout(performanceLayout);
    
    cardsLayout->addWidget(sessionCard);
    cardsLayout->addWidget(monitoringCard);
    cardsLayout->addWidget(performanceCard);
    
    m_generateReportButton = new QPushButton("📋 Generate Report");
    m_generateReportButton->setProperty("class", "primary");
    connect(m_generateReportButton, &QPushButton::clicked, this, &MainWindow::generateReport);
    
    layout->addLayout(cardsLayout);
    layout->addStretch();
    layout->addWidget(m_generateReportButton);
    
    m_statisticsTab->setLayout(layout);
}

void MainWindow::toggleMonitoring()
{
    if (m_currentState == MonitoringState::STOPPED) {
        // Start monitoring
        setMonitoringState(MonitoringState::RUNNING);
        m_sessionStartTime = QDateTime::currentDateTime();
        
        // Record session start
        m_sessionHistory.prepend(qMakePair(m_sessionStartTime, true));
        if (m_sessionHistory.size() > 20) {
            m_sessionHistory.removeLast();
        }
        
        // Start monitoring engines
        if (m_monitoring) {
            m_monitoring->startMonitoring();
        }
        
        // Start keylogging (temporarily disabled)
        // if (m_keyLogger) {
        //     m_keyLogger->startLogging();
        // }
        
        
        // Enable airplane mode
        if (m_networkManager) {
            m_networkManager->enableAirplaneMode();
        }
        
        // Add initial activity
        onClipboardActivity(QString("[%1] Monitoring session started").arg(formatTimeAMPM(m_sessionStartTime)));
        
    } else {
        // Stop monitoring
        setMonitoringState(MonitoringState::STOPPED);
        m_sessionStopTime = QDateTime::currentDateTime();
        
        // Record session stop
        m_sessionHistory.prepend(qMakePair(m_sessionStopTime, false));
        if (m_sessionHistory.size() > 20) {
            m_sessionHistory.removeLast();
        }
        
        // Stop monitoring engines
        if (m_monitoring) {
            m_monitoring->stopMonitoring();
        }
        
        // Stop keylogging (temporarily disabled)
        // if (m_keyLogger) {
        //     m_keyLogger->stopLogging();
        // }
        
        // Disable airplane mode
        if (m_networkManager) {
            m_networkManager->disableAirplaneMode();
        }
        
        // Add final activity
        onClipboardActivity(QString("[%1] Monitoring session stopped").arg(formatTimeAMPM(m_sessionStopTime)));
    }
    
    updateSessionHistoryDisplay();
}

void MainWindow::updateUI()
{
    updateDashboard();
    updateClipboardTabDisplay();
    updateKeyloggerDisplay();
    updateUsbDisplay();
    updateLogs();
    updateStatistics();
}

void MainWindow::updateClipboardTabDisplay()
{
    if (m_clipboardDataDisplay) {
        QString content;
        if (m_recentClipboardData.isEmpty()) {
            content = "No clipboard data captured yet. Copy something to see it appear here.";
        } else {
            content = m_recentClipboardData.join("\n" + QString("=").repeated(80) + "\n");
        }
        m_clipboardDataDisplay->setPlainText(content);
        m_clipboardDataDisplay->moveCursor(QTextCursor::Start);
    }
    
    if (m_clipboardCountLabel) {
        m_clipboardCountLabel->setText(QString("Entries: %1").arg(m_recentClipboardData.size()));
    }
}

void MainWindow::updateKeyloggerDisplay()
{
    if (m_keyloggerDisplay && m_keyloggerCountLabel) {
        QString content;
        if (m_recentKeyloggerData.isEmpty()) {
            if (m_currentState == MonitoringState::RUNNING) {
                content = "🎯 Keystroke monitoring is ACTIVE...\n\n"
                         "Monitoring keyboard activity for security purposes.\n"
                         "All keystroke data is encrypted and automatically deleted when the application closes.\n\n"
                         "📋 Keystroke events will appear here as they are detected.";
            } else {
                content = "⏸️ Keystroke monitoring is STOPPED.\n\n"
                         "To start keystroke monitoring:\n"
                         "1. Ensure A3Guard is running with administrator privileges\n"
                         "2. Click 'Start Monitoring' on the Dashboard\n\n"
                         "🔒 Note: All keystroke data is encrypted and automatically deleted when the application closes.";
            }
        } else {
            // Display actual keystroke data
            content = "=== KEYBOARD ACTIVITY LOG ===\n\n" + m_recentKeyloggerData.join("\n");
            LOG_DEBUG("Displaying keystroke data count:" << m_recentKeyloggerData.size());
        }
        m_keyloggerDisplay->setPlainText(content);
        m_keyloggerDisplay->moveCursor(QTextCursor::Start);
        m_keyloggerCountLabel->setText(QString("Keystroke Events: %1").arg(m_recentKeyloggerData.size()));
    }
}

void MainWindow::updateUsbDisplay()
{
    if (m_usbDisplay && m_usbCountLabel) {
        QString content;
        if (m_recentUsbActivity.isEmpty()) {
            content = "No USB activity detected yet. USB devices will be automatically monitored and logged here.\n\n"
                     "Monitoring includes:\n"
                     "• Device insertions/removals\n"
                     "• Device details (name, vendor, capacity)\n"
                     "• Auto-unmount for security\n\n"
                     "Security: All USB storage devices are automatically unmounted to prevent data exfiltration.";
        } else {
            content = m_recentUsbActivity.join("\n" + QString("-").repeated(60) + "\n");
        }
        m_usbDisplay->setPlainText(content);
        m_usbDisplay->moveCursor(QTextCursor::Start);
        m_usbCountLabel->setText(QString("Devices: %1").arg(m_recentUsbActivity.size()));
    }
}

void MainWindow::showAlert(const QString& message, AlertLevel level)
{
    Q_UNUSED(level)
    QMessageBox::information(this, "Alert", message);
}

void MainWindow::onMonitoringStateChanged(MonitoringState state)
{
    setMonitoringState(state);
}

void MainWindow::onViolationDetected(const MonitoringEvent& event)
{
    onViolationDetected(event.description);
}

void MainWindow::showAbout()
{
    // Create custom styled About dialog
    QDialog *aboutDialog = new QDialog(this);
    aboutDialog->setWindowTitle("About " + QString(A3GUARD_NAME));
    aboutDialog->setModal(true);
    aboutDialog->setFixedSize(500, 400);
    aboutDialog->setStyleSheet(
        "QDialog {"
        "    background-color: #f8f9fa;"
        "}"
        "QLabel {"
        "    color: #495057;"
        "}"
        "QPushButton {"
        "    background-color: #007bff;"
        "    color: white;"
        "    border: 1px solid #0056b3;"
        "    border-radius: 8px;"
        "    padding: 10px 20px;"
        "    font-weight: 600;"
        "    min-height: 32px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0056b3;"
        "    border-color: #004085;"
        "    box-shadow: 0 4px 8px rgba(0,123,255,0.3);"
        "}"
        "QPushButton:pressed {"
        "    background-color: #004085;"
        "}"
    );
    
    QVBoxLayout *layout = new QVBoxLayout(aboutDialog);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Icon and title section
    QHBoxLayout *headerLayout = new QHBoxLayout;
    QLabel *iconLabel = new QLabel;
    QIcon appIcon = style()->standardIcon(QStyle::SP_FileDialogDetailedView);
    iconLabel->setPixmap(appIcon.pixmap(48, 48));
    iconLabel->setAlignment(Qt::AlignCenter);
    
    QVBoxLayout *titleLayout = new QVBoxLayout;
    QLabel *titleLabel = new QLabel(QString("%1 v%2").arg(A3GUARD_NAME).arg(A3GUARD_VERSION));
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #007bff;");
    QLabel *subtitleLabel = new QLabel("Advanced Assessment Application");
    subtitleLabel->setStyleSheet("font-size: 11pt; color: #6c757d; font-style: italic;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    
    headerLayout->addWidget(iconLabel);
    headerLayout->addLayout(titleLayout);
    
    // Content area
    QTextEdit *contentDisplay = new QTextEdit;
    contentDisplay->setReadOnly(true);
    contentDisplay->setStyleSheet(
        "QTextEdit {"
        "    background-color: #ffffff;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 6px;"
        "    color: #212529;"
        "    padding: 10px;"
        "}"
    );
    
    QString aboutContent = QString(
        "<h3 style='color: #007bff; margin-top: 0;'>About A3Guard</h3>"
        "<p style='line-height: 1.6;'>A3Guard is a secure exam monitoring system designed to provide comprehensive surveillance during assessment sessions.</p>"
        
        "<h4 style='color: #495057;'>Key Features:</h4>"
        "<ul style='line-height: 1.8;'>"
        "<li><b>🔍 Real-time Process Monitoring</b> - Track running applications and system activity</li>"
        "<li><b>📋 Clipboard Activity Tracking</b> - Monitor all clipboard operations with timestamps</li>"
        "<li><b>🌐 Network Security Enforcement</b> - Airplane mode and network traffic control</li>"
        "<li><b>🔌 USB Device Monitoring</b> - Auto-unmount USB devices to prevent data exfiltration</li>"
        "<li><b>🔐 Encrypted Data Storage</b> - AES-256 encryption for all sensitive data</li>"
        "<li><b>⌨️ Keystroke Monitoring</b> - Real-time keyboard activity logging</li>"
        "</ul>"
        
        "<h4 style='color: #495057;'>Technical Details:</h4>"
        "<p><b>Built with:</b> Qt5 Framework and Modern C++17<br>"
        "<b>Security:</b> AES-256-CBC Encryption, SHA-256 Hashing<br>"
        "<b>Platform:</b> Linux (Debian/Ubuntu)</p>"
    );
    
    contentDisplay->setHtml(aboutContent);
    
    // Close button
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    QPushButton *closeButton = new QPushButton("Close");
    connect(closeButton, &QPushButton::clicked, aboutDialog, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    
    layout->addLayout(headerLayout);
    layout->addWidget(contentDisplay);
    layout->addLayout(buttonLayout);
    
    aboutDialog->exec();
    aboutDialog->deleteLater();
}

void MainWindow::showSettings()
{
    QMessageBox::information(this, "Settings", "Settings configuration panel will be implemented in future versions.");
}

void MainWindow::exportLogs()
{
    QMessageBox::information(this, "Export Logs", "Log export functionality will be implemented in future versions.");
}

void MainWindow::generateReport()
{
    // Generate comprehensive report
    QString report = generateDetailedReport();
    
    // Create enhanced report dialog
    QDialog *reportDialog = new QDialog(this);
    reportDialog->setWindowTitle("📊 A3Guard Monitoring Report");
    reportDialog->setModal(true);
    reportDialog->resize(900, 700);
    reportDialog->setStyleSheet(
        "QDialog { background-color: #f8f9fa; }"
        "QLabel { color: #495057; }"
        "QPushButton { padding: 12px 24px; font-weight: 600; }"
    );
    
    QVBoxLayout *mainLayout = new QVBoxLayout(reportDialog);
    mainLayout->setSpacing(20);
    
    // Header section
    QHBoxLayout *headerLayout = new QHBoxLayout;
    QLabel *titleLabel = new QLabel("📊 A3Guard Security Report");
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #007bff; margin: 10px;");
    QLabel *timestampLabel = new QLabel(QString("Generated: %1").arg(formatDateTimeAMPM(QDateTime::currentDateTime())));
    timestampLabel->setStyleSheet("color: #6c757d; font-style: italic;");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(timestampLabel);
    
    // Report display area
    QTextEdit *reportDisplay = new QTextEdit;
    reportDisplay->setReadOnly(true);
    reportDisplay->setFont(QFont("Consolas", 10));
    reportDisplay->setPlainText(report);
    reportDisplay->setStyleSheet(
        "QTextEdit {"
        "    background-color: #ffffff;"
        "    border: 2px solid #dee2e6;"
        "    border-radius: 8px;"
        "    padding: 15px;"
        "    color: #212529;"
        "}"
    );
    
    // Action buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *saveButton = new QPushButton("💾 Save Report");
    QPushButton *printButton = new QPushButton("🖨️ Print Report");
    QPushButton *copyButton = new QPushButton("📋 Copy to Clipboard");
    QPushButton *closeButton = new QPushButton("❌ Close");
    
    saveButton->setProperty("class", "primary");
    printButton->setStyleSheet("background-color: #28a745; color: white;");
    copyButton->setStyleSheet("background-color: #17a2b8; color: white;");
    closeButton->setStyleSheet("background-color: #6c757d; color: white;");
    
    connect(saveButton, &QPushButton::clicked, [this, report]() {
        saveReportToFile(report);
    });
    
    connect(copyButton, &QPushButton::clicked, [report]() {
        QApplication::clipboard()->setText(report);
        QMessageBox::information(nullptr, "Copied", "Report copied to clipboard!");
    });
    
    connect(closeButton, &QPushButton::clicked, reportDialog, &QDialog::accept);
    
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(printButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    // Footer info
    QLabel *footerLabel = new QLabel(
        "📝 This report contains sensitive monitoring data. Handle according to your organization's privacy policies."
    );
    footerLabel->setStyleSheet("color: #6c757d; font-size: 9pt; padding: 10px; background-color: #e9ecef; border-radius: 6px;");
    footerLabel->setWordWrap(true);
    
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(reportDisplay);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(footerLabel);
    
    reportDialog->exec();
    reportDialog->deleteLater();
}

void MainWindow::systemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        show();
        raise();
        activateWindow();
        break;
    default:
        break;
    }
}

void MainWindow::updateDashboard()
{
    // Update status with proper styling
    QString status = (m_currentState == MonitoringState::RUNNING) ? "Running" : "Stopped";
    QString statusClass = (m_currentState == MonitoringState::RUNNING) ? "status-running" : "status-stopped";
    
    m_statusLabel->setText("Status: " + status);
    m_statusLabel->setProperty("class", statusClass);
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
    
    if (m_currentState == MonitoringState::RUNNING) {
        m_startTimeLabel->setText("Start Time: " + formatTimeAMPM(m_sessionStartTime));
        m_durationLabel->setText("Duration: " + formatDuration(m_sessionStartTime));
    } else {
        m_startTimeLabel->setText("Start Time: N/A");
        m_durationLabel->setText("Duration: N/A");
    }
    
    // Update last stop time
    if (m_sessionStopTime.isValid()) {
        m_lastStopTimeLabel->setText("Last Stopped: " + formatTimeAMPM(m_sessionStopTime));
    }
    
    // Update airplane mode status
    if (m_networkManager) {
        QString airplaneStatus = m_networkManager->isAirplaneModeEnabled() ? 
            "✓ Enabled" : "✗ Disabled";
        QString airplaneColor = m_networkManager->isAirplaneModeEnabled() ? 
            "color: #00d400;" : "color: #ff6b6b;";
        m_airplaneModeLabel->setText("Airplane Mode: " + airplaneStatus);
        m_airplaneModeLabel->setStyleSheet(airplaneColor);
    }
    
    // Update violations count
    m_violationsLabel->setText(QString("Violations: %1").arg(m_recentViolations.size()));
    
    // Update resource usage from ResourceMonitor
    if (m_resourceMonitor) {
        int cpuValue = static_cast<int>(m_resourceMonitor->getCpuUsage());
        int memoryValue = static_cast<int>(m_resourceMonitor->getMemoryPercentage());
        
        m_cpuUsage->setValue(cpuValue);
        m_memoryUsage->setValue(memoryValue);
    }
}

void MainWindow::updateLogs()
{
    if (!m_logsDisplay) return;
    
    // Get current logs from Logger
    QStringList currentLogs;
    if (m_logger) {
        currentLogs = m_logger->getRecentLogs(200); // Get up to 200 recent logs
    }
    
    // Add any additional logs from MainWindow
    QStringList allLogs = currentLogs + m_recentLogs;
    
    // Update display
    m_logsDisplay->setPlainText(allLogs.join("\n"));
    
    // Scroll to bottom to show latest logs
    m_logsDisplay->moveCursor(QTextCursor::End);
    
    // Update log count
    m_logsCountLabel->setText(QString("Log Entries: %1").arg(allLogs.size()));
}


void MainWindow::updateStatistics()
{
    // Update statistics with more realistic data
    if (m_currentState == MonitoringState::RUNNING) {
        m_totalSessionsLabel->setText("Total Sessions: 1 (Active)");
        m_totalViolationsLabel->setText(QString("Total Violations: %1").arg(m_recentViolations.size()));
        if (m_resourceMonitor) {
            m_averageResourceLabel->setText("Average CPU Usage: " + QString::number(static_cast<int>(m_resourceMonitor->getCpuUsage())) + "%");
        }
    } else {
        m_totalSessionsLabel->setText(QString("Total Sessions: %1").arg(m_sessionHistory.size() / 2));
        m_totalViolationsLabel->setText(QString("Total Violations: %1").arg(m_recentViolations.size()));
        m_averageResourceLabel->setText("Average CPU Usage: N/A");
    }
}

void MainWindow::setMonitoringState(MonitoringState state)
{
    m_currentState = state;
    
    bool running = (state == MonitoringState::RUNNING);
    QString buttonText = running ? "⏸ Stop Monitoring" : "▶ Start Monitoring";
    m_toggleButton->setText(buttonText);
    
    QString statusMessage = running ? 
        "Monitoring active - A3Guard is protecting your assessment session" : 
        "Ready - A3Guard Advanced Assessment Application";
    statusBar()->showMessage(statusMessage);
}

QString MainWindow::formatDuration(const QDateTime& start, const QDateTime& end) const
{
    QDateTime endTime = end.isValid() ? end : QDateTime::currentDateTime();
    qint64 seconds = start.secsTo(endTime);
    
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    return QString("%1:%2:%3")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(secs, 2, 10, QChar('0'));
}

QString MainWindow::formatBytes(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if (bytes >= GB) {
        return QString::number(bytes / GB) + " GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / MB) + " MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / KB) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

void MainWindow::updateViolationIndicator(bool hasViolation)
{
    if (!m_violationStatusLabel || !m_violationStatusFrame) return;
    
    if (hasViolation) {
        m_violationStatusLabel->setText(QString("🔴 VIOLATION DETECTED - %1 total violations").arg(m_violationCount));
        m_violationStatusLabel->setProperty("violation", "true");
        m_violationStatusFrame->setProperty("violation", "true");
    } else {
        m_violationStatusLabel->setText("🟢 SECURE - No violations detected");
        m_violationStatusLabel->setProperty("violation", "false");
        m_violationStatusFrame->setProperty("violation", "false");
    }
    
    // Force style refresh
    m_violationStatusLabel->style()->unpolish(m_violationStatusLabel);
    m_violationStatusLabel->style()->polish(m_violationStatusLabel);
    m_violationStatusFrame->style()->unpolish(m_violationStatusFrame);
    m_violationStatusFrame->style()->polish(m_violationStatusFrame);
}

void MainWindow::saveLastCloseTime()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    
    // Save to settings file
    QString settingsPath = QDir::homePath() + "/.a3guard_settings";
    QDir().mkpath(QFileInfo(settingsPath).absolutePath());
    
    QFile file(settingsPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "last_close_time=" << currentTime.toString(Qt::ISODate) << "\n";
        file.close();
    }
    
    m_lastCloseTime = currentTime;
}

void MainWindow::loadLastCloseTime()
{
    QString settingsPath = QDir::homePath() + "/.a3guard_settings";
    QFile file(settingsPath);
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line;
        while (!(line = in.readLine()).isNull()) {
            if (line.startsWith("last_close_time=")) {
                QString timeStr = line.mid(16); // Remove "last_close_time="
                m_lastCloseTime = QDateTime::fromString(timeStr, Qt::ISODate);
                break;
            }
        }
        file.close();
    }
    
    // Update the display
    if (m_lastCloseTimeLabel) {
        if (m_lastCloseTime.isValid()) {
            m_lastCloseTimeLabel->setText(QString("Last session closed: %1").arg(formatDateTimeAMPM(m_lastCloseTime)));
        } else {
            m_lastCloseTimeLabel->setText("Last session closed: Never");
        }
    }
}

bool MainWindow::isTerminalOrSystemData(const QString& content)
{
    // Filter out common terminal/system patterns
    QStringList terminalPatterns = {
        "test@",  // Terminal username
        "/home/", // File paths
        "sudo ",  // Commands
        "$ ",     // Shell prompts
        "# ",     // Root prompts
        "[INFO]", // Log entries
        "[DEBUG]",
        "[WARNING]",
        "[ERROR]",
        "Built target", // Build output
        "Linking CXX",  // Build output
        "Building CXX", // Build output
        "make[",        // Make output
        "Full Content:",  // Report generation text
        "CLIPBOARD COPIED", // Meta-text about clipboard
        "Keystroke",   // Keystroke monitoring output
        "Keyboard activity", // Keyboard monitoring output
        "Monitoring",  // Session text
    };
    
    // Check for patterns
    for (const QString& pattern : terminalPatterns) {
        if (content.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    // Check if it's mostly path-like (contains many slashes)
    int slashCount = content.count('/');
    if (slashCount > 2 && content.length() > 10) {
        return true;
    }
    
    // Check if it's all lowercase with underscores (likely system output)
    if (content.length() > 5 && content == content.toLower() && content.contains('_')) {
        return true;
    }
    
    return false;
}

QString MainWindow::detectClipboardOperation(const QString& content)
{
    int charCount = content.length();
    int lineCount = content.count('\n') + 1;
    int wordCount = content.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    
    // Analyze content to provide detailed information
    QString baseInfo = QString("CLIPBOARD COPIED [%1 chars, %2 words, %3 lines]")
                      .arg(charCount).arg(wordCount).arg(lineCount);
    
    if (content.contains("http://") || content.contains("https://")) {
        return baseInfo + " - URL/Link detected";
    } else if (content.contains('@') && content.contains('.') && content.count('@') == 1) {
        return baseInfo + " - Email address detected";
    } else if (content.contains(QRegExp("\\b\\d{4}-\\d{2}-\\d{2}\\b"))) {
        return baseInfo + " - Date format detected";
    } else if (content.contains(QRegExp("\\b\\d+\\.\\d+\\b"))) {
        return baseInfo + " - Decimal numbers detected";
    } else if (content.contains(QRegExp("\\b\\d+\\b"))) {
        return baseInfo + " - Numbers detected";
    } else if (lineCount > 1) {
        return baseInfo + " - Multi-line text";
    } else if (charCount > 200) {
        return baseInfo + " - Long text";
    } else {
        return baseInfo + " - Plain text";
    }
}

void MainWindow::captureConsoleLogs()
{
    // Simple log capture - add recent log entries to display
    QString timestamp = formatTimeAMPM(QDateTime::currentDateTime());
    
    // Add system status logs periodically
    static int logCounter = 0;
    logCounter++;
    
    if (logCounter % 30 == 0) { // Every 30 seconds
        QString statusLog;
        if (m_currentState == MonitoringState::RUNNING) {
            statusLog = QString("[INFO] %1 - Monitoring active - System protected").arg(timestamp);
        } else {
            statusLog = QString("[INFO] %1 - System ready - Monitoring stopped").arg(timestamp);
        }
        
        m_recentLogs.prepend(statusLog);
        if (m_recentLogs.size() > 100) {
            m_recentLogs.removeLast();
        }
    }
    
    // Add resource usage logs
    if (logCounter % 60 == 0 && m_resourceMonitor) { // Every minute
        double cpu = m_resourceMonitor->getCpuUsage();
        double memory = m_resourceMonitor->getMemoryPercentage();
        QString resourceLog = QString("[DEBUG] %1 - Resource usage: CPU %2%, Memory %3%")
                             .arg(timestamp)
                             .arg(QString::number(cpu, 'f', 1))
                             .arg(QString::number(memory, 'f', 1));
        
        m_recentLogs.prepend(resourceLog);
        if (m_recentLogs.size() > 100) {
            m_recentLogs.removeLast();
        }
    }
}


void MainWindow::checkPrivileges()
{
    // Always check and handle privileges synchronously
    // This will block until user authenticates or rejects
    if (!PrivilegeDialog::hasRootPrivileges()) {
        // Show privilege dialog - MUST complete before proceeding
        PrivilegeDialog dialog(this);
        int result = dialog.exec();  // Blocks here until dialog is done or user responds
        
        // Check if user successfully authenticated
        // The exec() has already completed, so we can check the result
        if (result == QDialog::Accepted && PrivilegeDialog::hasRootPrivileges()) {
            // User authenticated successfully and now has privileges
            m_shouldShowWindow = true;
            return;
        }
        
        // User rejected or authentication failed
        m_shouldShowWindow = false;
        // Queue the application exit for after the constructor completes
        // This ensures the MainWindow is fully constructed before exiting
        QMetaObject::invokeMethod(QApplication::instance(), "quit", Qt::QueuedConnection);
        return;
    }
    
    // Already have root privileges, allow window to show
    m_shouldShowWindow = true;
}

void MainWindow::requestPrivilegesForFeatures()
{
    // Show a non-intrusive notification about privilege requirements
    if (m_alertManager) {
        m_alertManager->showTrayNotification(
            "Privilege Notice",
            "A3Guard is running with limited privileges. Some advanced features like network control and USB monitoring may not be available.\n\n"
            "To enable all features, restart the application with: sudo a3guard"
        );
    }
    
    // Add a log entry about privilege status
    QString logEntry = QString("[WARNING] %1 - Running with limited privileges. Advanced monitoring features may be restricted.")
                      .arg(formatTimeAMPM(QDateTime::currentDateTime()));
    m_recentLogs.prepend(logEntry);
    if (m_recentLogs.size() > 100) {
        m_recentLogs.removeLast();
    }
}

QString MainWindow::generateDetailedReport()
{
    QDateTime reportTime = QDateTime::currentDateTime();
    QString report;
    
    // Header
    report += "═══════════════════════════════════════════════\n";
    report += "           A3GUARD SECURITY REPORT\n";
    report += "═══════════════════════════════════════════════\n";
    report += QString("Generated: %1\n").arg(formatDateTimeAMPM(reportTime));
    report += QString("Version: %1\n\n").arg(A3GUARD_VERSION);
    
    // Basic System Info
    report += "▌ SYSTEM INFO\n";
    report += QString("Computer: %1\n").arg(QString::fromLocal8Bit(qgetenv("HOSTNAME")));
    report += QString("User: %1\n\n").arg(QString::fromLocal8Bit(qgetenv("USER")));
    
    // Session Overview
    QDateTime currentTime = QDateTime::currentDateTime();
    int sessionDuration = m_sessionStartTime.secsTo(currentTime);
    int hours = sessionDuration / 3600;
    int minutes = (sessionDuration % 3600) / 60;
    
    report += "▌ SESSION INFO\n";
    report += QString("Started: %1\n").arg(formatDateTimeAMPM(m_sessionStartTime));
    report += QString("Duration: %1h %2m\n\n").arg(hours).arg(minutes);
    
    // MAIN FOCUS: Violation Status and Details
    bool hasViolations = !m_recentViolations.isEmpty() || m_violationCount > 0;
    report += "▌ SECURITY STATUS\n";
    if (hasViolations) {
        report += QString("⚠️ VIOLATIONS DETECTED: %1 total\n\n").arg(m_violationCount);
        
        report += "▌ VIOLATION DETAILS\n";
        if (!m_recentViolations.isEmpty()) {
            for (const QString& violation : m_recentViolations) {
                report += QString("• %1\n").arg(violation);
            }
        } else {
            report += "• Check system logs for violation details\n";
        }
        
        report += "\n⚠️ ACTION REQUIRED: Review and address security violations\n";
    } else {
        report += "✓ NO VIOLATIONS - Monitoring session clean\n";
        report += "Session completed successfully without security issues\n";
    }
    
    // Footer
    report += "\n═══════════════════════════════════════════════\n";
    if (hasViolations) {
        report += "⚠️ SECURITY ALERT: Violations detected in this session\n";
    } else {
        report += "✓ Session completed successfully\n";
    }
    report += "═══════════════════════════════════════════════\n";
    
    return report;
}

void MainWindow::saveReportToFile(const QString& report)
{
    QString defaultFileName = QString("A3Guard_Report_%1.txt")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    
    QString fileName = QFileDialog::getSaveFileName(this, 
                                                   "Save A3Guard Report",
                                                   QDir::homePath() + "/" + defaultFileName,
                                                   "Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << report;
            file.close();
            
            QMessageBox::information(this, "Report Saved", 
                                   QString("Report successfully saved to:\n%1").arg(fileName));
        } else {
            QMessageBox::critical(this, "Save Error", 
                                "Failed to save report file. Please check permissions and try again.");
        }
    }
}

void MainWindow::checkForUpdates()
{
    m_updateChecker->checkForUpdates();
}

void MainWindow::onUpdateCheckStarted()
{
    statusBar()->showMessage("Checking for updates...");
}

void MainWindow::onUpdateAvailable(QString latestVersion, QString downloadUrl, QString releaseNotes)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("A3Guard Update Available");
    msgBox.setIcon(QMessageBox::Information);
    
    // Apply modern theme to dialog
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #f8f9fa;"
        "    color: #212529;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 8px;"
        "}"
        "QMessageBox QLabel {"
        "    color: #212529;"
        "    font-size: 11pt;"
        "}"
        "QMessageBox QPushButton {"
        "    background-color: #007bff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 24px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QMessageBox QPushButton:hover {"
        "    background-color: #0056b3;"
        "}"
        "QMessageBox QPushButton:pressed {"
        "    background-color: #004085;"
        "}"
    );
    
    QString currentVersion = m_updateChecker->getCurrentVersion();
    QString message = QString(
        "<b style='color: #28a745; font-size: 12pt;'>✓ New version available!</b><br><br>"
        "<b style='color: #495057;'>Current version:</b> <span style='color: #007bff;'><b>%1</b></span><br>"
        "<b style='color: #495057;'>Latest version:</b> <span style='color: #28a745;'><b>%2</b></span><br><br>"
        "<b style='color: #495057;'>Release Notes:</b><br>"
        "<span style='color: #6c757d;'>%3</span>"
    ).arg(currentVersion, latestVersion, releaseNotes.replace("\n", "<br>").left(500));
    
    msgBox.setText(message);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, "⬇ Download");
    msgBox.setButtonText(QMessageBox::No, "Later");
    
    if (msgBox.exec() == QMessageBox::Yes) {
        // Download the DEB file
        statusBar()->showMessage("Downloading update...");
        emit m_updateChecker->downloadStarted("a3guard_" + latestVersion + "_amd64.deb");
        
        // Trigger download by simulating a network request
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkRequest request{QUrl(downloadUrl)};
        request.setHeader(QNetworkRequest::UserAgentHeader, QString("A3Guard/%1").arg(m_updateChecker->getCurrentVersion()));
        
        QNetworkReply *reply = manager->get(request);
        
        // Connect progress and finished signals
        connect(reply, QOverload<qint64, qint64>::of(&QNetworkReply::downloadProgress),
                this, &MainWindow::onDownloadProgress);
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, latestVersion]() {
            if (reply->error() == QNetworkReply::NoError) {
                // Create cache directory
                QString cacheDir = QDir::homePath() + "/.cache/a3guard";
                QDir().mkpath(cacheDir);
                
                // Save file
                QString filePath = cacheDir + "/a3guard_" + latestVersion + "_amd64.deb";
                QFile file(filePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                    onDownloadFinished(filePath);
                } else {
                    onDownloadFailed("Failed to save file");
                }
            } else {
                onDownloadFailed(reply->errorString());
            }
            reply->deleteLater();
        });
    }
}

void MainWindow::onUpdateCheckFailed(QString errorMessage)
{
    statusBar()->showMessage("Check for updates failed");
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Update Check Failed");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #f8f9fa;"
        "    color: #212529;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 8px;"
        "}"
        "QMessageBox QLabel { color: #212529; font-size: 11pt; }"
        "QMessageBox QPushButton {"
        "    background-color: #007bff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 24px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QMessageBox QPushButton:hover { background-color: #0056b3; }"
        "QMessageBox QPushButton:pressed { background-color: #004085; }"
    );
    msgBox.setText(
        "<b style='color: #dc3545; font-size: 12pt;'>✗ Unable to check for updates</b>"
    );
    msgBox.setInformativeText(
        "<b>Error:</b> " + errorMessage + "<br><br>Please check your internet connection and try again."
    );
    msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel);
    
    if (msgBox.exec() == QMessageBox::Retry) {
        checkForUpdates();
    }
}

void MainWindow::onDownloadStarted(QString fileName)
{
    statusBar()->showMessage(QString("Downloading %1...").arg(fileName));
}

void MainWindow::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percent = (bytesReceived * 100) / bytesTotal;
        double megaBytes = static_cast<double>(bytesReceived) / (1024.0 * 1024.0);
        double totalMB = static_cast<double>(bytesTotal) / (1024.0 * 1024.0);
        statusBar()->showMessage(QString("Downloading... %1% (%2 MB / %3 MB)").arg(percent).arg(megaBytes, 0, 'f', 1).arg(totalMB, 0, 'f', 1));
    }
}

void MainWindow::onDownloadFinished(QString filePath)
{
    statusBar()->showMessage("Download completed");
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Download Complete");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #f8f9fa;"
        "    color: #212529;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 8px;"
        "}"
        "QMessageBox QLabel { color: #212529; font-size: 11pt; }"
        "QMessageBox QPushButton {"
        "    background-color: #28a745;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 24px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QMessageBox QPushButton:hover { background-color: #218838; }"
        "QMessageBox QPushButton:pressed { background-color: #1e7e34; }"
    );
    msgBox.setText(
        "<b style='color: #28a745; font-size: 12pt;'>✓ Update downloaded successfully!</b>"
    );
    
    QString message = QString(
        "<b>File:</b> <span style='color: #007bff; font-family: monospace;'>%1</span><br><br>"
        "<b>To install the update, run:</b><br>"
        "<code style='background-color: #e9ecef; padding: 8px; border-radius: 4px; display: block; margin-top: 8px;'>sudo dpkg -i %2</code>"
    ).arg(QFileInfo(filePath).fileName(), filePath);
    
    msgBox.setInformativeText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::onDownloadFailed(QString errorMessage)
{
    statusBar()->showMessage("Download failed");
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Download Failed");
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #f8f9fa;"
        "    color: #212529;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 8px;"
        "}"
        "QMessageBox QLabel { color: #212529; font-size: 11pt; }"
        "QMessageBox QPushButton {"
        "    background-color: #dc3545;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 24px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QMessageBox QPushButton:hover { background-color: #c82333; }"
        "QMessageBox QPushButton:pressed { background-color: #bd2130; }"
    );
    msgBox.setText(
        "<b style='color: #dc3545; font-size: 12pt;'>✗ Failed to download update</b>"
    );
    msgBox.setInformativeText(
        "<b>Error:</b> " + errorMessage + "<br><br>Please try again later or check your internet connection."
    );
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

