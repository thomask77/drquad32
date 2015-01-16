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
        bool bold = 0;
        bool underline = 0;
        bool blink = 0;
        bool reverse = 0;
    };

    Attributes  attributes;

    void parse(const QString &text);

signals:
    void attributesChanged(const Attributes &attributes);
    void printText(const QString &text);

private:
    int         state = 0;
    QString     buffer;

    void parseSgr(const QString &seq);
    void parseMulti(const QString &seq);
    void parseSingle(const QString &seq);
    void parseChar(char c);
};

#endif // ANSIPARSER_H
