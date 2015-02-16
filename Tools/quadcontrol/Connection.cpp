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

#include <QDebug>
#include <QByteArray>
#include <QTcpSocket>
#include <QFile>

#include "../../Libraries/cobs/cobsr.h"
#include "../../Libraries/crc/crc16.h"


Connection::Connection(QObject *parent)
    : QObject(parent)
{
}


Connection::~Connection()
{
}


bool Connection::openUrl(const QUrl &url)
{
    bool res = false;

    if (url.scheme() == "serial")
        res = openSerial(url.path(), url.query().toInt());
    else if (url.scheme() == "wifly")
        res = openSocket(url.host(), url.port());
    else if (url.scheme() == "file" || url.scheme() == "")
        res = openFile(url.path());
    else
        m_errorString = "Unknown URL scheme";

    if (res)
        m_url = url;

    return res;
}


bool Connection::openSerial(const QString &portName, int baudRate)
{
    auto serialPort = new QSerialPort(portName, this);

    if (!serialPort->open(QIODevice::ReadWrite) ||
        !serialPort->setBaudRate(baudRate) )
    {
        m_errorString = serialPort->errorString();
        serialPort->close();
        delete serialPort;
        return false;
    }

    return openIoDevice(serialPort);
}


bool Connection::openSocket(const QString &address, quint16 port)
{
    auto socket = new QTcpSocket(this);

    socket->connectToHost(address, port);

    if (!socket->waitForConnected(10000)) {
        m_errorString = socket->errorString();
        delete socket;
        return false;
    }

    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    return openIoDevice(socket);
}


bool Connection::openFile(const QString &fileName)
{
    auto file = new QFile(fileName);

    if (!file->open(QIODevice::ReadOnly)) {
        m_errorString = file->errorString();
        delete file;
        return false;
    }

    return openIoDevice(file);
}


bool Connection::openIoDevice(QIODevice *ioDevice)
{
    this->ioDevice = ioDevice;
    connect(ioDevice, &QIODevice::readyRead, this, &Connection::ioDevice_readyRead);

    emit connectionChanged();
    return true;
}


void Connection::close()
{
    if (ioDevice) {
        ioDevice->close();
        delete ioDevice;
        ioDevice = NULL;

        emit connectionChanged();
    }
}


bool Connection::isOpen()
{
    return ioDevice && ioDevice->isOpen();
}


QUrl Connection::getUrl()
{
    return isOpen() ? m_url : QUrl();
}


QString Connection::errorString()
{
    return m_errorString;
}


bool Connection::sendMessage(msg_header *msg)
{
    if (!isOpen())
        return false;

    QByteArray packet;

    encodeMessage(msg, &packet);
    packet += '\0';

    int res = ioDevice->write(packet);
    if (res < 0) {
        qDebug() << ioDevice->errorString();
        stats.tx_errors++;
        return false;
    }

    stats.tx_bytes += res;
    stats.tx_packets++;

    return true;
}


void Connection::ioDevice_readyRead()
{
    if (!isOpen())
        return;

    auto buf = ioDevice->readAll();
    rx_buf.append(buf);
    stats.rx_bytes += buf.size();

    parseMessages();
}


void Connection::parseMessages()
{
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
