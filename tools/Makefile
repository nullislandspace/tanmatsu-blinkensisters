# Host-side BMF tools. These build with the local compiler, not the ESP-IDF
# toolchain -- they run on your PC, not the badge.
#
#   bmfextract  list / unpack a .bmf archive
#   bmfcompress build a .bmf from a config file (from the upstream project)
#
# Both share main/bmf/bmfconvert.* with the game, so the format can only be
# read one way.

CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter
INCLUDES  = -I../main/bmf

BINARIES = bmfextract bmfcompress

.PHONY: all clean
all: $(BINARIES)

bmfextract: bmfextract.cpp ../main/bmf/bmfconvert.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

bmfcompress: bmfcompress.cpp ../main/bmf/bmfconvert.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

clean:
	rm -f $(BINARIES)
