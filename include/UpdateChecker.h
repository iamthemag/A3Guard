#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QDir>

class UpdateChecker : public QObject {
    Q_OBJECT
    
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker();
    
    void checkForUpdates();
    QString getCurrentVersion() const;
    QString getLatestVersion() const;
    QString getDownloadUrl() const;
    QString getReleaseNotes() const;
    bool isUpdateAvailable() const;
    
signals:
    void checkStarted();
    void updateAvailable(QString latestVersion, QString downloadUrl, QString releaseNotes);
    void noUpdateAvailable();
    void checkFailed(QString errorMessage);
    void downloadStarted(QString fileName);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(QString filePath);
    void downloadFailed(QString errorMessage);
    
private slots:
    void onGitHubResponseReceived(QNetworkReply *reply);
    void onNetworkError(QNetworkReply::NetworkError error);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(QNetworkReply *reply);
    
private:
    void parseGitHubResponse(const QByteArray &data);
    bool isNewerVersion(const QString &current, const QString &latest) const;
    QString getCacheDir() const;
    QString getDEBFileName(const QString &version) const;
    
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    QNetworkReply *m_downloadReply;
    QString m_latestVersion;
    QString m_downloadUrl;
    QString m_releaseNotes;
    QString m_downloadPath;
    QDateTime m_lastCheckTime;
    const int RATE_LIMIT_SECONDS = 3600;
    const int NETWORK_TIMEOUT_MS = 30000;
};

#endif // UPDATECHECKER_H
