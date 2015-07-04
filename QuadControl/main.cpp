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

#include <QApplication>
#include "MainWindow.h"
#include "VersionInfo.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName((char*)versionInfo.product_name);
    a.setApplicationVersion(
        QString("%1.%2.%3 %4")
            .arg(versionInfo.major)
            .arg(versionInfo.minor)
            .arg(versionInfo.patch)
            .arg((char*)versionInfo.vcs_id)
    );
    a.setOrganizationDomain("t-kindler.de");

    mainWindow = new MainWindow();
    mainWindow->show();

    auto res = a.exec();

    delete mainWindow;
    return res;
}
