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
#include <csignal>
#include <cstring>
#include <iostream>
#include <queue>
#include <string>

#include <sys/time.h>

#include "proj3.h"

#include "MoviePlayer.hpp"

#ifndef _REFRESH_DISPLAY_
#define _REFRESH_DISPLAY_
volatile sig_atomic_t MoviePlayer::refresh_display;
#endif

/**
 * Constructor
 *
 * Initialize a timer signal to control the rate at which 
 * frames are displayed. Based on:
 * http://www.informit.com/articles/article.aspx?p=23618&seqNum=14
 */
MoviePlayer::MoviePlayer(unsigned long fps, int show_fps, int tflag, int vflag) {
	struct sigaction sa;
	struct itimerval timer;

	this->fps = fps;
	this->show_fps = show_fps;
	this->tflag = tflag;
	this->vflag = vflag;
	this->secs = 0;
	this->usecs = 0;

	memset(&(this->last_frame_time), 0, sizeof(this->last_frame_time));
	gettimeofday(&(this->last_frame_time), NULL);

	// calculate the interval to achieve the given fps
	if (fps <= 1) {
		this->secs = 1;
	} else {
		this->usecs = 1000000 / fps;
	}

	if (this->vflag) {
		std::cout << "MoviePlayer: Computed delays for a framerate of " << fps << "Hz" << std::endl;
		std::cout << "\tseconds:  " << this->secs << std::endl;
		std::cout << "\tuseconds: " << this->usecs << std::endl;
	}

	// set up the timer handler to handle SIGALRM
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &MoviePlayer::handle_timer;
	sigaction(SIGALRM, &sa, NULL);

	// configure the timer
	timer.it_value.tv_sec = secs;
	timer.it_value.tv_usec = usecs;
	timer.it_interval.tv_sec = secs;
	timer.it_interval.tv_usec = usecs;

	setitimer(ITIMER_REAL, &timer, NULL);

	MoviePlayer::refresh_display = 1;
}

MoviePlayer::~MoviePlayer() {
	// disable the signal handler
	signal(SIGALRM, SIG_DFL);

	if (this->vflag)
		std::cout << "Prepping terminal" << std::endl;

	// clear the terminal and any set attributes
	this->clearAttributes();
}

/**
 * Clear the terminal in preparation for movie playback by
 * clearing the screen and moving the cursor to the top left
 */
void MoviePlayer::prepTerminal() {
	struct itimerval timer;
	// configure the timer
	timer.it_value.tv_sec = this->secs;
	timer.it_value.tv_usec = this->usecs;
	timer.it_interval.tv_sec = this->secs;
	timer.it_interval.tv_usec = this->usecs;

	setitimer(ITIMER_REAL, &timer, NULL);
	MoviePlayer::refresh_display = 1;

	if (!vflag)
		std::cout << "\x1B[2J\x1B[1;1H" << std::flush;
}

/**
 * Clear any set ANSI attributes
 */
void MoviePlayer::clearAttributes() {
	struct itimerval timer;

	// disable the timer
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &timer, NULL);

	MoviePlayer::refresh_display = 0;

	if (!vflag)
		std::cout << "\x1B[2J\x1B[1;1H\x1B[0m" << std::flush;
}

/**
 * Print a frame from the queue if the timer has triggered since
 * the last frame was printed
 * 
 * @param frame_queue The queue of frames that haven't been printed yet
 */
void MoviePlayer::printFrame(std::queue<std::string> *frame_queue) {
	if (refresh_display && frame_queue->size() > 0) {
		MoviePlayer::refresh_display = 0;

		// when in verbose mode, we disable movie-like playback
		// as this erases the verbose info
		if (!vflag) {
			// reset the cursor to the top left and clear the display
			std::cout << "\x1B[2J\x1B[1;1H";

			// print the frame
			std::cout << frame_queue->front();

			if (this->show_fps) {
				// go to the next line, and clear to the end of the terminal
				// this will remove the previous fps value
				std::cout << "\n" << this->computeFPS() << " fps";
			}

			std::cout << std::flush;
		}

		frame_queue->pop();
	}
}

sig_atomic_t MoviePlayer::getRefresh() {
	return MoviePlayer::refresh_display;
}

/**
 * Compute the instantaneous fps, and then update the stored time the frame was
 * printed.
 * 
 * @return The instantaneous fps
 */
double MoviePlayer::computeFPS() {
	struct timeval new_time;
	memset(&new_time, 0, sizeof(new_time));
	gettimeofday(&new_time, NULL);

	double sec_diff = (double) (new_time.tv_sec - this->last_frame_time.tv_sec);
	double usec_diff_sec = ((double) (new_time.tv_usec - this->last_frame_time.tv_usec)) / 1000000;

	this->last_frame_time = new_time;

	return 1.0 / (sec_diff + usec_diff_sec);
}

/**
 * SIGALRM signal handler. Updates static refresh_display member to
 * indicate that a new frame can be printed.
 */
void MoviePlayer::handle_timer(int sig) {
	MoviePlayer::refresh_display = 1;
}
