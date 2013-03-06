package com.cbz;

public class Context {
    
    static { System.loadLibrary("cbzjni"); }

    public int max_message_length = 0;
    
    public Logger logger;
    
    public native Node[] connect(Host[] hosts, int timeout);
    
    public native Ping[] ping(Node[] nodes, String message, int timeout);
    
    public native Pong[] pong(Node[] nodes, int timeout);
}
