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
    explicit GLWindow(class MainWindow *parent = 0);
    ~GLWindow();

private:
    Ui::GLWindow *ui;
    class MainWindow *mainWindow;

    void actionScan_triggered();
    void actionConnect_triggered();
    void actionDisconnect_triggered();
    void treewidget_currentItemChanged();
    void treewidget_itemActivated();
};

#endif // GLWINDOW_H
