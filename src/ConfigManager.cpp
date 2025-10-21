#include "ConfigManager.h"
#include <QSettings>
#include <QDir>

ConfigManager::ConfigManager(const QString& configPath, QObject *parent)
    : QObject(parent)
    , m_configPath(configPath)
    , m_isValid(false)
    , m_settings(nullptr)
{
}

ConfigManager::~ConfigManager()
{
    delete m_settings;
}

bool ConfigManager::initialize()
{
    m_settings = new QSettings(m_configPath, QSettings::IniFormat);
    loadDefaults();
    m_isValid = true;
    return true;
}

QString ConfigManager::getString(const QString& section, const QString& key, const QString& defaultValue) const
{
    if (!m_settings) return defaultValue;
    return m_settings->value(section + "/" + key, defaultValue).toString();
}

int ConfigManager::getInt(const QString& section, const QString& key, int defaultValue) const
{
    if (!m_settings) return defaultValue;
    return m_settings->value(section + "/" + key, defaultValue).toInt();
}

bool ConfigManager::getBool(const QString& section, const QString& key, bool defaultValue) const
{
    if (!m_settings) return defaultValue;
    return m_settings->value(section + "/" + key, defaultValue).toBool();
}

double ConfigManager::getDouble(const QString& section, const QString& key, double defaultValue) const
{
    if (!m_settings) return defaultValue;
    return m_settings->value(section + "/" + key, defaultValue).toDouble();
}

QStringList ConfigManager::getStringList(const QString& section, const QString& key, const QStringList& defaultValue) const
{
    if (!m_settings) return defaultValue;
    return m_settings->value(section + "/" + key, defaultValue).toStringList();
}

QString ConfigManager::getLogDir() const
{
    return getString("paths", "log_dir", DEFAULT_LOG_DIR);
}

QString ConfigManager::getDataDir() const
{
    return getString("paths", "data_dir", DEFAULT_DATA_DIR);
}

QString ConfigManager::getScreenshotDir() const
{
    return getString("paths", "screenshot_dir", DEFAULT_SCREENSHOT_DIR);
}

QString ConfigManager::getBackupDir() const
{
    return getString("paths", "backup_dir", DEFAULT_BACKUP_DIR);
}

QString ConfigManager::getIntegrityDir() const
{
    return getString("paths", "integrity_dir", DEFAULT_INTEGRITY_DIR);
}

QString ConfigManager::getKeyFile() const
{
    return getString("paths", "key_file", DEFAULT_KEY_FILE);
}

QString ConfigManager::getLogExtension() const
{
    return getString("files", "log_extension", A3_LOG_EXT);
}

QString ConfigManager::getScreenshotExtension() const
{
    return getString("files", "screenshot_extension", A3_SCREENSHOT_EXT);
}

QString ConfigManager::getBackupExtension() const
{
    return getString("files", "backup_extension", A3_BACKUP_EXT);
}

QString ConfigManager::getIntegrityExtension() const
{
    return getString("files", "integrity_extension", A3_INTEGRITY_EXT);
}

int ConfigManager::getScreenshotInterval() const
{
    return getInt("monitoring", "screenshot_interval", DEFAULT_SCREENSHOT_INTERVAL);
}

int ConfigManager::getNetworkCheckInterval() const
{
    return getInt("monitoring", "network_check_interval", DEFAULT_NETWORK_CHECK_INTERVAL);
}

int ConfigManager::getAppMonitorInterval() const
{
    return getInt("monitoring", "app_monitor_interval", DEFAULT_APP_MONITOR_INTERVAL);
}

int ConfigManager::getClipboardInterval() const
{
    return getInt("monitoring", "clipboard_interval", DEFAULT_CLIPBOARD_INTERVAL);
}

int ConfigManager::getIntegrityCheckInterval() const
{
    return getInt("monitoring", "integrity_check_interval", DEFAULT_INTEGRITY_CHECK_INTERVAL);
}

int ConfigManager::getResourceCheckInterval() const
{
    return getInt("monitoring", "resource_check_interval", DEFAULT_RESOURCE_CHECK_INTERVAL);
}

double ConfigManager::getMaxCpuUsage() const
{
    return getDouble("resources", "max_cpu_usage", DEFAULT_MAX_CPU_USAGE);
}

int ConfigManager::getMaxMemoryMB() const
{
    return getInt("resources", "max_memory_mb", DEFAULT_MAX_MEMORY_MB);
}

int ConfigManager::getMaxLogSizeMB() const
{
    return getInt("resources", "max_log_size_mb", DEFAULT_MAX_LOG_SIZE_MB);
}

QStringList ConfigManager::getDisabledInterfaces() const
{
    return getStringList("network", "disabled_interfaces", QStringList());
}

QStringList ConfigManager::getAllowedInterfaces() const
{
    return getStringList("network", "allowed_interfaces", QStringList());
}

bool ConfigManager::getBlockAllTraffic() const
{
    return getBool("network", "block_all_traffic", false);
}

bool ConfigManager::getAllowLocalhost() const
{
    return getBool("network", "allow_localhost", true);
}

QStringList ConfigManager::getWhitelistedLocalhostApps() const
{
    QString appsStr = getString("network", "whitelisted_localhost_apps", "jupyter,jupyter-notebook,jupyter-lab,tomcat,apache,httpd,nginx,xampp");
    return appsStr.split(',', Qt::SkipEmptyParts);
}

bool ConfigManager::getIntegrityCheckEnabled() const
{
    return getBool("security", "integrity_check_enabled", true);
}

bool ConfigManager::getVisualAlertsEnabled() const
{
    return getBool("alerts", "visual_alerts_enabled", true);
}

bool ConfigManager::getAudioAlertsEnabled() const
{
    return getBool("alerts", "audio_alerts_enabled", true);
}

int ConfigManager::getAlertTimeout() const
{
    return getInt("alerts", "alert_timeout", 5000);
}

int ConfigManager::getUiUpdateInterval() const
{
    return getInt("ui", "update_interval", 1000);
}

int ConfigManager::getMaxLogDisplay() const
{
    return getInt("ui", "max_log_display", 1000);
}

int ConfigManager::getThumbnailSize() const
{
    return getInt("ui", "thumbnail_size", 200);
}

bool ConfigManager::getResourceMonitoringEnabled() const
{
    return getBool("monitoring", "resource_monitoring_enabled", true);
}

QStringList ConfigManager::getWhitelistedApplications() const
{
    QString appsStr = getString("whitelist", "whitelisted_applications", "");
    return appsStr.split(',', Qt::SkipEmptyParts);
}

QStringList ConfigManager::getWhitelistedWindows() const
{
    QString windowsStr = getString("whitelist", "whitelisted_windows", "localhost,127.0.0.1,jupyter,tomcat,apache,xampp");
    return windowsStr.split(',', Qt::SkipEmptyParts);
}

QStringList ConfigManager::getWhitelistedUrls() const
{
    QString urlsStr = getString("whitelist", "whitelisted_urls", "localhost,127.0.0.1,::1");
    return urlsStr.split(',', Qt::SkipEmptyParts);
}

bool ConfigManager::createDirectories() const
{
    QStringList dirs = {getLogDir(), getDataDir(), getScreenshotDir(), 
                       getBackupDir(), getIntegrityDir()};
    
    for (const QString& dir : dirs) {
        if (!createDirectory(dir)) {
            return false;
        }
    }
    return true;
}

bool ConfigManager::validatePaths() const
{
    QStringList paths = {getLogDir(), getDataDir(), getScreenshotDir(), 
                        getBackupDir(), getIntegrityDir()};
    
    for (const QString& path : paths) {
        QDir dir(path);
        if (!dir.exists()) {
            return false;
        }
    }
    return true;
}

void ConfigManager::setValue(const QString& section, const QString& key, const QVariant& value)
{
    if (m_settings) {
        m_settings->setValue(section + "/" + key, value);
    }
}

bool ConfigManager::saveConfiguration()
{
    if (m_settings) {
        m_settings->sync();
        return true;
    }
    return false;
}

void ConfigManager::reloadConfiguration()
{
    if (m_settings) {
        m_settings->sync();
        loadDefaults();
    }
}

void ConfigManager::loadDefaults()
{
    // This method can be used to set default values if not present in config file
}

bool ConfigManager::createDirectory(const QString& path, int permissions) const
{
    QDir dir;
    if (!dir.exists(path)) {
        if (!dir.mkpath(path)) {
            return false;
        }
        // Note: Setting permissions would require platform-specific code
        Q_UNUSED(permissions)
    }
    return true;
}

QString ConfigManager::resolvePath(const QString& path) const
{
    // Basic implementation - could be expanded to handle relative paths, env vars, etc.
    return path;
}