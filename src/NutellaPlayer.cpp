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
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
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
	this->mp = new MoviePlayer(fps, fps_flag, tflag, vflag);
}

/**
 * Destructor
 */
NutellaPlayer::~NutellaPlayer() {
	if (vflag)
		std::cout << "NutellaPlayer: Calling destructor" << std::endl;
	if (this->sock >= 0)
		this->disconnect();
	if (this->mp != NULL)
		delete this->mp;
}

void NutellaPlayer::run() {
	this->mp->prepTerminal();
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

	if (this->tflag) {		// log to file
		std::ofstream time_log;

		// log the time taken in seconds
		time_log.open("log/total_reception_time.log", std::ios::out | std::ios::app);
		time_log << this->total_reception_time << std::endl;
		time_log.close();

		this->total_reception_time = 0;
	}

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

	if (this->tflag)
		this->setStartTime();
	bytes_read = recv(this->sock, buffer, BUFSIZE, MSG_DONTWAIT);

	if (bytes_read > 0) {
		// create a string, adding the leftovers from previous recv's
		// that could not be parsed into frames
		temp_buffer = partial_frame + std::string(buffer, bytes_read);
		bytes_read += partial_frame.size();
		partial_frame = "";

		if (this->vflag)
			std::cout << "NutellaPlayer: Read " << bytes_read << " bytes" << std::endl;

		if (vflag) {
			std::cout << "Temporary buffer:" << std::endl;
			std::cout << temp_buffer << std::endl;
		}

		// find any end delimiters
		while (1) {

			end_pos = temp_buffer.find("end\n", last_pos);

			// if the movie player is ready to display another frame
			// and we have one ready, put everything into the partial frame storage
			// so we don't impact the framerate too much
			if (this->mp->getRefresh() == 1 && this->frame_queue.size() > 0) {
				end_pos = std::string::npos;
			}

			if (end_pos != std::string::npos) {
				if (this->vflag) {
					std::cout << "NutellaPlayer: Found 'end' at " << end_pos << std::endl;
					std::cout << "\tlast_pos: " << last_pos << std::endl;
					std::cout << "\tend_pos:  " << end_pos << std::endl;
				}
				// create a new substring from last_pos to the beginning of 'end'
				temp_frame = std::string(temp_buffer, last_pos, end_pos - last_pos);

				// add to the frame queue
				this->frame_queue.push(temp_frame);

				// update last_pos to omit 'end\n'
				last_pos = end_pos + 4;

				if (this->vflag) {
					std::cout << "NutellaPlayer: Finished frame" << std::endl;
					std::cout << temp_frame << std::endl;
				}
			} else {
				// there are no remaining end delimiters
				// copy the remaining to partial_frame
				this->partial_frame += std::string(temp_buffer, last_pos);
				if (this->vflag) {
					std::cout << "NutellaPlayer: Frame incomplete:" << std::endl;
					std::cout << this->partial_frame << std::endl;
				}
				if (this->tflag)
					this->addTimeDiff();
				break;
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

/**
 * Set the stored start time
 */
void NutellaPlayer::setStartTime() {
	memset(&(this->start_time), 0, sizeof(this->start_time));
	gettimeofday(&(this->start_time), NULL);
}

/**
 * Compute the change in transfer time and add it to the total
 */
void NutellaPlayer::addTimeDiff() {
	double s_diff, us_diff;
	struct timeval finish_time;
	memset(&finish_time, 0, sizeof(finish_time));
	gettimeofday(&finish_time, NULL);

	// compute the difference
	s_diff = (double) finish_time.tv_sec - this->start_time.tv_sec;
	us_diff = (double) finish_time.tv_usec - this->start_time.tv_usec;

	this->total_reception_time += s_diff + (us_diff / 1000000);
}
