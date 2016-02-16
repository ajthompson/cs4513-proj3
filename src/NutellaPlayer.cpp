/**
 * NutellaPlayer.cpp
 *
 * The NutellaPlayer is forked off to handle receiving and playing
 * a movie.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "NutellaPlayer.hpp"

#include "proj3.h"

NutellaPlayer::NutellaPlayer(std::string title, std::string streamer_host, 
		int streamer_port, unsigned long fps, int fps_flag, int tflag, int vflag)
		: tflag(tflag), vflag(vflag), sock(-1), mp(NULL), partial_frame("") {
	this->title = title;

	/* Connect to the streamer */
	this->connectToStreamer(streamer_host, streamer_port);

	/* Send the title to the streamer */
	this->sendTitle();

	/* Create the movieplayer */
	this->mp = MoviePlayer::makeMoviePlayer(fps, fps_flag, tflag, vflag);
	this->mp->prepTerminal();
}

/**
 * Destructor
 */
NutellaPlayer::~NutellaPlayer() {
	if (vflag)
		std::cout << "NutellaPlayer: Calling destructor" << std::endl;
	if (this->sock >= 0)
		this->disconnect();
	delete this->mp;
}

void NutellaPlayer::run() {
	if (this->vflag)
		std::cout << "NutellaPlayer: run() started" << std::endl;
	while (this->sock >= 0 || this->frame_queue.size() > 0) {
		if (this->sock >= 0) {
			// if (this->vflag)
			// 	std::cout << "NutellaPlayer: Receiving from streamer" << std::endl;
			// we are still connected with the streamer
			this->receiveStream();
		}

		if (this->frame_queue.size() > 0) {
			this->mp->printFrame(&(this->frame_queue));
		}
	}

	// reset the terminal
	this->mp->clearAttributes();
	this->mp->prepTerminal();

	if (vflag)
		std::cout << "NutellaPlayer: run() finished" << std::endl;
}

void NutellaPlayer::connectToStreamer(std::string streamer_host, int streamer_port) {
	struct addrinfo servAddrInfo = getServInfo(streamer_host, streamer_port);

	if ((this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket()");
		exit(301);
	}

	if ((connect(this->sock, (struct sockaddr *) servAddrInfo.ai_addr, sizeof(servAddrInfo))) != 0) {
		perror("connect()");
		exit(302);
	}

	if (vflag) {
		std::stringstream ss;
		ss << this->sock;
		std::cout << "NutellaPlayer: Connected to socket " << ss.str() << std::endl;
	}
}

/**
 * Disconnect from the streamer
 */
void NutellaPlayer::disconnect() {
	if (this->sock >= 0) {
		close(this->sock);
		this->sock = -1;
	}
}

void NutellaPlayer::sendTitle() {
	if (this->vflag)
		std::cout << "NutellaPlayer: Sending title to streamer: " << this->title << std::endl;
	ssize_t bytes_sent = send(this->sock, this->title.c_str(), this->title.size(), 0);
	if (bytes_sent < 0)
		perror("send()");
}

void NutellaPlayer::receiveStream() {
	std::string temp_frame, temp_buffer;
	char buffer[BUFSIZE] = {};		
	size_t end_pos = 0, last_pos = 0;
	ssize_t bytes_read = 0;

	bytes_read = recv(this->sock, buffer, BUFSIZE, MSG_DONTWAIT);

	if (bytes_read > 0) {
		if (this->vflag)
			std::cout << "NutellaPlayer: Read " << bytes_read << " bytes" << std::endl;
		// create a string, adding the leftovers from previous recv's
		// that could not be parsed into frames
		temp_buffer = partial_frame + std::string(buffer, bytes_read);
		bytes_read += partial_frame.size();
		partial_frame = "";

		if (vflag) {
			std::cout << "Temporary buffer:" << std::endl;
			std::cout << temp_buffer << std::endl;
		}

		// find any end delimiters
		while (end_pos != std::string::npos) {
			end_pos = temp_buffer.find("end", last_pos);
			if (this->vflag)
				std::cout << "NutellaPlayer: Found 'end' at " << end_pos << std::endl;

			if (end_pos != std::string::npos) {
				// create a new substring from last_pos to the beginning of 'end'
				temp_frame = std::string(temp_buffer, last_pos, end_pos);

				// add to the frame queue
				this->frame_queue.push(temp_frame);

				// update last_pos to omit 'end'
				last_pos = end_pos + 3;

				if (this->vflag) {
					std::cout << "NutellaPlayer: Finished frame" << std::endl;
					std::cout << temp_frame << std::endl;
				}
			} else {
				// there are no remaining end delimiters
				// copy the remaining to partial_frame
				this->partial_frame = std::string(temp_buffer, last_pos, end_pos);
				// if (this->vflag)
				// 	std::cout << "NutellaPlayer: Frame incomplete" << std::endl;
			}
		}
	} else if (bytes_read == 0) {
		if (this->vflag)
			std::cout << "NutellaPlayer: Disconnecting" << std::endl;
		// the streamer has disconnected, so the socket should be closed
		this->disconnect();
	} else {
		int error = errno;
		// if (vflag && (error == EWOULDBLOCK || error == EAGAIN))
		// 	std::cout << "NutellaPlayer: No frame waiting" << std::endl;
		/* An error occured, check to make sure it was EAGAIN or EWOULDBLOCK,
		   and not something unexpected */
		if (!(error == EWOULDBLOCK || error == EAGAIN)) {
			perror("recv()");
		}
	}
}

struct addrinfo NutellaPlayer::getServInfo(std::string streamer_host, int streamer_port) {
	struct addrinfo hints, *servInfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	std::stringstream ss;
	ss << streamer_port;
	std::string s_port = ss.str();

	if (getaddrinfo(streamer_host.c_str(), s_port.c_str(), &hints, &servInfo) != 0) {
		perror("getaddrinfo()");
		exit(401);
	}

	return *servInfo;
} 
