#ifndef INTELHEXFILE_H
#define INTELHEXFILE_H

#include <QFile>
#include <QString>
#include <QByteArray>
#include <QTextStream>

class IntelHexFile
{
public:
    struct Section {
        uint32_t    offset;
        QByteArray  data;
    };

    QList<Section>  sections;

    IntelHexFile();
    IntelHexFile(const QString &fileName);
    IntelHexFile(QTextStream &stream);

    ~IntelHexFile();

    bool loadHex(const QString &fileName);
    bool loadHex(QTextStream &stream);

private:
    uint startAddress;
    uint highAddress;

    int  checksum(const QString &line);
    void addData(uint32_t address, const QByteArray &data);
    bool parseLine(const QString &line);
};

#endif // INTELHEXFILE_H
