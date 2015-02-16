#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QSerialPort>

class Connection : public QObject
{
    Q_OBJECT

public:
    struct Statistics {
        uint    rx_bytes;
        uint    rx_packets;
        uint    rx_errors;
        uint    tx_bytes;
        uint    tx_packets;
        uint    tx_errors;
    };

    Statistics  stats;
    QSerialPort serialPort;

    explicit Connection(QObject *parent = 0);
    ~Connection();

    void sendMessage(const QByteArray &message);
    void pollMessages();

    bool open(const QSerialPortInfo &serialPortInfo);
    void close();
    bool isOpen();
    QString portName();

signals:
    void messageReceived(const QByteArray &message);
    void connectionChanged();

private:
    QByteArray rx_buf;

    static QByteArray encodeMessage(const QByteArray &message);
    static QByteArray decodeMessage(const QByteArray &packet);
};


#endif // CONNECTION_H
