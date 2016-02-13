# set to use updated g++ on CCC machines, will use default location if that doesn't exist
# AKA on my own coputers
# 
# This was implemented due to the g++ installed in the typical /usr/bin/g++
# directory by package managers on the WPI servers being nearly 8 years old, 
# while a newer version is available in /usr/bin/local/g++
ifneq ("$(wildcard /usr/local/bin/g++)","")
GXX=/usr/local/bin/g++
else
GXX=/usr/bin/g++
endif

IDIR = ./inc
ODIR = ./obj
SDIR = ./src
BDIR = ./bin

__NUTELLA_SOURCES = $(SDIR)/nutella.cpp $(SDIR)/MoviePlayer.cpp

NUTELLA_SOURCES = $(__NUTELLA_SOURCES)

NUTELLA_OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(NUTELLA_SOURCES))

L_FLAGS = 

__CXX_FLAGS = -Werror -Wall -Iinc
CXX_FLAGS = $(__CXX_FLAGS)

VPATH = $(SDIR):$(IDIR):$(ODIR)

.PHONY: all

all: nutella

nutella: $(NUTELLA_OBJECTS)
	$(GCC) $(CXX_FLAGS) -o $(BDIR)/dsh $^ $(L_FLAGS)

$(ODIR)/%.o: %.c
	$(GCC) $(CXX_FLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/* $(LDIR)/*
