#include "BootProtocol.h"
#include "../../Bootloader/msg_structs.h"

BootProtocol::BootProtocol(Connection &connection) :
    connection(connection)
{

}

BootProtocol::~BootProtocol()
{

}

void BootProtocol::reset_hack()
{
    // TODO
}

void BootProtocol::enter()
{
    msg_boot_enter msg;
    msg.h.id = MSG_ID_BOOT_ENTER;
    msg.h.data_len = 4;
    msg.magic = 0xB00710AD;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::exit()
{
    msg_boot_exit msg;
    msg.h.id = MSG_ID_BOOT_EXIT;
    msg.h.data_len = 0;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::erase_sector(int sector)
{
    msg_boot_erase_sector msg;
    msg.h.id = MSG_ID_BOOT_ERASE_SECTOR;
    msg.h.data_len = 1;
    msg.sector = sector;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::write_data(int addr, const QByteArray &data)
{
    msg_boot_write_data msg;
    msg.h.id = MSG_ID_BOOT_WRITE_DATA;
    msg.h.data_len = 4 + 1 + data.size();
    msg.address = addr;
    msg.length = data.size();

    //TODO memcpy(msg.data, data.constData(), data.size());

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::verify(int addr, const QByteArray &data)
{
    msg_boot_verify msg;
    msg.h.id = MSG_ID_BOOT_VERIFY;
    msg.h.data_len = 4 + 4;

    msg.address = addr;
    msg.length = data.size();

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

