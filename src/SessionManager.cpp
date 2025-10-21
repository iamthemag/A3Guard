#include "SessionManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
    , m_pausedDuration(0)
    , m_sessionActive(false)
    , m_sessionPaused(false)
{
}

SessionManager::~SessionManager()
{
    if (m_sessionActive) {
        endSession();
    }
}

bool SessionManager::startSession(const QString& sessionId, const QString& examTitle)
{
    if (m_sessionActive) return false;
    
    m_sessionId = sessionId;
    m_examTitle = examTitle;
    m_startTime = QDateTime::currentDateTime();
    m_pausedDuration = 0;
    m_sessionActive = true;
    m_sessionPaused = false;
    
    emit sessionStarted(sessionId, examTitle);
    return true;
}

bool SessionManager::endSession()
{
    if (!m_sessionActive) return false;
    
    m_endTime = QDateTime::currentDateTime();
    m_sessionActive = false;
    m_sessionPaused = false;
    
    qint64 duration = getSessionDuration();
    emit sessionEnded(m_sessionId, duration);
    
    saveSessionData();
    return true;
}

bool SessionManager::pauseSession()
{
    if (!m_sessionActive || m_sessionPaused) return false;
    
    m_pauseTime = QDateTime::currentDateTime();
    m_sessionPaused = true;
    
    emit sessionPaused(m_sessionId);
    return true;
}

bool SessionManager::resumeSession()
{
    if (!m_sessionActive || !m_sessionPaused) return false;
    
    m_pausedDuration += m_pauseTime.msecsTo(QDateTime::currentDateTime());
    m_sessionPaused = false;
    
    emit sessionResumed(m_sessionId);
    return true;
}

bool SessionManager::isSessionActive() const { return m_sessionActive; }
bool SessionManager::isSessionPaused() const { return m_sessionPaused; }
QString SessionManager::getCurrentSessionId() const { return m_sessionId; }
QString SessionManager::getCurrentExamTitle() const { return m_examTitle; }
QDateTime SessionManager::getSessionStartTime() const { return m_startTime; }
QDateTime SessionManager::getSessionEndTime() const { return m_endTime; }

qint64 SessionManager::getSessionDuration() const
{
    if (!m_sessionActive) {
        return m_startTime.msecsTo(m_endTime) - m_pausedDuration;
    }
    
    QDateTime currentTime = m_sessionPaused ? m_pauseTime : QDateTime::currentDateTime();
    return m_startTime.msecsTo(currentTime) - m_pausedDuration;
}

bool SessionManager::saveSessionData()
{
    QDir().mkpath(DEFAULT_DATA_DIR);
    QString filePath = getSessionFilePath(m_sessionId);
    
    QJsonObject session;
    session["id"] = m_sessionId;
    session["examTitle"] = m_examTitle;
    session["startTime"] = m_startTime.toString(Qt::ISODate);
    session["endTime"] = m_endTime.toString(Qt::ISODate);
    session["duration"] = getSessionDuration();
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(session).toJson());
        emit sessionDataSaved(m_sessionId);
        return true;
    }
    
    return false;
}

bool SessionManager::loadSessionData(const QString& sessionId)
{
    QString filePath = getSessionFilePath(sessionId);
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject session = doc.object();
        
        m_sessionId = session["id"].toString();
        m_examTitle = session["examTitle"].toString();
        m_startTime = QDateTime::fromString(session["startTime"].toString(), Qt::ISODate);
        m_endTime = QDateTime::fromString(session["endTime"].toString(), Qt::ISODate);
        
        return true;
    }
    
    return false;
}

QString SessionManager::getSessionFilePath(const QString& sessionId) const
{
    return QString("%1/session_%2.json").arg(DEFAULT_DATA_DIR).arg(sessionId);
}