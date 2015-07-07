#ifndef GLWINDOW_H
#define GLWINDOW_H

#include <QMainWindow>

namespace Ui {
class GLWindow;
}

class GLWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GLWindow(QWidget *parent = 0);
    ~GLWindow();

private:
    Ui::GLWindow *ui;
};

#endif // GLWINDOW_H
