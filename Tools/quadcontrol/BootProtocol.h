#ifndef BOOTPROTOCOL_H
#define BOOTPROTOCOL_H

#include <QByteArray>
#include <QString>
#include "Connection.h"

class BootProtocol
{
public:
    BootProtocol(Connection &connection);
    ~BootProtocol();

    Connection &connection;

    void reset_hack();
    void enter();
    void exit();
    void erase_sector(int sector);
    void write_data(int addr, const QByteArray &data);
    void verify(int addr, const QByteArray &data);
    void write_hex_file(const QString &fileName);
};

#endif // BOOTPROTOCOL_H
