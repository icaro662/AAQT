#ifndef NBTREADER_H
#define NBTREADER_H

#include <QByteArray>
#include <QString>

class NbtReader
{
public:
    explicit NbtReader(const QByteArray &data);

    quint8 readByte();
    qint16 readShort();
    qint32 readInt();
    qint64 readLong();
    QString readString();
    void skip(int bytes);
    void skipPayload(quint8 tagType);
    bool atEnd() const;

private:
    QByteArray m_data;
    int m_pos;
};

#endif // NBTREADER_H