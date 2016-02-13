/**
 * NutellaSearch.hpp
 *
 * Handles user input and issues requests for a given movie.
 * If the movie is found, create the NutellaPlayer to receive
 * and play the streamed movie.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#ifndef NUTELLA_SEARCH_HPP_
#define NUTELLA_SEARCH_HPP_

#include "NutellaPlayer.hpp"

class NutellaSearch {
	int b_msock, l_msock;	// socket descriptors for multicast broadcast/listen
	unsigned long fps;
	int fps_flag;

	NutellaPlayer *np;

	static volatile sig_atomic_t check_mcast;

public:
	static NutellaSearch *makeNutellaSearch(int bsock, int lsock, unsigned int fps, int fps_flag);
	~NutellaSearch();

	void ,run();
private:
	NutellaSearch(int bsock, int lsock, unsigned int fps, int fps_flag);

	static void handle_timeout(int sig);
};

#endif