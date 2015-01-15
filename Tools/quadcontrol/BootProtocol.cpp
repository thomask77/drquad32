#include "BootProtocol.h"
#include "../../Bootloader/msg_structs.h"
#include "IntelHexFile.h"

#include <QDebug>
#include <QApplication>
#include <QThread>
#include <QTime>

static const int TIMEOUT_MS = 2000;

BootProtocol::BootProtocol(QWidget *parent, Connection &connection) :
    QObject(parent),
    progressDialog(parent),
    connection(connection)
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
    auto id = *(uint16_t*)message.constData();

    if (id == MSG_ID_BOOT_RESPONSE) {
        qDebug() << "BOOT_RESPONSE: " << message.mid(2).toHex();
        messageQueue.enqueue(message);
    }
}

void BootProtocol::bootGetResponse(int timeout)
{
    while (timeout > 0) {
        int t = std::min(10, timeout);
        QThread::msleep(t);
        timeout -= t;

        QApplication::processEvents();

        if (progressDialog.wasCanceled())
            return;
    }
}

void BootProtocol::bootEnter()
{
    // Hack: CTRL-C\nreset\n
    //
    connection.sendMessage("\03\nreset\n");

    msg_boot_enter msg;
    msg.h.id = MSG_ID_BOOT_ENTER;
    msg.magic = 0xB00710AD;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::bootExit()
{
    msg_boot_exit msg;
    msg.h.id = MSG_ID_BOOT_EXIT;
    msg.h.data_len = 0;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::bootEraseSector(uint sector)
{
    msg_boot_erase_sector msg;
    msg.h.id = MSG_ID_BOOT_ERASE_SECTOR;
    msg.sector = sector;

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::bootWriteData(uint addr, const QByteArray &data)
{
    static const int ACK_WINDOW = 16;

    int offset = 0;
    int pending = 0;

    while (offset < data.length()) {
        showProgress(
            offset * 100 / data.length(),
            QString().sprintf("Writing 0x%08x", addr + offset)
        );

        if (progressDialog.wasCanceled())
            return;

        msg_boot_write_data msg;

        auto chunk = data.mid(offset, sizeof(msg.data));

        msg.h.id = MSG_ID_BOOT_WRITE_DATA;
        msg.address = addr + offset;
        msg.length = chunk.size();

        memcpy(msg.data, chunk.constData(), chunk.size());

        connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );

        offset += chunk.size();

        // queue requests to mask the connection latency
        //
        if (pending >= ACK_WINDOW)
            bootGetResponse(100);
        else
            pending += 1;
    }

    // wait for pending responses
    //
    while (pending > 0) {
        bootGetResponse(100);
        pending--;
    }
}

void BootProtocol::bootVerify(uint addr, const QByteArray &data)
{
    msg_boot_verify msg;
    msg.h.id = MSG_ID_BOOT_VERIFY;
    msg.h.data_len = 4 + 4;

    msg.address = addr;
    msg.length = data.size();

    connection.sendMessage( QByteArray((char*)&msg, sizeof(msg)) );
}

void BootProtocol::showProgress(int value, const QString &text)
{
    qDebug("%d: %s", value,  qUtf8Printable(text));
    progressDialog.setValue(value);
    progressDialog.setLabelText(text);
}


#define CHECK_CANCEL()    \
    if (progressDialog.wasCanceled())   \
        goto cancel;                    \


void BootProtocol::writeHexFile(const QString &fileName)
{
    QTime t0, t_enter, t_erase, t_write, t_verify, t_total;

    progressDialog.reset();

    showProgress(0, "Loading " + fileName);

    IntelHexFile ih;
    if (!ih.loadHex(fileName))
        return;

    uint start_addr  = ih.sections[0].offset;
    uint end_addr    = ih.sections[0].offset +  ih.sections[0].data.size();
    const auto &data = ih.sections[0].data;

    t0 = QTime::currentTime();

    qDebug("Start 0x%08x", start_addr);
    qDebug("End   0x%08x", end_addr);

    for (int i=0; i<5; i++) {
        showProgress(i, "Entering bootloader");
        CHECK_CANCEL();

        bootEnter();
        bootGetResponse(100);
    }

    t_enter = QTime::currentTime();

    for (int i=4; i<12; i++) {
        showProgress(
            10 + 10 * (i-4) / (12-4),
            QString().sprintf("Erasing sector %d...", i)
        );
        CHECK_CANCEL();

        bootEraseSector(i);
        bootGetResponse(2000);
    }

    t_erase = QTime::currentTime();

    // skip initial 8 bytes (written after verify)
    //
    bootWriteData(start_addr + 8, data.mid(8));
    CHECK_CANCEL();

    t_write = QTime::currentTime();

    showProgress(85, "Verifying");
    CHECK_CANCEL();

    bootVerify(start_addr + 8, data.mid(8));

    t_verify = QTime::currentTime();

    showProgress(90, "Writing first 8 bytes");
    CHECK_CANCEL();

    bootWriteData(start_addr, data.mid(0, 8));

    showProgress(95, "Starting application");
    CHECK_CANCEL();

    bootExit();

    showProgress(100, "Done.");

    t_total =  QTime::currentTime();

    qDebug("  Enter:  %d ms", t0.msecsTo(t_enter));
    qDebug("  Erase:  %d ms", t_enter.msecsTo(t_erase));
    qDebug("  Write:  %d ms", t_erase.msecsTo(t_write));
    qDebug("  Verify: %d ms", t_write.msecsTo(t_verify));
    qDebug("  Total:  %d ms", t0.msecsTo(t_total));
    return;

cancel:
    return;
}

