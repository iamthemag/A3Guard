#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <memory>

#include "Common.h"

class ConfigManager;
class PrivilegeDialog;
class SecurityManager;
class Logger;
class MonitoringEngine;
class AlertManager;
class NetworkManager;
class ResourceMonitor;
class UpdateChecker;
// class KeyLogger; // Temporarily disabled

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<ConfigManager> config,
                       std::shared_ptr<SecurityManager> security,
                       std::shared_ptr<Logger> logger,
                       QWidget *parent = nullptr);
    ~MainWindow();
    
    bool shouldShowWindow() const { return m_shouldShowWindow; }

private:
    bool m_shouldShowWindow;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void toggleMonitoring();
    void updateUI();
    void showAlert(const QString& message, AlertLevel level);
    void onMonitoringStateChanged(MonitoringState state);
    void onViolationDetected(const MonitoringEvent& event);
    void onViolationDetected(const QString& violation);
    void onClipboardChanged();
    void onClipboardActivity(const QString& activity);
    void onKeystrokeDetected(const QString& keystroke);
    void onUsbDeviceDetected(const QString& deviceDetails);
    void captureConsoleLogs();
    void showAbout();
    void showSettings();
    void exportLogs();
    void generateReport();
    void systemTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void checkPrivileges();
    void checkForUpdates();
    void onUpdateCheckStarted();
    void onUpdateAvailable(QString latestVersion, QString downloadUrl, QString releaseNotes);
    void onUpdateCheckFailed(QString errorMessage);
    void onDownloadStarted(QString fileName);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(QString filePath);
    void onDownloadFailed(QString errorMessage);

private:
    void setupUI();
    void setupMenuBar();
    void setupSystemTray();
    void setupStatusBar();
    void applyModernStyling();
    
    void createDashboardTab();
    void createClipboardTab();
    void createKeyloggerTab();
    void createUsbTab();
    void createLogsTab();
    void createStatisticsTab();
    
    void updateDashboard();
    void updateClipboardTabDisplay();
    void updateKeyloggerDisplay();
    void updateUsbDisplay();
    void updateLogs();
    void updateStatistics();
    
    void setMonitoringState(MonitoringState state);
    QString formatDuration(const QDateTime& start, const QDateTime& end = QDateTime()) const;
    QString formatBytes(qint64 bytes) const;
    QString formatTimeAMPM(const QDateTime& dateTime) const;
    QString formatDateTimeAMPM(const QDateTime& dateTime) const;
    void updateClipboardDisplay();
    void updateViolationsDisplay();
    void updateSessionHistoryDisplay();
    void saveLastCloseTime();
    void loadLastCloseTime();
    void updateViolationIndicator(bool hasViolation);
    bool isTerminalOrSystemData(const QString& content);
    QString detectClipboardOperation(const QString& content);
    void requestPrivilegesForFeatures();
    QString generateDetailedReport();
    void saveReportToFile(const QString& report);
    
    // Core components
    std::shared_ptr<ConfigManager> m_config;
    std::shared_ptr<SecurityManager> m_security;
    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<MonitoringEngine> m_monitoring;
    std::shared_ptr<AlertManager> m_alertManager;
    std::shared_ptr<NetworkManager> m_networkManager;
    std::shared_ptr<ResourceMonitor> m_resourceMonitor;
    UpdateChecker* m_updateChecker;
    // std::shared_ptr<KeyLogger> m_keyLogger; // Temporarily disabled
    
    // UI Components
    QTabWidget* m_tabWidget;
    QTimer* m_updateTimer;
    QTimer* m_logCaptureTimer;
    QSystemTrayIcon* m_systemTray;
    QMenu* m_trayMenu;
    
    // Violation status indicator (top of window)
    QFrame* m_violationStatusFrame;
    QLabel* m_violationStatusLabel;
    QLabel* m_lastCloseTimeLabel;
    
    // Dashboard tab
    QWidget* m_dashboardTab;
    QPushButton* m_toggleButton;
    QLabel* m_statusLabel;
    QLabel* m_startTimeLabel;
    QLabel* m_durationLabel;
    QLabel* m_airplaneModeLabel;
    QLabel* m_violationsLabel;
    QLabel* m_lastStopTimeLabel;
    QTextEdit* m_clipboardActivityDisplay;
    QTextEdit* m_violationsDisplay;
    QTextEdit* m_sessionHistoryDisplay;
    QProgressBar* m_cpuUsage;
    QProgressBar* m_memoryUsage;
    QGroupBox* m_statusGroup;
    QGroupBox* m_resourceGroup;
    QGroupBox* m_securityGroup;
    QGroupBox* m_clipboardGroup;
    QGroupBox* m_violationsGroup;
    QGroupBox* m_sessionHistoryGroup;
    
    // Logs tab
    QWidget* m_logsTab;
    QTextEdit* m_logsDisplay;
    QPushButton* m_exportLogsButton;
    QLabel* m_logsCountLabel;
    
    // Clipboard tab
    QWidget* m_clipboardTab;
    QTextEdit* m_clipboardDataDisplay;
    QLabel* m_clipboardCountLabel;
    
    // Keylogger tab
    QWidget* m_keyloggerTab;
    QTextEdit* m_keyloggerDisplay;
    QLabel* m_keyloggerCountLabel;
    
    // USB tab
    QWidget* m_usbTab;
    QTextEdit* m_usbDisplay;
    QLabel* m_usbCountLabel;
    
    // Statistics tab
    QWidget* m_statisticsTab;
    QLabel* m_totalSessionsLabel;
    QLabel* m_totalViolationsLabel;
    QLabel* m_totalAppsLabel;
    QLabel* m_averageResourceLabel;
    QPushButton* m_generateReportButton;
    
    // Menu actions
    QAction* m_aboutAction;
    
    // State
    MonitoringState m_currentState;
    QDateTime m_sessionStartTime;
    QDateTime m_sessionStopTime;
    SessionSummary m_currentSession;
    QStringList m_recentLogs;
    
    // Activity tracking
    QStringList m_recentClipboardActivity;
    QStringList m_recentClipboardData;
    QStringList m_recentKeyloggerData;
    QStringList m_recentUsbActivity;
    QStringList m_recentViolations;
    QList<QPair<QDateTime, bool>> m_sessionHistory; // bool: true=start, false=stop
    int m_violationCount;
    QDateTime m_lastCloseTime;
    
    // Constants
    static const int MAX_LOG_DISPLAY;
};

#endif // MAINWINDOW_H