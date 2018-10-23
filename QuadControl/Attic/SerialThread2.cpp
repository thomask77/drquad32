/*  HVPMIC demonstration application for the HVPMIC project
 *
 *  This application controls the 2980 16 channel supervisor
 *  circuit on the HVPMIC power board and the load generator
 *  on the associated load board.
 *  Communication with the PC graphical UI is done via the
 *  serial port at 2 Mbaud with a COBSR protocol and CRC.
 *
 *  (C) 2016 Linear technology and Cornelius Consult
 *
 *  Author: Werner Cornelius (wernerp@cornelius-consult.de)
 *          Bochum, Germany
 *
 *  IMPORTANT notice:
 *  The whole project contains files that are published under
 *  the GNU PUBLIC License. This is especially valid for the
 *  QCustomplot module used.
 *  If you want to use and distribute the code please inform
 *  yourself and care about the license conditions given.
 *
 */

#include "SerialThread2.h"
#include <QDebug>
#include <QMetaObject>


SerialThread2::SerialThread2()
{
    this->moveToThread(&m_thread);
    m_serial.moveToThread(&m_thread);

    connect(&m_serial, &QSerialPort::readyRead, this, &SerialThread2::serial_readyRead);

    m_thread.start();
    m_thread.setPriority(QThread::HighPriority);
}


SerialThread2::~SerialThread2()
{
    close();
    m_thread.quit();
    m_thread.wait();
}


bool SerialThread2::open(const QString &portName, int baudRate)
{
    if (QThread::currentThread() != this->thread()) {
        bool result;

        QMetaObject::invokeMethod(
            this, "open", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(QString, portName), Q_ARG(int, baudRate)
        );

        return result;
    }

    close();
    m_serial.setPortName(portName);

    if (!m_serial.open(QIODevice::ReadWrite))
        goto fail;

    if (!m_serial.setBaudRate(baudRate))
        goto fail;

    return true;

fail:
    close();
    return false;
}


void SerialThread2::close()
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(
            this, "close", Qt::BlockingQueuedConnection
        );
        return;
    }

    m_serial.close();
}


int SerialThread2::write(const QByteArray &bytes)
{
    if (QThread::currentThread() != this->thread()) {
        int result;
        QMetaObject::invokeMethod(
            this, "write", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(int, result), Q_ARG(QByteArray, bytes)
        );
        return result;
    }

    return m_serial.write(bytes);
}


void SerialThread2::serial_readyRead()
{
    // qDebug() << QThread::currentThread() << m_serial.thread();
    emit bytesReceived( m_serial.readAll() );
}

