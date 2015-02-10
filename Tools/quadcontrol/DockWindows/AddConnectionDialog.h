#ifndef ADDCONNECTIONDIALOG_H
#define ADDCONNECTIONDIALOG_H

#include <QDialog>
#include "ui_AddConnectionDialog.h"

namespace Ui {
class AddConnectionDialog;
}

class AddConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddConnectionDialog(QWidget *parent = 0);
    ~AddConnectionDialog();

    Ui::AddConnectionDialog *ui;
};

#endif // ADDCONNECTIONDIALOG_H
