#ifndef BOOTPROTOCOL_H
#define BOOTPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QQueue>

#include "Connection.h"
#include "QProgressDialogEx.h"

class BootProtocol : public QWidget
{
    Q_OBJECT

public:
    BootProtocol(QWidget *parent, Connection &connection);
    ~BootProtocol();

    bool sendHexFile(const QString &fileName);

    QString errorString();

private:
    QString m_errorString;
    QProgressDialogEx progressDialog;
    QQueue<msg_generic> messageQueue;
    Connection &connection;   

    void connection_messageReceived(const msg_generic &msg);
    void showProgress(int value, const QString &text);

    bool bootGetResponse(msg_boot_response *response, int timeout = 500);
    bool bootResetHack();
    bool bootEnter();
    bool bootExit();
    bool bootEraseSector(uint sector);
    bool bootWriteDataAsync(uint addr, const QByteArray &data);
    bool bootWriteData(uint addr, const QByteArray &data, int ack_window = 10);
    bool bootVerifyData(uint addr, const QByteArray &data);
};

#endif // BOOTPROTOCOL_H
