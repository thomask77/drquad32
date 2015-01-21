#ifndef WIFLYLISTENER_H
#define WIFLYLISTENER_H

#include <QObject>
#include <QUdpSocket>
#include <QDateTime>

class WiFlyListener : public QObject
{
    Q_OBJECT

public:
    explicit WiFlyListener(QObject *parent = 0);
    ~WiFlyListener();

    struct ClientInfo {
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
    };

    QMap<QHostAddress, ClientInfo>   clients;

private:
    static const int BROADCAST_PORT = 55555;

    QUdpSocket  socket;

    bool handlePacket(const QByteArray &packet, const QHostAddress &addr);
    void socket_readyRead();
};

#endif // WIFLYLISTENER_H
