/**
 * NutellaPlayer.cpp
 *
 * The NutellaPlayer is forked off to handle receiving and playing
 * a movie.
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */

#include "proj3.h"

#include "NutellaPlayer.h"


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
	while (this->socket >= 0 && this->frame_queue.size() >= 0) {
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

}

void NutellaPlayer::receiveStream() {
	std::string temp_line, temp_buffer;
	char buffer[1024];

	ssize_t bytesRead = recv
}