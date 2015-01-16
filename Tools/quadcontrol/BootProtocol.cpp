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

#include "BootProtocol.h"
#include "../../Bootloader/msg_structs.h"
#include "../../Libraries/crc/crc32.h"
#include "IntelHexFile.h"
#include "MainWindow.h"

#include <QDebug>
#include <QApplication>
#include <QThread>
#include <QTime>

static const int TIMEOUT_MS = 2000;

BootProtocol::BootProtocol(QWidget *parent, Connection &connection)
    : QWidget(parent)
    , progressDialog(this)
    , connection(connection)
{
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setWindowTitle("Updating...");

    connect(&connection, &Connection::messageReceived, this, &BootProtocol::connection_messageReceived);
}

BootProtocol::~BootProtocol()
{
}

void BootProtocol::connection_messageReceived(const QByteArray &message)
{
    messageQueue.enqueue(message);
}

void BootProtocol::showProgress(int value, const QString &text)
{
    // qDebug("%2d: %s", value, qPrintable(text));
    progressDialog.setValue(value);
    progressDialog.setLabelText(text);
}


/********** Low level functions **********/

QByteArray BootProtocol::bootGetResponse(int timeout)
{
    while (timeout > 0) {
        if (progressDialog.wasCanceled()) {
            m_errorString = "User canceled";
            return QByteArray();
        }

        while (!messageQueue.isEmpty()) {
            auto msg = messageQueue.dequeue();
            auto id = *(uint16_t*)msg.constData();

            if (id == MSG_ID_BOOT_RESPONSE) {
                // qDebug() << "BOOT_RESPONSE: " << msg.mid(2).toHex();
                return msg;
            }
        }

        QApplication::processEvents();
        int t = std::min(10, timeout);
        QThread::msleep(t);
        timeout -= t;
    }

    m_errorString = "Time out";
    return QByteArray();
}

int BootProtocol::bootEnter()
{
    // Hack: CTRL-C\nreset\n
    //
    connection.sendMessage("\03\nreset\n");

    msg_boot_enter msg;
    msg.h.id = MSG_ID_BOOT_ENTER;
    msg.magic = 0xB00710AD;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );

    auto response = bootGetResponse(1000);
    if (response.isNull())
        return -1;

    return *(char*)response.mid(2).constData();
}

int BootProtocol::bootExit()
{
    msg_boot_exit msg;
    msg.h.id = MSG_ID_BOOT_EXIT;
    msg.h.data_len = 0;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );

    auto response = bootGetResponse(100);
    if (response.isNull())
        return -1;

    return *(char*)response.mid(2).constData();
}

int BootProtocol::bootEraseSector(uint sector)
{
    msg_boot_erase_sector msg;
    msg.h.id = MSG_ID_BOOT_ERASE_SECTOR;
    msg.sector = sector;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );

    auto response = bootGetResponse(2000);
    if (response.isNull())
        return -1;

    return *(char*)response.mid(2).constData();
}

int BootProtocol::bootWriteDataAsync(uint addr, const QByteArray &data, size_t size, off_t  offset)
{
    msg_boot_write_data msg;

    msg.h.id = MSG_ID_BOOT_WRITE_DATA;
    msg.address = addr;
    msg.length = std::min(size, sizeof(msg.data));
    memcpy(msg.data, data.constData() + offset, msg.length);

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );

    return msg.length;
}

int BootProtocol::bootWriteData(uint addr, const QByteArray &data, size_t size, off_t offset)
{
    static const int ACK_WINDOW = 16;
    int pending = 0;

    while ((unsigned)offset < size) {
        showProgress(
            offset * 100 / data.length(),
            QString().sprintf("Writing 0x%08x", addr)
        );

        int res = bootWriteDataAsync(addr, data, data.size() - offset, offset);
        if (res < 0)
            return -1;

        offset += res;
        addr   += res;

        // queue requests to mask the connection latency
        //
        if (pending >= ACK_WINDOW) {
            auto response = bootGetResponse(100);
            if (response.isNull())
                return -1;
        }
        else {
            pending += 1;
        }
    }

    // wait for pending responses
    //
    while (pending > 0) {
        auto response = bootGetResponse(100);
        if (response.isNull())
            return -1;

        pending--;
    }

    return size;
}

int BootProtocol::bootVerifyData(uint addr, const QByteArray &data, size_t size, off_t offset)
{
    msg_boot_verify msg;
    msg.h.id = MSG_ID_BOOT_VERIFY;
    msg.h.data_len = 4 + 4;

    msg.address = addr;
    msg.length = size;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );

    auto response = bootGetResponse(250);
    if (response.isNull())
        return -1;

    auto local_crc = crc32_init();
    local_crc = crc32_update(local_crc, (const uchar*)data.constData() + offset, size);
    local_crc = crc32_finalize(local_crc);

    auto remote_crc = *(uint*)response.mid(2).constData();

    // qDebug("Local CRC : %08x", local_crc);
    // qDebug("Remote CRC: %08x", remote_crc);

    if  (remote_crc != local_crc) {
        m_errorString = "CRC check failed";
        return -1;
    }

    return 1;
}


/********** Firmware update functions **********/

#define CHECK_CANCEL()    \
    if (progressDialog.wasCanceled())   \
        goto cancel;                    \


bool BootProtocol::sendHexFile(const QString &fileName)
{
    QTime t0, t_enter, t_erase, t_write, t_verify, t_total;

    progressDialog.reset();
    progressDialog.show();

    showProgress(0, "Loading " + fileName);

    IntelHexFile ih;
    if (!ih.loadHex(fileName)) {
        m_errorString = ih.errorString();
        return false;
    }

    uint start_addr  = ih.sections[0].offset;
    uint end_addr    = ih.sections[0].offset +  ih.sections[0].data.size();
    const auto &data = ih.sections[0].data;

    t0 = QTime::currentTime();

    qDebug("Start 0x%08x", start_addr);
    qDebug("End   0x%08x", end_addr);

    for (int i=0; i<5; i++) {
        showProgress(i, "Entering bootloader");
        bootEnter();
        CHECK_CANCEL();
    }

    t_enter = QTime::currentTime();

    for (int i=4; i<12; i++) {
        showProgress(
            10 + 10 * (i-4) / (12-4),
            QString().sprintf("Erasing sector %d...", i)
        );
        bootEraseSector(i);
        CHECK_CANCEL();
    }

    t_erase = QTime::currentTime();

    // skip initial 8 bytes (written after verify)
    //
    bootWriteData(start_addr + 8, data, data.size() - 8, 8);
    CHECK_CANCEL();

    t_write = QTime::currentTime();

    showProgress(85, "Verifying");
    bootVerifyData(start_addr + 8, data, data.size() - 8, 8);
    CHECK_CANCEL();

    t_verify = QTime::currentTime();

    showProgress(90, "Writing first 8 bytes");
    bootWriteData(start_addr, data, 8, 0);
    CHECK_CANCEL();

    showProgress(95, "Starting application");
    bootExit();
    CHECK_CANCEL();

    showProgress(100, "Done.");

    t_total =  QTime::currentTime();

    qDebug("  Enter:  %d ms", t0.msecsTo(t_enter));
    qDebug("  Erase:  %d ms", t_enter.msecsTo(t_erase));
    qDebug("  Write:  %d ms", t_erase.msecsTo(t_write));
    qDebug("  Verify: %d ms", t_write.msecsTo(t_verify));
    qDebug("  Total:  %d ms", t0.msecsTo(t_total));
    return true;

cancel:
    return true;
}


QString BootProtocol::errorString() const
{
    return m_errorString;
}
