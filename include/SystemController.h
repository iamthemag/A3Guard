#pragma once

#include <QObject>
#include <QProcess>
#include <memory>
#include "Common.h"

class SystemController : public QObject {
    Q_OBJECT

public:
    explicit SystemController(QObject* parent = nullptr);
    ~SystemController();

    bool killProcess(const QString& processName);
    bool killProcessById(int pid);
    QStringList getRunningProcesses();
    
    bool blockApplication(const QString& application);
    bool unblockApplication(const QString& application);
    bool isApplicationBlocked(const QString& application) const;
    
    bool lockSystem();
    bool unlockSystem();
    
    void enforceKioskMode();
    void exitKioskMode();
    bool isKioskModeActive() const;

signals:
    void processKilled(const QString& processName, int pid);
    void applicationBlocked(const QString& application);
    void systemLocked();
    void kioskModeChanged(bool active);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QStringList m_blockedApplications;
    bool m_systemLocked;
    bool m_kioskModeActive;
    std::unique_ptr<QProcess> m_currentProcess;
    
    bool executeCommand(const QString& command, const QStringList& arguments = QStringList());
};