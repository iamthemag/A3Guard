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
class SecurityManager;
class Logger;
class MonitoringEngine;
class AlertManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<ConfigManager> config,
                       std::shared_ptr<SecurityManager> security,
                       std::shared_ptr<Logger> logger,
                       QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void toggleMonitoring();
    void updateUI();
    void showAlert(const QString& message, AlertLevel level);
    void onMonitoringStateChanged(MonitoringState state);
    void onViolationDetected(const MonitoringEvent& event);
    void showAbout();
    void showSettings();
    void exportLogs();
    void generateReport();
    void systemTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupUI();
    void setupMenuBar();
    void setupSystemTray();
    void setupStatusBar();
    
    void createDashboardTab();
    void createLogsTab();
    void createScreenshotsTab();
    void createStatisticsTab();
    
    void updateDashboard();
    void updateLogs();
    void updateScreenshots();
    void updateStatistics();
    
    void setMonitoringState(MonitoringState state);
    QString formatDuration(const QDateTime& start, const QDateTime& end = QDateTime()) const;
    QString formatBytes(qint64 bytes) const;
    
    // Core components
    std::shared_ptr<ConfigManager> m_config;
    std::shared_ptr<SecurityManager> m_security;
    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<MonitoringEngine> m_monitoring;
    std::shared_ptr<AlertManager> m_alertManager;
    
    // UI Components
    QTabWidget* m_tabWidget;
    QTimer* m_updateTimer;
    QSystemTrayIcon* m_systemTray;
    QMenu* m_trayMenu;
    
    // Dashboard tab
    QWidget* m_dashboardTab;
    QPushButton* m_toggleButton;
    QLabel* m_statusLabel;
    QLabel* m_startTimeLabel;
    QLabel* m_durationLabel;
    QLabel* m_airplaneModeLabel;
    QLabel* m_violationsLabel;
    QProgressBar* m_cpuUsage;
    QProgressBar* m_memoryUsage;
    QGroupBox* m_statusGroup;
    QGroupBox* m_resourceGroup;
    QGroupBox* m_securityGroup;
    
    // Logs tab
    QWidget* m_logsTab;
    QTextEdit* m_logsDisplay;
    QPushButton* m_clearLogsButton;
    QPushButton* m_exportLogsButton;
    QLabel* m_logsCountLabel;
    
    // Screenshots tab
    QWidget* m_screenshotsTab;
    QScrollArea* m_screenshotsArea;
    QWidget* m_screenshotsContainer;
    QGridLayout* m_screenshotsLayout;
    QLabel* m_screenshotsCountLabel;
    QPushButton* m_clearScreenshotsButton;
    
    // Statistics tab
    QWidget* m_statisticsTab;
    QLabel* m_totalSessionsLabel;
    QLabel* m_totalViolationsLabel;
    QLabel* m_totalScreenshotsLabel;
    QLabel* m_totalAppsLabel;
    QLabel* m_averageResourceLabel;
    QPushButton* m_generateReportButton;
    
    // Menu actions
    QAction* m_startAction;
    QAction* m_stopAction;
    QAction* m_settingsAction;
    QAction* m_aboutAction;
    QAction* m_exitAction;
    
    // State
    MonitoringState m_currentState;
    QDateTime m_sessionStartTime;
    SessionSummary m_currentSession;
    QStringList m_recentLogs;
    
    // Constants
    static const int MAX_LOG_DISPLAY;
    static const int SCREENSHOT_COLUMNS;
    static const int THUMBNAIL_SIZE;
};

#endif // MAINWINDOW_H