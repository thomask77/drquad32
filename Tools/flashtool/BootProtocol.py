import crcmod
import time
from datetime import datetime
from intelhex import IntelHex
from Dispatcher import dispatcher
from msg_structs import *
from Queue import Queue


class BootProtocol:
    def __init__(self):
        self.conn = dispatcher.conn
        self.crc32 = crcmod.predefined.Crc("crc-32")
        self.queue = Queue()

        print "BootProtocol: subscribe()"
        dispatcher.subscribe(self.queue, msg_boot_response.MSG_ID)

    def __del__(self):
        print "BootProtocol: unsubscribe()"
        dispatcher.unsubscribe(self.queue, msg_boot_response.MSG_ID)

    def get_response(self):
        return self.queue.get(timeout=2)

    def reset_hack(self):
        # CTRL-C\n reset \n
        self.conn.send_packet("\03\nreset\n")
        time.sleep(1)

    def enter(self):
        msg = msg_boot_enter(magic=0xB00710AD)
        self.conn.send_packet(msg.pack())
        return self.get_response()
        
    def exit(self):
        msg = msg_boot_exit()
        self.conn.send_packet(msg.pack())
        return self.get_response()
        
    def erase_sector(self, sector):
        msg = msg_boot_erase_sector(sector)
        self.conn.send_packet(msg.pack())
        return self.get_response()

    def write_data(self, addr, data, ack_window=0, progress=None):
        offset = 0
        pending = 0

        while offset < len(data):
            chunk = min(len(data) - offset, 240)

            msg = msg_boot_write_data(addr + offset, data[offset:offset+chunk])
            self.conn.send_packet(msg.pack())

            offset += chunk

            if progress is not None:
                progress(offset)

            # queue requests to mask the connection latency
            #
            if pending >= ack_window:
                self.get_response()
            else:
                pending += 1

        # wait for outstanding responses
        #
        while pending > 0:
            self.get_response()
            pending -= 1

    def verify_data(self, addr, data):
        msg = msg_boot_verify_data(addr, len(data))
        self.conn.send_packet(msg.pack())

        res = self.get_response()

        msg_id, remote_crc = struct.unpack("<HL", res)
        local_crc = self.crc32.new(data).crcValue

        if local_crc != remote_crc:
            print("Checksum error: %08x != %08x" % (remote_crc, local_crc))
            return False

        return True

    def write_hex_file(self, filename, progress):
        """
        Send a complete intel .hex-File to the microcontroller
        :param filename: File name of the .hex File
        :param progress: Callback function (percent, text) to report progress
        :return: None
        """
        progress(0, "Loading %s" % filename)

        print "Loading \"%s\"" % filename
        ih = IntelHex(filename)
        start_addr = ih.minaddr()
        end_addr = ih.maxaddr()
        data = ih.tobinstr()

        print "  Start 0x%08x" % start_addr
        print "  End   0x%08x" % end_addr

        progress(2, "Resetting")
        self.reset_hack()

        progress(5, "Entering bootloader")
        self.enter()

        # hack.. discard bytes
        try:
            self.conn._recv_packet()
        except:
            pass

        t0 = datetime.now()

        for i in range(4, 12):
            p = 10 + 10 * (i-4) / (12-4)
            progress(p, "Erasing sector %d..." % i)
            self.erase_sector(i)

        t_erase = datetime.now()

        # skip initial 8 bytes
        #
        self.write_data(
            start_addr + 8, data[8:], ack_window=10,
            progress=lambda x: progress(
                20 + 60 * x / len(data[8:]),
                "Writing 0x%08x..." % (start_addr + 8 + x)
            )
        )

        t_write = datetime.now()

        progress(85, "Verifying")
        self.verify_data(start_addr + 8, data[8:])

        t_verify = datetime.now()

        progress(90, "Writing first 8 bytes")
        self.write_data(start_addr, data[:8])

        progress(95, "Starting application")
        self.exit()

        progress(100, "Done.")

        t_total = datetime.now()

        print "Done."
        print "  Erase:  ", str(t_erase - t0)
        print "  Write:  ", str(t_write - t_erase)
        print "  Verify: ", str(t_verify - t_write)
        print "  Total:  ", str(t_total - t0)
