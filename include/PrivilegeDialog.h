#ifndef PRIVILEGEDIALOG_H
#define PRIVILEGEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QTimer>
#include <QProcess>

class PrivilegeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrivilegeDialog(QWidget *parent = nullptr);
    
    bool requestElevation();
    static bool hasRootPrivileges();
    static bool elevatePrivileges();

private slots:
    void onElevateClicked();
    void onCancelClicked();
    void onPasswordChanged();

private:
    void setupUI();
    bool authenticateWithSudo();
    bool authenticateWithPkexec();
    
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_iconLayout;
    QHBoxLayout* m_buttonLayout;
    
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QLineEdit* m_passwordEdit;
    QPushButton* m_elevateButton;
    QPushButton* m_cancelButton;
    
    bool m_authenticated;
    QProcess* m_elevationProcess;
};

#endif // PRIVILEGEDIALOG_H