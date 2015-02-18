#include "AddConnectionDialog.h"

#include "ui_AddConnectionDialog.h"

AddConnectionDialog::AddConnectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddConnectionDialog)
{
    ui->setupUi(this);
}

AddConnectionDialog::~AddConnectionDialog()
{
    delete ui;
}
