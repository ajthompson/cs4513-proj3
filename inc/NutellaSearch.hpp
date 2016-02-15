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

#include <csignal>

#include "NutellaPlayer.hpp"

class NutellaSearch {
	int tflag, vflag;		// flags
	int q_msock, r_msock;	// socket descriptors for multicast broadcast/listen
	unsigned long fps;
	int fps_flag;

	NutellaPlayer *np;

public:
	static volatile sig_atomic_t check_mcast;
	static NutellaSearch *makeNutellaSearch(unsigned long fps, int fps_flag, int tflag, int vflag);
	~NutellaSearch();

	void run();
private:
	NutellaSearch(unsigned long fps, int fps_flag, int tflag, int vflag);

	static void handle_timeout(int sig);
};

#endif