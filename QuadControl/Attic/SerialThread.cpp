#include "SerialThread.h"
#include <QDebug>
#include <QMutexLocker>


/**
 * This is a workaround for problems in QSerialPort on Windows.
 *
 * QSerialPort will loose receive bytes when the owning thread's
 * loop isn't executed often enough. Therefore it can't be used
 * directly from the GUI thread (setReadBufferSize doesn't work
 * either, tried everything already!).
 *
 */

SerialThread::SerialThread(QObject *parent)
    : QThread(parent)
{
}


SerialThread::~SerialThread()
{
    close();
}


bool SerialThread::open(const QString &portName, int baudRate)
{
    close();

    m_quit = false;
    m_errorString = "";
    m_portName = portName;
    m_baudRate = baudRate;

    start();
    m_init_sem.acquire();

    return m_errorString == "";
}


bool SerialThread::isOpen() const
{
    return isRunning();
}


void SerialThread::close()
{
    if (!isOpen())
        return;

    m_quit = true;
    wait();
}


int SerialThread::write(const QByteArray &bytes)
{
    if (!isOpen()) {
        m_errorString = "Port not open";
        return -1;
    }

    QMutexLocker lock(&m_tx_buf_mtx);
    m_tx_buf.append(bytes);

    return bytes.size();
}


void SerialThread::run()
{
    setPriority(HighPriority);

    QSerialPort serial;
    serial.setPortName(m_portName);

    if (!serial.open(QIODevice::ReadWrite))
        goto fail;

    if (!serial.setBaudRate(m_baudRate))
        goto fail;

    m_init_sem.release();

    while (!m_quit) {
        if (serial.waitForReadyRead(1)) {
            auto buf = serial.readAll();
            emit bytesReceived(buf);
        }

        {
            QMutexLocker lock(&m_tx_buf_mtx);
            if (m_tx_buf.size()) {
                serial.write(m_tx_buf);
                m_tx_buf.clear();
            }
        }
    }

    return;

fail:
    m_init_sem.release();
    m_errorString = serial.errorString();
    qDebug() << m_errorString;
}

