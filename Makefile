# set to use updated g++ on CCC machines, will use default location if that doesn't exist
# AKA on my own coputers
# 
# This was implemented due to the g++ installed in the typical /usr/bin/g++
# directory by package managers on the WPI servers being nearly 8 years old, 
# while a newer version is available in /usr/bin/local/g++
ifneq ("$(wildcard /usr/local/bin/g++)","")
GXX=/usr/local/bin/g++
GCC=/usr/local/bin/gcc
else
GXX=/usr/bin/g++
GCC=/usr/bin/gcc
endif

IDIR = ./inc
ODIR = ./obj
SDIR = ./src
BDIR = ./bin

__NUTELLA_CXX_SOURCES = $(SDIR)/main.cpp $(SDIR)/NutellaServer.cpp $(SDIR)/NutellaStreamer.cpp $(SDIR)/NutellaPlayer.cpp $(SDIR)/MoviePlayer.cpp

__NUTELLA_C___SOURCES = $(SDIR)/msock.c

NUTELLA_CXX_SOURCES = $(__NUTELLA_CXX_SOURCES)
NUTELLA_C___SOURCES = $(__NUTELLA_C___SOURCES)

__NUTELLA_CXX_OBJECTS = $(patsubst $(SDIR)/%.cpp,$(ODIR)/%.obj,$(NUTELLA_CXX_SOURCES))
__NUTELLA_C___OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(NUTELLA_C___SOURCES))

NUTELLA_OBJECTS = $(__NUTELLA_CXX_OBJECTS) $(__NUTELLA_C___OBJECTS)

L_FLAGS = 

__CXX_FLAGS = -Werror -Wall -Iinc
CXX_FLAGS = $(__CXX_FLAGS)

__C_FLAGS = -Werror -Wall -Iinc
C_FLAGS = $(__CXX_FLAGS)

VPATH = $(SDIR):$(IDIR):$(ODIR)

.PHONY: all

all: nutella

nutella: $(NUTELLA_OBJECTS)
	$(GCC) $(CXX_FLAGS) -o $(BDIR)/nutella $^ $(L_FLAGS)

$(ODIR)/%.obj: %.cpp
	$(GCC) $(CXX_FLAGS) -c $< -o $@

$(ODIR)/%.o: %.c
	$(GCC) $(C_FLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/* $(LDIR)/*
