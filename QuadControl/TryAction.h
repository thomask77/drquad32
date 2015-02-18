#ifndef TRYACTION_H
#define TRYACTION_H

#include <functional>
#include <QApplication>
#include <QMessageBox>
#include <QString>

#include "MainWindow.h"


inline bool tryAction(
    std::function<bool()> action,
    std::function<QString()> errorString,
    const QCursor &cursor = Qt::WaitCursor
)
{
    bool res = false;
    while (!res) {
        if (cursor.shape() != Qt::ArrowCursor) {
            QApplication::setOverrideCursor(cursor);
            QApplication::processEvents();
        }

        res = action();

        if (cursor.shape() != Qt::ArrowCursor) {
            QApplication::restoreOverrideCursor();
        }

        if (!res && QMessageBox::critical(
            mainWindow, "Error", errorString(),
            QMessageBox::Abort | QMessageBox::Retry
        ) != QMessageBox::Retry) {
            break;
        }
    }

    return res;
}


#endif // TRYACTION_H
