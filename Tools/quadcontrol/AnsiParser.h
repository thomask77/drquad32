#ifndef ANSIPARSER_H
#define ANSIPARSER_H

#include <QObject>

class AnsiParser : public QObject
{
    Q_OBJECT

public:
    explicit AnsiParser(QObject *parent = 0);
    ~AnsiParser();

    struct Attributes {
        int foreground;
        int background;
        bool bold;
        bool underline;
        bool blink;
        bool reverse;

        Attributes() :
            foreground(7),
            background(),
            bold(),
            underline(),
            blink(),
            reverse()
        {
        }
    };

    void parse(const QString &text);

signals:
    void attributesChanged(const Attributes &attributes);
    void printText(const QString &text);

private:
    int         state;
    QString     buffer;
    Attributes  attributes;

    void parseSgr(const QString &seq);
    void parseMulti(const QString &seq);
    void parseSingle(const QString &seq);
    void parseChar(char c);
};

#endif // ANSIPARSER_H
