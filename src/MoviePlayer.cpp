/**
 * MoviePlayer.cpp
 *
 * Prepares the terminal for printing, and prints frames
 * to the terminal, at a refresh rate of 10Hz if possible
 *
 * (Include option for 60Hz for PCMR)
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include <iostream>
#include <queue>

#include <signal.h>
#include <sys/time.h>

#include "proj3.h"

#include "MoviePlayer.h"

/**
 * Constructor
 *
 * Initialize a timer signal to control the rate at which 
 * frames are displayed. Based on:
 * http://www.informit.com/articles/article.aspx?p=23618&seqNum=14
 */
MoviePlayer::MoviePlayer(unsigned long fps, int show_fps) {
	struct sigaction sa;
	struct itimerval timer;
	long usecs;

	this->fps = fps;
	this->show_fps = show_fps; 

	// calculate the interval to achieve the given fps
	usecs = 1000000 / this->fps;

	// set up the timer handler to handle SIGVTALARM
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &MoviePlayer::handle_timer;
	sigaction(SIGALARM, &sa, NULL);

	// configure the timer
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = usecs;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = usecs;

	setitimer(ITIMER_REAL, &timer, NULL);
}

MoviePlayer::~MoviePlayer() {
	// disable the signal handler
	sigaction(SIGALARM, SIG_DFL, NULL);

	// clear the terminal and any set attributes
	this->prepTerminal();
	cout << "\x1B[0m" << std::flush;
}

/**
 * Factory function to create the MoviePlayer and set the refresh variable
 * 
 * @param  fps      The desired framerate in frames per second
 * @param  show_fps If not 0, the fps will be printed below the image
 * 
 * @return     A pointer to the new MoviePlayer
 */
MoviePlayer *MoviePlayer::makeMoviePlayer(unsigned long fps, int show_fps) {
	MoviePlayer *mp = new MoviePlayer(fps, show_fps);

	// set the refresh variable
	MoviePlayer::refresh_display = 0;
}

/**
 * Clear the terminal in preparation for movie playback by
 * clearing the screen and moving the cursor to the top left
 */
void MoviePlayer::prepTerminal() {
	cout << "\x1B[2J\x1B[1;1H" << std::flush;
}

/**
 * Print a frame from the queue if the timer has triggered since
 * the last frame was printed
 * 
 * @param frame_queue The queue of frames that haven't been printed yet
 */
void MoviePlayer::printFrame(std::queue<std::string> frame_queue) {
	if (refresh_display && frame_queue.size() > 0) {
		MoviePlayer::refresh_display = 0;

		// reset the cursor to the top left
		cout << "\x1B[1;1H";

		// print the frame
		cout << frame_queue.pop();

		if (this->show_fps) {
			
		}
	}
}


