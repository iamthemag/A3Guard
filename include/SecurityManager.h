#ifndef SECURITYMANAGER_H
#define SECURITYMANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QTimer>
#include <QFileSystemWatcher>
#include <memory>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include "Common.h"

class ConfigManager;
class Logger;

class SecurityManager : public QObject
{
    Q_OBJECT

public:
    explicit SecurityManager(std::shared_ptr<ConfigManager> config, 
                           std::shared_ptr<Logger> logger, 
                           QObject *parent = nullptr);
    ~SecurityManager();

    // Initialization
    bool initialize();
    bool isInitialized() const { return m_initialized; }

    // Encryption/Decryption
    QByteArray encrypt(const QByteArray& data);
    QByteArray decrypt(const QByteArray& encryptedData);
    QString encryptString(const QString& str);
    QString decryptString(const QByteArray& encryptedData);

    // File operations
    bool encryptFile(const QString& filePath, const QString& outputPath = QString());
    bool decryptFile(const QString& encryptedPath, const QString& outputPath = QString());
    bool secureDelete(const QString& filePath, int passes = 3);

    // Integrity checking
    QString calculateFileHash(const QString& filePath);
    bool storeFileIntegrity(const QString& filePath);
    bool verifyFileIntegrity(const QString& filePath);
    QStringList verifyDirectoryIntegrity(const QString& dirPath, const QString& extension = QString());

    // Key management
    bool regenerateKey();
    bool backupKey(const QString& backupPath);

signals:
    void integrityViolationDetected(const QString& filePath);
    void encryptionError(const QString& error);
    void integrityCheckCompleted(int violations);

private slots:
    void performIntegrityCheck();
    void onFileChanged(const QString& path);

private:
    // Key management
    bool generateKey();
    bool loadKey();
    bool saveKey();

    // Internal encryption
    QByteArray performEncryption(const QByteArray& data, bool encrypt);

    // Integrity helpers
    QString generateIntegrityFileName(const QString& filePath);
    bool saveIntegrityData(const QString& filePath, const QString& hash, qint64 size);
    bool loadIntegrityData(const QString& filePath, QString& hash, qint64& size);

    std::shared_ptr<ConfigManager> m_config;
    std::shared_ptr<Logger> m_logger;
    
    // Encryption context
    EVP_CIPHER_CTX* m_encryptCtx;
    EVP_CIPHER_CTX* m_decryptCtx;
    QByteArray m_key;
    QByteArray m_iv;
    
    // Integrity checking
    QTimer* m_integrityTimer;
    QFileSystemWatcher* m_fileWatcher;
    QStringList m_protectedFiles;
    
    // State
    bool m_initialized;
    QString m_keyFilePath;
    QString m_integrityDir;
    
    static const int KEY_SIZE = 32;  // 256-bit key
    static const int IV_SIZE = 16;   // 128-bit IV
};

#endif // SECURITYMANAGER_H