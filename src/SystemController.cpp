#include "SystemController.h"

SystemController::SystemController(QObject* parent)
    : QObject(parent)
    , m_systemLocked(false)
    , m_kioskModeActive(false)
    , m_currentProcess(std::make_unique<QProcess>(this))
{
    connect(m_currentProcess.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SystemController::onProcessFinished);
}

SystemController::~SystemController()
{
}

bool SystemController::killProcess(const QString& processName)
{
    return executeCommand("pkill", QStringList() << processName);
}

bool SystemController::killProcessById(int pid)
{
    return executeCommand("kill", QStringList() << QString::number(pid));
}

QStringList SystemController::getRunningProcesses()
{
    return QStringList();
}

bool SystemController::blockApplication(const QString& application)
{
    if (!m_blockedApplications.contains(application)) {
        m_blockedApplications.append(application);
        emit applicationBlocked(application);
        return true;
    }
    return false;
}

bool SystemController::unblockApplication(const QString& application)
{
    return m_blockedApplications.removeAll(application) > 0;
}

bool SystemController::isApplicationBlocked(const QString& application) const
{
    return m_blockedApplications.contains(application);
}

bool SystemController::lockSystem()
{
    m_systemLocked = true;
    emit systemLocked();
    return executeCommand("loginctl", QStringList() << "lock-session");
}

bool SystemController::unlockSystem()
{
    m_systemLocked = false;
    return true;
}

void SystemController::enforceKioskMode()
{
    m_kioskModeActive = true;
    emit kioskModeChanged(true);
}

void SystemController::exitKioskMode()
{
    m_kioskModeActive = false;
    emit kioskModeChanged(false);
}

bool SystemController::isKioskModeActive() const
{
    return m_kioskModeActive;
}

void SystemController::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
}

bool SystemController::executeCommand(const QString& command, const QStringList& arguments)
{
    m_currentProcess->start(command, arguments);
    return m_currentProcess->waitForStarted();
}