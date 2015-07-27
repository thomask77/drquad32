/**
 * Simple ANSI color parser
 * see http://man7.org/linux/man-pages/man4/console_codes.4.html
 *
 * Copyright (C)2015 Thomas Kindler <mail_drquad@t-kindler.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AnsiParser.h"

#include <QStringList>
#include <QDebug>


AnsiParser::AnsiParser(QObject *parent)
    : QObject(parent)
{
}


AnsiParser::~AnsiParser()
{
}


void AnsiParser::parseSgr(const QList<int> &params)
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

    emit changeAttributes(attributes);
}


void AnsiParser::parseModes(const QList<int> &params, bool set)
{
    for (auto p: params) {
        switch (p) {
        case 4:  modes.overwrite = set; break;
        case 12: modes.echo = set;      break;
        }
    }

    emit changeModes(modes);
}


void AnsiParser::parseCursor(const QList<int> &params, char command)
{
    int n = params.count() ? params[0] : 1;
    switch (command) {
    case 'A': emit moveCursor(0, -n); break;
    case 'B': emit moveCursor(0,  n); break;
    case 'C': emit moveCursor( n, 0); break;
    case 'D': emit moveCursor(-n, 0); break;
    }
}


void AnsiParser::parseMulti(const QString &seq)
{
    char command = seq.right(1)[0].toLatin1();

    QList<int> pn;
    for (auto s: seq.left(seq.length()-1).split(';')) {
        bool ok;
        int  v = s.toInt(&ok);
        if (ok) pn.append(v);
    }

    switch (command) {
    case 'm':           parseSgr(pn);               break;
    case 'h':           parseModes(pn, false);      break;
    case 'l':           parseModes(pn, true);       break;
    case 'A' ... 'D':   parseCursor(pn, command);   break;
    case 'H':           emit home();                break;
    case 'J':           emit clear();               break;
    case 'K':           emit eraseEOL();            break;
    case 'P':           emit deleteChar(pn.count() > 0 ? pn[0] : 1);    break;
    default:            qDebug() << "unknown sequence: " << seq;        break;
    }
}


void AnsiParser::parseSingle(const QChar)
{
    // TODO
}


void AnsiParser::parseChar(char c)
{
    switch (state) {
    case 0:
        if (c == 27) {
            // start of control sequence
            //
            if (buffer != "") {
                emit printText(buffer);
                buffer = "";
            }
            state = 1;
        }
        else {
            buffer += c;
        }
        break;

    case 1:
        if (c == '[') {
            // start of multi character sequence
            //
            state = 2;
        }
        else if (c >= 64 && c <= 95) {
            // single character sequence
            //
            parseSingle(c);
            state = 0;
        }
        else {
            // invalid sequence, treat as text
            //
            buffer += c;
            state = 0;
        }
        break;

    case 2:
        // multi character sequence
        //
        buffer += c;
        if (c >= 64 && c <= 126) {
            parseMulti(buffer);
            buffer = "";
            state = 0;
        }
        break;
    }
}


void AnsiParser::parse(const QString &text)
{
    for (char c: text.toLatin1())
        parseChar(c);

    // flush remaining text
    //
    if (state == 0 && buffer != "") {
        emit printText(buffer);
        buffer = "";
    }
}
