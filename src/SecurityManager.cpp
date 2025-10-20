#include "SecurityManager.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QRandomGenerator>
#include <openssl/err.h>

SecurityManager::SecurityManager(std::shared_ptr<ConfigManager> config,
                               std::shared_ptr<Logger> logger,
                               QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_logger(logger)
    , m_encryptCtx(nullptr)
    , m_decryptCtx(nullptr)
    , m_integrityTimer(new QTimer(this))
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_initialized(false)
{
    m_keyFilePath = m_config->getKeyFile();
    m_integrityDir = m_config->getIntegrityDir();
    
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    
    // Setup integrity checking timer
    connect(m_integrityTimer, &QTimer::timeout, this, &SecurityManager::performIntegrityCheck);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &SecurityManager::onFileChanged);
}

SecurityManager::~SecurityManager()
{
    if (m_encryptCtx) {
        EVP_CIPHER_CTX_free(m_encryptCtx);
    }
    if (m_decryptCtx) {
        EVP_CIPHER_CTX_free(m_decryptCtx);
    }
    
    // Cleanup OpenSSL
    EVP_cleanup();
    ERR_free_strings();
}

bool SecurityManager::initialize()
{
    try {
        // Create directories
        QDir().mkpath(QFileInfo(m_keyFilePath).absolutePath());
        QDir().mkpath(m_integrityDir);
        
        // Load or generate key
        if (!loadKey()) {
            LOG_INFO("Generating new encryption key");
            if (!generateKey()) {
                LOG_ERROR("Failed to generate encryption key");
                return false;
            }
        }
        
        // Initialize cipher contexts
        m_encryptCtx = EVP_CIPHER_CTX_new();
        m_decryptCtx = EVP_CIPHER_CTX_new();
        
        if (!m_encryptCtx || !m_decryptCtx) {
            LOG_ERROR("Failed to create cipher contexts");
            return false;
        }
        
        // Start integrity checking if enabled
        if (m_config->getIntegrityCheckEnabled()) {
            int interval = m_config->getIntegrityCheckInterval();
            m_integrityTimer->start(interval);
            LOG_INFO("Integrity checking enabled with" << interval << "ms interval");
        }
        
        m_initialized = true;
        LOG_INFO("SecurityManager initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("SecurityManager initialization failed:" << e.what());
        return false;
    }
}

bool SecurityManager::generateKey()
{
    try {
        // Generate random key
        m_key.resize(KEY_SIZE);
        if (RAND_bytes(reinterpret_cast<unsigned char*>(m_key.data()), KEY_SIZE) != 1) {
            LOG_ERROR("Failed to generate random key");
            return false;
        }
        
        // Generate random IV
        m_iv.resize(IV_SIZE);
        if (RAND_bytes(reinterpret_cast<unsigned char*>(m_iv.data()), IV_SIZE) != 1) {
            LOG_ERROR("Failed to generate random IV");
            return false;
        }
        
        return saveKey();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Key generation failed:" << e.what());
        return false;
    }
}

bool SecurityManager::loadKey()
{
    try {
        QFile keyFile(m_keyFilePath);
        if (!keyFile.exists()) {
            return false;
        }
        
        if (!keyFile.open(QIODevice::ReadOnly)) {
            LOG_ERROR("Cannot open key file for reading:" << m_keyFilePath);
            return false;
        }
        
        QByteArray keyData = keyFile.readAll();
        keyFile.close();
        
        if (keyData.size() != KEY_SIZE + IV_SIZE) {
            LOG_ERROR("Invalid key file size");
            return false;
        }
        
        m_key = keyData.left(KEY_SIZE);
        m_iv = keyData.mid(KEY_SIZE, IV_SIZE);
        
        LOG_DEBUG("Encryption key loaded successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Key loading failed:" << e.what());
        return false;
    }
}

bool SecurityManager::saveKey()
{
    try {
        QFile keyFile(m_keyFilePath);
        if (!keyFile.open(QIODevice::WriteOnly)) {
            LOG_ERROR("Cannot open key file for writing:" << m_keyFilePath);
            return false;
        }
        
        // Combine key and IV
        QByteArray keyData = m_key + m_iv;
        keyFile.write(keyData);
        keyFile.close();
        
        // Set secure permissions (600)
        keyFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        
        LOG_DEBUG("Encryption key saved successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Key saving failed:" << e.what());
        return false;
    }
}

QByteArray SecurityManager::encrypt(const QByteArray& data)
{
    return performEncryption(data, true);
}

QByteArray SecurityManager::decrypt(const QByteArray& encryptedData)
{
    return performEncryption(encryptedData, false);
}

QString SecurityManager::encryptString(const QString& str)
{
    QByteArray data = str.toUtf8();
    QByteArray encrypted = encrypt(data);
    return encrypted.toBase64();
}

QString SecurityManager::decryptString(const QByteArray& encryptedData)
{
    QByteArray data = QByteArray::fromBase64(encryptedData);
    QByteArray decrypted = decrypt(data);
    return QString::fromUtf8(decrypted);
}

QByteArray SecurityManager::performEncryption(const QByteArray& data, bool encrypt)
{
    if (!m_initialized) {
        LOG_ERROR("SecurityManager not initialized");
        return QByteArray();
    }
    
    try {
        EVP_CIPHER_CTX* ctx = encrypt ? m_encryptCtx : m_decryptCtx;
        
        // Initialize cipher
        if (EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                             reinterpret_cast<const unsigned char*>(m_key.data()),
                             reinterpret_cast<const unsigned char*>(m_iv.data()),
                             encrypt ? 1 : 0) != 1) {
            LOG_ERROR("Failed to initialize cipher");
            return QByteArray();
        }
        
        // Prepare output buffer
        QByteArray result;
        result.resize(data.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        
        int outLen = 0;
        int totalLen = 0;
        
        // Process data
        if (EVP_CipherUpdate(ctx, 
                           reinterpret_cast<unsigned char*>(result.data()),
                           &outLen,
                           reinterpret_cast<const unsigned char*>(data.data()),
                           data.size()) != 1) {
            LOG_ERROR("Failed to process data");
            return QByteArray();
        }
        totalLen = outLen;
        
        // Finalize
        if (EVP_CipherFinal_ex(ctx,
                             reinterpret_cast<unsigned char*>(result.data()) + totalLen,
                             &outLen) != 1) {
            LOG_ERROR("Failed to finalize cipher");
            return QByteArray();
        }
        totalLen += outLen;
        
        result.resize(totalLen);
        return result;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Encryption/decryption failed:" << e.what());
        return QByteArray();
    }
}

bool SecurityManager::encryptFile(const QString& filePath, const QString& outputPath)
{
    try {
        QFile inputFile(filePath);
        if (!inputFile.open(QIODevice::ReadOnly)) {
            LOG_ERROR("Cannot open file for encryption:" << filePath);
            return false;
        }
        
        QString outPath = outputPath.isEmpty() ? filePath + ".enc" : outputPath;
        QFile outputFile(outPath);
        if (!outputFile.open(QIODevice::WriteOnly)) {
            LOG_ERROR("Cannot open output file:" << outPath);
            return false;
        }
        
        // Read and encrypt data
        QByteArray data = inputFile.readAll();
        inputFile.close();
        
        QByteArray encrypted = encrypt(data);
        if (encrypted.isEmpty()) {
            LOG_ERROR("Encryption failed for file:" << filePath);
            return false;
        }
        
        outputFile.write(encrypted);
        outputFile.close();
        
        // Set secure permissions
        outputFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        
        LOG_DEBUG("File encrypted:" << filePath << "->" << outPath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("File encryption failed:" << e.what());
        return false;
    }
}

bool SecurityManager::decryptFile(const QString& encryptedPath, const QString& outputPath)
{
    try {
        QFile inputFile(encryptedPath);
        if (!inputFile.open(QIODevice::ReadOnly)) {
            LOG_ERROR("Cannot open encrypted file:" << encryptedPath);
            return false;
        }
        
        QString outPath = outputPath;
        if (outPath.isEmpty()) {
            outPath = encryptedPath;
            if (outPath.endsWith(".enc")) {
                outPath.chop(4);
            }
        }
        
        QFile outputFile(outPath);
        if (!outputFile.open(QIODevice::WriteOnly)) {
            LOG_ERROR("Cannot open output file:" << outPath);
            return false;
        }
        
        // Read and decrypt data
        QByteArray encryptedData = inputFile.readAll();
        inputFile.close();
        
        QByteArray decrypted = decrypt(encryptedData);
        if (decrypted.isEmpty()) {
            LOG_ERROR("Decryption failed for file:" << encryptedPath);
            return false;
        }
        
        outputFile.write(decrypted);
        outputFile.close();
        
        LOG_DEBUG("File decrypted:" << encryptedPath << "->" << outPath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("File decryption failed:" << e.what());
        return false;
    }
}

QString SecurityManager::calculateFileHash(const QString& filePath)
{
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_ERROR("Cannot open file for hashing:" << filePath);
            return QString();
        }
        
        QCryptographicHash hash(QCryptographicHash::Sha256);
        
        // Read file in chunks to handle large files efficiently
        const int bufferSize = 8192;
        while (!file.atEnd()) {
            QByteArray buffer = file.read(bufferSize);
            hash.addData(buffer);
        }
        
        file.close();
        return hash.result().toHex();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Hash calculation failed:" << e.what());
        return QString();
    }
}

bool SecurityManager::storeFileIntegrity(const QString& filePath)
{
    if (!m_config->getIntegrityCheckEnabled()) {
        return true;
    }
    
    try {
        QString hash = calculateFileHash(filePath);
        if (hash.isEmpty()) {
            return false;
        }
        
        QFileInfo fileInfo(filePath);
        qint64 size = fileInfo.size();
        
        return saveIntegrityData(filePath, hash, size);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to store integrity data:" << e.what());
        return false;
    }
}

bool SecurityManager::verifyFileIntegrity(const QString& filePath)
{
    if (!m_config->getIntegrityCheckEnabled()) {
        return true;
    }
    
    try {
        QString storedHash;
        qint64 storedSize;
        
        if (!loadIntegrityData(filePath, storedHash, storedSize)) {
            LOG_WARNING("No integrity data found for:" << filePath);
            return false;
        }
        
        // Check if file exists
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            LOG_ERROR("File missing:" << filePath);
            emit integrityViolationDetected(filePath);
            return false;
        }
        
        // Check file size
        if (fileInfo.size() != storedSize) {
            LOG_ERROR("Size mismatch for:" << filePath);
            emit integrityViolationDetected(filePath);
            return false;
        }
        
        // Check file hash
        QString currentHash = calculateFileHash(filePath);
        if (currentHash != storedHash) {
            LOG_ERROR("Hash mismatch for:" << filePath);
            emit integrityViolationDetected(filePath);
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Integrity verification failed:" << e.what());
        return false;
    }
}

QStringList SecurityManager::verifyDirectoryIntegrity(const QString& dirPath, const QString& extension)
{
    QStringList violations;
    
    try {
        QDir dir(dirPath);
        if (!dir.exists()) {
            violations << QString("Directory missing: %1").arg(dirPath);
            return violations;
        }
        
        QStringList filters;
        if (!extension.isEmpty()) {
            filters << "*" + extension;
        }
        
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
        for (const QFileInfo& fileInfo : files) {
            if (!verifyFileIntegrity(fileInfo.absoluteFilePath())) {
                violations << fileInfo.absoluteFilePath();
            }
        }
        
        return violations;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Directory integrity check failed:" << e.what());
        violations << QString("Directory check error: %1").arg(e.what());
        return violations;
    }
}

bool SecurityManager::secureDelete(const QString& filePath, int passes)
{
    try {
        QFile file(filePath);
        if (!file.exists()) {
            return true; // Already gone
        }
        
        qint64 fileSize = file.size();
        
        if (!file.open(QIODevice::WriteOnly)) {
            LOG_ERROR("Cannot open file for secure deletion:" << filePath);
            return false;
        }
        
        // Overwrite with random data multiple times
        for (int pass = 0; pass < passes; ++pass) {
            file.seek(0);
            
            qint64 remaining = fileSize;
            while (remaining > 0) {
                const int chunkSize = qMin(remaining, static_cast<qint64>(8192));
                QByteArray randomData(chunkSize, 0);
                
                // Fill with random data
                QRandomGenerator* rng = QRandomGenerator::global();
                for (int i = 0; i < chunkSize; ++i) {
                    randomData[i] = static_cast<char>(rng->bounded(256));
                }
                
                file.write(randomData);
                remaining -= chunkSize;
            }
            
            file.flush();
        }
        
        file.close();
        
        // Finally remove the file
        bool removed = file.remove();
        if (removed) {
            LOG_DEBUG("File securely deleted:" << filePath);
        } else {
            LOG_ERROR("Failed to remove file after overwriting:" << filePath);
        }
        
        return removed;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Secure deletion failed:" << e.what());
        return false;
    }
}

void SecurityManager::performIntegrityCheck()
{
    try {
        int violations = 0;
        
        // Check log files
        QStringList logViolations = verifyDirectoryIntegrity(
            m_config->getLogDir(), m_config->getLogExtension());
        violations += logViolations.size();
        
        // Check screenshot files
        QStringList screenshotViolations = verifyDirectoryIntegrity(
            m_config->getScreenshotDir(), m_config->getScreenshotExtension());
        violations += screenshotViolations.size();
        
        emit integrityCheckCompleted(violations);
        
        if (violations > 0) {
            LOG_WARNING("Integrity check completed with" << violations << "violations");
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Integrity check failed:" << e.what());
    }
}

void SecurityManager::onFileChanged(const QString& path)
{
    LOG_WARNING("Protected file changed:" << path);
    if (!verifyFileIntegrity(path)) {
        emit integrityViolationDetected(path);
    }
}

QString SecurityManager::generateIntegrityFileName(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.fileName();
    return QDir(m_integrityDir).absoluteFilePath(baseName + m_config->getIntegrityExtension());
}

bool SecurityManager::saveIntegrityData(const QString& filePath, const QString& hash, qint64 size)
{
    try {
        QString integrityFile = generateIntegrityFileName(filePath);
        
        QJsonObject data;
        data["file_path"] = filePath;
        data["hash"] = hash;
        data["size"] = size;
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        QJsonDocument doc(data);
        QString jsonString = doc.toJson(QJsonDocument::Compact);
        
        // Encrypt the integrity data
        QByteArray encrypted = encrypt(jsonString.toUtf8());
        if (encrypted.isEmpty()) {
            LOG_ERROR("Failed to encrypt integrity data");
            return false;
        }
        
        QFile file(integrityFile);
        if (!file.open(QIODevice::WriteOnly)) {
            LOG_ERROR("Cannot open integrity file for writing:" << integrityFile);
            return false;
        }
        
        file.write(encrypted);
        file.close();
        
        // Set secure permissions
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        
        // Add to file watcher
        if (!m_fileWatcher->files().contains(filePath)) {
            m_fileWatcher->addPath(filePath);
        }
        
        LOG_DEBUG("Integrity data saved for:" << filePath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save integrity data:" << e.what());
        return false;
    }
}

bool SecurityManager::loadIntegrityData(const QString& filePath, QString& hash, qint64& size)
{
    try {
        QString integrityFile = generateIntegrityFileName(filePath);
        
        QFile file(integrityFile);
        if (!file.exists()) {
            return false;
        }
        
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_ERROR("Cannot open integrity file for reading:" << integrityFile);
            return false;
        }
        
        QByteArray encryptedData = file.readAll();
        file.close();
        
        // Decrypt the integrity data
        QByteArray decryptedData = decrypt(encryptedData);
        if (decryptedData.isEmpty()) {
            LOG_ERROR("Failed to decrypt integrity data");
            return false;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
        if (!doc.isObject()) {
            LOG_ERROR("Invalid integrity data format");
            return false;
        }
        
        QJsonObject data = doc.object();
        hash = data["hash"].toString();
        size = data["size"].toInteger();
        
        return !hash.isEmpty();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load integrity data:" << e.what());
        return false;
    }
}