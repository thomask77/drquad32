#include "UpdateWindow.h"
#include "ui_UpdateWindow.h"

#include <QSettings>
#include <QFileSystemModel>
#include <QCompleter>
#include <QMimeData>
#include <QFileDialog>
#include <QDebug>
#include <QProgressDialog>
#include <QMessageBox>
#include <QByteArray>
#include <QTime>

#include "MainWindow.h"
#include "IntelHexFile.h"

UpdateWindow::UpdateWindow(MainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::UpdateWindow),
    mainWindow(parent),
    progressDialog(this),
    bootProtocol(this, parent->connection)
{
    ui->setupUi(this);

    ui->lineEdit->setText( QSettings().value("update_filename").toString() );

    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setWindowTitle("Updating...");

    connect(ui->lineEdit, &QLineEdit::textChanged, this, &UpdateWindow::lineEdit_textChanged);
    connect(ui->browseButton, &QPushButton::clicked, this, &UpdateWindow::browseButton_clicked);
    connect(ui->updateButton, &QPushButton::clicked, this, &UpdateWindow::updateButton_clicked);
    connect(&mainWindow->connection, &Connection::connectionChanged, this, &UpdateWindow::connectionChanged);
    connect(&bootProtocol, &BootProtocol::showProgress, this, &UpdateWindow::bootProtocol_showProgress);

    auto model = new QFileSystemModel(this);
    model->setRootPath(QDir::currentPath());
    model->setNameFilters(QStringList { "*.hex" });
    model->setNameFilterDisables(true);

    auto completer = new QCompleter(this);
    completer->setModel(model);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    // Note: This call may be slow on some windows machines.
    // Check if there are any dead network drives or CD-ROMs
    // that take a long time to load.
    //
    ui->lineEdit->setCompleter(completer);
}

void UpdateWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->urls().size() == 1 &&
        event->mimeData()->urls()[0].isLocalFile())
        event->acceptProposedAction();
}

void UpdateWindow::dropEvent(QDropEvent *event)
{
    ui->lineEdit->setText(
        event->mimeData()->urls()[0].toLocalFile()
    );
}

void UpdateWindow::browseButton_clicked()
{
    auto fn = QFileDialog().getOpenFileName(
        this, "Open file", ui->lineEdit->text(),
        "Intel HEX (*.hex);;All Files (*.*)"
    );

    if (fn == "")
        return;

    ui->lineEdit->setText(fn);
}


void UpdateWindow::updateButton_clicked()
{
    bootProtocol.startUpdate( ui->lineEdit->text() );

//    QMessageBox::critical(
//        this, "Firmware Update Failed",
//        "Something went wrong!!1"
//    );
}

void UpdateWindow::lineEdit_textChanged()
{
    QSettings().setValue("update_filename", ui->lineEdit->text());
}


UpdateWindow::~UpdateWindow()
{
    delete ui;
}

void UpdateWindow::connectionChanged()
{
    auto c = (Connection*)sender();
    ui->updateButton->setEnabled( c->isOpen() );
}


void UpdateWindow::bootProtocol_showProgress(int value, const QString &text)
{
    qDebug("%d: %s", value, qPrintable(text));

    progressDialog.setValue(value);
    progressDialog.setLabelText(text);

    if (progressDialog.wasCanceled())
        bootProtocol.cancelUpdate();
}


/*
def write_data(self, addr, data, ack_window=0, progress=None):
    offset = 0
    pending = 0

    while offset < len(data):
        chunk = min(len(data) - offset, 240)

        msg = msg_boot_write_data(addr + offset, data[offset:offset+chunk])
        self.conn.send_packet(msg.pack())

        offset += chunk

        if progress is not None:
            progress(offset)

        # queue requests to mask the connection latency
        #
        if pending >= ack_window:
            self.get_response()
        else:
            pending += 1

    # wait for outstanding responses
    #
    while pending > 0:
        self.get_response()
        pending -= 1
*/


/*
    BootProtocol boot(this, mainWindow->connection);

    progressDialog.show();

    progress(0, "Loading \"" + fileName + "\"");

    IntelHexFile ih;

    if (!ih.loadHex(fileName)) {
        QMessageBox::critical(
            this, "Firmware Update Failed", "Can't load hex file"
        );
        return;
    }

    uint start_addr  = ih.sections[0].offset;
    uint end_addr    = ih.sections[0].offset +  ih.sections[0].data.size();
    const auto &data = ih.sections[0].data;

    qDebug("  Start 0x%08x", start_addr);
    qDebug("  End   0x%08x", end_addr);

    progress(2, "Resetting controller");
    boot.reset_hack();      // TODO: DON'T

    progress(5, "Entering bootloader");
    boot.enter();           // TODO: Try more often!

//    # hack.. discard bytes
//    try:
//        self.conn._recv_packet()
//    except:
//        pass

    auto t0 = QTime::currentTime();

    for (int i=4; i<12; i++) {
        int p = 10 + 10 * (i-4) / (12-4);
        progress(p, QString().sprintf("Erasing sector %d...", i));
        boot.eraseSector(i);
    }

    auto t_erase = QTime::currentTime();

    // skip initial 8 bytes (written after verify)
    // TODO: Progress callback
    //
    boot.writeData( start_addr + 8, data.mid(8) );

//    self.write_data(
//        start_addr + 8, data[8:], ack_window=10,
//        progress=lambda x: progress(
//            20 + 60 * x / len(data[8:]),
//            "Writing 0x%08x..." % (start_addr + 8 + x)
//        )
//    )

    auto t_write = QTime::currentTime();

    progress(85, "Verifying");
    boot.verify(start_addr + 8, data.mid(8));

    auto t_verify = QTime::currentTime();

    progress(90, "Writing first 8 bytes");
    boot.writeData(start_addr, data.left(8));

    progress(95, "Starting application");
    boot.exit();

    progress(100, "Done.");

    auto t_total = QTime::currentTime();

    qDebug("  Erase:  %d ms", t0.msecsTo(t_erase));
    qDebug("  Write:  %d ms", t_erase.msecsTo(t_write));
    qDebug("  Verify: %d ms", t_write.msecsTo(t_verify));
    qDebug("  Total:  %d ms", t0.msecsTo(t_total));
*/

