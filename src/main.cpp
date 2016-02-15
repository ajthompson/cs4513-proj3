/**
 * main.cpp
 *
 * Main file for the Nutella P2P ASCII movie streaming program
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include "NutellaServer.hpp"

int main(int argc, char** argv) {
	NutellaServer *server = new NutellaServer(argc, argv);
	server->run();
	delete server;
}