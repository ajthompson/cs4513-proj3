######### NOT UPDATED FOR PROGRAM 3 YET ###########
# Just placeholder for general structure

# set to use updated g++ on CCC machines, will use default location if that doesn't exist
# AKA on my own coputers
ifneq ("$(wildcard /usr/local/bin/gcc)","")
GCC=/usr/local/bin/gcc
else
GCC=/usr/bin/gcc
endif

################################
##### BEGIN TEST VARIABLES #####
################################
SERVER = ec2-54-191-127-23.us-west-2.compute.amazonaws.com
PORT = 40000
CREDENTIALS = ~/.ssh/cs4513.pem
USERNAME = ubuntu
NUM_ITERATIONS = 100

##############################
##### END TEST VARIABLES #####
##############################

IDIR = ./inc
ODIR = ./obj
SDIR = ./src
BDIR = ./bin
LDIR = ./log
TDIR = ./test

__SHARED_SOURCES = $(SDIR)/proj2.c
__CLIENT_SOURCES = $(SDIR)/dsh.c
__SERVER_SOURCES = $(SDIR)/dss.c

CLIENT_SOURCES = $(__CLIENT_SOURCES) $(__SHARED_SOURCES)
SERVER_SOURCES = $(__SERVER_SOURCES) $(__SHARED_SOURCES)

CLIENT_OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(CLIENT_SOURCES))
SERVER_OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SERVER_SOURCES))

L_FLAGS = -lcrypt

__C_FLAGS = -Werror -Wall -Iinc -std=gnu99 -D_POSIX_C_SOURCE=199309L
C_FLAGS = $(__C_FLAGS)

VPATH = $(SDIR):$(IDIR):$(ODIR)

.PHONY: all

all: dsh dss

dsh: $(CLIENT_OBJECTS)
	$(GCC) $(C_FLAGS) -o $(BDIR)/dsh $^ $(L_FLAGS)

dss: $(SERVER_OBJECTS)
	$(GCC) $(C_FLAGS) -o $(BDIR)/dss $^ $(L_FLAGS)

$(ODIR)/%.o: %.c
	$(GCC) $(C_FLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/* $(LDIR)/*

.PHONY: test

test: all
	$(TDIR)/benchmark.bash $(SERVER) $(PORT) $(CREDENTIALS) $(USERNAME) $(NUM_ITERATIONS)
