#ifndef GLTESTWINDOW_H
#define GLTESTWINDOW_H

#include <QMainWindow>

namespace Ui {
class GLTestWindow;
}

class GLTestWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GLTestWindow(QWidget *parent = 0);
    ~GLTestWindow();

private:
    Ui::GLTestWindow *ui;
};

#endif // GLTESTWINDOW_H
