/**
 * NutellaSearch.cpp
 *
 * Handles user input and issues requests for a given movie.
 * If the movie is found, create the NutellaPlayer to receive
 * and play the streamed movie.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include <iostream>
#include <sstream>
#include <string>

#include "msock.h"

#include "NutellaSearch.hpp"

/**
 * Factory method to initialize the static member of NutellaSearch
 * 
 * @param  bsock The multicast broadcast socket
 * @param  lsock The multicast listen socket
 * @return       A pointer to a NutellaSearch object
 */
static NutellaSearch *NutellaSearch::makeNutellaSearch(int bsock, int lsock, unsigned int fps, int fps_flag) {
	NutellaSearch::check_mcast = 1;

	return new NutellaSearch(bsock, lsock, fps, fps_flag);
}

/**
 * Constructor
 */
NutellaSearch::NutellaSearch(int bsock, int lsock, unsigned long fps, int fps_flag) 
		: b_msock(bsock), l_msock(lsock), fps(fps), fps_flag(fps_flag), np(NULL) {
	struct sigaction sa;
	struct itimerval timer;

	// set up the timer handler to handle SIGALARM
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &NutellaSearch::handle_timeout;
	sigaction(SIGALARM, &sa, NULL);

	// configure the timer to not do anything yet
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &timer, NULL);
}

/**
 * Destructor
 */
NutellaSearch::~NutellaSearch() {
	// we only use the sockets inside of this class
	close(this->b_msock);
	close(this->l_msock);
}

void NutellaSearch::run() {
	std::string title, recv_title;
	std::string host, port;
	struct itimerval timer;
	char buffer[BUFSIZE];
	int bytes_recv;

	// set up timer values to reset timer
	timer.it_value.tv_sec = 1;		// trigger after 1 second
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;	// don't reset the timer after expiration
	timer.it_interval.tv_usec = 0;

	while (1) {
		// prompt the user for input
		std::cout << "Enter movie title, or 'quit' to exit: " << std::flush;

		std::getline(std::cin, title);

		if (title.size() <= 0) {
			std::cout << "Title invalid" << std::endl;
			continue;
		} else if (title.compare("quit") == 0) {
			break;
		}

		// multicast the title
		msend(this->b_msock, title.c_str(), title.size());

		// set up timeout timer
		setitimer(ITIMER_REAL, &timer, NULL);

		// wait for a reply or timeout
		do {
			if ((bytes_recv = mrecv(this->l_msock, buffer, BUFSIZE)) > 0) {
				// parse the received value to ensure it is a response to
				// this query
				std::stringstream received(string(buffer, bytes_recv));
				std::getline(stringstream, recv_title);
				if (title.compare(recv_title) != 0) {
					// title didn't match
					continue;
				}

				std::getline(stringstream, host);
				std::getline(stringstream, port);
				break;
			}
		} while (check_mcast);

		// create the Nutella Player
		NutellaPlayer *np = new NutellaPlayer(title, host, atoi(port.c_str()), this->fps, this->fps_flag);
		np->run();
	}
}


static void NutellaSearch::handle_timeout(int sig) {
	NutellaSearch::check_mcast = 0;
}
