Alec Thompson
CS 4513 - Nutella - A P2P Streaming Movie System

Assignment: http://web.cs.wpi.edu/~cs4513/c16/projects/proj3/index.html

Included Files and Folders:
	bin/					- compiled binaries
	inc/					- header files
		MoviePlayer.hpp
		msock.h
		NutellaPlayer.hpp
		NutellaSearch.hpp
		NutellaServer.hpp
		NutellaStreamer.hpp
		proj3.h
	log/					- empty directory for log files
							- if using logging, the binary must be run from repository folder
	movies/					- storage for 4 ASCII movie files
		matrixlong.txt 		- matrix.txt, repeating 10 times for benchmarking
		matrix.txt 			- an ASCII version of a scene from the Matrix 
							- from http://www.justbewise.net/matrix.html?D0=10
		starwars.txt 		- created by Christopher Tashjian
		walk.txt 			- provided in assignment
	obj/					- compiled object files
	src/					- source files
		main.cpp
		MoviePlayer.cpp
		msock.c
		NutellaPlayer.cpp
		MutellaSearch.cpp
		NutellaServer.cpp
		NutellaStreamer.cpp
	Makefile
	README.md

Design Notes:
	I departed from the assignment in a few ways. The most notable one is in handling
	the transmission and playback rates of movies. Instead of controlling the playback
	rate by limiting the transmision rate, I opted for a more Netflix-like solution 
	where the movie is transmitted faster, and the playback occurs within a loop controlled
	by a timer callback. While this causes a higher rate of network traffic for a short time,
	the server can then disconnect before playback has finished, and the client will
	continue playing.

	I also chose to provide more support for more playback rates, as I felt 10fps was far
	too cinematic. The rate still defaults to that, but I have reliably tested up to a
	glorious 60fps with the matrixlong movie on my own desktop. Running on the CCC servers
	at faster than 10 fps is unreliable with the matrix files - likely due to the limitations
	of ssh and my internat connection. On a local network, especially using just a terminal,
	the only limitations should be the computer's hardware, the refresh rate of the
	monitor and terminal interface.

	The next major difference is in how the ports used are handled. The multicast ports are
	hardcoded to 7000 and 7001. The unicast port used by the NutellaStreamer class, which
	handles waiting on connections and streaming the movie is assigned dynamically,
	and the appropriate port is transmitted to the peer requesting a movie.

	Finally, the program is capable of responsing to multiple peers at once - the streamer
	process forks off children to handle each connection request. The program has been
	tested with three clients - one of them acting as the server.

	One element of note is that the program has two options for specifying a movie
	directory. The first is to set the environment variable NUTELLA. The second is to
	use the --directory (-d) command line option. If using a relative directory
	for either of these, the binary MUST be run from the appropriate directory that
	the movie path is relative to.

	Another thing to note is that for timing logging to work, there must be a folder named
	./log in the directory the executable is being run from. The empty log directory
	is included within the repository for this purpose.

	Movie Files:

		ASCII movie files are structured as a series of frames ended by one of two
		delimiters - 'end' or 'stop'. An example can be seen below:

		<frame0>
		end
		<frame2>
		end
		<frame3>
		end

		This file would print out <frame0>, changing to <frame1>, and then ending
		with <frame3> before clearing the screen.

		The movie file must end with a delimiter, otherwise the final frame before
		EOF will not be output.

Usage:

	Building:

		To build the program, run 'make' or 'make all' in the base directory of the
		repository.

		To clean up built files, run 'make clean'. NOTE: Running 'make clean' will remove
		log files.

	Running:

		nutelle - P2P ASCII movie streaming

		nutella [{-p | --passive} | {-c | --client-only}] [-t | --timed] [-d | --directory <directory>] [-v | --verbose] [-f | --fps] [-s | --show-fps] [-h | --help]
			-p Run in passive mode and do not act as a client 	(1)
			-c Run in client only mode							(2)
			-t Log timing information to the ./log folder
			-d Specify the movie directory						(2)
			-v Print verbose output
			-f Specify the framerate in frames per second
			-s Show FPS during client playback
			-h Print this debug information

		(1) passive mode does not provide an interface for movies to be requested
		(2) in client mode, a movie directory does not need to be specified

		The repository is structured to allow easy testing from it's base directory.
		An example command to run a full nutella peer using the included movies
		directory, which plays back movies at 60fps and displays the fps below
		the movie is below:

		./bin/nutella -d ./movies -s -f 60

		For a server-only instance:

		./bin/nutella -d ./movies -p

		And for a client-only instance:

		./bin/nutella -c -s -f 60
