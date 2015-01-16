/**
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

#include "Connection.h"

#include <QByteArray>
#include <QDebug>

#include "../../Libraries/cobs/cobsr.h"
#include "../../Libraries/crc/crc16.h"

Connection::Connection(QObject *parent)
    : QObject(parent)
{
    connect(&timer, &QTimer::timeout, this, &Connection::pollMessages);

    timer.setInterval(10);
}

Connection::~Connection()
{
}

bool Connection::open(const QSerialPortInfo &serialPortInfo)
{
    close();

    serialPort.setPort(serialPortInfo);
    serialPort.setBaudRate(115200);

    if (!serialPort.open(QIODevice::ReadWrite)) {
        qDebug() << serialPort.errorString();
        return false;
    }

    emit connectionChanged();
    timer.start();
    return true;
}

void Connection::close()
{
    if (!serialPort.isOpen())
        return;

    timer.stop();
    serialPort.close();
    emit connectionChanged();
}

bool Connection::isOpen()
{
    return serialPort.isOpen();
}

QString Connection::portName()
{
    return serialPort.portName();
}

void Connection::sendMessage(const QByteArray &message)
{
    // TODO: mid(4) stört.
    //
    auto packet = encodeMessage(message .mid(4) );

    packet.append((char)0);

    if (serialPort.write(packet) >= 0) {
        stats.tx_bytes += packet.size();
        stats.tx_packets++;
    }
    else {
        stats.tx_errors++;
    }
}

void Connection::pollMessages()
{
    if (!isOpen())
        return;

    auto buf = serialPort.readAll();
    stats.rx_bytes += buf.size();
    rx_buf.append(buf);

    for (;;) {
        int pos = rx_buf.indexOf('\0');
        if (pos < 0)
            return;

        auto message = decodeMessage(rx_buf.left(pos));
        if (!message.isEmpty()) {
            stats.rx_packets++;
            emit messageReceived(message);
        }
        else {
            stats.rx_errors++;
        }

        rx_buf = rx_buf.mid(pos+1);
    }
}

QByteArray Connection::encodeMessage(const QByteArray &message)
{
    uint16_t crc;
    crc = crc16_init();
    crc = crc16_update(crc, (const uchar*)message.constData(), message.size());
    crc = crc16_finalize(crc);

    QByteArray packet_raw;
    packet_raw.append((char*)&crc, sizeof(crc));
    packet_raw.append(message);

    QByteArray packet_cobsr(
        COBSR_ENCODE_DST_BUF_LEN_MAX(packet_raw.size()), 0
    );

    auto r = cobsr_encode(
        (uint8_t*)packet_cobsr.data(), packet_cobsr.size(),
        (const uint8_t *)packet_raw.constData(), packet_raw.size()
    );

    Q_ASSERT(r.status == COBSR_ENCODE_OK);

    packet_cobsr.resize(r.out_len);

    return packet_cobsr;
}

QByteArray Connection::decodeMessage(const QByteArray &packet)
{
    QByteArray message(
        COBSR_DECODE_DST_BUF_LEN_MAX(packet.size()), 0
    );

    auto result = cobsr_decode(
        (uint8_t*)message.data(), message.size(),
        (const uint8_t *)packet.constData(), packet.size()
    );

    if (result.status != COBSR_DECODE_OK) {
        qDebug("COBS/R error %d", result.status);
        return QByteArray();
    }

    message.resize(result.out_len);

    if (message.size() < 2 + 2) {
        qDebug("Packet too short");
        return QByteArray();
    }

    uint16_t crc_remote = *(uint16_t*)message.constData();

    uint16_t crc_local;
    crc_local = crc16_init();
    crc_local = crc16_update(crc_local, (const uchar*)message.constData() + 2, message.size() - 2);
    crc_local = crc16_finalize(crc_local);

    if (crc_local != crc_remote) {
        qDebug("Checksum error. Expected 0x%04x, got 0x%04x", crc_local, crc_remote);
        return QByteArray();
    }

    return message.mid(2);
}

