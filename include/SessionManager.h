#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include <memory>
#include "Common.h"

class SessionManager : public QObject {
    Q_OBJECT

public:
    explicit SessionManager(QObject* parent = nullptr);
    ~SessionManager();

    bool startSession(const QString& sessionId, const QString& examTitle);
    bool endSession();
    bool pauseSession();
    bool resumeSession();
    
    bool isSessionActive() const;
    bool isSessionPaused() const;
    
    QString getCurrentSessionId() const;
    QString getCurrentExamTitle() const;
    QDateTime getSessionStartTime() const;
    QDateTime getSessionEndTime() const;
    qint64 getSessionDuration() const;
    
    bool saveSessionData();
    bool loadSessionData(const QString& sessionId);

signals:
    void sessionStarted(const QString& sessionId, const QString& examTitle);
    void sessionEnded(const QString& sessionId, qint64 duration);
    void sessionPaused(const QString& sessionId);
    void sessionResumed(const QString& sessionId);
    void sessionDataSaved(const QString& sessionId);

private:
    QString m_sessionId;
    QString m_examTitle;
    QDateTime m_startTime;
    QDateTime m_endTime;
    QDateTime m_pauseTime;
    qint64 m_pausedDuration;
    bool m_sessionActive;
    bool m_sessionPaused;
    
    QString getSessionFilePath(const QString& sessionId) const;
};