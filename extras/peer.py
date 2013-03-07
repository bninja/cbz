#!/usr/bin/env python
from argparse import ArgumentParser
import errno
import random
import select
import socket
import SocketServer
import string
from time import sleep


class Handler(SocketServer.BaseRequestHandler):

    TERMINAL = '\r\n'
    PONG_ALPHABET = string.ascii_letters + string.digits

    def __init__(self, request, client_address, server):
        self.buffer = ''
        SocketServer.BaseRequestHandler.__init__(
            self, request, client_address, server)

    def handle(self):
        if not self.server.ping and not self.server.pong:
            return
        while True:
            if self.server.ping:
                msg = self._ping()
            if self.server.pong:
                self._pong()

    def _ping(self):
        idx = self.buffer.find(self.TERMINAL, 0)
        if idx != -1:
            msg = self.buffer[:idx]
            self.buffer = self.buffer[idx + len(self.TERMINAL):]
        else:
            while True:
                if self.server.recv_delay:
                    sleep(self.server.recv_delay)
                try:
                    data = self.request.recv(self.server.recv_size)
                except socket.error, e:
                    if e.errno not in (errno.EWOULDBLOCK, errno.EAGAIN):
                        raise
                    select.select([self.request], [], [], self.server.recv_wait)
                    continue
                start = max(0, len(self.buffer) - len(self.TERMINAL))
                self.buffer += data
                idx = self.buffer.find(self.TERMINAL, start)
                if idx == -1:
                    continue
                msg = self.buffer[:idx]
                self.buffer = self.buffer[idx + len(self.TERMINAL):]
                break
        print 'ping from', '{0}:{1}'.format(*self.client_address)
        print len(msg), msg


    def _pong(self):
        print 'pong to', '{0}:{1}'.format(*self.client_address)

        lines = []
        for i in range(0, self.server.pong_size, self.server.pong_line_size):
            size = self.server.pong_line_size
            size -= max(0, (i + size) - self.server.pong_size)
            if not size:
                break
            line = ''.join(random.choice(self.PONG_ALPHABET) for _ in range(size))
            lines.append(line)
        msg = '\n'.join(lines) + self.TERMINAL

        frag_off = 0
        while frag_off < len(msg):
            if self.server.send_delay:
                sleep(self.server.send_delay)
            frag = msg[frag_off:frag_off + self.server.send_size]
            try:
                sent = self.request.send(frag)
            except socket.error, e:
                if e.errno not in (errno.EWOULDBLOCK, errno.EAGAIN):
                    raise
                select.select([], [self.request], [], self.server.send_wait)
                continue
            print msg[frag_off:frag_off + sent]
            frag_off += sent

class Server(SocketServer.ThreadingMixIn, SocketServer.TCPServer):

    daemon_threads = True

    allow_reuse_address = True

    def __init__(self,
            server_address,
            ping,
            recv_delay,
            recv_wait,
            recv_size,
            pong,
            pong_size,
            pong_line_size,
            send_delay,
            send_wait,
            send_size):
        SocketServer.TCPServer.__init__(self, server_address, Handler)
        self.ping = ping
        self.recv_delay = recv_delay
        self.recv_wait = recv_wait
        self.recv_size = recv_size
        self.pong = pong
        self.pong_size = pong_size
        self.pong_line_size = pong_line_size
        self.send_delay = send_delay
        self.send_wait = send_wait
        self.send_size = send_size

    def get_request(self):
        request = SocketServer.TCPServer.get_request(self)
        request[0].setblocking(0)
        return request


def main():
    parser = ArgumentParser(add_help=False)
    parser.add_argument('-h', '--host', default='127.0.0.1')
    parser.add_argument('-p', '--port', type=int, default=7000)
    parser.add_argument('-d', '--delay', type=int, default=0)
    parser.add_argument('-w', '--wait', type=int, default=0)
    parser.add_argument('--no-ping', dest='ping', action='store_false', default=True)
    parser.add_argument('--ping-size', type=int, default=60)
    parser.add_argument('--ping-line-size', type=int, default=40)
    parser.add_argument('--no-pong', dest='pong', action='store_false', default=True)
    parser.add_argument('--xfer-unit', type=int, default=1024)
    args = parser.parse_args()

    server = Server(
        (args.host, args.port),
        ping=args.ping,
        recv_delay=args.delay,
        recv_wait=args.wait,
        recv_size=args.xfer_unit,
        pong=args.pong,
        pong_size=args.ping_size,
        pong_line_size=args.ping_line_size,
        send_delay=args.delay,
        send_wait=args.wait,
        send_size=args.xfer_unit,
        )
    server.serve_forever()


if __name__ == '__main__':
    main()
