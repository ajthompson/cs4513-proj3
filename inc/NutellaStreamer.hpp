/**
 * NutellaStreamer.h
 *
 * The NutellaStreamer is forked off to handle streaming a movie
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#ifndef NUTELLA_STREAMER_HPP_
#define NUTELLA_STREAMER_HPP_

#include <string>

class NutellaStreamer {
	int vflag;
	int port;
	std::string address, s_port;

	int l_socket, s_socket;	// socket for listening and streaming respectively
	std::string moviepath;

public:
	NutellaStreamer(std::string dir, int vflag);
	~NutellaStreamer();
	void run();

	void deactivate();
	std::string getResponseMessage(std::string title);
private:
	pid_t waitForConnection();
	void receiveTitle();
	void streamMovie();
	void sendFrame(std::string frame);
	void disconnect();
};

#endif