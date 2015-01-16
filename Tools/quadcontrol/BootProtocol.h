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

    QString errorString() const;

private:
    QString m_errorString;

    QProgressDialogEx progressDialog;
    QQueue<QByteArray> messageQueue;

    Connection &connection;
    void connection_messageReceived(const QByteArray &message);

    void showProgress(int value, const QString &text);

    QByteArray bootGetResponse(int timeout);

    int  bootEnter();
    int  bootExit();
    int  bootEraseSector(uint sector);
    int  bootWriteDataAsync(uint addr, const QByteArray &data, size_t size, off_t offset);
    int  bootWriteData(uint addr, const QByteArray &data, size_t size, off_t offset);
    int  bootVerifyData(uint addr, const QByteArray &data, size_t size, off_t offset);

};

#endif // BOOTPROTOCOL_H
