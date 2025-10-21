#pragma once

#include <QObject>
#include <QNetworkInterface>
#include <memory>
#include "Common.h"

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    void enableAirplaneMode();
    void disableAirplaneMode();
    bool isAirplaneModeEnabled() const;
    
    void blockAllTraffic();
    void allowAllTraffic();
    bool isTrafficBlocked() const;
    
    QStringList getActiveInterfaces();

signals:
    void networkStateChanged(bool enabled);
    void interfaceStateChanged(const QString& interface, bool enabled);

private slots:
    void onNetworkStateChanged();

private:
    bool m_airplaneModeEnabled;
    bool m_trafficBlocked;
    QStringList m_disabledInterfaces;
    QStringList m_previousActiveInterfaces;
    
    QStringList getActiveNetworkInterfaces();
    void setInterfaceState(const QString& interface, bool enabled);
    void configureFirewall(bool block);
};