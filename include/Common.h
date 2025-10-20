#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QDateTime>
#include <QDebug>
#include <memory>

// Version information
#define A3GUARD_VERSION "1.0.0"
#define A3GUARD_NAME "A3Guard"

// File extensions
#define A3_LOG_EXT ".a3log"
#define A3_SCREENSHOT_EXT ".a3img"  
#define A3_BACKUP_EXT ".a3bak"
#define A3_INTEGRITY_EXT ".a3int"

// Default paths
#define DEFAULT_CONFIG_PATH "/etc/a3guard/a3guard.conf"
#define DEFAULT_LOG_DIR "/var/log/a3guard"
#define DEFAULT_DATA_DIR "/var/lib/a3guard"
#define DEFAULT_SCREENSHOT_DIR "/var/lib/a3guard/screenshots"
#define DEFAULT_BACKUP_DIR "/var/lib/a3guard/backup"
#define DEFAULT_INTEGRITY_DIR "/var/lib/a3guard/integrity"
#define DEFAULT_KEY_FILE "/etc/a3guard/a3guard.key"

// Monitoring intervals (milliseconds)
#define DEFAULT_SCREENSHOT_INTERVAL 120000    // 2 minutes
#define DEFAULT_NETWORK_CHECK_INTERVAL 30000  // 30 seconds
#define DEFAULT_APP_MONITOR_INTERVAL 5000     // 5 seconds
#define DEFAULT_CLIPBOARD_INTERVAL 2000       // 2 seconds
#define DEFAULT_INTEGRITY_CHECK_INTERVAL 60000 // 1 minute
#define DEFAULT_RESOURCE_CHECK_INTERVAL 30000  // 30 seconds

// Resource limits
#define DEFAULT_MAX_CPU_USAGE 10.0      // 10%
#define DEFAULT_MAX_MEMORY_MB 100       // 100 MB
#define DEFAULT_MAX_LOG_SIZE_MB 10      // 10 MB

// Event types
enum class EventType {
    MONITORING_START,
    MONITORING_STOP,
    AIRPLANE_MODE_ON,
    AIRPLANE_MODE_OFF,
    USB_INSERTED,
    USB_REMOVED,
    NETWORK_VIOLATION,
    APP_LAUNCHED,
    APP_CLOSED,
    WINDOW_FOCUS_CHANGED,
    CLIPBOARD_CHANGED,
    SCREENSHOT_TAKEN,
    INTEGRITY_VIOLATION,
    RESOURCE_VIOLATION,
    SYSTEM_ERROR
};

// Alert levels
enum class AlertLevel {
    INFO,
    WARNING,
    CRITICAL,
    VIOLATION
};

// Monitoring state
enum class MonitoringState {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

// Network interface types
enum class NetworkInterface {
    WIFI,
    BLUETOOTH,
    ETHERNET,
    ALL
};

// Event structure
struct MonitoringEvent {
    EventType type;
    QDateTime timestamp;
    QString description;
    QString details;
    AlertLevel level;
    
    MonitoringEvent() : timestamp(QDateTime::currentDateTime()), level(AlertLevel::INFO) {}
    
    MonitoringEvent(EventType t, const QString& desc, AlertLevel lvl = AlertLevel::INFO)
        : type(t), timestamp(QDateTime::currentDateTime()), description(desc), level(lvl) {}
};

// Resource usage structure
struct ResourceUsage {
    double cpuPercent;
    double memoryMB;
    QDateTime timestamp;
    
    ResourceUsage() : cpuPercent(0.0), memoryMB(0.0), timestamp(QDateTime::currentDateTime()) {}
};

// Session summary structure
struct SessionSummary {
    QDateTime startTime;
    QDateTime endTime;
    int totalScreenshots;
    int networkViolations;
    int usbInsertions;
    int appChanges;
    int clipboardChanges;
    int integrityViolations;
    QStringList applications;
    ResourceUsage maxResourceUsage;
    
    SessionSummary() : totalScreenshots(0), networkViolations(0), usbInsertions(0), 
                      appChanges(0), clipboardChanges(0), integrityViolations(0) {}
};

// Utility macros
#define LOG_DEBUG(msg) qDebug() << "[DEBUG]" << QDateTime::currentDateTime().toString(Qt::ISODate) << msg
#define LOG_INFO(msg) qDebug() << "[INFO]" << QDateTime::currentDateTime().toString(Qt::ISODate) << msg
#define LOG_WARNING(msg) qWarning() << "[WARNING]" << QDateTime::currentDateTime().toString(Qt::ISODate) << msg
#define LOG_ERROR(msg) qCritical() << "[ERROR]" << QDateTime::currentDateTime().toString(Qt::ISODate) << msg

// Helper functions
inline QString eventTypeToString(EventType type) {
    switch(type) {
        case EventType::MONITORING_START: return "monitoring_start";
        case EventType::MONITORING_STOP: return "monitoring_stop";
        case EventType::AIRPLANE_MODE_ON: return "airplane_mode_on";
        case EventType::AIRPLANE_MODE_OFF: return "airplane_mode_off";
        case EventType::USB_INSERTED: return "usb_inserted";
        case EventType::USB_REMOVED: return "usb_removed";
        case EventType::NETWORK_VIOLATION: return "network_violation";
        case EventType::APP_LAUNCHED: return "app_launched";
        case EventType::APP_CLOSED: return "app_closed";
        case EventType::WINDOW_FOCUS_CHANGED: return "window_focus_changed";
        case EventType::CLIPBOARD_CHANGED: return "clipboard_changed";
        case EventType::SCREENSHOT_TAKEN: return "screenshot_taken";
        case EventType::INTEGRITY_VIOLATION: return "integrity_violation";
        case EventType::RESOURCE_VIOLATION: return "resource_violation";
        case EventType::SYSTEM_ERROR: return "system_error";
        default: return "unknown";
    }
}

inline QString alertLevelToString(AlertLevel level) {
    switch(level) {
        case AlertLevel::INFO: return "info";
        case AlertLevel::WARNING: return "warning";
        case AlertLevel::CRITICAL: return "critical";
        case AlertLevel::VIOLATION: return "violation";
        default: return "unknown";
    }
}

#endif // COMMON_H