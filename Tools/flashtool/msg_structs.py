# File generated from "../../Bootloader/msg_structs.h" on 2014-07-03
#
import struct


class msg_nop:
    """NOP packet (can be used as a keep-alive)"""
    MSG_ID = 0x0000
    LAYOUT = "<H"

    def __init__(self):
        pass

    @classmethod
    def unpack(cls, message):
        id_, = struct.unpack(cls.LAYOUT, message)
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID)


class msg_boot_enter:
    """Enter bootloader"""
    MSG_ID = 0xB000
    LAYOUT = "<HL"

    def __init__(self, magic=0):
        self.magic = magic

    @classmethod
    def unpack(cls, message):
        id_, cls.magic, = struct.unpack(cls.LAYOUT, message)
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID, self.magic)


class msg_boot_response:
    """Bootloader command response"""
    MSG_ID = 0xB001
    LAYOUT = "<HLBB"

    def __init__(self, address=0, length=0, data=""):
        self.address = address
        self.length = length
        self.data = data

    @classmethod
    def unpack(cls, message):
        id_, cls.address, cls.length, cls.data, = struct.unpack(cls.LAYOUT, message)
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID, self.address, self.length, self.data)


class msg_boot_read_data:
    """Read flash memory"""
    MSG_ID = 0xB002
    LAYOUT = "<HLB"

    def __init__(self, address=0, length=0):
        self.address = address
        self.length = length

    @classmethod
    def unpack(cls, message):
        id_, cls.address, cls.length, = struct.unpack(cls.LAYOUT, message)
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID, self.address, self.length)


class msg_boot_write_data:
    """Write flash memory"""
    MSG_ID = 0xB004
    LAYOUT = "<HLB"

    def __init__(self, address=0, data=""):
        self.address = address
        self.length = len(data)
        self.data = data

    @classmethod
    def unpack(cls, message):
        id_, cls.address, cls.length, = struct.unpack(cls.LAYOUT, message[0:7])
        cls.data = message[7:]
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID, self.address, self.length) + self.data


class msg_boot_verify_data:
    """Verify flash memory"""
    MSG_ID = 0xB003
    LAYOUT = "<HLL"

    def __init__(self, address=0, length=0):
        self.address = address
        self.length = length

    @classmethod
    def unpack(cls, message):
        id_, cls.address, cls.length, = struct.unpack(cls.LAYOUT, message)
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID, self.address, self.length)


class msg_boot_erase_sector:
    """Erase flash sector"""
    MSG_ID = 0xB005
    LAYOUT = "<HB"

    def __init__(self, sector=0):
        self.sector = sector

    @classmethod
    def unpack(cls, message):
        id_, cls.sector, = struct.unpack(cls.LAYOUT, message)
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID, self.sector)


class msg_boot_exit:
    """Exit bootloader"""
    MSG_ID = 0xB00F
    LAYOUT = "<H"

    def __init__(self):
        pass

    @classmethod
    def unpack(cls, message):
        id_, = struct.unpack(cls.LAYOUT, message)
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID)


class msg_shell_to_pc:
    """Shell connection Vehicle -> PC"""
    MSG_ID = 0xC000
    LAYOUT = "<H"

    def __init__(self, data=""):
        self.data = data

    @classmethod
    def unpack(cls, message):
        id_, = struct.unpack(cls.LAYOUT, message[0:2])
        cls.data = message[2:]
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID) + self.data


class msg_shell_from_pc:
    """Shell connection PC -> Vehicle"""
    MSG_ID = 0xC001    
    LAYOUT = "<H"

    def __init__(self, data=""):
        self.data = data

    @classmethod
    def unpack(cls, message):
        id_, = struct.unpack(cls.LAYOUT, message[0:2])
        cls.data = message[2:]
        if id != cls.MSG_ID:
            raise ValueError("id %d != %d" % (id_, cls.MSG_ID))
        return cls

    def pack(self):
        return struct.pack(self.LAYOUT, self.MSG_ID) + self.data
