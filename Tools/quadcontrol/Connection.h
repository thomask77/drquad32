#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>

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

    bool open(const QSerialPortInfo &serialPortInfo);
    void close();
    bool isOpen();

    void sendMessage(const QByteArray &message);

signals:
    void messageReceived(const QByteArray &message);
    void connectionChanged();

private:
    QSerialPort serialPort;
    QByteArray rx_buf;
    QTimer timer;

    void pollMessages();

    static QByteArray encodeMessage(const QByteArray &message);
    static QByteArray decodeMessage(const QByteArray &packet);
};


#endif // CONNECTION_H
