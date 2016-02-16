/**
 * main.cpp
 *
 * Main file for the Nutella P2P ASCII movie streaming program
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include <csignal>
#include <cstdlib>

#include "NutellaServer.hpp"

NutellaServer *server;

void handle_sigint(int sig);

int main(int argc, char** argv) {
	// setup sigint handler so this can exit gracefully
	struct sigaction sa;
	sa.sa_handler = handle_sigint;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	// create and run the server
	server = new NutellaServer(argc, argv);
	server->run();
	delete server;
}

void handle_sigint(int sig) {
	delete server;
	exit(0);
}