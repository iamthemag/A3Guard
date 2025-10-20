#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QDir>
#include <memory>
#include "Common.h"

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(const QString& configPath = DEFAULT_CONFIG_PATH, 
                          QObject *parent = nullptr);
    ~ConfigManager();

    // Initialization
    bool initialize();
    bool isValid() const { return m_isValid; }

    // Configuration access
    QString getString(const QString& section, const QString& key, 
                     const QString& defaultValue = QString()) const;
    int getInt(const QString& section, const QString& key, int defaultValue = 0) const;
    bool getBool(const QString& section, const QString& key, bool defaultValue = false) const;
    double getDouble(const QString& section, const QString& key, double defaultValue = 0.0) const;
    QStringList getStringList(const QString& section, const QString& key, 
                             const QStringList& defaultValue = QStringList()) const;

    // Path configurations
    QString getLogDir() const;
    QString getDataDir() const;
    QString getScreenshotDir() const;
    QString getBackupDir() const;
    QString getIntegrityDir() const;
    QString getKeyFile() const;

    // File extensions
    QString getLogExtension() const;
    QString getScreenshotExtension() const;
    QString getBackupExtension() const;
    QString getIntegrityExtension() const;

    // Monitoring intervals
    int getScreenshotInterval() const;
    int getNetworkCheckInterval() const;
    int getAppMonitorInterval() const;
    int getClipboardInterval() const;
    int getIntegrityCheckInterval() const;
    int getResourceCheckInterval() const;

    // Resource limits
    double getMaxCpuUsage() const;
    int getMaxMemoryMB() const;
    int getMaxLogSizeMB() const;

    // Network configuration
    QStringList getDisabledInterfaces() const;
    QStringList getAllowedInterfaces() const;
    bool getBlockAllTraffic() const;

    // Security configuration
    bool getIntegrityCheckEnabled() const;

    // Alert configuration
    bool getVisualAlertsEnabled() const;
    bool getAudioAlertsEnabled() const;
    int getAlertTimeout() const;

    // UI configuration
    int getUiUpdateInterval() const;
    int getMaxLogDisplay() const;
    int getThumbnailSize() const;

    // Resource monitoring
    bool getResourceMonitoringEnabled() const;

    // Directory management
    bool createDirectories() const;
    bool validatePaths() const;

    // Configuration updates
    void setValue(const QString& section, const QString& key, const QVariant& value);
    bool saveConfiguration();
    void reloadConfiguration();

signals:
    void configurationChanged();
    void pathError(const QString& path, const QString& error);

private:
    void loadDefaults();
    bool createDirectory(const QString& path, int permissions = 0755) const;
    QString resolvePath(const QString& path) const;

    QSettings* m_settings;
    QString m_configPath;
    bool m_isValid;

    // Cached values for performance
    mutable QString m_logDir;
    mutable QString m_dataDir;
    mutable QString m_screenshotDir;
    mutable QString m_backupDir;
    mutable QString m_integrityDir;
    mutable QString m_keyFile;
};

#endif // CONFIGMANAGER_H