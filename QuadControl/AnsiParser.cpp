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


AnsiParser::AnsiParser(QObject *parent)
    : QObject(parent)
{
}


AnsiParser::~AnsiParser()
{
}


void AnsiParser::parseSgr(const QString &seq)
{
    for (auto &v: seq.split(";")) {
        bool ok;
        int n = v.toInt(&ok);

        if (!ok)
            continue;

        switch(n) {
        case 0:         attributes = Attributes();      break;
        case 1:         attributes.bold = true;         break;
        case 4:         attributes.underline = true;    break;
        case 5:         attributes.blink = true;        break;
        case 7:         attributes.reverse = true;      break;
        case 30 ... 37: attributes.foreground = n - 30; break;
        case 39:        attributes.foreground = 7;      break;
        case 40 ... 47: attributes.background = n - 40; break;
        case 49:        attributes.background = 0;      break;
        }
    }

    emit changeAttributes(attributes);
}


void AnsiParser::parseMulti(const QString &seq)
{
    if (seq.endsWith("m")) {
        // select graphic rendition
        //
        parseSgr( seq.mid(1, seq.length()-2) );
    }

    if (seq.endsWith("4h"))
        emit changeInsertMode(true);

    if (seq.endsWith("4l"))
        emit changeInsertMode(false);

    if (seq.endsWith("K"))
        emit eraseEOL();

    if (seq.endsWith("P"))
        emit deleteChar();

    if (seq.endsWith("C"))
        emit moveCursor(1, 0);

    if (seq.endsWith("D"))
        emit moveCursor(-1, 0);

}


void AnsiParser::parseSingle(const QString &)
{
    // TODO
}

void AnsiParser::parseChar(char c)
{
    switch (state) {
    case 0:
        if (c == 27) {
            // start of escape sequence
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
        buffer += c;

        if (c == '[') {
            // start of multi character sequence
            //
            state = 2;
        }
        else if (c >= 64 && c <= 95) {
            // single character sequence
            //
            parseSingle(buffer);
            buffer = "";
            state = 0;
        }
        else {
            // invalid sequence
            //
            emit printText(buffer);
            buffer = "";
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
