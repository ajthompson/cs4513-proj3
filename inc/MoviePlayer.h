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
#ifndef MOVIE_PLAYER_H_
#define MOVIE_PLAYER_H_

class MoviePlayer {
	unsigned long fps;
	int show_fps;

	// indicate that we should publish a new frame
	static volatile sig_atomic_t refresh_display;

public:
	// factory, to properly initialize refresh
	static MoviePlayer *makeMoviePlayer(int fps, int show_fps);

	~MoviePlayer();

	void prepTerminal();
	void printFrame(std::queue<std::string> frame_queue);

private:
	MoviePlayer(int fps);				// constructor
	static void handle_timer(int sig);	// SIGALARM handler
};

#endif