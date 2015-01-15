#ifndef BOOTPROTOCOL_H
#define BOOTPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QQueue>
#include <QProgressDialog>

#include "Connection.h"

class BootProtocol : public QObject
{
    Q_OBJECT

public:
    BootProtocol(QWidget *parent, Connection &connection);
    ~BootProtocol();

    void writeHexFile(const QString &fileName);

private:
    QProgressDialog progressDialog;
    QQueue<QByteArray> messageQueue;

    Connection &connection;
    void connection_messageReceived(const QByteArray &message);

    void showProgress(int value, const QString &text);

    void bootGetResponse(int timeout);
    void bootEnter();
    void bootExit();
    void bootEraseSector(uint sector);
    void bootWriteData(uint addr, const QByteArray &data);
    void bootVerify(uint addr, const QByteArray &data);
};

#endif // BOOTPROTOCOL_H
