CXX=g++
CXXFLAGS=-std=c++0x -Wall -MMD

SOURCES=
OBJECTS=
DEPENDS=
BUILDDIRS=

EXEC=bld/D-NES

DIRS=bld bld/Emulator bld/Emulator/log bld/Emulator/mappers bld/FrontEnd bld/FrontEnd/utilities

include src/Emulator/subdir.mk
include src/Emulator/log/subdir.mk
include src/Emulator/mappers/subdir.mk
include src/FrontEnd/subdir.mk
include src/FrontEnd/utilities/subdir.mk

all: CXXFLAGS+=-O3
all: $(EXEC)

debug: CXXFLAGS+=-g
debug: $(EXEC)

$(EXEC): $(OBJECTS)
	@echo 'Building Target: $@'
	$(CXX) $(CXXFLAGS) -o bld/D-NES $(OBJECTS) -lboost_iostreams -lboost_system -lboost_filesystem `wx-config --cxxflags` `wx-config --libs`
	@echo ''

$(OBJECTS): | $(DIRS)

$(DIRS):
	mkdir -p $(DIRS)

.PHONY: clean

clean:
	rm -rf $(OBJECTS) $(DEPENDS) bld/D-NES

-include $(DEPENDS)
