package com.cbz;

public class Node {
    
    static { System.loadLibrary("cbzjni"); }
    
    public Host host;
    public Result result;
    public Context ctx;
    public long handle;
    
    public Node(Host host_, Result result_, Context ctx_, long handle_) {
        host = host_;
        result = result_;
        ctx = ctx_;
        handle = handle_;
    }
    
    public Ping ping(String message, int timeout) {
        Node nodes[] = { this };
        return ctx.ping(nodes, message, timeout)[0];
    }
    
    public Pong pong(int timeout) {
        Node nodes[] = { this };
        return ctx.pong(nodes, timeout)[0];
    }
    
    public boolean isConnected() {
        return handle != 0;
    }
    
    public native void disconnect();
    
    protected void finalize() {
        disconnect();
    }
}
