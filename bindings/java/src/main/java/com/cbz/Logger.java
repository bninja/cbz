package com.cbz;

public interface Logger {
	
	public enum Level {
		DEBUG,
		INFO,
		WARNING, 
		ERROR,
	};
	
	public void log(Level level, String msg);

}
