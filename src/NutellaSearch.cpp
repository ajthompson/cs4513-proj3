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

#include <signal.h>
#include <sys/time.h>
// #include <sys/types.h>

#include "NutellaSearch.hpp"

#include "msock.h"
#include "proj3.h"

#ifndef CHECK_MCAST_
#define CHECK_MCAST_
volatile sig_atomic_t NutellaSearch::check_mcast;
#endif

/**
 * Factory method to initialize the static member of NutellaSearch
 * 
 * @param  bsock The multicast broadcast socket
 * @param  lsock The multicast listen socket
 * @return       A pointer to a NutellaSearch object
 */
NutellaSearch *NutellaSearch::makeNutellaSearch(unsigned long fps, int fps_flag, int tflag, int vflag) {
	// NutellaSearch::check_mcast = 1;

	return new NutellaSearch(fps, fps_flag, tflag, vflag);
}

/**
 * Constructor
 */
NutellaSearch::NutellaSearch(unsigned long fps, int fps_flag, int tflag, int vflag) 
		: tflag(tflag), vflag(vflag), q_msock(-1), r_msock(-1), fps(fps), fps_flag(fps_flag), np(NULL) {
	struct sigaction sa;
	struct itimerval timer;

	// set up the timer handler to handle SIGALRM
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &NutellaSearch::handle_timeout;
	sigaction(SIGALRM, &sa, NULL);

	// configure the timer to not do anything yet
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &timer, NULL);

	// setup multicast sockets
	if ((this->q_msock = msockcreate(SEND, DEFAULT_MCAST_QUERY_ADDR, DEFAULT_MCAST_QUERY_PORT)) == -1) {
		std::cout << "Failed to create multicast socket at port " << DEFAULT_MCAST_QUERY_PORT;
		std::cout << " of " << DEFAULT_MCAST_QUERY_ADDR << std::endl;
	}

	if ((this->r_msock = msockcreate(RECV, DEFAULT_MCAST_RESPONSE_ADDR, DEFAULT_MCAST_RESPONSE_PORT)) == -1) {
		std::cout << "Failed to create multicast socket at port " << DEFAULT_MCAST_RESPONSE_PORT;
		std::cout << " of " << DEFAULT_MCAST_RESPONSE_ADDR << std::endl;
	}

	if (this->vflag) {
		std::cout << "MCast Query Socket: " << this->q_msock << std::endl;
		std::cout << "MCast Response Socket: " << this->r_msock << std::endl;
	}
}

/**
 * Destructor
 */
NutellaSearch::~NutellaSearch() {
	// we only use the sockets inside of this class
	if (this->q_msock >= 0)
		msockdestroy(this->q_msock);
	if (this->r_msock >= 0)
		msockdestroy(this->r_msock);
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
			// send SIGINT to all NutellaServer processes
			killpg(0, SIGINT);
			break;
		}

		// multicast the title
		if (msend(this->q_msock, title.c_str(), title.size()) == -1) {
			perror("msend()");
			continue;
		}

		// set up timeout timer
		setitimer(ITIMER_REAL, &timer, NULL);

		// wait for a reply or timeout
		do {
			if ((bytes_recv = mrecv(this->r_msock, buffer, BUFSIZE, WNOHANG)) > 0) {
				// parse the received value to ensure it is a response to
				// this query
				std::stringstream received(std::string(buffer, bytes_recv));
				std::getline(received, recv_title);
				if (title.compare(recv_title) != 0) {
					// title didn't match
					continue;
				}

				std::getline(received, host);
				std::getline(received, port);
				break;
			}
		} while (NutellaSearch::check_mcast);

		// create the Nutella Player
		NutellaPlayer *np = new NutellaPlayer(title, host, atoi(port.c_str()), this->fps, this->fps_flag);
		np->run();
	}
}


void NutellaSearch::handle_timeout(int sig) {
	NutellaSearch::check_mcast = 0;
}
