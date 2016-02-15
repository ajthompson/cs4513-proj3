/**
 * NutellaStreamer.h
 *
 * The NutellaStreamer is forked off to handle streaming a movie
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "proj3.h"
#include "NutellaStreamer.hpp"

/**
 * Constructor
 * 
 * @param port The port to bind to
 * @param dir  The directory containing the movies
 */
NutellaStreamer::NutellaStreamer(std::string dir, int vflag) 
		: vflag(vflag), l_socket(-1), s_socket(-1), moviepath(dir) {
	// setup a socket to stream
	if ((this->l_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket()");
		exit(301);
	}

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(0);		// get any open port
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(this->l_socket, (struct sockaddr *) &address, sizeof(address)) != 0) {
		perror("bind()");
		exit(302);
	}

	if (listen(this->l_socket, MAX_CONNECTION_QUEUE) != 0) {
		perror("listen()");
		exit(303);
	}

	// get the port
	struct sockaddr_in s_in;
	socklen_t len = sizeof(s_in);

	if (getsockname(l_socket, (struct sockaddr *) &s_in, &len) == -1) {
		perror("getsockname()");
		exit(304);
	} else {
		this->port = ntohs(s_in.sin_port);
		std::stringstream ss;
		ss << this->port;
		this->s_port = ss.str();
	}

	// get the IP address
	struct ifaddrs *ifaddr, *ifa;
	
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs()");
		exit(305);
	}

	if (vflag)
		std::cout << "Getting network interfaces" << std::endl;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		struct sockaddr_in *addr = (struct sockaddr_in *) ifa->ifa_addr;

		if (vflag) {
			std::cout << "\t" << ifa->ifa_name << std::endl;
			std::cout << "\t\t" << inet_ntoa(addr->sin_addr) << std::endl;
			if (ifa->ifa_addr->sa_family == AF_PACKET)
				std::cout << "\t\tAF_PACKET" << std::endl;
			else if (ifa->ifa_addr->sa_family == AF_INET)
				std::cout << "\t\tAF_INET" << std::endl;
			else if (ifa->ifa_addr->sa_family == AF_INET6)
				std::cout << "\t\tAF_INET6" << std::endl;
		}

		if ((strncmp(ifa->ifa_name, "eth", 3) == 0 || strncmp(ifa->ifa_name, "wlan", 3) == 0) 
			&& ifa->ifa_addr->sa_family == AF_INET) {
			// we've found an IP address
			this->address = std::string(inet_ntoa(addr->sin_addr));
			break;
		}
	}
}

/**
 * Destructor
 */
NutellaStreamer::~NutellaStreamer() {
	if (this->vflag)
		std::cout << "NutellaStreamer: Running destructor" << std::endl;
	this->disconnect();
}

/**
 * Run the main loop of the class.
 *
 * This waits for connections, which then forks, and
 * if this is the child, performs the necessary actions
 * to stream a requested movie.
 */
void NutellaStreamer::run() {
	pid_t pid;

	pid = this->waitForConnection();

	// if this is the child process, stream the movie
	if (pid == 0) {			// child
		// get the name of the movie
		this->receiveTitle();
		this->streamMovie();
	}
}

/**
 * Closes the objects sockets
 * 
 * Used for the streamer kept in the parent process,
 * as it is forked to be able to wait for connections
 */
void NutellaStreamer::deactivate() {
	this->disconnect();
}

/**
 * Construct a response message with the title of the movie, the address
 * of the server, and the unicast port to connect on.
 *
 * The message is formatted as follows:
 *
 * title\n
 * address\n
 * port\n
 * 
 * @param  title The title of the requested movie
 * @return       the response message
 */
std::string NutellaStreamer::getResponseMessage(std::string title) {
	if (this->vflag) {
		std::cout << "Generating response message: " << std::endl;
		std::cout << "\t" << title << std::endl;
		std::cout << "\t" << this->address << std::endl;
		std::cout << "\t" << this->s_port << std::endl;
	}
	return title + "\n" + this->address + "\n" + this->s_port;
}

/**
 * Wait for connections to the listening socket, then fork
 * to handle accepted connections
 * 
 * @return The pid of the forked process (or 0 if child)
 */
pid_t NutellaStreamer::waitForConnection() {
	pid_t pid;

	while(1) {
		// accept a connection
		struct sockaddr_in address;
		socklen_t addr_len = sizeof(address);
		memset(&address, 0, addr_len);

		if ((this->s_socket = accept(this->l_socket, (struct sockaddr *) &address, &addr_len)) == -1) {
			perror("accept()");
			exit(304);
		}

		if (this->vflag)
			std::cout << "Accepted connection" << std::endl;

		// fork the process
		pid = fork();

		if (pid == 0) {
			// close the listening socket
			close(this->l_socket);
			this->l_socket = -1;

			return pid;
		} else if (pid > 0) {
			// close the streaming socket
			close(this->s_socket);
			this->s_socket = -1;
		} else {
			perror("fork()");
		}
	}

	return pid;
}

/**
 * Receive the title of the file to be streamed
 */
void NutellaStreamer::receiveTitle() {
	std::string title = "/";
	char buffer[BUFSIZE];
	ssize_t bytes_recv;
	int try_count = 0;	// keep track of how many errors we got
						// this prevents only part of the title being
						// received if we called recv before the entire
						// payload was waiting at the socket

	while ((bytes_recv = recv(this->s_socket, buffer, BUFSIZE, MSG_DONTWAIT)) > 0 && try_count < 5) {
		if (bytes_recv < 0) {
			if (!(errno == EWOULDBLOCK || errno == EAGAIN)) {
				// we got an unexpected error
				perror("recv()");
			}

			try_count++;
		} else if (bytes_recv > 0) {
			title += std::string(buffer, bytes_recv);
		}
	}

	this->moviepath += title;
}

/**
 * Read the movie from the file and transmit each frame to the client
 */
void NutellaStreamer::streamMovie() {
	std::string frame = "";
	// open the movie file
	std::ifstream movie(this->moviepath.c_str());

	for (std::string line; getline(movie, line);) {

		// check if it's the end of a frame
		if (line.compare("end")) {
			frame += line;

			if (this->vflag)
				std::cout << "Read frame:\n" << frame << std::endl;

			// send the frame
			this->sendFrame(line);

			// reset the frame string
			frame = "";
			continue;
		}

		// add the line to the frame, and readd the stripped newline
		frame += line + "\n";
	}

	// close the file and disconnect
	movie.close();
	this->disconnect();
}

/**
 * Send a frame to the client
 * @param frame A string consisting of a frame of the movie
 */
void NutellaStreamer::sendFrame(std::string frame) {
	ssize_t bytes_sent = send(this->s_socket, frame.c_str(), frame.size(), 0);
	if (bytes_sent < 0)
		perror("send()");
}

/**
 * Disconnect from the client/listening
 */
void NutellaStreamer::disconnect() {
	if (l_socket >= 0) {
		close(l_socket);
		l_socket = -1;
	}
	if (s_socket >= 0) {
		close(s_socket);
		s_socket = -1;
	}
}
