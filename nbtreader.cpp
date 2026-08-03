#include "nbtreader.h"

NbtReader::NbtReader(const QByteArray &data)
    : m_data(data)
    , m_pos(0)
{
}

quint8 NbtReader::readByte()
{
    quint8 value = static_cast<quint8>(m_data.at(m_pos));
    m_pos += 1;
    return value;
}

qint16 NbtReader::readShort()
{
    quint16 value = (static_cast<quint8>(m_data.at(m_pos)) << 8)
    | static_cast<quint8>(m_data.at(m_pos + 1));
    m_pos += 2;
    return static_cast<qint16>(value);
}

qint32 NbtReader::readInt()
{
    quint32 value = (static_cast<quint8>(m_data.at(m_pos))     << 24)
    | (static_cast<quint8>(m_data.at(m_pos + 1)) << 16)
        | (static_cast<quint8>(m_data.at(m_pos + 2)) << 8)
        |  static_cast<quint8>(m_data.at(m_pos + 3));
    m_pos += 4;
    return static_cast<qint32>(value);
}

qint64 NbtReader::readLong()
{
    quint64 value = 0;
    for (int i = 0; i < 8; i++) {
        value = (value << 8) | static_cast<quint8>(m_data.at(m_pos + i));
    }
    m_pos += 8;
    return static_cast<qint64>(value);
}

QString NbtReader::readString()
{
    qint16 length = readShort();
    QByteArray bytes = m_data.mid(m_pos, length);
    m_pos += length;
    return QString::fromUtf8(bytes);
}

void NbtReader::skip(int bytes)
{
    m_pos += bytes;
}

void NbtReader::skipPayload(quint8 tagType)
{
    switch (tagType) {
    case 1: skip(1); break;                          // Byte
    case 2: skip(2); break;                           // Short
    case 3: skip(4); break;                           // Int
    case 4: skip(8); break;                            // Long
    case 5: skip(4); break;                             // Float
    case 6: skip(8); break;                              // Double
    case 7: {                                             // Byte Array
        qint32 length = readInt();
        skip(length);
        break;
    }
    case 8: {                                             // String
        readString(); // already advances position correctly
        break;
    }
    case 9: {                                             // List
        quint8 elementType = readByte();
        qint32 count = readInt();
        for (int i = 0; i < count; i++) {
            skipPayload(elementType);
        }
        break;
    }
    case 10: {                                            // Compound
        while (true) {
            quint8 childType = readByte();
            if (childType == 0) break; // End tag
            readString(); // child name, discarded
            skipPayload(childType);
        }
        break;
    }
    case 11: {                                            // Int Array
        qint32 count = readInt();
        skip(count * 4);
        break;
    }
    case 12: {                                            // Long Array
        qint32 count = readInt();
        skip(count * 8);
        break;
    }
    default:
        break;
    }
}

bool NbtReader::atEnd() const
{
    return m_pos >= m_data.size();
}