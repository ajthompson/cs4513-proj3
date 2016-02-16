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
#include <sys/time.h>
#include <unistd.h>

#include "proj3.h"
#include "NutellaStreamer.hpp"

/**
 * Constructor
 * 
 * @param port The port to bind to
 * @param dir  The directory containing the movies
 */
NutellaStreamer::NutellaStreamer(std::string dir, int vflag, int tflag) 
		: vflag(vflag), tflag(tflag), l_socket(-1), s_socket(-1), moviepath(dir), 
		total_bytes_sent(0), total_transfer_time(0.0) {
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
	// prevent timing logging
	this->tflag = 0;
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
			std::cout << "Accepted connection on socket " << this->s_socket << std::endl;

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

	bytes_recv = recv(this->s_socket, buffer, BUFSIZE, 0);
	if (bytes_recv > 0) {
		title += std::string(buffer, bytes_recv);
		if (vflag) {
			std::cout << "NutellaStreamer: Received title: " << std::endl;
			std::cout << "\tbytes_recv: " << bytes_recv << std::endl;
			std::cout << "\ttitle:      " << title << std::endl;
		}
	} else if (bytes_recv < 0) {
		std::cout << "NutellaStreamer: Failed to receive message" << std::endl;
	} else {
		std::cout << "NutellaStreamer: Received " << bytes_recv << " bytes" << std::endl;
	}

	this->moviepath += title;

	if (vflag)
		std::cout << "NutellaStreamer: Received title " << this->moviepath << std::endl;
}

/**
 * Read the movie from the file and transmit each frame to the client
 */
void NutellaStreamer::streamMovie() {
	std::string frame = "";
	// open the movie file
	std::ifstream movie(this->moviepath.c_str());

	if (this->tflag)
		this->setStartTime();
	for (std::string line; getline(movie, line);) {
		int end_cmp, stop_cmp;

		// check if it's the end of a frame
		if ((end_cmp = line.compare("end")) == 0 || (stop_cmp = line.compare("stop")) == 0) {
			frame += "end\n";

			if (this->vflag)
				std::cout << "NutellaStreamer: Sending frame ending with '" << line << "'" << std::endl;

			// send the frame
			this->sendFrame(frame);

			// reset the frame string
			frame = "";
			continue;
		}

		// add the line to the frame, and readd the stripped newline
		frame += line + "\n";

		if (this->vflag) {
			std::cout << "NutellaStreamer: Adding line: " << line << std::endl;
		}
	}
	if (this->tflag)
		this->addTimeDiff();

	if (this->tflag) {		// log to file
		std::ofstream byte_log, time_log;

		// log the number of bytes written
		byte_log.open("log/bytes_transferred.log", std::ios::out | std::ios::app);
		byte_log << this->total_bytes_sent << std::endl;
		byte_log.close();

		// log the time taken in seconds
		time_log.open("log/total_transfer_time.log", std::ios::out | std::ios::app);
		time_log << this->total_transfer_time << std::endl;
		time_log.close();

		// reset the bytes sent and transfer time
		this->total_bytes_sent = 0;
		this->total_transfer_time = 0;
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
	if (this->vflag) {
		std::cout << "NutellaStreamer: Sending:" << std::endl;
		std::cout << frame << std::endl;
	}
	ssize_t bytes_sent = send(this->s_socket, frame.c_str(), frame.size(), 0);
	if (this->vflag)
		std::cout << "NutellaStreamer: Sent " << bytes_sent << " bytes" << std::endl;
	if (this->tflag)	// packet paylod + TCP header (20 bytes) + IP header (20 bytes) + Ethernet frame (24 bytes)
		this->total_bytes_sent += bytes_sent + 64;
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

/**
 * Set the stored start time
 */
void NutellaStreamer::setStartTime() {
	memset(&(this->start_time), 0, sizeof(this->start_time));
	gettimeofday(&(this->start_time), NULL);
}

/**
 * Compute the change in transfer time and add it to the total
 */
void NutellaStreamer::addTimeDiff() {
	double s_diff, us_diff;
	struct timeval finish_time;
	memset(&finish_time, 0, sizeof(finish_time));
	gettimeofday(&finish_time, NULL);

	// compute the difference
	s_diff = (double) finish_time.tv_sec - this->start_time.tv_sec;
	us_diff = (double) finish_time.tv_usec - this->start_time.tv_usec;

	this->total_transfer_time += s_diff + (us_diff / 1000000);
}
