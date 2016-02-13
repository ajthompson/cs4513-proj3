/**
 * Nutella.h
 *
 * Main object implementing the nutella P2P media server.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#ifndef NUTELLA_H_
#define NUTELLA_H_

class Nutella {
	int pflag;
	int file_found;
public:
	Nutella(int cast_port, int recv_port, int pflag);
private:
};

#endif