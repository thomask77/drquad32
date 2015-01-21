#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>

#include "../../Bootloader/msg_structs.h"


class Connection : public QObject
{
    Q_OBJECT

public:
    explicit Connection(QObject *parent = 0);
    ~Connection();

    struct Statistics {
        uint    rx_bytes;
        uint    rx_packets;
        uint    rx_errors;
        uint    tx_bytes;
        uint    tx_packets;
        uint    tx_errors;
    };

    Statistics stats = Statistics();

    QString portName();
    QString errorString();

    bool open(const QSerialPortInfo &serialPortInfo);
    void close();
    bool isOpen();

    void sendMessage(msg_header *msg);

signals:
    void messageReceived(const msg_generic &msg);
    void connectionChanged();

private:
    QString m_errorString;
    QSerialPort serialPort;
    QByteArray rx_buf;
    QTimer timer;

    void pollMessages();

    bool encodeMessage(msg_header *msg, QByteArray *packet);
    bool decodeMessage(const QByteArray &packet, msg_generic *msg);
};




#endif // CONNECTION_H
