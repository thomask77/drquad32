#ifndef BOOTPROTOCOL_H
#define BOOTPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QQueue>

#include "Connection.h"
#include "QProgressDialogEx.h"

class BootProtocol : public QObject
{
    Q_OBJECT

public:
    BootProtocol(Connection &connection, QObject *parent = 0);
    ~BootProtocol();

    QString errorString();

    int ack_window = 10;

    bool bootReboot();
    bool sendHexFile(const QString &fileName);

private:
    QString m_errorString;
    QProgressDialogEx progressDialog;
    QQueue<msg_generic> messageQueue;
    Connection &connection;   

    void connection_messageReceived(const msg_generic &msg);
    void showProgress(int value, const QString &text);

    bool bootGetResponse(msg_boot_response *response, int timeout = 2000);
    bool bootEnter();
    bool bootExit();
    bool bootEraseSector(uint sector);
    bool bootWriteDataAsync(uint addr, const QByteArray &data);
    bool bootWriteData(uint addr, const QByteArray &data);
    bool bootVerifyData(uint addr, const QByteArray &data);
};

#endif // BOOTPROTOCOL_H
