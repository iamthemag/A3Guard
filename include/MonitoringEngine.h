#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include "Common.h"
#include "ConfigManager.h"

class MonitoringEngine : public QObject {
    Q_OBJECT

public:
    explicit MonitoringEngine(std::shared_ptr<ConfigManager> configManager, QObject* parent = nullptr);
    ~MonitoringEngine();

    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

signals:
    void suspiciousActivityDetected(const QString& description);
    void applicationStarted(const QString& application);
    void windowChanged(const QString& windowTitle);
    void clipboardChanged();
    void usbDeviceDetected(const QString& device);
    void keystrokeDetected(const QString& keyInfo);

private slots:
    void checkApplications();
    void checkWindows();
    void checkClipboard();
    void checkUSBDevices();
    void checkKeystrokes();

private:
    bool m_monitoring;
    std::unique_ptr<QTimer> m_appTimer;
    std::unique_ptr<QTimer> m_windowTimer;
    std::unique_ptr<QTimer> m_clipboardTimer;
    std::unique_ptr<QTimer> m_usbTimer;
    std::unique_ptr<QTimer> m_keystrokeTimer;
    
    QString m_lastClipboard;
    QString m_lastWindow;
    int m_keystrokeCount;
    
    std::shared_ptr<ConfigManager> m_configManager;
    
    void unmountUSBDevice(const QString& device);
    bool isApplicationWhitelisted(const QString& appName) const;
    bool isWindowWhitelisted(const QString& windowTitle) const;
    bool isUrlWhitelisted(const QString& url) const;
};