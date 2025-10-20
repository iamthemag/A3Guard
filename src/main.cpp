#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMessageBox>
#include <QStyleFactory>
#include <QFont>
#include <unistd.h>
#include <memory>

#include "MainWindow.h"
#include "ConfigManager.h"
#include "SecurityManager.h"
#include "Logger.h"
#include "Common.h"

void setupApplication(QApplication& app)
{
    app.setApplicationName(EXAMGUARD_NAME);
    app.setApplicationVersion(EXAMGUARD_VERSION);
    app.setOrganizationName("ExamGuard");
    app.setApplicationDisplayName("ExamGuard - Exam Monitoring System");

    // Set application icon
    app.setWindowIcon(QIcon(":/icons/examguard.png"));
    
    // Use a clean, professional style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Dark theme for professional appearance
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    
    // Set application font
    QFont font("Ubuntu", 9);
    app.setFont(font);
}

bool checkRootPrivileges()
{
    if (getuid() != 0) {
        QMessageBox::critical(nullptr, "Permission Denied", 
            "ExamGuard requires administrator privileges to function properly.\n\n"
            "Please run as root using one of these methods:\n"
            "• sudo examguard\n"
            "• pkexec examguard\n"
            "• Use the desktop launcher (requires authentication)");
        return false;
    }
    return true;
}

void showHelp()
{
    qDebug() << "ExamGuard - Exam Monitoring System v" << EXAMGUARD_VERSION;
    qDebug() << "";
    qDebug() << "Usage: ExamGuard [options]";
    qDebug() << "";
    qDebug() << "Options:";
    qDebug() << "  --daemon                Run as daemon (no GUI)";
    qDebug() << "  --generate-key          Generate new encryption key";
    qDebug() << "  --verify-integrity      Verify file integrity";
    qDebug() << "  --config <file>         Use custom config file";
    qDebug() << "  --verbose               Enable verbose logging";
    qDebug() << "  --help                  Show this help";
    qDebug() << "  --version               Show version information";
    qDebug() << "";
    qDebug() << "Examples:";
    qDebug() << "  sudo examguard                    # Run GUI as root";
    qDebug() << "  sudo examguard --daemon           # Run as background service";
    qDebug() << "  sudo examguard --generate-key     # Generate new encryption key";
    qDebug() << "";
    qDebug() << "For more information, see the README.md file or man page.";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    setupApplication(app);

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("ExamGuard - Advanced Exam Monitoring System");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption daemonOption(QStringList() << "daemon", 
        "Run as daemon (no GUI)");
    parser.addOption(daemonOption);
    
    QCommandLineOption generateKeyOption(QStringList() << "generate-key", 
        "Generate new encryption key");
    parser.addOption(generateKeyOption);
    
    QCommandLineOption verifyIntegrityOption(QStringList() << "verify-integrity", 
        "Verify file integrity");
    parser.addOption(verifyIntegrityOption);
    
    QCommandLineOption configOption(QStringList() << "config", 
        "Use custom config file", "file");
    parser.addOption(configOption);
    
    QCommandLineOption verboseOption(QStringList() << "verbose", 
        "Enable verbose logging");
    parser.addOption(verboseOption);
    
    parser.process(app);

    // Check for root privileges
    if (!checkRootPrivileges()) {
        return 1;
    }

    try {
        // Initialize configuration
        QString configFile = parser.value(configOption);
        if (configFile.isEmpty()) {
            configFile = DEFAULT_CONFIG_PATH;
        }
        
        auto config = std::make_shared<ConfigManager>(configFile);
        if (!config->initialize()) {
            QMessageBox::critical(nullptr, "Configuration Error",
                QString("Failed to load configuration from: %1\n\n"
                       "Please check if the file exists and is readable.").arg(configFile));
            return 1;
        }

        // Initialize logger
        auto logger = std::make_shared<Logger>(config);
        if (!logger->initialize()) {
            QMessageBox::critical(nullptr, "Logger Error",
                "Failed to initialize logging system.\n\n"
                "Please check directory permissions and disk space.");
            return 1;
        }

        // Enable verbose logging if requested
        if (parser.isSet(verboseOption)) {
            logger->setVerbose(true);
        }

        LOG_INFO("ExamGuard starting - Version" << EXAMGUARD_VERSION);

        // Initialize security manager
        auto security = std::make_shared<SecurityManager>(config, logger);
        if (!security->initialize()) {
            QMessageBox::critical(nullptr, "Security Error",
                "Failed to initialize security system.\n\n"
                "This may indicate:\n"
                "• Missing encryption key file\n"
                "• Insufficient permissions\n"
                "• Corrupted security data");
            return 1;
        }

        // Handle command line options
        if (parser.isSet(generateKeyOption)) {
            LOG_INFO("Generating new encryption key...");
            if (security->regenerateKey()) {
                qDebug() << "New encryption key generated successfully.";
                LOG_INFO("New encryption key generated");
                return 0;
            } else {
                qDebug() << "Failed to generate new encryption key.";
                LOG_ERROR("Failed to generate new encryption key");
                return 1;
            }
        }

        if (parser.isSet(verifyIntegrityOption)) {
            LOG_INFO("Verifying file integrity...");
            
            // Verify log files
            QStringList logViolations = security->verifyDirectoryIntegrity(
                config->getLogDir(), config->getLogExtension());
            
            // Verify screenshot files
            QStringList screenshotViolations = security->verifyDirectoryIntegrity(
                config->getScreenshotDir(), config->getScreenshotExtension());
            
            int totalViolations = logViolations.size() + screenshotViolations.size();
            
            if (totalViolations == 0) {
                qDebug() << "File integrity verification passed.";
                LOG_INFO("File integrity verification passed");
                return 0;
            } else {
                qDebug() << "File integrity violations found:" << totalViolations;
                for (const QString& violation : logViolations + screenshotViolations) {
                    qDebug() << " -" << violation;
                }
                LOG_ERROR("File integrity violations found:" << totalViolations);
                return 1;
            }
        }

        if (parser.isSet(daemonOption)) {
            LOG_INFO("Running in daemon mode (no GUI)");
            qDebug() << "Daemon mode is not yet implemented.";
            qDebug() << "Use systemd service instead: sudo systemctl start examguard";
            return 1;
        }

        // Create and show main window
        LOG_INFO("Starting GUI mode");
        MainWindow window(config, security, logger);
        window.show();

        // Handle application quit signal
        QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
            LOG_INFO("ExamGuard shutting down");
        });

        LOG_INFO("ExamGuard GUI ready");
        return app.exec();

    } catch (const std::exception& e) {
        QString errorMsg = QString("Fatal error during startup: %1").arg(e.what());
        LOG_ERROR(errorMsg);
        QMessageBox::critical(nullptr, "Fatal Error", errorMsg);
        return 1;
    } catch (...) {
        QString errorMsg = "Unknown fatal error during startup";
        LOG_ERROR(errorMsg);
        QMessageBox::critical(nullptr, "Fatal Error", errorMsg);
        return 1;
    }
}