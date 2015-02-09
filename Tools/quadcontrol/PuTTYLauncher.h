#ifndef PUTTYLAUNCHER_H
#define PUTTYLAUNCHER_H

#include <QObject>
#include <QProcess>
#include <QMap>


class PuTTYLauncher : public QObject
{
    Q_OBJECT

public:
    explicit PuTTYLauncher(QObject *parent = 0);
    ~PuTTYLauncher();

#ifdef Q_OS_WIN
    QString puttyFilename = "C:\\Program Files (x86)\\PuTTY\\putty.exe";
#else
    QString puttyFilename = "/usr/bin/putty";
#endif

    QString sessionName = "QuadControl";
    QMap<QString, QVariant>  settings;
    QString errorString();

    bool openUrl(const QString &path);

signals:
    void started();
    void finished(int exitCode);

private:
    QString  m_errorString;
    QProcess process;

    bool openSession();
};

#endif // PUTTYLAUNCHER_H
