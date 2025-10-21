#include "PrivilegeDialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QProcessEnvironment>
#include <QDir>
#include <QDebug>
#include <unistd.h>
#include <sys/types.h>

PrivilegeDialog::PrivilegeDialog(QWidget *parent)
    : QDialog(parent)
    , m_authenticated(false)
    , m_elevationProcess(nullptr)
{
    setupUI();
    setModal(true);
    setFixedSize(520, 300);
    setWindowTitle("Administrator Privileges Required");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    // Apply light theme styling
    setStyleSheet(
        "QDialog {"
        "    background-color: #f8f9fa;"
        "    color: #212529;"
        "}"
        "QLabel {"
        "    color: #495057;"
        "    font-size: 10pt;"
        "}"
        "QPushButton {"
        "    background-color: #e9ecef;"
        "    color: #495057;"
        "    border: 1px solid #ced4da;"
        "    border-radius: 8px;"
        "    padding: 10px 20px;"
        "    font-weight: 500;"
        "    min-height: 32px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #dee2e6;"
        "    border-color: #adb5bd;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #ced4da;"
        "}"
    );
}

void PrivilegeDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_iconLayout = new QHBoxLayout();
    m_buttonLayout = new QHBoxLayout();
    
    // Icon
    m_iconLabel = new QLabel();
    QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    m_iconLabel->setPixmap(icon.pixmap(48, 48));
    m_iconLabel->setAlignment(Qt::AlignCenter);
    
    // Title and message
    m_titleLabel = new QLabel("Administrator Access Required");
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2c5aa0;");
    
    m_messageLabel = new QLabel(
        "A3Guard requires administrator privileges to function properly.\n\n"
        "The following features require elevated permissions:\n"
        "• Network control (airplane mode)\n"
        "• USB device monitoring\n"
        "• System-wide application tracking\n"
        "• File system monitoring\n"
        "• Secure screenshot capture\n\n"
        "Please grant administrator privileges to continue or exit the application.");
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet("margin: 10px 0px;");
    
    // Buttons
    m_elevateButton = new QPushButton("Request Privileges");
    m_elevateButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #007bff;"
        "    color: white;"
        "    border: 1px solid #0056b3;"
        "    padding: 10px 20px;"
        "    border-radius: 8px;"
        "    font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0056b3;"
        "    border-color: #004085;"
        "    box-shadow: 0 4px 8px rgba(0,123,255,0.3);"
        "}"
        "QPushButton:pressed {"
        "    background-color: #004085;"
        "}");
    
    m_cancelButton = new QPushButton("Exit Application");
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #dc3545;"
        "    color: white;"
        "    border: 1px solid #c82333;"
        "    padding: 10px 20px;"
        "    border-radius: 8px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c82333;"
        "    border-color: #bd2130;"
        "    box-shadow: 0 4px 8px rgba(220,53,69,0.3);"
        "}");
    
    // Layout assembly
    m_iconLayout->addWidget(m_iconLabel);
    
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->addWidget(m_titleLabel);
    textLayout->addWidget(m_messageLabel);
    m_iconLayout->addLayout(textLayout);
    
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_elevateButton);
    
    m_mainLayout->addLayout(m_iconLayout);
    m_mainLayout->addStretch();
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_elevateButton, &QPushButton::clicked, this, &PrivilegeDialog::onElevateClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &PrivilegeDialog::onCancelClicked);
}

bool PrivilegeDialog::hasRootPrivileges()
{
    return getuid() == 0;
}

bool PrivilegeDialog::elevatePrivileges()
{
    if (hasRootPrivileges()) {
        return true;
    }
    
    PrivilegeDialog dialog;
    return dialog.requestElevation();
}

bool PrivilegeDialog::requestElevation()
{
    if (hasRootPrivileges()) {
        return true;
    }
    
    // Show modal dialog and block until user responds
    int result = exec();
    
    // If we get here with Accepted and authenticated, success
    return (result == QDialog::Accepted && m_authenticated);
}

void PrivilegeDialog::onElevateClicked()
{
    m_elevateButton->setEnabled(false);
    m_elevateButton->setText("Requesting...");
    
    // Try pkexec first (modern Linux systems)
    if (authenticateWithPkexec()) {
        m_authenticated = true;
        accept();
        return;
    }
    
    // Show instructions for manual elevation
    QMessageBox::critical(this, "Privilege Elevation Failed",
        "A3Guard requires administrator privileges to function.\n\n"
        "Please restart A3Guard with elevated privileges using:\n\n"
        "• sudo a3guard\n"
        "• pkexec a3guard\n\n"
        "The application will exit.");
    
    // Reset button and reject dialog
    m_elevateButton->setEnabled(true);
    m_elevateButton->setText("Request Privileges");
    reject();
}

void PrivilegeDialog::onCancelClicked()
{
    reject(); // Close the dialog with Rejected status
    // The requestElevation() function will handle QApplication::quit()
}

void PrivilegeDialog::onPasswordChanged()
{
    // Enable elevate button when password is entered
    m_elevateButton->setEnabled(!m_passwordEdit->text().isEmpty());
}

bool PrivilegeDialog::authenticateWithPkexec()
{
    // Use pkexec to get authentication and restart the application with elevated privileges
    QString appPath = QApplication::applicationFilePath();
    
    // Get X11 display information
    QString display = qgetenv("DISPLAY");
    if (display.isEmpty()) {
        display = ":0";
    }
    
    // Grant X11 access to local connections (needed for root to access X display)
    QProcess xhostProcess;
    xhostProcess.start("xhost", QStringList() << "+local:");
    xhostProcess.waitForFinished(2000);
    
    // Construct the command with proper X11 socket access
    // The key is to pass the DISPLAY variable so pkexec knows which X display to use
    QString cmd = QString("pkexec env DISPLAY=%1 %2")
                  .arg(display, appPath);
    
    // Execute the command detached so it runs independently
    bool success = QProcess::startDetached("bash", QStringList() << "-c" << cmd);
    
    if (success) {
        // Successfully started the elevation process
        // Exit this non-elevated instance after a brief delay to allow dialog to close
        QTimer::singleShot(500, QApplication::instance(), &QApplication::quit);
        return true;
    }
    
    return false;
}

bool PrivilegeDialog::authenticateWithSudo()
{
    // This is more complex and would require handling sudo password input
    // For now, we'll rely on pkexec or manual restart
    return false;
}