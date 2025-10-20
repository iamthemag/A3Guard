#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QMutex>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <memory>
#include "Common.h"

class ConfigManager;
class SecurityManager;

class Logger : public QObject
{
    Q_OBJECT

public:
    explicit Logger(std::shared_ptr<ConfigManager> config, QObject *parent = nullptr);
    ~Logger();

    bool initialize();
    void setSecurityManager(std::shared_ptr<SecurityManager> security);
    void setVerbose(bool verbose) { m_verbose = verbose; }

    // Logging methods
    void log(const MonitoringEvent& event);
    void logInfo(const QString& message);
    void logWarning(const QString& message);
    void logError(const QString& message);
    void logDebug(const QString& message);

    // Log retrieval
    QStringList getRecentLogs(int count = 100) const;
    QStringList getAllLogs() const;
    bool exportLogs(const QString& outputPath) const;

    // Log management
    void clearLogs();
    void rotateLogs();
    qint64 getLogSize() const;
    int getLogCount() const;

signals:
    void newLogEntry(const QString& entry);
    void logRotated();

private slots:
    void checkLogRotation();

private:
    void writeLogEntry(const QString& entry);
    QString formatLogEntry(const MonitoringEvent& event) const;
    QString formatLogEntry(const QString& level, const QString& message) const;
    bool rotateLogFile();
    void loadExistingLogs();

    std::shared_ptr<ConfigManager> m_config;
    std::shared_ptr<SecurityManager> m_security;
    
    QString m_logDir;
    QString m_currentLogFile;
    QFile* m_logFile;
    QTextStream* m_logStream;
    QTimer* m_rotationTimer;
    
    mutable QMutex m_logMutex;
    QStringList m_recentLogs;
    
    bool m_verbose;
    bool m_initialized;
    qint64 m_maxLogSize;
    int m_maxLogEntries;
    
    static const int DEFAULT_MAX_LOG_ENTRIES;
};

#endif // LOGGER_H