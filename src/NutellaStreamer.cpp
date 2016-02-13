/**
 * NutellaStreamer.h
 *
 * The NutellaStreamer is forked off to handle streaming a movie
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include <fstream>

#include "proj3.h"
#include "NutellaStreamer.hpp"

/**
 * Constructor
 * 
 * @param port The port to bind to
 * @param dir  The directory containing the movies
 */
NutellaStreamer::NutellaStreamer(int port, std::string dir) 
		: port(port), l_socket(-1), s_socket(-1), moviepath(dir) {
	// setup socket for streaming
	if ((this->l_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket()");
		exit(301);
	}

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(this->port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	// set socket to be able to be resused quickly
	int optval = 1;
	setsockopt(this->l_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	if (bind(this->l_socket, (struct sockaddr *) &address, sizeof(address)) != 0) {
		perror("bind()");
		exit(302);
	}

	if (listen(this->l_socket, MAX_CONNECTION_QUEUE) != 0) {
		perror("listen()");
		exit(303);
	}
}

/**
 * Destructor
 */
NutellaStreamer::~NutellaStreamer() {
	this->disconnect();
}


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
			title += string(buffer, bytes_recv);
		}
	}

	this->moviepath += title;
}

/**
 * Read the movie from the file and transmit each frame to the client
 */
void streamMovie() {
	std::string frame = "";
	// open the movie file
	std::ifstream movie(this->moviepath);

	for (std::string line; getline(input, line);) {

		// check if it's the end of a frame
		if (line.compare("end")) {
			frame += line;

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
void sendFrame(std::string frame) {
	ssize_t bytes_sent = send(this->s_socket, frame.c_str(), frame.size());
	if (bytes_sent < 0)
		perror("send()");
}

/**
 * Disconnect from the client/listening
 */
void NutellaStreamer::disconnect() {
	if (l_socket >= 0)
		close(l_socket);
	if (s_socket >= 0)
		close(m_socket);
}
