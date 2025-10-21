#include "NetworkManager.h"
#include <QProcess>
#include <QDebug>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_airplaneModeEnabled(false)
    , m_trafficBlocked(false)
{
    // Store initial interface states for restoration
    getActiveInterfaces();
}

NetworkManager::~NetworkManager()
{
    // Restore network access if airplane mode was enabled
    if (m_airplaneModeEnabled) {
        disableAirplaneMode();
    }
}

void NetworkManager::enableAirplaneMode()
{
    if (m_airplaneModeEnabled) return;
    
    // Store current active interfaces for restoration
    m_previousActiveInterfaces = getActiveNetworkInterfaces();
    
    // Method 1: Use rfkill to block all wireless devices
    QProcess rfkillProcess;
    rfkillProcess.start("rfkill", QStringList() << "block" << "all");
    rfkillProcess.waitForFinished(2000);
    
    // Method 2: Use NetworkManager to disable networking
    QProcess nmProcess;
    nmProcess.start("nmcli", QStringList() << "networking" << "off");
    nmProcess.waitForFinished(2000);
    
    // Method 3: Disable ethernet interfaces manually
    for (const QString& interface : m_previousActiveInterfaces) {
        if (interface.startsWith("eth") || interface.startsWith("enp") || interface.startsWith("ens")) {
            QProcess ifdownProcess;
            ifdownProcess.start("ip", QStringList() << "link" << "set" << interface << "down");
            ifdownProcess.waitForFinished(1000);
        }
    }
    
    m_airplaneModeEnabled = true;
    emit networkStateChanged(false);
    qDebug() << "Airplane mode enabled - all network interfaces disabled";
}

void NetworkManager::disableAirplaneMode()
{
    if (!m_airplaneModeEnabled) return;
    
    // Method 1: Use rfkill to unblock all wireless devices
    QProcess rfkillProcess;
    rfkillProcess.start("rfkill", QStringList() << "unblock" << "all");
    rfkillProcess.waitForFinished(2000);
    
    // Method 2: Use NetworkManager to enable networking
    QProcess nmProcess;
    nmProcess.start("nmcli", QStringList() << "networking" << "on");
    nmProcess.waitForFinished(2000);
    
    // Method 3: Re-enable ethernet interfaces
    for (const QString& interface : m_previousActiveInterfaces) {
        if (interface.startsWith("eth") || interface.startsWith("enp") || interface.startsWith("ens")) {
            QProcess ifupProcess;
            ifupProcess.start("ip", QStringList() << "link" << "set" << interface << "up");
            ifupProcess.waitForFinished(1000);
        }
    }
    
    m_airplaneModeEnabled = false;
    emit networkStateChanged(true);
    qDebug() << "Airplane mode disabled - network interfaces restored";
}

bool NetworkManager::isAirplaneModeEnabled() const
{
    return m_airplaneModeEnabled;
}

void NetworkManager::blockAllTraffic()
{
    // Use iptables to block all traffic
    QProcess iptablesProcess;
    iptablesProcess.start("iptables", QStringList() << "-I" << "OUTPUT" << "1" << "-j" << "DROP");
    iptablesProcess.waitForFinished(1000);
    
    m_trafficBlocked = true;
}

void NetworkManager::allowAllTraffic()
{
    // Remove iptables blocking rule
    QProcess iptablesProcess;
    iptablesProcess.start("iptables", QStringList() << "-D" << "OUTPUT" << "-j" << "DROP");
    iptablesProcess.waitForFinished(1000);
    
    m_trafficBlocked = false;
}

bool NetworkManager::isTrafficBlocked() const
{
    return m_trafficBlocked;
}

QStringList NetworkManager::getActiveInterfaces()
{
    return getActiveNetworkInterfaces();
}

QStringList NetworkManager::getActiveNetworkInterfaces()
{
    QProcess process;
    process.start("ip", QStringList() << "link" << "show" << "up");
    process.waitForFinished(2000);
    
    QString output = process.readAllStandardOutput();
    QStringList interfaces;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        if (line.contains(": ") && !line.startsWith(" ")) {
            // Extract interface name from lines like "2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP>"
            QString interfacePart = line.split(": ")[1];
            QString interfaceName = interfacePart.split(":")[0];
            
            // Skip loopback interface
            if (interfaceName != "lo") {
                interfaces << interfaceName;
            }
        }
    }
    
    return interfaces;
}

void NetworkManager::onNetworkStateChanged()
{
    // Implementation for network state change handling
}

void NetworkManager::setInterfaceState(const QString& interface, bool enabled)
{
    QString state = enabled ? "up" : "down";
    QProcess process;
    process.start("ip", QStringList() << "link" << "set" << interface << state);
    process.waitForFinished(1000);
}

void NetworkManager::configureFirewall(bool block)
{
    if (block) {
        blockAllTraffic();
    } else {
        allowAllTraffic();
    }
}
