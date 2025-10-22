#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMessageBox>
#include <QStyleFactory>
#include <QFont>
#include <QIcon>
#include <QPalette>
#include <unistd.h>
#include <memory>

#include "MainWindow.h"
#include "ConfigManager.h"
#include "SecurityManager.h"
#include "Logger.h"
#include "Common.h"
#include "PrivilegeDialog.h"

// Global logger removed for now

void setupApplication(QApplication& app)
{
    app.setApplicationName(A3GUARD_NAME);
    app.setApplicationVersion(A3GUARD_VERSION);
    app.setOrganizationName("A3Guard");
    app.setApplicationDisplayName("A3Guard - Advanced Assessment Application");

    // Set application icon
    app.setWindowIcon(QIcon(":/icons/a3guard.png"));
    
    // Use modern Fusion style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Modern semi-dark theme
    QPalette modernPalette;
    // Background colors - semi-dark
    modernPalette.setColor(QPalette::Window, QColor(45, 45, 48));          // Main window background
    modernPalette.setColor(QPalette::Base, QColor(60, 63, 65));            // Input fields, lists
    modernPalette.setColor(QPalette::AlternateBase, QColor(50, 50, 53));   // Alternating rows
    
    // Text colors
    modernPalette.setColor(QPalette::WindowText, QColor(220, 220, 220));   // Main text
    modernPalette.setColor(QPalette::Text, QColor(255, 255, 255));         // Input text
    modernPalette.setColor(QPalette::BrightText, QColor(255, 100, 100));   // Bright accent text
    
    // Button colors
    modernPalette.setColor(QPalette::Button, QColor(70, 73, 75));          // Button background
    modernPalette.setColor(QPalette::ButtonText, QColor(220, 220, 220));   // Button text
    
    // Selection colors - modern blue accent
    modernPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));      // Selection background
    modernPalette.setColor(QPalette::HighlightedText, Qt::white);          // Selected text
    
    // Links and tooltips
    modernPalette.setColor(QPalette::Link, QColor(100, 149, 237));         // Cornflower blue
    modernPalette.setColor(QPalette::LinkVisited, QColor(147, 112, 219));  // Medium slate blue
    modernPalette.setColor(QPalette::ToolTipBase, QColor(70, 73, 75));     // Tooltip background
    modernPalette.setColor(QPalette::ToolTipText, QColor(220, 220, 220));  // Tooltip text
    
    // Disabled state
    modernPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));
    modernPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
    modernPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
    
    app.setPalette(modernPalette);
    
    // Set application font
    QFont font("Ubuntu", 9);
    app.setFont(font);
}

bool checkRootPrivileges()
{
    if (getuid() != 0) {
        QMessageBox::critical(nullptr, "Permission Denied", 
            "A3Guard requires administrator privileges to function properly.\n\n"
            "Please run as root using one of these methods:\n"
            "• sudo a3guard\n"
            "• pkexec a3guard\n"
            "• Use the desktop launcher (requires authentication)");
        return false;
    }
    return true;
}

bool requestPrivilegesIfNeeded()
{
    if (PrivilegeDialog::hasRootPrivileges()) {
        return true;
    }
    
    // Show privilege dialog after GUI is initialized
    return true; // Allow startup without privileges
}

void showHelp()
{
    qDebug() << "A3Guard - Advanced Assessment Application v" << A3GUARD_VERSION;
    qDebug() << "";
    qDebug() << "Usage: A3Guard [options]";
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
    qDebug() << "  sudo a3guard                    # Run GUI as root";
    qDebug() << "  sudo a3guard --daemon           # Run as background service";
    qDebug() << "  sudo a3guard --generate-key     # Generate new encryption key";
    qDebug() << "";
    qDebug() << "For more information, see the README.md file or man page.";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    setupApplication(app);

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("A3Guard - Advanced Assessment Application");
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

    // For command-line operations, still require root
    if (parser.isSet(generateKeyOption) || parser.isSet(verifyIntegrityOption) || parser.isSet(daemonOption)) {
        if (!checkRootPrivileges()) {
            return 1;
        }
    }

    try {
        // Initialize configuration with fallback paths for user-space execution
        QString configFile = parser.value(configOption);
        if (configFile.isEmpty()) {
            configFile = DEFAULT_CONFIG_PATH;
            // Fallback to user config if system config doesn't exist or isn't readable
            if (!QFile::exists(configFile) || !QFileInfo(configFile).isReadable()) {
                QString userConfigDir = QDir::homePath() + "/.config/a3guard";
                QDir().mkpath(userConfigDir);
                configFile = userConfigDir + "/a3guard.conf";
                
                // Create basic user config if it doesn't exist
                if (!QFile::exists(configFile)) {
                    QFile defaultConfig(":/config/a3guard.conf"); // Try resource first
                    if (!defaultConfig.exists()) {
                        // Create minimal config
                        QFile userConfig(configFile);
                        if (userConfig.open(QIODevice::WriteOnly | QIODevice::Text)) {
                            QTextStream out(&userConfig);
            out << "[monitoring]\n";
            out << "network_check_interval=30000\n";
                            out << "[security]\n";
                            out << "enable_encryption=true\n";
                            userConfig.close();
                        }
                    }
                }
            }
        }
        
        auto config = std::make_shared<ConfigManager>(configFile);
        if (!config->initialize()) {
            // Try to continue with minimal functionality
            LOG_WARNING("Failed to load configuration from:" << configFile << "- continuing with defaults");
        }

        // Initialize logger with fault tolerance
        auto logger = std::make_shared<Logger>(config);
        if (!logger->initialize()) {
            LOG_WARNING("Failed to initialize full logging system - continuing with console logging only");
            // Continue without file logging - use console only
        }

        // Enable verbose logging if requested
        if (parser.isSet(verboseOption)) {
            logger->setVerbose(true);
        }

        LOG_INFO("A3Guard starting - Version" << A3GUARD_VERSION);

        // Initialize security manager
        auto security = std::make_shared<SecurityManager>(config, logger);
        if (!security->initialize()) {
            LOG_WARNING("Failed to initialize full security system - continuing with limited functionality");
            // Continue without full security features
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
            
            int totalViolations = logViolations.size();
            
            if (totalViolations == 0) {
                qDebug() << "File integrity verification passed.";
                LOG_INFO("File integrity verification passed");
                return 0;
            } else {
                qDebug() << "File integrity violations found:" << totalViolations;
                for (const QString& violation : logViolations) {
                    qDebug() << " -" << violation;
                }
                LOG_ERROR("File integrity violations found:" << totalViolations);
                return 1;
            }
        }

        if (parser.isSet(daemonOption)) {
            LOG_INFO("Running in daemon mode (no GUI)");
            qDebug() << "Daemon mode is not yet implemented.";
            qDebug() << "Use systemd service instead: sudo systemctl start a3guard";
            return 1;
        }

        // Create and show main window
        LOG_INFO("Starting GUI mode");
        MainWindow window(config, security, logger);
        
        // Only show the window if privilege check passed
        if (window.shouldShowWindow()) {
            window.show();
        } else {
            // Privilege check failed, application will exit
            LOG_INFO("Application exiting due to failed privilege check");
            return 1;
        }

        // Handle application quit signal
        QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
            LOG_INFO("A3Guard shutting down");
        });

        LOG_INFO("A3Guard GUI ready");
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