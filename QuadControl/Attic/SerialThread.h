#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H

#include <QObject>
#include <QSerialPort>
#include <QSemaphore>
#include <QThread>
#include <QMutex>

class SerialThread : public QThread
{
    Q_OBJECT

    QString     m_portName;
    int         m_baudRate;

    QByteArray  m_tx_buf;
    QMutex      m_tx_buf_mtx;
    QSemaphore  m_init_sem;

    volatile bool m_quit;
    QString     m_errorString;

    void run() override;

public:
    SerialThread(QObject *parent = 0);
    ~SerialThread() override;

    bool open(const QString &portName, int baudRate);
    bool isOpen() const;
    void close();

    QString portName() const    { return m_portName; }
    int     baudRate() const    { return m_baudRate; }
    QString errorString() const { return m_errorString; }

signals:
    void bytesReceived(const QByteArray &bytes);

public slots:
    int write(const QByteArray &bytes);
};

#endif // SERIALTHREAD_H
