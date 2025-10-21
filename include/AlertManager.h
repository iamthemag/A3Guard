#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <memory>
#include "Common.h"

class AlertManager : public QObject {
    Q_OBJECT

public:
    explicit AlertManager(QObject* parent = nullptr);
    ~AlertManager();

    void showAlert(const QString& title, const QString& message, AlertType type = AlertType::Warning);
    void showTrayNotification(const QString& title, const QString& message);
    void playAlertSound(AlertType type = AlertType::Warning);
    
    void setAlertsEnabled(bool enabled);
    bool areAlertsEnabled() const;
    
    void setSoundEnabled(bool enabled);
    bool isSoundEnabled() const;

signals:
    void alertShown(const QString& title, const QString& message, AlertType type);
    void userResponse(bool accepted);

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    bool m_alertsEnabled;
    bool m_soundEnabled;
    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    
    QMessageBox::Icon getMessageBoxIcon(AlertType type) const;
    QSystemTrayIcon::MessageIcon getTrayIcon(AlertType type) const;
};