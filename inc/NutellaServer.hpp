/**
 * NutellaServer.hpp
 *
 * Overall controlling class for a nutella server, and handles
 * the logic of streaming, acting as a client, and responding
 * to multicast requests.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#ifndef NUTELLA_SERVER_HPP_
#define NUTELLA_SERVER_HPP_

#include <string>
#include <vector>

#include "NutellaSearch.hpp"
#include "NutellaStreamer.hpp"

class NutellaServer {
	// command line flags and arguments
	int pflag, tflag, dflag, vflag, sflag;
	unsigned long fps;
	std::string moviedir;

	// titles of locally available movies
	std::vector<std::string> titles;

	// multicast sockets
	int q_msock, r_msock;

	// pids of child processes
	std::vector<pid_t> pids;

	NutellaSearch *nsearch;
	NutellaStreamer *nstream;

public:
	NutellaServer(int argc, char **argv);
	~NutellaServer();

	void run();
private:
	int handleQuery();
	void usage();
};

#endif
