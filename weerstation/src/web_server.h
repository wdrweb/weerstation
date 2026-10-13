#pragma once

class Storage;

class WebServer
{
public:
	~WebServer();
	int start(Storage *source);
	int stop();

private:
	bool running = false;
};
