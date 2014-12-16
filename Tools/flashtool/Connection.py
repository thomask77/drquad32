import serial
import socket
import cobsr_packet

from PyQt4.QtCore import *


class TimeoutError(Exception):
    pass


class SerialStream():
    def __init__(self, port):
        self.serial = serial.Serial()
        self.serial.port = port
        self.serial.baudrate = 115200
        self.serial.rtscts = True
        self.serial.timeout = 0.1
        self.serial.open()

    def close(self):
        self.serial.close()

    def read(self, size):
        data = self.serial.read(size)
        if len(data) == 0:
            raise TimeoutError()
        return data

    def write(self, data):
        return self.serial.write(data)


class TCPStream():
    def __init__(self, address):
        self.socket = socket.socket()
        self.socket.settimeout(2)
        self.socket.connect((address, 2000))
        self.socket.settimeout(0.1)

    def close(self):
        self.socket.close()

    def read(self, size):
        try:
            return self.socket.recv(size)
        except socket.timeout:
            raise TimeoutError()

    def write(self, data):
        return self.socket.send(data)


class ConnectionStats:
    def __init__(self):
        self.rx_bytes = 0
        self.rx_packets = 0
        self.rx_errors = 0

        self.tx_bytes = 0
        self.tx_packets = 0


class Connection(QObject):
    ConnectionChanged = pyqtSignal(bool)

    def __init__(self):
        QObject.__init__(self)
        self.stats = ConnectionStats()
        self.stream = None
        self.rx_buf = []
        self.rx_pos = 0

    def is_open(self):
        return self.stream is not None

    def open(self, name):
        if self.is_open():
            self.close()

        # TODO: Better URL parsing
        #
        if name.startswith("tcp://"):
            self.stream = TCPStream(name[6:])
        else:
            self.stream = SerialStream(name)

        self.ConnectionChanged.emit(True)

    def close(self):
        if not self.is_open():
            return

        self.stream.close()
        self.stream = None

        self.ConnectionChanged.emit(False)

    def read_packet(self):
        if not self.is_open():
            return

        # Fetch new data if necessary
        #
        if self.rx_pos == len(self.rx_buf):
            data = self.stream.read(4096)
            self.stats.rx_bytes += len(data)
            self.buffer.append(data)

        # See if there is a packet
        #
        while self.rx_pos < len(self.rx_buf):
            if self.rx_buf[self.rx_pos] == "\0":
                # end-of-packet
                #
                p = None
                try:
                    p = cobsr_packet.decode(self.rx_buf[:self.rx_pos-1])
                    self.stats.rx_packets += 1

                except cobsr_packet.PacketError:
                    self.stats.rx_errors += 1

                self.rx_buf = self.rx_buf[self.rx_pos+1:]
                self.rx_pos = 0
                return p

            self.rx_pos += 1

        return None

    def send_packet(self, p):
        if not self.is_open():
            return

        data = cobsr_packet.encode(p)

        self.stream.write(data)
        self.stats.tx_bytes += len(data)
        self.stats.tx_packets += 1


connection = Connection()