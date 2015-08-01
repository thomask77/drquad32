#include "TerminalWidget.h"
#include "ui_TerminalWidget.h"
#include <QDebug>
#include <QScrollBar>
#include <QPainter>
#include <QApplication>
#include <QClipboard>


static const QBrush ansiPalette[] = {
    QBrush(QColor("#222")), QBrush(QColor("#C00")),
    QBrush(QColor("#2C0")), QBrush(QColor("#CC0")),
    QBrush(QColor("#00C")), QBrush(QColor("#C0C")),
    QBrush(QColor("#0CC")), QBrush(QColor("#CCC")),
    QBrush(QColor("#444")), QBrush(QColor("#F44")),
    QBrush(QColor("#4F4")), QBrush(QColor("#FF4")),
    QBrush(QColor("#44F")), QBrush(QColor("#F4F")),
    QBrush(QColor("#4FF")), QBrush(QColor("#FFF"))
};


static const QMap<int, QString> keymap {
    { Qt::Key_Up      , "\x1B[A"  },
    { Qt::Key_Down    , "\x1B[B"  },
    { Qt::Key_Right   , "\x1B[C"  },
    { Qt::Key_Left    , "\x1B[D"  },
    { Qt::Key_Home    , "\x1B[1~" },
    { Qt::Key_Insert  , "\x1B[2~" },
    { Qt::Key_Delete  , "\x1B[3~" },
    { Qt::Key_End     , "\x1B[4~" },
    { Qt::Key_PageUp  , "\x1B[5~" },
    { Qt::Key_PageDown, "\x1B[6~" }
};


TerminalWidget::TerminalWidget(QWidget *parent) :
    QPlainTextEdit(parent)
{
    setReadOnly(true);
    setContextMenuPolicy(Qt::NoContextMenu);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameShape(QFrame::NoFrame);
    setMaximumBlockCount(2000);

    // Copy text on selection change
    //
    connect(this, &TerminalWidget::selectionChanged, this, &copy);

    auto p = palette();
    p.setColor(QPalette::All, QPalette::Base, ansiPalette[0].color());
    p.setColor(QPalette::All, QPalette::Text, ansiPalette[7].color());
    setPalette(p);

    auto f = font();
#ifdef Q_OS_WIN
    f.setFamily("Lucida Console");
#else
    f.setFamily("Liberation Mono");
#endif
    setFont(f);

    cursor = textCursor();
}


TerminalWidget::~TerminalWidget()
{
}


QString TerminalWidget::read(size_t maxSize)
{
    auto ret = txBuffer.left(maxSize);
    txBuffer = txBuffer.mid(maxSize);
    return ret;
}


void TerminalWidget::write(const QString &text)
{
    setUpdatesEnabled(false);

    for (char c: text.toLatin1())
        parseChar(c);

    if (rxState == 0 && rxBuffer != "") {
        printText(rxBuffer);
        rxBuffer.clear();
    }

    setTextCursor(cursor);
    setUpdatesEnabled(true);
}


void TerminalWidget::reset()
{
    modes = Modes();
    attributes = Attributes();
}


bool TerminalWidget::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        auto ke = reinterpret_cast<QKeyEvent*>(e);

        if (ke->modifiers() == Qt::ShiftModifier && ke->key() == Qt::Key_Insert) {
            // Paste on SHIFT+Insert
            //
            txBuffer += QApplication::clipboard()->text();
            return true;
        }
        else if (ke->modifiers() == Qt::ShiftModifier && (
                        ke->key() == Qt::Key_Home   || ke->key() == Qt::Key_End  ||
                        ke->key() == Qt::Key_PageUp || ke->key() == Qt::Key_PageDown)
                    )
        {
            // Let QPlainTextEdit handle scroll commands
            //
            ke->setModifiers(0);
            return QPlainTextEdit::event(e);
        }
        else if (keymap[ke->key()] != "") {
            // Special ANSI key
            //
            txBuffer += keymap[ke->key()];
            return true;
        }
        else if (ke->text() != "") {
            // Normal text
            //
            txBuffer += ke->text();
            return true;
        }
    }
    else if (e->type() == QEvent::MouseButtonPress ||
             e->type() == QEvent::MouseButtonDblClick )
    {
        auto me = reinterpret_cast<QMouseEvent*>(e);
        if (me->button() == Qt::RightButton) {
            // Paste on right mouse button
            //
            txBuffer += QApplication::clipboard()->text();
            return true;
        }
    }
    else if (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut) {
        // Repaint on focus change
        //
        viewport()->repaint();
    }
    else if (e->type() == QEvent::FontChange) {
        // Recalc cursor size on CTRL+Wheel font change
        //
        setCursorWidth(fontMetrics().width(' '));
    }

    return QPlainTextEdit::event(e);
}


void TerminalWidget::paintEvent(QPaintEvent *e)
{
    QPlainTextEdit::paintEvent(e);

    if (!modes.hideCursor) {
        QRect rect(cursorRect(cursor));
        QPainter painter(viewport());

        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);

        if (hasFocus()) {
            rect.adjust(0, 0, 1, 1);
            painter.fillRect(rect, Qt::white);
        }
        else {
            painter.setPen(Qt::white);
            painter.drawRect(rect);
        }
    }
}


void TerminalWidget::printText(const QString &text)
{
    if (modes.overwrite) {
        int pos = cursor.position();

        cursor.movePosition(QTextCursor::EndOfLine);
        if (text.endsWith('\n'))
            cursor.movePosition(QTextCursor::Right);

        int eol = cursor.position();

        cursor.setPosition(pos);
        cursor.movePosition(
            QTextCursor::Right, QTextCursor::KeepAnchor,
            std::min(eol - pos, text.length())
        );
    }

    cursor.insertText(text);
}


void TerminalWidget::parseSgr(const QList<QString> &params)
{
    for (auto p: params) {
        bool ok;
        int n = p.toInt(&ok);
        if (!ok) continue;

        switch(n) {
        case 0:         attributes = Attributes();          break;
        case 1:         attributes.bold = true;             break;
        case 4:         attributes.underline = true;        break;
        case 5:         attributes.blink = true;            break;
        case 7:         attributes.reverse = true;          break;
        case 30 ... 37: attributes.foreground = n - 30;     break;
        case 39:        attributes.foreground = 7;          break;
        case 40 ... 47: attributes.background = n - 40;     break;
        case 49:        attributes.background = 0;          break;
        }
    }

    int fg = (attributes.foreground + (attributes.bold ? 8 : 0)) & 15;
    int bg = attributes.background & 15;

    QTextCharFormat f;
    f.setForeground(ansiPalette[fg]);
    f.setBackground(ansiPalette[bg]);

    cursor.setCharFormat(f);
}


void TerminalWidget::parseModes(char command, const QList<QString> &params)
{
    bool set = command == 'l';

    for (auto p: params) {
        if      (p == "4")      modes.overwrite = set;
        else if (p == "12")     modes.echo = set;
        else if (p == "?25")    modes.hideCursor = set;
    }
}


void TerminalWidget::parseCursor(char command, const QList<QString> &params)
{
    int n = params.count() ? params[0].toInt() : 1;

    switch (command) {
    case 'A': cursor.movePosition(QTextCursor::Up,    QTextCursor::MoveAnchor, n); break;
    case 'B': cursor.movePosition(QTextCursor::Down,  QTextCursor::MoveAnchor, n); break;
    case 'C': cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, n); break;
    case 'D': cursor.movePosition(QTextCursor::Left,  QTextCursor::MoveAnchor, n); break;
    }
}


void TerminalWidget::parseMulti(const QString &seq)
{
    char command = seq.right(1)[0].toLatin1();
    auto params  = seq.left(seq.length()-1).split(';', QString::SkipEmptyParts);

    switch (command) {
    case 'm':
        parseSgr(params);
        break;

    case 'h':
    case 'l':
        parseModes(command, params);
        break;

    case 'A' ... 'D':
        parseCursor(command, params);
        break;

    case 'H':
        cursor.movePosition(QTextCursor::Start);
        break;

    case 'J':
        cursor.select(QTextCursor::Document);
        cursor.removeSelectedText();
        break;

    case 'K':
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        break;

    case 'P': {
        int n = params.count() ? params[0].toInt() : 1;
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, n);
        cursor.removeSelectedText();
        }
        break;

    default:
        qDebug() << "unknown sequence: " << seq;
        break;
    }
}


void TerminalWidget::parseSingle(const QChar &seq)
{
    // TODO
}


void TerminalWidget::parseChar(char c)
{
    switch (rxState) {
    case 0:
        if (c == 27) {
            // start of control sequence
            //
            printText(rxBuffer);
            rxBuffer.clear();
            rxState = 1;
        }
        else if (c == '\r') {
            printText(rxBuffer);
            rxBuffer.clear();
            cursor.movePosition(QTextCursor::StartOfLine);
        }
        else if (c == '\n') {
            printText(rxBuffer);
            rxBuffer.clear();
            cursor.movePosition(QTextCursor::EndOfLine);
            printText("\n");
        }
        else if (c == '\a') {
            QApplication::beep();
        }
        else {
            rxBuffer += c;
        }
        break;

    case 1:
        if (c == '[') {
            // start of multi character sequence
            //
            rxState = 2;
        }
        else if (c >= 64 && c <= 95) {
            // single character sequence
            //
            parseSingle(c);
            rxState = 0;
        }
        else {
            // invalid sequence, treat as text
            //
            rxBuffer += c;
            rxState = 0;
        }
        break;

    case 2:
        // multi character sequence
        //
        rxBuffer += c;
        if (c >= 64 && c <= 126) {
            parseMulti(rxBuffer);
            rxBuffer.clear();
            rxState = 0;
        }
        break;
    }
}

