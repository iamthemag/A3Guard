#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTimer>

class UpdateCheckerDialog : public QDialog {
    Q_OBJECT
public:
    UpdateCheckerDialog(QWidget *parent = nullptr) : QDialog(parent), m_manager(new QNetworkAccessManager(this)), m_reply(nullptr) {
        setupUI();
        applyTheme();
        checkUpdates();
    }
private:
    void setupUI() {
        setWindowTitle("A3Guard - Check for Updates");
        setFixedSize(450, 200);
        setModal(true);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(15);
        layout->setContentsMargins(20, 20, 20, 20);
        QLabel *titleLabel = new QLabel("Checking for Updates...");
        titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c5aa0;");
        m_infoLabel = new QLabel("Connecting to GitHub API");
        m_infoLabel->setStyleSheet("color: #495057; font-size: 11px;");
        m_progressBar = new QProgressBar();
        m_progressBar->setRange(0, 0);
        m_progressBar->setStyleSheet("QProgressBar { border: 1px solid #dee2e6; border-radius: 4px; background-color: #f8f9fa; height: 24px; } QProgressBar::chunk { background-color: #007bff; border-radius: 3px; }");
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        m_closeButton = new QPushButton("Close");
        m_closeButton->setEnabled(false);
        m_closeButton->setMinimumWidth(80);
        m_closeButton->setStyleSheet("QPushButton { background-color: #6c757d; color: white; border: none; border-radius: 6px; padding: 8px 16px; } QPushButton:hover { background-color: #5a6268; }");
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(m_closeButton);
        layout->addWidget(titleLabel);
        layout->addWidget(m_infoLabel);
        layout->addWidget(m_progressBar);
        layout->addStretch();
        layout->addLayout(buttonLayout);
    }
    void applyTheme() {
        setStyleSheet("QDialog { background-color: #f8f9fa; color: #212529; } QLabel { color: #495057; }");
    }
    void checkUpdates() {
        QUrl url("https://api.github.com/repos/iamthemag/A3Guard/releases/latest");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, "A3Guard-UpdateChecker/1.0");
        m_reply = m_manager->get(request);
        connect(m_reply, &QNetworkReply::finished, this, &UpdateCheckerDialog::onFinished);
        QTimer::singleShot(15000, this, [this]() { if (m_reply && m_reply->isRunning()) { m_reply->abort(); showError("Connection timeout."); } });
    }
private slots:
    void onFinished() {
        if (!m_reply) return;
        if (m_reply->error() != QNetworkReply::NoError) { showError("Failed to connect: " + m_reply->errorString()); m_reply->deleteLater(); return; }
        QByteArray data = m_reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) { showError("Invalid response from server."); m_reply->deleteLater(); return; }
        QJsonObject obj = doc.object();
        QString latestVersion = obj["tag_name"].toString();
        if (latestVersion.startsWith('v')) latestVersion = latestVersion.mid(1);
        m_reply->deleteLater();
        bool updateAvailable = isNewerVersion("1.0.0", latestVersion);
        m_progressBar->setRange(0, 1);
        m_progressBar->setValue(1);
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Update Check Complete");
        msgBox.setStyleSheet("QMessageBox { background-color: #f8f9fa; } QMessageBox QLabel { color: #212529; } QMessageBox QPushButton { background-color: #007bff; color: white; border: none; border-radius: 6px; padding: 8px 24px; } QMessageBox QPushButton:hover { background-color: #0056b3; }");
        if (updateAvailable) {
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText("<b style='color: #28a745; font-size: 12pt;'>✓ Update Available!</b>");
            msgBox.setInformativeText("<b>Current:</b> <span style='color: #007bff;'><b>1.0.0</b></span><br><b>Latest:</b> <span style='color: #28a745;'><b>" + latestVersion + "</b></span><br><br><a href='https://github.com/iamthemag/A3Guard/releases'>Download from GitHub</a>");
        } else {
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText("<b style='color: #28a745; font-size: 12pt;'>✓ You're Up to Date</b>");
            msgBox.setInformativeText("<b>Current:</b> <span style='color: #007bff;'><b>1.0.0</b></span><br><b>Latest:</b> <span style='color: #28a745;'><b>" + latestVersion + "</b></span>");
        }
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        accept();
    }
    void showError(const QString &msg) {
        m_progressBar->setRange(0, 1);
        m_progressBar->setValue(1);
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Update Check Failed");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStyleSheet("QMessageBox { background-color: #f8f9fa; } QMessageBox QPushButton { background-color: #dc3545; color: white; border: none; border-radius: 6px; padding: 8px 24px; } QMessageBox QPushButton:hover { background-color: #c82333; }");
        msgBox.setText("<b style='color: #dc3545; font-size: 12pt;'>✗ Unable to Check for Updates</b>");
        msgBox.setInformativeText(msg);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        accept();
    }
    bool isNewerVersion(const QString &current, const QString &latest) const {
        QStringList cp = current.split('.');
        QStringList lp = latest.split('.');
        while (cp.size() < 3) cp.append("0");
        while (lp.size() < 3) lp.append("0");
        for (int i = 0; i < 3; ++i) {
            int cn = cp[i].toInt();
            int ln = lp[i].toInt();
            if (ln > cn) return true;
            if (ln < cn) return false;
        }
        return false;
    }
private:
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_reply;
    QLabel *m_infoLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_closeButton;
};

#include "a3guard-update-checker.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    UpdateCheckerDialog dialog;
    dialog.show();
    return app.exec();
}
