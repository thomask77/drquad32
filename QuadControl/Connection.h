#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QUrl>

#include "msg_structs.h"
#include "cobsr_codec.h"

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

    bool openUrl(const QUrl &url);
    QUrl getUrl();

    QString errorString();

    void close();
    bool isOpen();

    bool sendMessage(msg_header *msg);

signals:
    void messageReceived(const msg_generic &msg);
    void connectionChanged();

private:
    QIODevice   *ioDevice = NULL;
    QUrl        m_url;
    QString     m_errorString;

    msg_generic rx_message;
    cobsr_decoder_state decoder_state;

    bool openIoDevice(QIODevice *ioDevice);
    bool openSerial(const QString &getUrl, int baudRate);
    bool openSocket(const QString &address, quint16 port);
    bool openFile(const QString &fileName);

    void resetDecoder();
    void ioDevice_readyRead();

    bool       decodeMessage(QByteArray *buf);
    QByteArray encodeMessage(msg_header *msg);
};


#endif // CONNECTION_H
