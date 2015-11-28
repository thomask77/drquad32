/**
 * Copyright (C)2015 Thomas Kindlerrr <mail_drquad@t-kindler.de>
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

#include "cobsr_codec.h"
#include "crc16.h"
#include "errors.h"


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
        !serialPort->setBaudRate(baudRate) ||
        !serialPort->setFlowControl(QSerialPort::HardwareControl))
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
    resetDecoder();

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


QString hexdump(QByteArray a)
{
    QString s;

    for (uint8_t c: a)
        s += QString().sprintf("%02x ", c);

    return s;
}


QByteArray Connection::encodeMessage(msg_header *msg)
{
    msg->crc = crc16_init();
    msg->crc = crc16_update(msg->crc, (uchar*)&msg->id, 2 + msg->data_len);
    msg->crc = crc16_finalize(msg->crc);

    QByteArray packet;

    packet.resize( COBSR_ENCODE_DST_BUF_LEN_MAX(2 + 2 + msg->data_len) );

    cobsr_encoder_state enc = {
        /* in */        (char*)&msg->crc,
        /* in_end */    (char*)&msg->crc + 2 + 2 + msg->data_len,
        /* out */       (char*)packet.data(),
        /* out_end */   (char*)packet.data() + packet.size(),
        /* mode, run */ COBSR_NORMAL, 0
    };

    cobsr_encode(&enc);

    packet.resize(enc.out - packet.data());

    return packet;
}


bool Connection::sendMessage(msg_header *msg)
{
    if (!isOpen())
        return false;

    auto packet = encodeMessage(msg);

    /*
    qDebug() << "RAW:   " << hexdump(QByteArray((char*)&msg->crc, 2 + 2 + msg->data_len));
    qDebug() << "COBSR: " << hexdump(packet);
    */

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


void Connection::resetDecoder()
{
    decoder_state.out = (char*)&rx_message.h.crc;
    decoder_state.out_end = (char*)&rx_message.h.crc + 2 + 2 + sizeof(rx_message.data);
    decoder_state.code = 0;
    decoder_state.run = 0;
}


bool Connection::decodeMessage(QByteArray *buf)
{
    decoder_state.in = buf->data();
    decoder_state.in_end = buf->data() + buf->size();

    int complete = cobsr_decode(&decoder_state);

    size_t in_len = decoder_state.in - buf->data();
    size_t out_len = decoder_state.out - (char*)&rx_message.h.crc;

    stats.rx_bytes += in_len;
    *buf = buf->mid(in_len);

    if (!complete)
        return false;

    // Message received, check length and CRC
    //
    resetDecoder();

    if (out_len < 2 + 2) {
        qDebug() << "Message too short";
        stats.rx_errors++;
        return false;
    }

    rx_message.h.data_len = out_len -2 -2;

    uint16_t crc;
    crc = crc16_init();
    crc = crc16_update(crc, (uchar*)&rx_message.h.id, 2 + rx_message.h.data_len);
    crc = crc16_finalize(crc);

    if (crc != rx_message.h.crc) {
        qDebug() << QByteArray((char*)&rx_message.h.crc, 2 + 2 + rx_message.h.data_len).toHex();
        qDebug() << QString().sprintf("CRC expected 0x%04x, got 0x%04x", crc, rx_message.h.crc);
        return false;
    }

    stats.rx_packets++;
    return true;
}


void Connection::ioDevice_readyRead()
{
    if (!isOpen())
        return;

    auto buf = ioDevice->readAll();

    while (buf.size()) {
        if (decodeMessage(&buf))
            emit messageReceived(rx_message);
    }
}
