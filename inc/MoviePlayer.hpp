/**
 * MoviePlayer.h
 *
 * Prepares the terminal for printing, and prints frames
 * to the terminal, at a refresh rate of 10Hz if possible
 *
 * (Include option for 60Hz for PCMR)
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#ifndef MOVIE_PLAYER_HPP_
#define MOVIE_PLAYER_HPP_

#include <csignal>

class MoviePlayer {
	unsigned long fps;
	int show_fps;

	// fps logging
	struct timeval last_frame_time;

public:
	// indicate that we should publish a new frame
	static volatile sig_atomic_t refresh_display;
	// factory, to properly initialize refresh
	static MoviePlayer *makeMoviePlayer(unsigned long fps, int show_fps);

	~MoviePlayer();

	void prepTerminal();
	void printFrame(std::queue<std::string> *frame_queue);

private:
	MoviePlayer(unsigned long fps, int show_fps);				// constructor

	double computeFPS();
	static void handle_timer(int sig);	// SIGALARM handler
};

#endif