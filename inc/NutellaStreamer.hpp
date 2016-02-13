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
	int port;
	int l_socket, s_socket;	// socket for listening and streaming respectively
	std::string moviepath;

public:
	NutellaStreamer(int socket, std::string dir);
	~NutellaStreamer();
	void run();
private:
	pid_t waitForConnection();
	void receiveTitle();
	void streamMovie();
	void sendFrame(std::string frame);
	void disconnect();
};

#endif