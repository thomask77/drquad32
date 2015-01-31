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
#include <QUuid>
#include <QFile>
#include <QDir>


PuTTYLauncher::PuTTYLauncher(QObject *parent)
    : QObject(parent)
{
    connect(&process, &QProcess::started, this, &PuTTYLauncher::started);
    connect(&process, (void (QProcess::*)(int))&QProcess::finished, this, &PuTTYLauncher::finished);

    settings["ScrollbackLines"] = 10000;
    settings["WarnOnClose"] = 0;
    settings["TermWidth"] = 80;     // EGA style :)
    settings["TermHeight"] = 43;
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


bool PuTTYLauncher::startSerial(const QString &portName, int baudRate)
{
    settings["WinTitle"] = "PuTTY - " + portName + "@" + QString::number(baudRate);
    settings["Protocol"] = "serial";
    settings["SerialLine"] = portName;
    settings["SerialSpeed"] = baudRate;
    settings["SerialFlowControl"] = 2;  // 2 = RTS/CTS

    return startSession();
}


bool PuTTYLauncher::startRaw(const QString &hostName, int port)
{
    settings["WinTitle"] = "PuTTY - " + hostName + "@" + QString::number(port);
    settings["Protocol"] = "raw";
    settings["HostName"] = hostName;
    settings["PortNumber"] = port;
    settings["LocalEcho"] = 1;  // 0 = On, 1 = Off, 2 = Auto
    settings["LocalEdit"] = 1;  // 0 = On, 1 = Off, 2 = Auto

    return startSession();
}


bool PuTTYLauncher::startSession()
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
    QFile file(QDir::homePath() + "/.putty/sessions/" + sessionName);

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

        /*        switch (process.error()) {
        case QProcess::FailedToStart: m_errorString = "FailedToStart";
        case QProcess::Crashed      : m_errorString = "Crashed";
        case QProcess::Timedout     : m_errorString = "Timedout";
        case QProcess::WriteError   : m_errorString = "WriteError";
        case QProcess::ReadError    : m_errorString = "ReadError";
        case QProcess::UnknownError : m_errorString = "UnknownError";
        } */
        return false;
    }

    return true;
}
