#ifndef ANSIPARSER_H
#define ANSIPARSER_H

#include <QObject>

class AnsiParser : public QObject
{
    Q_OBJECT

public:
    AnsiParser(QObject *parent = 0);
    ~AnsiParser();

    struct Attributes {
        int foreground = 7;
        int background = 0;
        bool bold = false;
        bool underline = false;
        bool blink = false;
        bool reverse = false;
    };

    struct Modes {
        bool overwrite = false;
        bool echo = false;
    };

    Attributes  attributes;
    Modes       modes;

    void parse(const QString &text);

signals:
    void changeAttributes(const Attributes &attributes);
    void changeModes(const Modes &modes);
    void moveCursor(int x, int y);
    void printText(const QString &text);
    void deleteChar();
    void eraseEOL();

private:
    int         state = 0;
    QString     buffer;

    void parseSgr(const QList<int> &params);
    void parseModes(const QList<int> &params, bool set);
    void parseCursor(const QList<int> &params, char command);

    void parseMulti(const QString &seq);
    void parseSingle(const QChar seq);
    void parseChar(char c);
};

#endif // ANSIPARSER_H
