#include "AlertManager.h"

AlertManager::AlertManager(QObject* parent)
    : QObject(parent), m_alertsEnabled(true), m_soundEnabled(true)
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon = std::make_unique<QSystemTrayIcon>(this);
        connect(m_trayIcon.get(), &QSystemTrayIcon::activated,
                this, &AlertManager::onTrayIconActivated);
    }
}

AlertManager::~AlertManager() = default;

void AlertManager::showAlert(const QString& title, const QString& message, AlertType type)
{
    if (!m_alertsEnabled) return;
    
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(getMessageBoxIcon(type));
    msgBox.exec();
    
    emit alertShown(title, message, type);
}

void AlertManager::showTrayNotification(const QString& title, const QString& message)
{
    if (m_trayIcon) {
        m_trayIcon->showMessage(title, message);
    }
}

void AlertManager::playAlertSound(AlertType type)
{
    if (m_soundEnabled) {
        // Basic implementation - would use system sound APIs
    }
}

void AlertManager::setAlertsEnabled(bool enabled) { m_alertsEnabled = enabled; }
bool AlertManager::areAlertsEnabled() const { return m_alertsEnabled; }
void AlertManager::setSoundEnabled(bool enabled) { m_soundEnabled = enabled; }
bool AlertManager::isSoundEnabled() const { return m_soundEnabled; }

void AlertManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    Q_UNUSED(reason)
}

QMessageBox::Icon AlertManager::getMessageBoxIcon(AlertType type) const
{
    switch (type) {
        case AlertType::Info: return QMessageBox::Information;
        case AlertType::Warning: return QMessageBox::Warning;
        case AlertType::Critical: return QMessageBox::Critical;
        case AlertType::Error: return QMessageBox::Critical;
        default: return QMessageBox::Information;
    }
}

QSystemTrayIcon::MessageIcon AlertManager::getTrayIcon(AlertType type) const
{
    switch (type) {
        case AlertType::Info: return QSystemTrayIcon::Information;
        case AlertType::Warning: return QSystemTrayIcon::Warning;
        case AlertType::Critical: return QSystemTrayIcon::Critical;
        case AlertType::Error: return QSystemTrayIcon::Critical;
        default: return QSystemTrayIcon::Information;
    }
}