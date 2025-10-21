#include "ResourceMonitor.h"
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDebug>
#include <unistd.h>
#include <fstream>
#include <string>

ResourceMonitor::ResourceMonitor(QObject* parent)
    : QObject(parent)
    , m_monitoring(false)
    , m_timer(std::make_unique<QTimer>(this))
    , m_cpuUsage(0.0)
    , m_memoryUsage(0)
    , m_cpuLimit(DEFAULT_MAX_CPU_USAGE)
    , m_memoryLimit(DEFAULT_MAX_MEMORY_MB * 1024 * 1024)
    , m_lastTotalTime(0)
    , m_lastIdleTime(0)
{
    connect(m_timer.get(), &QTimer::timeout, this, &ResourceMonitor::checkResources);
    
    // Get process ID for memory monitoring
    m_pid = getpid();
    
    // Initialize CPU calculation values
    getCpuTimes(m_lastTotalTime, m_lastIdleTime);
}

ResourceMonitor::~ResourceMonitor()
{
    stopMonitoring();
}

void ResourceMonitor::startMonitoring()
{
    if (!m_monitoring) {
        m_monitoring = true;
        m_timer->start(DEFAULT_RESOURCE_CHECK_INTERVAL);
    }
}

void ResourceMonitor::stopMonitoring()
{
    if (m_monitoring) {
        m_monitoring = false;
        m_timer->stop();
    }
}

bool ResourceMonitor::isMonitoring() const
{
    return m_monitoring;
}

double ResourceMonitor::getCpuUsage() const { return m_cpuUsage; }
quint64 ResourceMonitor::getMemoryUsage() const { return m_memoryUsage; }

double ResourceMonitor::getMemoryPercentage() const
{
    quint64 totalMemory = getTotalSystemMemory();
    if (totalMemory > 0) {
        return (static_cast<double>(m_memoryUsage) / totalMemory) * 100.0;
    }
    return 0.0;
}

void ResourceMonitor::setCpuLimit(double limit) { m_cpuLimit = limit; }
void ResourceMonitor::setMemoryLimit(quint64 limit) { m_memoryLimit = limit; }
double ResourceMonitor::getCpuLimit() const { return m_cpuLimit; }
quint64 ResourceMonitor::getMemoryLimit() const { return m_memoryLimit; }

void ResourceMonitor::checkResources()
{
    m_cpuUsage = calculateCpuUsage();
    m_memoryUsage = calculateMemoryUsage();
    
    if (m_cpuUsage > m_cpuLimit) {
        emit cpuLimitExceeded(m_cpuUsage, m_cpuLimit);
    }
    
    if (m_memoryUsage > m_memoryLimit) {
        emit memoryLimitExceeded(m_memoryUsage, m_memoryLimit);
    }
    
    emit resourceUpdate(m_cpuUsage, m_memoryUsage);
}

double ResourceMonitor::calculateCpuUsage()
{
    unsigned long long totalTime, idleTime;
    getCpuTimes(totalTime, idleTime);
    
    unsigned long long totalDiff = totalTime - m_lastTotalTime;
    unsigned long long idleDiff = idleTime - m_lastIdleTime;
    
    double cpuUsage = 0.0;
    if (totalDiff > 0) {
        cpuUsage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
    }
    
    // Update last values for next calculation
    m_lastTotalTime = totalTime;
    m_lastIdleTime = idleTime;
    
    return cpuUsage;
}

quint64 ResourceMonitor::calculateMemoryUsage()
{
    // Read memory usage from /proc/self/status
    QFile file("/proc/self/status");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("VmRSS:")) {
            // Extract memory value in kB
            QStringList parts = line.split(QRegExp("\\s+"));
            if (parts.size() >= 2) {
                bool ok;
                quint64 memoryKB = parts[1].toULongLong(&ok);
                if (ok) {
                    return memoryKB * 1024; // Convert to bytes
                }
            }
            break;
        }
    }
    
    return 0;
}

void ResourceMonitor::getCpuTimes(unsigned long long& totalTime, unsigned long long& idleTime)
{
    // Read CPU times from /proc/stat
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        totalTime = idleTime = 0;
        return;
    }
    
    QString line = file.readLine(); // First line contains overall CPU stats
    file.close();
    
    // Format: cpu user nice system idle iowait irq softirq steal guest guest_nice
    QStringList parts = line.split(QRegExp("\\s+"));
    if (parts.size() >= 5) {
        unsigned long long user = parts[1].toULongLong();
        unsigned long long nice = parts[2].toULongLong();
        unsigned long long system = parts[3].toULongLong();
        unsigned long long idle = parts[4].toULongLong();
        unsigned long long iowait = (parts.size() > 5) ? parts[5].toULongLong() : 0;
        unsigned long long irq = (parts.size() > 6) ? parts[6].toULongLong() : 0;
        unsigned long long softirq = (parts.size() > 7) ? parts[7].toULongLong() : 0;
        
        totalTime = user + nice + system + idle + iowait + irq + softirq;
        idleTime = idle + iowait;
    } else {
        totalTime = idleTime = 0;
    }
}

quint64 ResourceMonitor::getTotalSystemMemory() const
{
    // Read total memory from /proc/meminfo
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 8ULL * 1024 * 1024 * 1024; // Default to 8GB if can't read
    }
    
    QTextStream in(&file);
    QString line = in.readLine(); // First line is MemTotal
    file.close();
    
    if (line.startsWith("MemTotal:")) {
        QStringList parts = line.split(QRegExp("\\s+"));
        if (parts.size() >= 2) {
            bool ok;
            quint64 memoryKB = parts[1].toULongLong(&ok);
            if (ok) {
                return memoryKB * 1024; // Convert to bytes
            }
        }
    }
    
    return 8ULL * 1024 * 1024 * 1024; // Default to 8GB
}
