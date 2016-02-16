/**
 * NutellaServer.cpp
 *
 * Overall controlling class for a nutella server, and handles
 * the logic of streaming, acting as a client, and responding
 * to multicast requests.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include <algorithm>
#include <cstdio>
#include <iostream>

#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "proj3.h"
#include "msock.h"
#include "NutellaServer.hpp"

NutellaServer::NutellaServer(int argc, char **argv) 
		: pflag(0), tflag(0), dflag(0), vflag(0), sflag(0), cflag(0), fps(DEFAULT_FPS),
		q_msock(-1), r_msock(-1), nsearch(NULL), nstream(NULL) {

	int c;
	int option_index = 0;	// store the index of the option

	// specify the possible options
	static struct option long_options[] =
		{
			{"passive", no_argument, NULL, 'p'},			// only acts as a streamer
			{"client-only", no_argument, NULL, 'c'},		// Acts only as a client
			{"timed", no_argument, NULL, 't'},				// write timing log files to the log directory
			{"directory", required_argument, NULL, 'd'},	// pass the movie dir by argument
			{"verbose", no_argument, NULL, 'v'},			// print additional debug info
			{"fps", required_argument, NULL, 'f'},			// specify the desired fps
			{"show-fps", no_argument, NULL, 's'},			// show the fps below the movie
			{"help", no_argument, NULL, 'h'},				// print help info
			{0, 0, 0, 0}
		};

	while ((c = getopt_long(argc, argv, "pctd:vf:sh", long_options, &option_index)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'p':
				this->pflag = 1;
				break;
			case 'c':
				this->cflag = 1;
				break;
			case 't':
				this->tflag = 1;
				break;
			case 'd':
				this->dflag = 1;
				this->moviedir = std::string(optarg);
				break;
			case 'v':
				this->vflag = 1;
				break;
			case 'f':
				this->fps = atoi(optarg);
				break;
			case 's':
				this->sflag = 1;
				break;
			case 'h':
				this->usage();
				break;
			case '?':
				std::cout << "Unrecognized option '" << c << "'" << std::endl;
				this->usage();
				break;
		}
	}

	if (!cflag) {
		/* If the movie directory wasn't passed by argument, check the
		   NUTELLA environment variable */
		if (!this->dflag) {
			const char *envname = "NUTELLA";
			char *envdir = getenv(envname);
			if (!envdir) {
				std::cout << "Movie directory must be set by either the ";
				std::cout << " --directory flag (-d) or by the NUTELLA ";
				std::cout << "environment variable" << std::endl;
				this->usage();
			}
			this->moviedir = std::string(envdir);
		}

		// check that the movie directory exists
		struct stat s;
		if (stat(this->moviedir.c_str(), &s) != -1) {
			if (!S_ISDIR(s.st_mode)) {
				std::cout << "Movie directory " << this->moviedir << " must be a directory." << std::endl;
				exit(101);
			}
		} else {
			std::cout << "Movie directory " << this->moviedir << " must exist." << std::endl;
			exit(102);
		}
	}

	if (this->vflag) {
		// flags
		if (this->pflag)
			std::cout << "Passive mode enabled" << std::endl;
		if (this->tflag)
			std::cout << "Timer logging enabled" << std::endl;
		if (this->dflag && !cflag)
			std::cout << "Directory " << this->moviedir << " specified by -d" << std::endl;
		else if (!this->dflag && !cflag)
			std::cout << "Directory " << this->moviedir << " specified by NUTELLA" << std::endl;
		if (this->sflag && !this->pflag)
			std::cout << "FPS display enabled" << std::endl;
		if (!this->pflag)
			std::cout << "Framerate set to " << this->fps << "Hz" << std::endl;
	}

	if (!cflag) {
		DIR *dir = NULL;
		struct dirent *entry;

		// open the movie directory
		if ((dir = opendir(this->moviedir.c_str())) == NULL) {
			perror("opendir()");
			exit(103);
		}

		// save the base filenames of movies to a vector
		while ((entry = readdir(dir))) {
			// ignore directories
			if (entry->d_type == DT_DIR)
				continue;
			this->titles.push_back(std::string(entry->d_name));
		}

		closedir(dir);

		// sort the vector
		std::sort(this->titles.begin(), this->titles.end());

		// setup multicast sockets
		if ((this->q_msock = msockcreate(RECV, DEFAULT_MCAST_QUERY_ADDR, DEFAULT_MCAST_QUERY_PORT)) == -1) {
			std::cout << "Failed to create multicast socket at port " << DEFAULT_MCAST_QUERY_PORT;
			std::cout << " of " << DEFAULT_MCAST_QUERY_ADDR << std::endl;
		}

		if ((this->r_msock = msockcreate(SEND, DEFAULT_MCAST_RESPONSE_ADDR, DEFAULT_MCAST_RESPONSE_PORT)) == -1) {
			std::cout << "Failed to create multicast socket at port " << DEFAULT_MCAST_RESPONSE_PORT;
			std::cout << " of " << DEFAULT_MCAST_RESPONSE_ADDR << std::endl;
		}

		if (this->vflag) {
			std::cout << "NutellaServer: MCast Query Socket: " << this->q_msock << std::endl;
			std::cout << "NutellaServer: MCast Response Socket: " << this->r_msock << std::endl;
		}

		// create a streamer
		nstream = new NutellaStreamer(this->moviedir, this->vflag, this->tflag);
	}
}

/**
 * Destructor
 */
NutellaServer::~NutellaServer() {
	if (this->vflag)
		std::cout << "NutellaServer: Running destructor" << std::endl;
	msockdestroy(this->q_msock);
	msockdestroy(this->r_msock);

	if (this->nsearch != NULL)
		delete this->nsearch;
	if (this->nstream != NULL)
		delete this->nstream;

	// wait for all children to exit
	for (std::vector<pid_t>::iterator it = this->pids.begin(); it != this->pids.end(); ++it) {
		waitpid(*it, NULL, 0);
	}
}

/**
 * Run the main loop of the server
 */
void NutellaServer::run() {
	// if we aren't in passive mode, create a fork for NutellaSearch to accept input
	if (!this->pflag) {
		pid_t pid = fork();

		if (pid == 0) {		// child
			nsearch = NutellaSearch::makeNutellaSearch(this->fps, this->sflag, this->tflag, this->vflag);
			nsearch->run();
		}

		this->pids.push_back(pid);
	}

	if (!this->cflag) {
		// fork off a streamer to handle accepting connections
		pid_t pid_stream = fork();

		if (pid_stream == 0) {
			if (vflag)
				std::cout << "NutellaServer nstream child: Calling msockdestroy: " << std::endl;
			// child, close the multicast ports
			msockdestroy(this->q_msock);
			msockdestroy(this->r_msock);

			this->nstream->run();
		} else if (pid_stream > 0) {
			this->pids.push_back(pid_stream);

			// deactivate the streamer, so it will only serve to generate response messages
			this->nstream->deactivate();
		}

		while (1) {
			handleQuery();

			// check to see if any pids have terminated
			for (std::vector<pid_t>::iterator it = this->pids.begin(); it != this->pids.end();) {
				if (waitpid(*it, NULL, WNOHANG) > 0) {
					// the process whose pid we checked exited
					it = this->pids.erase(it);
				} else {
					// otherwise, increment the iterator
					++it;
				}
			}
		}
	}

	// wait for all children to exit
	for (std::vector<pid_t>::iterator it = this->pids.begin(); it != this->pids.end(); ++it) {
		waitpid(*it, NULL, 0);
	}
}

/**
 * Handle multicast movie requests, and issue a response
 * if the movie exists
 *
 * @return 0 if successful, -1 if an error occurs
 */
int NutellaServer::handleQuery() {
	char buffer[BUFSIZE];
	ssize_t bytes_recv;
	std::string received_title;

	// wait for a request on the socket
	if ((bytes_recv = mrecv(this->q_msock, buffer, BUFSIZE, 0)) == -1) {
		return -1;
	}

	received_title = std::string(buffer, bytes_recv);

	if (this->vflag)
		std::cout << "Received title: " << received_title + ".txt" << std::endl;

	if (std::binary_search(this->titles.begin(), this->titles.end(), received_title)) {
		std::string response = this->nstream->getResponseMessage(received_title);
		msend(this->r_msock, response.c_str(), response.size());
	}

	return 0;
}

void NutellaServer::usage() {
	std::cout << "nutella - P2P ASCII movie streaming" << std::endl;
	std::cout << "Usage: " << std::endl;
	std::cout << "\tnutella [{-p | --passive} | {-c | --client-only}] [-t | --timed] [-d | --directory <directory>] [-v | --verbose] [-s --show-fps] [-h | --help]" << std::endl;
	std::cout << "\t\t-p Run in passive mode and do not act as a client" << std::endl;
	std::cout << "\t\t-c Run in client only mode" << std::endl;
	std::cout << "\t\t-t Log timing information to the ./log folder" << std::endl;
	std::cout << "\t\t-d Specify the movie directory" << std::endl;
	std::cout << "\t\t-v Print verbose output" << std::endl;
	std::cout << "\t\t-s Show FPS during client playback" << std::endl;
	std::cout << "\t\t-h Print this debug information" << std::endl;
	exit(104);
}
