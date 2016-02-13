/**
 * NutellaPlayer.cpp
 *
 * The NutellaPlayer is forked off to handle receiving and playing
 * a movie.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#include "errno.h"

#include "proj3.h"

#include "NutellaPlayer.hpp"


NutellaPlayer::NutellaPlayer(std::string title, std::string streamer_host, 
		int streamer_port, unsigned long fps, int fps_flag)
		: socket(-1), mp(NULL), partial_frame("") {

	/* Connect to the streamer */
	this->connect(streamer_host, streamer_port);

	/* Send the title to the streamer */
	this->sendTitle();

	/* Create the movieplayer */
	this->mp = MoviePlayer::makeMoviePlayer(fps, fps_flag);
	this->mp->prepTerminal();
}

/**
 * Destructor
 */
NutellaPlayer::~NutellaPlayer() {
	if (this->socket >= 0)
		this->disconnect();
	delete this->mp;
}

int NutellaPlayer::run() {
	while (this->socket >= 0 || this->frame_queue.size() >= 0) {
		if (this->socket >= 0) {
			// we are still connected with the streamer
			receiveStream();
		}

		this->mp->printFrame(&(this->frame_queue));
	}
}

void NutellaPlayer::connect(std::string streamer_host, int streamer_port) {
	struct addrinfo servAddrInfo = getServInfo(streamer_host, streamer_port);

	if ((this->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket()");
		exit(301);
	}

	if ((connect(this->socket, (struct sockaddr *) servAddrInfo.ai_addr, sizeof(servAddrInfo))) != 0) {
		perror("connect()");
		exit(302);
	}
}

/**
 * Disconnect from the streamer
 */
void NutellaPlayer::disconnect() {
	close(this->socket);
	this->socket = -1;
}

void NutellaPlayer::sendTitle() {
	ssize_t bytes_sent = send(this->socket, this->title.c_str(), this->title.size());
	if (bytes_sent < 0)
		perror("send()");
}

void NutellaPlayer::receiveStream() {
	std::string temp_frame, temp_buffer;
	char buffer[BUFSIZE];		
	size_t end_pos = 0, last_pos = 0;
	ssize_t bytes_read;

	bytes_read = recv(this->socket, buffer, BUFSIZE, MSG_DONTWAIT);

	if (bytes_read > 0) {
		// create a string, adding the leftovers from previous recv's
		// that could not be parsed into frames
		temp_buffer = partial_frame + string(buffer, bytesRead);
		bytes_read += partial_frame.size();

		// find any end delimiters
		while (end_pos != string::npos) {
			end_pos = temp_buffer.find("end", last_pos);

			if (end_pos != string::npos) {
				// create a new substring from last_pos to the beginning of 'end'
				temp_frame = string(temp_buffer, last_pos, end_pos);

				// add to the frame queue
				this->frame_queue.push(temp_frame);

				// update last_pos to omit 'end\n'
				last_pos += 3;
			} else {
				// there are no remaining end delimiters
				// copy the remaining to partial_frame
				this->partial_frame = string(temp_buffer, last_pos, end_pos);
			}
		}
	} else if (bytesRead == 0) {
		// the streamer has disconnected, so the socket should be closed
		this->disconnect();
	} else {
		/* An error occured, check to make sure it was EAGAIN or EWOULDBLOCK,
		   and not something unexpected */
		if (!(errno == EWOULDBLOCK || errno == EAGAIN)) {
			perror("recv()");
		}
	}
}