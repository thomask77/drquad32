#include "BootProtocol.h"
#include "../../Bootloader/msg_structs.h"
#include <QDebug>

BootProtocol::BootProtocol(QObject *parent, Connection &connection) :
    QObject(parent),
    connection(connection)
{
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &BootProtocol::timer_timeout);
    connect(&connection, &Connection::messageReceived, this, &BootProtocol::connection_messageReceived);
}

BootProtocol::~BootProtocol()
{

}

/********** Bootloader commands **********/

void BootProtocol::resetHack()
{
    // CTRL-C\nreset\n
    //
    connection.sendMessage("\03\nreset\n");
}

void BootProtocol::enter()
{
    msg_boot_enter msg;
    msg.h.id = MSG_ID_BOOT_ENTER;
    msg.h.data_len = 4;
    msg.magic = 0xB00710AD;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) .mid(4) );
}

void BootProtocol::exit()
{
    msg_boot_exit msg;
    msg.h.id = MSG_ID_BOOT_EXIT;
    msg.h.data_len = 0;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::eraseSector(uint sector)
{
    msg_boot_erase_sector msg;
    msg.h.id = MSG_ID_BOOT_ERASE_SECTOR;
    msg.h.data_len = 1;
    msg.sector = sector;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) .mid(4) );
}

void BootProtocol::writeData(uint addr, const QByteArray &data)
{
    msg_boot_write_data msg;
    msg.h.id = MSG_ID_BOOT_WRITE_DATA;
    msg.h.data_len = 4 + 1 + data.size();
    msg.address = addr;
    msg.length = data.size();

    //TODO memcpy(msg.data, data.constData(), data.size());

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::verify(uint addr, const QByteArray &data)
{
    msg_boot_verify msg;
    msg.h.id = MSG_ID_BOOT_VERIFY;
    msg.h.data_len = 4 + 4;

    msg.address = addr;
    msg.length = data.size();

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

/********** Firmware update state-machine **********/

void BootProtocol::nextState(State state)
{
    this->state = state;
    this->stateIndex = 0;

    // deferred call
    //
    QTimer::singleShot(0, this, SLOT(nextStateSlot()));
}

void BootProtocol::nextStateSlot()
{
    handleMessage(NULL);
}

void BootProtocol::connection_messageReceived(const QByteArray &message)
{
    auto id = *(uint16_t*)message.constData();

    if (id == MSG_ID_BOOT_RESPONSE) {
        qDebug() << "BOOT_RESPONSE: " << message.mid(2).toHex();
        handleMessage(&message);
    }
}

void BootProtocol::handleMessage(const QByteArray *message)
{
    qDebug() << "handleMessage: " << message;

    switch (state) {
    case State::Idle:
    case State::Canceled:
    case State::Failed:
    case State::Done:
        return;

    case State::Enter:
        resetHack();
        enter();

        state = State::Erase;
        stateIndex = 0;
        break;

    case State::Erase:
        if (stateIndex < 12) {
            eraseSector(stateIndex + 4);
            stateIndex++;
        }
        else {
            // nextState(State::WriteApp);
            nextState(State::Done);
        }
        break;
/*
    case State::WriteApp:
        if (stateIndex < 100) {
            do {
                // writeData(xx, yy, zz);
                stateIndex++;
            } while (stateIndex < ACK_WINDOW);
        }
        else {
            nextState(State::VerifyApp);
        }
        break;

    case State::VerifyApp:

        if (stateIndex++ == 0) {
            // verify();
        }
        else {
            int local_checksum = 0;
            int remote_checksum = 0;

            if (local_checksum == remote_checksum)
                nextState(State::WriteHeader);
            else
                nextState(State::Failed);
        }
        break;

    case State::WriteHeader:
        nextState(State::Reboot);
        break;

    case State::Reboot:
        nextState(State::Done);
        break;

    default:
        qDebug("Unexpected message");
        state = State::Failed;
        break;
*/
    }
/*
    // restart timeout
    //
    timer.start(TIMEOUT_MS);
    emit showProgress((int)state, "...");
*/
}

void BootProtocol::startUpdate(const IntelHexFile &hexFile)
{
    this->hexFile = &hexFile;
    nextState(State::Enter);
}

void BootProtocol::cancelUpdate()
{
    showProgress(95, "User canceled");

    timer.stop();
    nextState(State::Canceled);
}

void BootProtocol::timer_timeout()
{
    showProgress(95, "Timeout");
    nextState(State::Failed);
}

