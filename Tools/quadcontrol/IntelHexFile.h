#ifndef INTELHEXFILE_H
#define INTELHEXFILE_H

#include <QString>
#include <QTextStream>

class IntelHexFile
{
public:
    IntelHexFile();
    IntelHexFile(const QString &fileName);
    IntelHexFile(QTextStream &stream);

    ~IntelHexFile();

    struct Section {
        uint  offset;
        QByteArray  data;
    };

    QList<Section>  sections;

    bool loadHex(const QString &fileName);
    bool loadHex(QTextStream &stream);

    QString errorString() const;

private:
    QString m_errorString;

    uint m_startAddress = 0;
    uint m_highAddress = 0;

    int  checksum(const QString &line);
    void addData(uint address, const QByteArray &data);
    bool parseLine(const QString &line);
};

#endif // INTELHEXFILE_H
