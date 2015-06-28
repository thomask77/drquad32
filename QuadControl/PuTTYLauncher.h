#ifndef PUTTYLAUNCHER_H
#define PUTTYLAUNCHER_H

#include <QWidget>
#include <QProcess>
#include <QMap>


class PuTTYLauncher : public QWidget
{
    Q_OBJECT

public:
    explicit PuTTYLauncher(QWidget *parent = 0);
    ~PuTTYLauncher();

    QString puttyFilename;
    QString sessionName = "QuadControl";
    QMap<QString, QVariant>  settings;
    QString errorString();

    bool openUrl(const QUrl &url);

signals:
    void started();
    void finished(int exitCode);

private:
    QString  m_errorString;
    QProcess process;

    bool openSession();
};

#endif // PUTTYLAUNCHER_H
