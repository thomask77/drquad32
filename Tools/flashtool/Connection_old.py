import struct
import socket
import serial
import threading

from Dispatcher import dispatcher
from util import hexdump


class ConnectionBase:
    def __init__(self):
        self.stop_request = False

        self.thread = threading.Thread(target=self._thread_func)
        self.thread.daemon = True

        self.description = ""

    def close(self):
        self.stop_request = True
        self.thread.join()

    def _read(self, size):
        raise NotImplementedError("Must be implemented in subclass")

    def _write(self, data):
        raise NotImplementedError("Must be implemented in subclass")

    def _thread_func(self):
        print "RxThread started"

        data = bytearray()
        while not self.stop_request:
            # TODO: Refactor into PacketReader
            #
            try:
                b = self._read(1)
                self.stats.rx_bytes += len(b)
            except TimeoutError:
                # No bytes received
                #
                continue

            if b == "":
                # Connection closed by peer
                #
                break

            if b == "\0":
                # End-of-Packet marker
                #
                try:
                    msg = self._decode_packet(str(data))
                    dispatcher.publish(msg)
                except PacketError, ex:
                    print str(ex)
                data = ""
                continue

            # Append byte
            #
            data += b

        print "RxThread terminated"





class SerialConnection(ConnectionBase):
    def __init__(self, port, baudrate=115200):
        ConnectionBase.__init__(self)
        self.serial = serial.Serial(
            port=port, baudrate=baudrate,
            rtscts=True, timeout=0.1
        )

        self.description = "%s@%d" % (port, baudrate)
        self.thread.start()

    def close(self):
        ConnectionBase.close(self)
        self.serial.close()

    def _read(self, size):
        data = self.serial.read(size)
        if len(data) == 0:
            raise TimeoutError()
        return data

    def _write(self, data):
        return self.serial.write(data)


class RawTCPConnection(ConnectionBase):
    def __init__(self, address, port=2000):
        ConnectionBase.__init__(self)

        self.socket = socket.socket()
        self.socket.settimeout(2)
        self.socket.connect((address, port))

        self.socket.settimeout(0.1)

        self.description = "%s:%d" % (address, port)
        self.thread.start()

    def close(self):
        ConnectionBase.close(self)
        self.socket.close()

    def _read(self, size):
        try:
            return self.socket.recv(size)
        except socket.timeout:
            raise TimeoutError()

    def _write(self, data):
        return self.socket.send(data)

