import struct
import cobsr
import crcmod


crc16 = crcmod.predefined.Crc("crc-16")


class PacketError(Exception):
    pass


def decode(data):
    # Decode COBS/R
    #
    try:
        data = cobsr.decode(data)
    except cobsr.DecodeError as ex:
        raise PacketError("COBS/R error: %s" % str(ex))

    if len(data) < 2 + 2:
        raise PacketError("Packet too short")

    # Check CRC
    #
    header_crc, = struct.unpack("<H", data[0:2])
    data_crc = crc16.new(data[2:]).crcValue

    if header_crc != data_crc:
        raise PacketError("CRC error: %04x != %04x" % (header_crc, data_crc))

    return data[2:]


def encode(data):
    # Prepend the checksum
    #
    crc = crc16.new(data).crcValue
    data = struct.pack("<H", crc) + data

    # Encode with COBS/R
    #
    return cobsr.encode(data)

