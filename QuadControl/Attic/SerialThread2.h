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

#ifndef SERIALTHREAD2_H
#define SERIALTHREAD2_H

#include <QObject>
#include <QSerialPort>
#include <QThread>

class SerialThread2 : public QObject
{
    Q_OBJECT

    QThread         m_thread;
    QSerialPort     m_serial;

    void    serial_readyRead();

public:
    SerialThread2();
    ~SerialThread2() override;

    bool    isOpen()        const { return m_serial.isOpen();       }
    QString portName()      const { return m_serial.portName();     }
    int     baudRate()      const { return m_serial.baudRate();     }
    QString errorString()   const { return m_serial.errorString();  }

public slots:
    bool    open(const QString &portName, int baudRate);
    void    close();
    int     write(const QByteArray &bytes);

signals:
    void    bytesReceived(const QByteArray &bytes);
};

#endif // SERIALTHREAD2_H
