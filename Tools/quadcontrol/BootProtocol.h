#ifndef BOOTPROTOCOL_H
#define BOOTPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include "IntelHexFile.h"
#include "Connection.h"


class BootProtocol : public QObject
{
    Q_OBJECT

public:
    BootProtocol(QObject *parent, Connection &connection);
    ~BootProtocol();

    void startUpdate(const IntelHexFile &hexFile);
    void cancelUpdate();


public slots:
    void nextStateSlot();

signals:
    void showProgress(int value, const QString &text);

private:
    const IntelHexFile *hexFile;
    uint  address;

    const int TIMEOUT_MS = 2000;
    const int ACK_WINDOW = 16;

    enum class State {
        Idle,
        Enter,
        Erase,
        WriteApp,
        VerifyApp,
        WriteHeader,
        Reboot,

        Done,
        Failed,
        Canceled
    };

    State state = State::Idle;
    int stateIndex = 0;

    void handleMessage(const QByteArray *message);
    void nextState(State state);

    Connection &connection;
    void connection_messageReceived(const QByteArray &message);

    QTimer timer;
    void timer_timeout();

    void resetHack();
    void enter();
    void exit();
    void eraseSector(uint sector);
    void writeData(uint addr, const QByteArray &data);
    void verify(uint addr, const QByteArray &data);
};

#endif // BOOTPROTOCOL_H
