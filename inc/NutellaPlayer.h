/**
 * NutellaPlayer.h
 *
 * The NutellaPlayer is forked off to handle receiving and playing
 * a movie.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#ifndef NUTELLA_PLAYER_H_
#define NUTELLA_PLAYER_H_

#include "MoviePlayer.h"

class NutellaPlayer {
	int socket;
	std::string title;

	MoviePlayer *mp;
	std::queue<std::string> frame_queue;
	std::string partial_frame;

public:
	/* Constructor and Destructor */
	NutellaPlayer(std::string title, std::string streamer_host, int streamer_port, unsigned long fps, int fps_flag);
	~NutellaPlayer();

	void run();

private:
	void connect(std::string streamer_host, int streamer_port);
	void disconnect();
	void sendTitle();
	void receiveStream();
};

#endif