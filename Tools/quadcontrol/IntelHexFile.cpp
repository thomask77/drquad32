#include "IntelHexFile.h"
#include <QDebug>

IntelHexFile::IntelHexFile() :
    startAddress(), highAddress()
{

}

IntelHexFile::IntelHexFile(const QString &fileName) :
    IntelHexFile()
{
    loadHex(fileName);
}

IntelHexFile::IntelHexFile(QTextStream &stream) :
    IntelHexFile()
{
    loadHex(stream);
}

IntelHexFile::~IntelHexFile()
{

}

bool IntelHexFile::loadHex(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();
        return false;
    }

    QTextStream stream(&file);
    return loadHex(stream);
}

bool IntelHexFile::loadHex(QTextStream &stream)
{
    int lineNumber = 0;
    highAddress = 0;

    while (!stream.atEnd()) {
        auto line = stream.readLine();
        lineNumber++;

        if (!parseLine(line)) {
            qDebug("Invalid data in line %d", lineNumber);
            return false;
        }
    }

    return true;
}

int IntelHexFile::checksum(const QString &line)
{
    uint8_t checksum = 0;

    for (int i=1; i < line.length(); i+=2) {
        bool ok;
        checksum += line.mid(i, 2).toUInt(&ok, 16);

        if (!ok)
            return -1;
    }

    return checksum;
}

void IntelHexFile::addData(uint32_t address, const QByteArray &data)
{
    if (sections.isEmpty() ||
            address != sections.last().offset + sections.last().data.size() )
    {
        // Start a new section if necessary
        //
        Section s;
        s.offset = address;
        sections.append(s);
    }

    sections.last().data.append(data);
}

bool IntelHexFile::parseLine(const QString &line)
{
    bool ok;

    if (line[0] != ':' || checksum(line) != 0)
        return false;

    uint length = line.mid(1, 2).toUInt(&ok, 16);
    if (!ok)
        return false;

    uint address = line.mid(3, 4).toUInt(&ok, 16) + highAddress;
    if (!ok)
        return false;

    uint type = line.mid(7, 2).toUInt(&ok, 16);
    if (!ok)
        return false;

    QByteArray data(length, 0);
    for (uint i=0; i<length; i++) {
        data[i] = line.mid(9 + i*2, 2).toUInt(&ok, 16);
        if (!ok)
            return false;
    }

    switch (type) {
    case 0:
        // data record
        //
        addData(address, data);
        return true;

    case 1:
        // end of file
        //
        return true;

    case 2:
        // extended segment address
        //
        highAddress = (((uint)data[0] << 8) | (uint)data[1]) * 16;
        return true;

    case 3:
        // start segment address
        //
        startAddress = (((uint)data[0] << 8) | (uint)data[1]) * 16 +
                       (((uint)data[2] << 8) | (uint)data[3]);
        return true;

    case 4:
        // extended linear address
        //
        highAddress = ((uint)data[0] << 24) | ((uint)data[1] << 16);
        return true;

    case 5:
        // start linear address
        //
        startAddress = ((uint)data[0] << 24) | ((uint)data[1] << 16) |
                       ((uint)data[2] <<  8) | ((uint)data[3] <<  0) ;
        return true;

    default:
        return false;
    }
}
