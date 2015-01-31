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
    QSerialPort serialPort;
    QString m_errorString;
    QByteArray rx_buf;
    QTimer timer;

    void pollMessages();

    bool encodeMessage(msg_header *msg, QByteArray *packet);
    bool decodeMessage(const QByteArray &packet, msg_generic *msg);
};

// allg. URL evtl. besser?
// Connection::open(const QString &url);
// QUrl
//
// ser://COM12@115200
// wifly://host:port
// file://test.log
//

class NullConnection : public Connection {
    Q_OBJECT

public:
    NullConnection(QObject *parent = 0);

    ~NullConnection()
    {
    }
};


class SerialConnection : public Connection {
    Q_OBJECT

public:
    SerialConnection(const QString &portName, QObject *parent = 0);

    ~SerialConnection()
    {
    }

};


class WiFlyConnection : public Connection {
    Q_OBJECT

public:
    WiFlyConnection(const QString &hostName, quint16 port, QObject *parent = 0);

    ~WiFlyConnection()
    {
    }
};


class LogFileConnection : public Connection {
    Q_OBJECT

public:
    LogFileConnection(const QString &fileName, QObject *parent = 0);

    ~LogFileConnection()
    {
    }
};


#endif // CONNECTION_H
