#include "TerminalWidget.h"
#include "ui_TerminalWidget.h"
#include <QDebug>
#include <QScrollBar>


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
    QWidget(parent)
{
    plainTextEdit = new QPlainTextEdit();

    plainTextEdit->setContextMenuPolicy(Qt::NoContextMenu);
    plainTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    plainTextEdit->setFrameShape(QFrame::NoFrame);
    plainTextEdit->setUndoRedoEnabled(false);

    plainTextEdit->installEventFilter(this);

    auto p = plainTextEdit->palette();
    p.setColor(QPalette::All, QPalette::Base, ansiPalette[0].color());
    p.setColor(QPalette::All, QPalette::Text, ansiPalette[7].color());
    plainTextEdit->setPalette(p);

    auto font = plainTextEdit->font();
#ifdef Q_OS_WIN
    font.setFamily("Lucida Console");
#else
    font.setFamily("Liberation Mono");
#endif
    plainTextEdit->setFont(font);

    cursor = plainTextEdit->textCursor();

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plainTextEdit);
}


TerminalWidget::~TerminalWidget()
{
}


void TerminalWidget::clear()
{
    plainTextEdit->clear();
}


QString TerminalWidget::toPlainText()
{
    return plainTextEdit->toPlainText();
}


void TerminalWidget::setLineWrapMode(QPlainTextEdit::LineWrapMode mode)
{
    plainTextEdit->setLineWrapMode(mode);
}


bool TerminalWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == plainTextEdit) {
        if (event->type() == QEvent::KeyPress) {
            auto k = (QKeyEvent *)event;

            if (keymap[k->key()] != "")
                txBuffer += keymap[k->key()];
            else
                txBuffer += k->text();

            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}


QString TerminalWidget::read(size_t maxSize)
{
    auto ret = txBuffer.left(maxSize);
    txBuffer = txBuffer.mid(maxSize);
    return ret;
}


void TerminalWidget::write(const QString &text)
{
    auto vsb = plainTextEdit->verticalScrollBar();
    auto atBottom = vsb->value() == vsb->maximum();

    plainTextEdit->setUpdatesEnabled(false);

    for (char c: text.toLatin1())
        parseChar(c);

    if (rxState == 0 && rxBuffer != "") {
        printText(rxBuffer);
        rxBuffer.clear();
    }

    if (atBottom)
        vsb->setValue(vsb->maximum());

    plainTextEdit->setUpdatesEnabled(true);
}


void TerminalWidget::printText(const QString &text)
{
    if (modes.overwrite) {
        int pos = cursor.position();

        cursor.movePosition(QTextCursor::EndOfLine);
        int eol = cursor.position();

        cursor.setPosition(pos);
        cursor.movePosition(
            QTextCursor::Right, QTextCursor::KeepAnchor,
            std::max(text.length(), eol - pos)
        );

        cursor.removeSelectedText();
    }

    cursor.insertText(text);
    plainTextEdit->setTextCursor(cursor);
}


void TerminalWidget::parseSgr(const QList<int> &params)
{
    for (auto p: params) {
        switch(p) {
        case 0:         attributes = Attributes();      break;
        case 1:         attributes.bold = true;         break;
        case 4:         attributes.underline = true;    break;
        case 5:         attributes.blink = true;        break;
        case 7:         attributes.reverse = true;      break;
        case 30 ... 37: attributes.foreground = p - 30; break;
        case 39:        attributes.foreground = 7;      break;
        case 40 ... 47: attributes.background = p - 40; break;
        case 49:        attributes.background = 0;      break;
        }
    }

    unsigned fg = attributes.foreground + (attributes.bold ? 8 : 0);
    unsigned bg = attributes.background;

    format.setForeground(ansiPalette[fg & 15]);
    format.setBackground(ansiPalette[bg & 15]);

    cursor.setCharFormat(format);
}


void TerminalWidget::parseModes(const QList<int> &params, bool set)
{
    for (auto p: params) {
        switch (p) {
        case 4:  modes.overwrite = set; break;
        case 12: modes.echo = set;      break;
        }
    }
}


void TerminalWidget::parseCursor(const QList<int> &params, char command)
{
    int n = params.count() ? params[0] : 1;

    switch (command) {
    case 'A': cursor.movePosition(QTextCursor::Up,    QTextCursor::MoveAnchor, n); break;
    case 'B': cursor.movePosition(QTextCursor::Down,  QTextCursor::MoveAnchor, n); break;
    case 'C': cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, n); break;
    case 'D': cursor.movePosition(QTextCursor::Left,  QTextCursor::MoveAnchor, n); break;
    }

    plainTextEdit->setTextCursor(cursor);
}


void TerminalWidget::parseMulti(const QString &seq)
{
    char command = seq.right(1)[0].toLatin1();

    QList<int> pn;
    for (auto s: seq.left(seq.length()-1).split(';')) {
        bool ok;
        int  v = s.toInt(&ok);
        if (ok) pn.append(v);
    }

    switch (command) {
    case 'm':
        parseSgr(pn);
        break;

    case 'h':
        parseModes(pn, false);
        break;

    case 'l':
        parseModes(pn, true);
        break;

    case 'C' ... 'D':
        parseCursor(pn, command);
        break;

    case 'H':
        cursor.movePosition(QTextCursor::Start);
        plainTextEdit->setTextCursor(cursor);
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
        int n = pn.count() > 0 ? pn[0] : 1;
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, n);
        cursor.removeSelectedText();
        }
        break;

    default:
        qDebug() << "unknown sequence: " << seq;
        break;
    }
}


void TerminalWidget::parseSingle(const QChar)
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



