#ifndef QPROGRESSDIALOGEX_H
#define QPROGRESSDIALOGEX_H

#include <QProgressDialog>
#include <QtWinExtras>


class QProgressDialogEx : public QProgressDialog
{
    Q_OBJECT

public:
    QWinTaskbarButton winTaskbarButton;

    QProgressDialogEx(QWidget *parent = 0, Qt::WindowFlags flags = 0)
        : QProgressDialog(parent, flags)
        , winTaskbarButton(this)
    {

    }

    virtual ~QProgressDialogEx()
    {
        winTaskbarButton.progress()->setVisible(false);
    }

    void setValue(int progress)
    {
        QProgressDialog::setValue(progress);

        if (!winTaskbarButton.window()) {
            winTaskbarButton.setWindow(parentWidget()->topLevelWidget()->windowHandle());
            winTaskbarButton.progress()->setVisible(true);
        }

        winTaskbarButton.progress()->setValue(progress);
    }
};

#endif // QPROGRESSDIALOGEX_H
