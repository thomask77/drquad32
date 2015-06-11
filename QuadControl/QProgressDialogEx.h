/**
 * QProgressDialog subclass with Windows 7 TaskbarButton progress bar
 *
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
#ifndef QPROGRESSDIALOGEX_H
#define QPROGRESSDIALOGEX_H

#include <QProgressDialog>

#ifdef Q_OS_WIN
#include <QtWinExtras>
#endif


class QProgressDialogEx : public QProgressDialog
{
    Q_OBJECT

public:
    #ifdef Q_OS_WIN
    QWinTaskbarButton winTaskbarButton;

    QProgressDialogEx(QWidget *parent = 0, Qt::WindowFlags flags = 0)
        : QProgressDialog(parent, flags)
        , winTaskbarButton(parent)
    {
        if (parent)
            winTaskbarButton.setWindow(parent->windowHandle());
    }

    virtual ~QProgressDialogEx()
    {
        winTaskbarButton.progress()->setVisible(false);
    }

    void setValue(int progress)
    {
        QProgressDialog::setValue(progress);
        winTaskbarButton.progress()->setValue(progress);
        winTaskbarButton.progress()->setVisible(true);
    }
    #else

    // Inherit constructors
    //
    using QProgressDialog::QProgressDialog;

    #endif
};

#endif // QPROGRESSDIALOGEX_H
