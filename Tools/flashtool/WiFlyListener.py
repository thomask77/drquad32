# UDP broadcast receiver for Roving Networks/Microchip
# WiFly Wireless LAN Modules
#
import struct
import string
from datetime import datetime

from PyQt4.QtCore import *
from PyQt4.QtNetwork import *

BROADCAST_PORT = 55555


def zero_terminate(s):
    n = string.find(s, "\0")
    return s[0:n] if n > 0 else s


def unpack_info(data):
    info = {
        "ap_mac":       struct.unpack(">BBBBBB", data[0:6]),
        "channel":      struct.unpack(">B", data[6])[0],
        "rssi":         struct.unpack(">B", data[7])[0],
        "local_port":   struct.unpack(">H", data[8:10])[0],
        "rtc_value":    struct.unpack(">L", data[10:14])[0],
        "u_bat":        struct.unpack(">H", data[14:16])[0],
        "gpio_pins":    struct.unpack(">H", data[16:18])[0],
        "ascii_time":   struct.unpack(">13s", data[18:31])[0],
        "version":      struct.unpack(">26s", data[32:58])[0],
        "device_id":    struct.unpack(">32s", data[60:92])[0],
        "boot_time":    struct.unpack(">H", data[92:94])[0],
        "sensors":      struct.unpack(">HHHHHHHH", data[94:110])
    }

    info["ap_mac"] = ":".join(["%02x" % x for x in info["ap_mac"]])

    # The string fields are zero-terminated
    #
    info["ascii_time"] = string.split(info["ascii_time"], "\0")[0]
    info["version"] = string.split(info["version"], "\0")[0]
    info["device_id"] = string.split(info["device_id"], "\0")[0]

    # Timestamp to filter out old entries
    #
    info["last_seen"] = datetime.now()
    return info


class WiFlyListener(QObject):
    def __init__(self):
        QObject.__init__(self)

        # List of clients
        #
        self.clients = {}

        # Open socket to receive UDP broadcasts
        #
        self.socket = QUdpSocket(self)
        self.socket.bind(BROADCAST_PORT, QUdpSocket.ShareAddress)
        self.socket.readyRead.connect(self.update)

    def update(self):
        while self.socket.hasPendingDatagrams():
            (data, host, port) = self.socket.readDatagram(4096)
            host = host.toString()

            try:
                self.clients[host] = unpack_info(data)

            except struct.error as exc:
                # Ignore non-WiFly messages
                #
                print "Can't unpack WiFly broadcast message from %s: %s" % (host, str(exc))
                pass
