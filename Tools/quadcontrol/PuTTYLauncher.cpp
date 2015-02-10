/**
 * Launch PuTTY on Windows and Linux with a customizable configuration
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

#include "PuTTYLauncher.h"
#include <QDebug>
#include <QSettings>
#include <QSerialPortInfo>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QUrl>

PuTTYLauncher::PuTTYLauncher(QObject *parent)
    : QObject(parent)
{
    connect(&process, &QProcess::started, this, &PuTTYLauncher::started);
    connect(&process, (void (QProcess::*)(int))&QProcess::finished, this, &PuTTYLauncher::finished);
}


PuTTYLauncher::~PuTTYLauncher()
{
    if (process.state() == QProcess::Running)
        process.terminate();
}


QString PuTTYLauncher::errorString()
{
    return m_errorString;
}


bool PuTTYLauncher::openUrl(const QUrl &url)
{
    settings["ScrollbackLines"] = 10000;
    settings["WarnOnClose"] = 0;
    settings["TermWidth"] = 80;     // EGA style :)
    settings["TermHeight"] = 43;

    settings["WinTitle"] = "PuTTY - " + url.toString();

    if (url.scheme() == "serial") {
        settings["Protocol"] = "serial";
        settings["SerialLine"] = QSerialPortInfo(url.path()).systemLocation();
        settings["SerialSpeed"] = url.query().toInt();
        settings["SerialFlowControl"] = 2;  // 2 = RTS/CTS
    }
    else if (url.scheme() == "wifly") {
        settings["Protocol"] = "raw";
        settings["HostName"] = url.host();
        settings["PortNumber"] = url.port();
        settings["LocalEcho"] = 1;  // 0 = On, 1 = Off, 2 = Auto
        settings["LocalEdit"] = 1;  // 0 = On, 1 = Off, 2 = Auto
    }
    else {
        m_errorString = "Unknown URL scheme";
        return false;
    }

    return openSession();
}


bool PuTTYLauncher::openSession()
{
#ifdef Q_OS_WIN
    QSettings registry(
        "HKEY_CURRENT_USER\\Software\\SimonTatham\\PuTTY\\Sessions",
        QSettings::NativeFormat
    );

    registry.beginGroup(sessionName);

    for (auto k: settings.keys())
        registry.setValue(k, settings[k]);

    registry.endGroup();
    registry.sync();

#else
    auto sessionPath = QDir::homePath() + "/.putty/sessions/";
    QDir().mkpath(sessionPath);

    QFile file(sessionPath + sessionName);

    if (!file.open(QIODevice::WriteOnly)) {
        m_errorString = file.errorString();
        return false;
    }

    QTextStream out(&file);

    for (auto k: settings.keys())
        out << k << "=" << settings[k].toString() << "\n";

    file.close();

#endif

    process.start(puttyFilename, QStringList({ "-load", sessionName }));

    if (!process.waitForStarted()) {
        m_errorString = process.errorString();
        return false;
    }

    return true;
}
