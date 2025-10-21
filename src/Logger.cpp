#include "Logger.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QMutexLocker>

const int Logger::DEFAULT_MAX_LOG_ENTRIES = 1000;
const QString Logger::DEFAULT_HIDDEN_LOG_DIR = "/home/test/A3Guard/.a3guard-data/logs";

Logger::Logger(std::shared_ptr<ConfigManager> config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_logFile(nullptr)
    , m_logStream(nullptr)
    , m_rotationTimer(new QTimer(this))
    , m_verbose(false)
    , m_initialized(false)
    , m_maxLogSize(DEFAULT_MAX_LOG_SIZE_MB * 1024 * 1024)
    , m_maxLogEntries(DEFAULT_MAX_LOG_ENTRIES)
{
    // Use hidden directory for logs
    m_logDir = DEFAULT_HIDDEN_LOG_DIR;
    QDir().mkpath(m_logDir);
    connect(m_rotationTimer, &QTimer::timeout, this, &Logger::checkLogRotation);
}

Logger::~Logger()
{
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
    delete m_logStream;
}

bool Logger::initialize()
{
    m_currentLogFile = QString("%1/a3guard.log").arg(m_logDir);
    
    // Create/open log file
    m_logFile = new QFile(m_currentLogFile);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << m_currentLogFile;
        return false;
    }
    
    m_logStream = new QTextStream(m_logFile);
    m_logStream->setAutoDetectUnicode(true);
    
    // Load existing logs from file
    loadExistingLogs();
    
    // Start rotation timer (check every hour)
    m_rotationTimer->start(3600000);
    
    m_initialized = true;
    return true;
}

void Logger::setSecurityManager(std::shared_ptr<SecurityManager> security)
{
    m_security = security;
}

void Logger::log(const MonitoringEvent& event)
{
    QString entry = formatLogEntry(event);
    writeLogEntry(entry);
}

void Logger::logInfo(const QString& message)
{
    QString entry = formatLogEntry("INFO", message);
    writeLogEntry(entry);
}

void Logger::logWarning(const QString& message)
{
    QString entry = formatLogEntry("WARNING", message);
    writeLogEntry(entry);
}

void Logger::logError(const QString& message)
{
    QString entry = formatLogEntry("ERROR", message);
    writeLogEntry(entry);
}

void Logger::logDebug(const QString& message)
{
    if (m_verbose) {
        QString entry = formatLogEntry("DEBUG", message);
        writeLogEntry(entry);
    }
}

QStringList Logger::getRecentLogs(int count) const
{
    QMutexLocker locker(&m_logMutex);
    return m_recentLogs.mid(0, qMin(count, m_recentLogs.size()));
}

QStringList Logger::getLogsFromLastHours(int hours) const
{
    QMutexLocker locker(&m_logMutex);
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-hours * 3600);
    QStringList recentLogs;
    
    for (const QString& logEntry : m_recentLogs) {
        // Extract timestamp from log entry format: [YYYY-MM-DDTHH:mm:ss] LEVEL: message
        if (logEntry.startsWith("[")) {
            int endBracket = logEntry.indexOf("]");
            if (endBracket > 0) {
                QString timestampStr = logEntry.mid(1, endBracket - 1);
                QDateTime logTime = QDateTime::fromString(timestampStr, Qt::ISODate);
                
                if (logTime.isValid() && logTime >= cutoffTime) {
                    recentLogs.append(logEntry);
                }
            }
        }
    }
    
    return recentLogs;
}

QStringList Logger::getAllLogs() const
{
    return getRecentLogs(m_maxLogEntries);
}

bool Logger::exportLogs(const QString& outputPath) const
{
    QFile exportFile(outputPath);
    if (!exportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&exportFile);
    for (const QString& log : getAllLogs()) {
        out << log << "\n";
    }
    
    return true;
}

void Logger::rotateLogs() { /* Basic stub */ }
qint64 Logger::getLogSize() const { return m_logFile ? m_logFile->size() : 0; }
int Logger::getLogCount() const { return m_recentLogs.size(); }

void Logger::checkLogRotation() { /* Basic stub */ }

void Logger::writeLogEntry(const QString& entry)
{
    QMutexLocker locker(&m_logMutex);
    
    // Add to in-memory list
    m_recentLogs.prepend(entry);
    if (m_recentLogs.size() > m_maxLogEntries) {
        m_recentLogs.removeLast();
    }
    
    // Write to disk if file is open
    if (m_logStream) {
        *m_logStream << entry << Qt::endl;
        m_logStream->flush();
    }
    
    emit newLogEntry(entry);
}

QString Logger::formatLogEntry(const MonitoringEvent& event) const
{
    return QString("[%1] %2: %3 - %4")
           .arg(event.timestamp.toString(Qt::ISODate))
           .arg(alertLevelToString(event.level))
           .arg(eventTypeToString(event.type))
           .arg(event.description);
}

QString Logger::formatLogEntry(const QString& level, const QString& message) const
{
    return QString("[%1] %2: %3")
           .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
           .arg(level)
           .arg(message);
}

bool Logger::rotateLogFile() { return true; }

void Logger::loadExistingLogs()
{
    QFile logFile(m_currentLogFile);
    if (!logFile.exists()) return;
    
    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&logFile);
        QStringList existingLogs;
        
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                existingLogs.prepend(line); // Add to front to maintain chronological order
            }
        }
        logFile.close();
        
        QMutexLocker locker(&m_logMutex);
        // Keep only the most recent entries
        if (existingLogs.size() > m_maxLogEntries) {
            existingLogs = existingLogs.mid(0, m_maxLogEntries);
        }
        m_recentLogs = existingLogs;
    }
}
