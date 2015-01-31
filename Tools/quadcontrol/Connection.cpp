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
        m_errorString = serialPort.errorString();
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

QString Connection::errorString()
{
    return m_errorString;
}

void Connection::sendMessage(msg_header *msg)
{
    QByteArray packet;

    encodeMessage(msg, &packet);
    packet += '\0';

    if (serialPort.write(packet) >= 0) {
        stats.tx_bytes += packet.size();
        stats.tx_packets++;
    }
    else {
        qDebug() << serialPort.errorString();
        stats.tx_errors++;
    }
}

void Connection::pollMessages()
{
    if (!isOpen())
        return;

    auto buf = serialPort.readAll();
    rx_buf.append(buf);

    stats.rx_bytes += buf.size();

    for (;;) {
        int pos = rx_buf.indexOf('\0');
        if (pos < 0)
            return;

        msg_generic msg;

        if (decodeMessage(rx_buf.left(pos), &msg)) {
            stats.rx_packets++;
            emit messageReceived(msg);
        }
        else {
            qDebug() << m_errorString;
            stats.rx_errors++;
        }

        rx_buf = rx_buf.mid(pos + 1);
    }
}

bool Connection::encodeMessage(msg_header *msg, QByteArray *packet)
{
    msg->crc = crc16_init();
    msg->crc = crc16_update(msg->crc, (uchar*)&msg->id, 2 + msg->data_len);
    msg->crc = crc16_finalize(msg->crc);

    packet->resize(
        COBSR_ENCODE_DST_BUF_LEN_MAX(2 + 2 + msg->data_len)
    );

    auto r = cobsr_encode(
        (uchar*)packet->data(), packet->size(),
        (uchar*)&msg->crc, 2 + 2 + msg->data_len
    );

    if (r.status != COBSR_ENCODE_OK) {
        m_errorString = QString().sprintf("COBS/R error %d", r.status);
        return false;
    }

    packet->resize(r.out_len);

    return true;
}

bool Connection::decodeMessage(const QByteArray &packet, msg_generic *msg)
{
    auto r = cobsr_decode(
        (uchar*)&msg->h.crc, 2 + 2 + sizeof(msg_generic::data),
        (uchar*)packet.data(), packet.size()
    );

    if (r.status != COBSR_DECODE_OK) {
        m_errorString = QString().sprintf("COBS/R error %d", r.status);
        return false;
    }

    if (r.out_len < 2 + 2) {
        m_errorString = QString().sprintf("Packet too short");
        return false;
    }

    msg->h.data_len = r.out_len -2 -2;

    uint16_t crc;
    crc = crc16_init();
    crc = crc16_update(crc, (uchar*)&msg->h.id, 2 + msg->h.data_len);
    crc = crc16_finalize(crc);

    if (crc != msg->h.crc) {
        qDebug() << QByteArray((char*)&msg->h.crc, 2 + 2 + msg->h.data_len).toHex();
        m_errorString = QString().sprintf("CRC expected 0x%04x, got 0x%04x", crc, msg->h.crc);
        return false;
    }

    return true;
}
