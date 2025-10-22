#include "UpdateChecker.h"
#include "Common.h"
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QTimer>
#include <QDebug>
#include <QStandardPaths>
#include <QFile>
#include <QIODevice>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_downloadReply(nullptr)
    , m_lastCheckTime(QDateTime::currentDateTime().addSecs(-RATE_LIMIT_SECONDS))
{
}

UpdateChecker::~UpdateChecker()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
    }
}

void UpdateChecker::checkForUpdates()
{
    // Rate limiting: prevent checking too frequently
    QDateTime now = QDateTime::currentDateTime();
    // Only enforce rate limit if we've successfully checked before (not on first check)
    if (!m_latestVersion.isEmpty() && m_lastCheckTime.secsTo(now) < RATE_LIMIT_SECONDS) {
        int secondsLeft = RATE_LIMIT_SECONDS - m_lastCheckTime.secsTo(now);
        emit checkFailed(QString("Already checked recently. Please wait %1 seconds.").arg(secondsLeft));
        return;
    }
    
    // Abort any pending requests
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    emit checkStarted();
    m_lastCheckTime = now;
    
    // Query GitHub API for latest release
    QUrl url("https://api.github.com/repos/iamthemag/A3Guard/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString("A3Guard/%1").arg(A3GUARD_VERSION));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Set timeout using a timer
    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, [this, timeoutTimer]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
        emit checkFailed("Request timed out. Check your internet connection.");
        timeoutTimer->deleteLater();
    });
    
    m_currentReply = m_networkManager->get(request);
    
    connect(m_currentReply, &QNetworkReply::finished,
            this, &UpdateChecker::onGitHubResponseReceived);
    
    connect(m_currentReply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &UpdateChecker::onNetworkError);
    
    timeoutTimer->start(NETWORK_TIMEOUT_MS);
}

void UpdateChecker::onGitHubResponseReceived(QNetworkReply *reply)
{
    if (!reply) {
        reply = m_currentReply;
    }
    
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        qWarning() << "GitHub API request failed:" << errorMsg;
        emit checkFailed(QString("Failed to check for updates: %1").arg(errorMsg));
        reply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    m_currentReply = nullptr;
    
    if (responseData.isEmpty()) {
        emit checkFailed("Empty response from GitHub API.");
        return;
    }
    
    parseGitHubResponse(responseData);
}

void UpdateChecker::parseGitHubResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON response from GitHub";
        emit checkFailed("Failed to parse update information.");
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // Extract version from tag_name (e.g., "v1.1.0" -> "1.1.0")
    QString tagName = obj["tag_name"].toString();
    if (tagName.isEmpty()) {
        qWarning() << "No tag_name in GitHub response";
        emit checkFailed("Invalid release information.");
        return;
    }
    
    m_latestVersion = tagName.startsWith('v') ? tagName.mid(1) : tagName;
    
    // Extract release notes from body
    m_releaseNotes = obj["body"].toString();
    if (m_releaseNotes.isEmpty()) {
        m_releaseNotes = "No release notes available.";
    }
    
    // Find DEB asset
    m_downloadUrl.clear();
    QJsonArray assets = obj["assets"].toArray();
    
    for (const QJsonValue &assetValue : assets) {
        QJsonObject asset = assetValue.toObject();
        QString assetName = asset["name"].toString();
        
        // Look for .deb file matching the version
        if (assetName.endsWith(".deb")) {
            m_downloadUrl = asset["browser_download_url"].toString();
            qDebug() << "Found DEB asset:" << assetName << "URL:" << m_downloadUrl;
            break;
        }
    }
    
    if (m_downloadUrl.isEmpty()) {
        qWarning() << "No DEB file found in release assets";
        emit checkFailed("No installation package found for this release.");
        return;
    }
    
    // Compare versions
    if (isNewerVersion(getCurrentVersion(), m_latestVersion)) {
        qDebug() << "Update available:" << getCurrentVersion() << "->" << m_latestVersion;
        emit updateAvailable(m_latestVersion, m_downloadUrl, m_releaseNotes);
    } else {
        qDebug() << "Already up to date:" << getCurrentVersion();
        emit noUpdateAvailable();
    }
}

bool UpdateChecker::isNewerVersion(const QString &current, const QString &latest) const
{
    // Parse semantic versions: "1.2.3" -> [1, 2, 3]
    QStringList currParts = current.split('.');
    QStringList latestParts = latest.split('.');
    
    // Pad with zeros if needed
    while (currParts.size() < 3) currParts.append("0");
    while (latestParts.size() < 3) latestParts.append("0");
    
    // Compare major.minor.patch
    for (int i = 0; i < 3; ++i) {
        bool currOk = false, latestOk = false;
        int currNum = currParts[i].toInt(&currOk);
        int latestNum = latestParts[i].toInt(&latestOk);
        
        if (!currOk) currNum = 0;
        if (!latestOk) latestNum = 0;
        
        if (latestNum > currNum) {
            return true;  // Newer version found
        }
        if (latestNum < currNum) {
            return false;  // Current is newer
        }
        // If equal, continue to next part
    }
    
    return false;  // Same version
}

void UpdateChecker::onNetworkError(QNetworkReply::NetworkError error)
{
    if (m_currentReply) {
        QString errorMsg = m_currentReply->errorString();
        qWarning() << "Network error:" << error << errorMsg;
        emit checkFailed(QString("Network error: %1").arg(errorMsg));
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void UpdateChecker::onDownloadFinished(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        qWarning() << "Download failed:" << errorMsg;
        emit downloadFailed(QString("Download failed: %1").arg(errorMsg));
        reply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }
    
    // Ensure cache directory exists
    QDir cacheDir(getCacheDir());
    if (!cacheDir.exists()) {
        if (!QDir().mkpath(getCacheDir())) {
            emit downloadFailed("Failed to create cache directory.");
            reply->deleteLater();
            m_downloadReply = nullptr;
            return;
        }
    }
    
    // Save downloaded file
    QString fileName = getDEBFileName(m_latestVersion);
    QString filePath = getCacheDir() + "/" + fileName;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        emit downloadFailed("Failed to save download file.");
        reply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }
    
    file.write(reply->readAll());
    file.close();
    
    m_downloadPath = filePath;
    qDebug() << "Download completed:" << filePath;
    emit downloadFinished(filePath);
    
    reply->deleteLater();
    m_downloadReply = nullptr;
}

QString UpdateChecker::getCurrentVersion() const
{
    return QString(A3GUARD_VERSION);
}

QString UpdateChecker::getLatestVersion() const
{
    return m_latestVersion;
}

QString UpdateChecker::getDownloadUrl() const
{
    return m_downloadUrl;
}

QString UpdateChecker::getReleaseNotes() const
{
    return m_releaseNotes;
}

bool UpdateChecker::isUpdateAvailable() const
{
    return isNewerVersion(getCurrentVersion(), m_latestVersion);
}

QString UpdateChecker::getCacheDir() const
{
    // Use ~/.cache/a3guard/
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty()) {
        // Fallback to home directory
        cacheDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.cache/a3guard";
    }
    return cacheDir;
}

QString UpdateChecker::getDEBFileName(const QString &version) const
{
    return QString("a3guard_%1_amd64.deb").arg(version);
}
