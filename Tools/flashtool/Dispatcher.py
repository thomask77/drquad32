import struct


class Dispatcher:
    def __init__(self):
        self.subscribers = {}
        self.conn = None

    def subscribe(self, queue, msg_id):
        if not msg_id in self.subscribers:
            self.subscribers[msg_id] = []
        self.subscribers[msg_id].append(queue)

    def unsubscribe(self, queue, msg_id):
        self.subscribers[msg_id].remove(queue)

    def publish(self, msg):
        msg_id, = struct.unpack("<H", msg[0:2])

        if not msg_id in self.subscribers:
            print "Unhandled message ID 0x%04x" % msg_id
        else:
            for q in self.subscribers[msg_id]:
                q.put(msg)


dispatcher = Dispatcher()
