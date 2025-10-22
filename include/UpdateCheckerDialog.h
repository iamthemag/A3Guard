#ifndef UPDATECHECKERDIALOG_H
#define UPDATECHECKERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class UpdateCheckerDialog : public QDialog {
    Q_OBJECT
public:
    UpdateCheckerDialog(QWidget *parent = nullptr);
    
private:
    void setupUI();
    void applyTheme();
    void checkUpdates();
    
private slots:
    void onFinished();
    void showError(const QString &msg);
    
private:
    bool isNewerVersion(const QString &current, const QString &latest) const;
    
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_reply;
    QLabel *m_infoLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_closeButton;
};

#endif // UPDATECHECKERDIALOG_H
