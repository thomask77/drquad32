#ifndef WIFLYLISTENER_H
#define WIFLYLISTENER_H

#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QDateTime>


inline bool qMapLessThanKey(const QHostAddress &a, const QHostAddress &b)
{
    return a.toString() < b.toString();
}


struct WiFlyClientInfo
{
    QHostAddress    address;
    QDateTime       lastSeen;
    QString         apMacAddress;
    uint8_t         channel;
    uint8_t         rssi;
    uint16_t        localPort;
    uint32_t        rtcValue;
    uint16_t        uBat;
    uint16_t        gpioPins;
    QString         asciiTime;
    QString         version;
    QString         deviceId;
    uint16_t        bootTime_ms;
    uint16_t        sensors[8];

    bool fromPacket(const QByteArray &packet, const QHostAddress &addr);
};


class WiFlyListener : public QObject
{
    Q_OBJECT

public:
    explicit WiFlyListener(QObject *parent = 0);
    ~WiFlyListener();

    QMap<QHostAddress, WiFlyClientInfo> clients;

private:
    static const int BROADCAST_PORT = 55555;

    QUdpSocket  socket;
    void socket_readyRead();
};

#endif // WIFLYLISTENER_H
