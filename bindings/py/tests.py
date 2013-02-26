#!/usr/bin/env python
from argparse import ArgumentParser, ArgumentTypeError
import logging
import re
import socket
import sys

import cbz


logger = logging.getLogger(__name__)


def test_cxn_fail(args):
    ctx = cbz.Context(log=log)
    
    peers = resolve_peers(args.peers)
    results, nodes = zip(*ctx.connect(peers))
    assert([cbz.ERR_CONNECT] * len(results) == list(results))


def test_pong_timeout(args):
    ctx = cbz.Context(log=log)
    
    peers = resolve_peers(args.peers)
    results, nodes = zip(*ctx.connect(peers))
    assert([cbz.ERR_CONNECT] * len(results) == list(results))
    
    results = ctx.ping(nodes, 'ping')
    assert([cbz.OK] * len(results) == list(results))
    
    try:
        ctx.pong(args)
    except cbz.Error, ex:
        assert(ex.errno == cbz.ERR_TIMEOUT)
    else:
        assert False
    assert([cbz.ERR_CONNECT] * len(results) == list(results))


def test_pong_max_msg(args):
    ctx = cbz.Context(log=log, max_msg_len=10)
    
    peers = resolve_peers(args.peers)
    results, nodes = zip(*ctx.connect(peers))
    assert([cbz.OK] * len(results) == list(results))
    
    results = ctx.ping(nodes, 'ping')
    assert([cbz.OK] * len(results) == list(results))
    
    results, msgs, nodes = zip(*ctx.pong(nodes))
    assert([cbz.ERR_MAX_MSG] * len(results) == list(results))


def test_ping_empty(args):
    ctx = cbz.Context(log=log)
    
    peers = resolve_peers(args.peers)
    results, nodes = zip(*ctx.connect(peers))
    assert([cbz.OK] * len(results) == list(results))
    
    results, nodes = zip(*ctx.ping(nodes, ''))
    assert([cbz.OK] * len(results) == list(results))
    
    results, msgs, nodes = zip(*ctx.pong(nodes))
    assert([cbz.OK] * len(results) == list(results))


def test_ping_pong(args):
    ctx = cbz.Context(log=log)
    
    peers = resolve_peers(args.peers)
    results, nodes = zip(*ctx.connect(peers))
    assert([cbz.OK] * len(results) == list(results))
    
    results, nodes = zip(*ctx.ping(nodes, 'ping'))
    assert([cbz.OK] * len(results) == list(results))
     
    results, msgs, nodes = zip(*ctx.pong(nodes))
    for result, msg, node in zip(results, msgs, nodes):
        print '{0}:{1}'.format(node.address, node.port), result 
        if result == cbz.OK:
            print msg
    assert([cbz.OK] * len(results) == list(results))
        

def test_champion(args):
    ctx = cbz.Context(log=log)
    
    peers = resolve_peers(args.peers)
    results, nodes = zip(*ctx.connect(peers))
    nodes = filter(None, nodes)
    
    while nodes:
        results = ctx.ping(nodes, 'ping')
        nodes = [node for result, node in results if result == cbz.OK]
         
        results = ctx.pong(nodes)
        for result, msg, node in results:
            print '{0}:{1}'.format(node.address, node.port), result 
            if result == cbz.OK:
                print msg
        nodes = [node for result, msg, node in results if result == cbz.OK]


TESTS = {
    'cxn-fail': test_cxn_fail,
    'pong-timeout': test_pong_timeout,
    'pong-max-msg': test_pong_max_msg,
    'ping-empty': test_ping_empty,
    'ping-pong': test_ping_pong,
    'champion': test_champion,
    }


loggers = {
    cbz.LOG_DBG: logger.debug,
    cbz.LOG_INFO: logger.info,
    cbz.LOG_WARN: logger.warn,
    cbz.LOG_ERR: logger.error,
    }


def log(level, s):
    loggers.get(level, logger.info)(s)
    return cbz.OK


def resolve_peers(peers):
    return [
        (socket.getaddrinfo(host, port, socket.AF_INET)[0][-1])
        for host, port in peers
        ]


def parse_peer(s):
    m = re.match(r'^((?P<host>\.+?)\:)?(?P<port>\d+)$', s)
    if not m:
        raise ArgumentTypeError('Malformed peer "{0}"'.format(s))
    host = m.group('host') or '127.0.0.1'
    port = int(m.group('port'))
    return (host, port)

    
def main():
    # args
    arg_parser = ArgumentParser()
    arg_parser.add_argument('test', nargs=1, choices=TESTS.keys())
    arg_parser.add_argument('peers', nargs='+', type=parse_peer, action='append', default=[], help='[host:]port')
    arg_parser.add_argument('-l', '--log-level', choices=['debug', 'info', 'warn', 'error'], default='warn')
    args = arg_parser.parse_args()
    args.test = args.test[0]
    args.peers = args.peers[0]
    
    # logging
    logger.setLevel(getattr(logging, args.log_level.upper()))
    handler = logging.StreamHandler(sys.stderr)
    fmt = logging.Formatter('%(asctime)s : %(levelname)s : %(name)s : %(message)s')
    handler.setFormatter(fmt)
    logger.addHandler(handler)
    
    # test!
    TESTS[args.test](args)


if __name__ == '__main__':
    main()
