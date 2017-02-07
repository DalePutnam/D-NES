CXX = g++
CXXFLAGS = -std=c++11 -Wall -MMD -D_LINUX

SOURCE_DIR = src
BUILD_DIR = bld
EXEC = D-NES

OBJECTS =

all: CXXFLAGS += -O2
all: $(EXEC)

debug: CXXFLAGS += -g
debug: $(EXEC)

include src/Emulator/subdir.mk
include src/FrontEnd/subdir.mk

DEPENDS = $(OBJECTS:.o=.d)

$(EXEC): $(OBJECTS)
	@echo 'Building Target: $@'
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) `wx-config --cxxflags --libs`
	@echo ''

.PHONY: clean

clean:
	rm -rf bld D-NES

-include $(DEPENDS)
