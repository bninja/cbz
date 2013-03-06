package com.cbz;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.List;

import org.junit.Before;
import org.junit.Test;

import com.cbz.Context;
import com.cbz.Host;
import com.cbz.Node;
import com.cbz.Ping;
import com.cbz.Pong;
import com.cbz.Result;

public class Tests {
	
	protected Context ctx;
	
	protected class ConsoleLogger implements Logger {
		
		@Override
		public void log(Level level, String msg) {
			String line = String.format("[%s] - %s", level.name().toLowerCase(), msg);
			System.out.println(line);
		}

	}
	
	@Before
    public void setUp() {
		ctx = new Context();
		ctx.logger = new ConsoleLogger();
    }

	private Node[] _connect(Host[] hosts, int timeout, Result result) {
		Node[] nodes = ctx.connect(hosts, timeout);
		assertEquals(hosts.length, nodes.length);
		for (Node node : nodes) {
			if (node.result == Result.OK)
				assertTrue(node.isConnected());
			else
				assertFalse(node.isConnected());
			assertEquals(node.result, result);
		}
		return nodes;
	}
	
	private Ping[] _ping(Node[] nodes, String message, int timeout, Result result) {
		Ping[] pings = ctx.ping(nodes, message, timeout);
		assertEquals(nodes.length, pings.length);
		for(int i = 0; i < nodes.length; i += 1) {
			System.out.println(String.format("pinged %s:%d - %s", nodes[i].host.address, nodes[i].host.port, pings[i].result));
			assertEquals(pings[i].result, result);
		}
		return pings;
	}
	
	private Pong[] _pong(Node[] nodes, int timeout, Result result) {
		Pong[] pongs = ctx.pong(nodes, timeout);
		assertEquals(nodes.length, pongs.length);
		for(int i = 0; i < nodes.length; i += 1) {
			System.out.println(String.format("ponged %s:%d - %s", nodes[i].host.address, nodes[i].host.port, pongs[i].result));
			System.out.println(pongs[i].message);
			assertEquals(pongs[i].result, result);
		}
		return pongs;
	}

	@Test
    public void testConnectionFail() {
		Host hosts[] = {
		    new Host("127.0.0.1", 7000),
		    new Host("127.0.0.1", 7001),
		    new Host("127.0.0.1", 7002),
		};
		_connect(hosts, 0, Result.ERR_CONNECT);
    }
    
    @Test
    public void testPongTimeout() {
    	Context ctx = new Context();
		
		Host hosts[] = {
		    new Host("127.0.0.1", 7000),
		};
		Node[] nodes = _connect(hosts, 0, Result.OK);
		
		_ping(nodes, "ping", 0, Result.OK);
		
		Pong[] pongs = ctx.pong(nodes, 1);
		assertEquals(nodes.length, pongs.length);
		for (Pong pong : pongs) {
			assertEquals(pong.result, Result.ERR_TIMEOUT);
		}
    }
    
    @Test
    public void testPongMaxMessagLength() {
    	Host hosts[] = {
		    new Host("127.0.0.1", 7000),
		};
		Node[] nodes = _connect(hosts, 0, Result.OK);
		
		_ping(nodes, "ping", 0, Result.OK);
		
		ctx.max_message_length = 1;
		Pong[] pongs = ctx.pong(nodes, 1);
		assertEquals(nodes.length, pongs.length);
		for (Pong pong : pongs) {
			assertEquals(Result.ERR_MAX_MSG, pong.result);
		}
    }

    @Test
    public void testPingPong() {
    	Host hosts[] = {
		    new Host("127.0.0.1", 7000),
		};
		Node[] nodes = _connect(hosts, 0, Result.OK);
		_ping(nodes, "ping", 0, Result.OK);
		_pong(nodes, 0, Result.OK);
    }
    
    @Test
    public void testChampion() {
    	Host hosts[] = {
    	    new Host("127.0.0.1", 7000),
    	    new Host("127.0.0.1", 7001),
    	    new Host("127.0.0.1", 7002),
    	    new Host("127.0.0.1", 7003),
		};
    	Node[] nodes = ctx.connect(hosts, 0);
    	List<Node> activeNodes;
    	
    	while (nodes.length != 0) {
    		activeNodes = new ArrayList<Node>();
    		Ping[] pings = ctx.ping(nodes, "ping", 0);
    		for (int i = 0; i < pings.length; i += 1) {
    			System.out.println(String.format("%s:%d - %s", nodes[i].host.address, nodes[i].host.port, pings[i].result));
    			if (pings[i].result == Result.OK) {
    				activeNodes.add(nodes[i]);
    			}
    		}
    		nodes = activeNodes.toArray(new Node[activeNodes.size()]);
    		
    		activeNodes = new ArrayList<Node>();
    		Pong[] pongs = ctx.pong(nodes, 0);
    		for (int i = 0; i < pongs.length; i += 1) {
    			System.out.println(String.format("%s:%d - %s", nodes[i].host.address, nodes[i].host.port, pongs[i].result));
    			System.out.println(pongs[i].message);
    			if (pongs[i].result == Result.OK) {
    				activeNodes.add(nodes[i]);
    			}
    		}
    		nodes = activeNodes.toArray(new Node[activeNodes.size()]);
    	}
    }

}
