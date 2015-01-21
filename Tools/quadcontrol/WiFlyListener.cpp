/**
 * UDP broadcast receiver for Roving Networks/Microchip WiFly Modules
 *
 * Copyright (C)2015 Thomas Kindler <mail_drquad@t-kindler.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WiFlyListener.h"
#include <QtEndian>


#pragma pack(push, 1)
struct BroadcastPacket {
    uint8_t     apMacAddress[6];    // 0-5
    uint8_t     channel;            // 6
    uint8_t     rssi;               // 7
    uint16_t    be_localPort;       // 8
    uint32_t    be_rtcValue;        // 10
    uint16_t    be_uBat;            // 14
    uint16_t    be_gpioPins;        // 16
    char        asciiTime[13+1];    // 18
    char        version[26+2];      // 32
    char        deviceId[32];       // 60
    uint16_t    be_bootTime;        // 92
    uint16_t    be_sensors[8];      // 94
};
#pragma pack(pop)


inline bool qMapLessThanKey(const QHostAddress &key1, const QHostAddress &key2)
{
    return key1.toString() < key2.toString();
}


WiFlyListener::WiFlyListener(QObject *parent)
    : QObject(parent)
    , socket(this)
{
    socket.bind(BROADCAST_PORT, QUdpSocket::ShareAddress);
    connect(&socket, &QUdpSocket::readyRead, this, &WiFlyListener::socket_readyRead);
}


WiFlyListener::~WiFlyListener()
{
}


bool WiFlyListener::handlePacket(const QByteArray &packet, const QHostAddress &addr)
{
    if (packet.size() != sizeof(BroadcastPacket))
        return false;

    auto &p = *(const BroadcastPacket *)packet.constData();

    ClientInfo info;

    info.lastSeen = QDateTime::currentDateTime();

    info.apMacAddress.sprintf(
        "%02x:%02x:%02x:%02x:%02x:%02x",
        p.apMacAddress[0], p.apMacAddress[1],
        p.apMacAddress[2], p.apMacAddress[3],
        p.apMacAddress[4], p.apMacAddress[5]
    );

    info.channel    = p.channel;
    info.rssi       = p.rssi;
    info.localPort  = qFromBigEndian(p.be_localPort);
    info.rtcValue   = qFromBigEndian(p.be_rtcValue);
    info.uBat       = qFromBigEndian(p.be_uBat);
    info.gpioPins   = qFromBigEndian(p.be_gpioPins);

    info.asciiTime.sprintf("%.*s", (int)sizeof(p.asciiTime), p.asciiTime);
    info.version.sprintf("%.*s", (int)sizeof(p.version), p.version);
    info.deviceId.sprintf("%.*s", (int)sizeof(p.deviceId), p.deviceId);
    info.bootTime_ms = qFromBigEndian(p.be_bootTime);

    for (int i=0; i<8; i++)
        info.sensors[i] = qFromBigEndian(p.be_sensors[i]);

    clients[addr] = info;

    return true;
}


void WiFlyListener::socket_readyRead()
{
    while (socket.hasPendingDatagrams()) {
        QByteArray      packet;
        QHostAddress    addr;

        packet.resize(socket.pendingDatagramSize());
        socket.readDatagram(packet.data(), packet.size(), &addr);

        // qDebug(
        //     "WiFly message from %s: %s",
        //     qPrintable(addr.toString()),
        //     qPrintable(packet.toHex())
        // );

        handlePacket(packet, addr);
    }
}
