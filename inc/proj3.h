/**
 * proj3.h
 *
 * Overarching header including global macros
 *
 * Alec Thompson - ajthompson@wpi.edu
 * February 2016
 */
#ifndef PROJ3_H_
#define PROJ3_H_

// networking
#define BUFSIZE              1024
#define MAX_CONNECTION_QUEUE 10

#define DEFAULT_MCAST_QUERY_PORT    7000
#define DEFAULT_MCAST_RESPONSE_PORT 7001

#define DEFAULT_MCAST_QUERY_ADDR    "239.0.0.1"
#define DEFAULT_MCAST_RESPONSE_ADDR  "239.0.0.2"

// movie player
#define DEFAULT_FPS          10

#endif