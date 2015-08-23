#ifndef INPUTMAPPINGWINDOW_H
#define INPUTMAPPINGWINDOW_H

#include <QWidget>

namespace Ui {
class InputMappingWindow;
}

class InputMappingWindow : public QWidget
{
    Q_OBJECT

public:
    explicit InputMappingWindow(QWidget *parent = 0);
    ~InputMappingWindow();

public slots:
    void valueChanged();

private:
    Ui::InputMappingWindow *ui;
};

#endif // INPUTMAPPINGWINDOW_H
