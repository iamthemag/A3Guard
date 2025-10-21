#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include "Common.h"

class ResourceMonitor : public QObject {
    Q_OBJECT

public:
    explicit ResourceMonitor(QObject* parent = nullptr);
    ~ResourceMonitor();

    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    double getCpuUsage() const;
    quint64 getMemoryUsage() const;
    double getMemoryPercentage() const;
    
    void setCpuLimit(double limit);
    void setMemoryLimit(quint64 limit);
    
    double getCpuLimit() const;
    quint64 getMemoryLimit() const;

signals:
    void cpuLimitExceeded(double current, double limit);
    void memoryLimitExceeded(quint64 current, quint64 limit);
    void resourceUpdate(double cpu, quint64 memory);

private slots:
    void checkResources();

private:
    bool m_monitoring;
    std::unique_ptr<QTimer> m_timer;
    
    double m_cpuUsage;
    quint64 m_memoryUsage;
    double m_cpuLimit;
    quint64 m_memoryLimit;
    
    // CPU calculation variables
    unsigned long long m_lastTotalTime;
    unsigned long long m_lastIdleTime;
    pid_t m_pid;
    
    double calculateCpuUsage();
    quint64 calculateMemoryUsage();
    void getCpuTimes(unsigned long long& totalTime, unsigned long long& idleTime);
    quint64 getTotalSystemMemory() const;
};