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
    bootProtocol(this, parent->connection)
{
    ui->setupUi(this);

    ui->lineEdit->setText( QSettings().value("update_filename").toString() );

    connect(ui->lineEdit, &QLineEdit::textChanged, this, &UpdateWindow::lineEdit_textChanged);
    connect(ui->browseButton, &QPushButton::clicked, this, &UpdateWindow::browseButton_clicked);
    connect(ui->updateButton, &QPushButton::clicked, this, &UpdateWindow::updateButton_clicked);
    connect(&mainWindow->connection, &Connection::connectionChanged, this, &UpdateWindow::connectionChanged);

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
    bootProtocol.writeHexFile(ui->lineEdit->text());
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
