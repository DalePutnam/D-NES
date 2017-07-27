CXX = g++
CXXFLAGS = -std=c++11 -Wall -MMD

SOURCE_DIR = src
BUILD_DIR = bld
EXEC = D-NES

####################################################################
# Emulator Definitions
####################################################################

EMU_SOURCE_DIR = $(SOURCE_DIR)/Emulator
EMU_BUILD_DIR = $(BUILD_DIR)/Emulator

EMU_SOURCES = $(shell find $(EMU_SOURCE_DIR) -type f -name '*.cc')
EMU_OBJECTS = $(subst src,$(BUILD_DIR),$(EMU_SOURCES:.cc=.o))
OBJECTS += $(EMU_OBJECTS)
DEPENDS += $(EMU_OBJECTS:.o=.d)

EMU_INCLUDE_DIRS = $(addprefix -I,$(shell find $(EMU_SOURCE_DIR) -type d)) `pkg-config --cflags gtk+-3.0`

####################################################################
# FrontEnd Definitions
####################################################################

FE_SOURCE_DIR = $(SOURCE_DIR)/FrontEnd
FE_BUILD_DIR = $(BUILD_DIR)/FrontEnd

FE_SOURCES = $(shell find $(FE_SOURCE_DIR) -type f -name '*.cc')
FE_OBJECTS = $(subst src,$(BUILD_DIR),$(FE_SOURCES:.cc=.o))
OBJECTS += $(FE_OBJECTS)
DEPENDS += $(FE_OBJECTS:.o=.d)

FE_INCLUDE_DIRS += $(addprefix -I,$(shell find $(FE_SOURCE_DIR) -type d))
FE_INCLUDE_DIRS += $(EMU_INCLUDE_DIRS)

####################################################################
# Build Rules
####################################################################

all: CXXFLAGS += -O2
all: $(EXEC)

debug: CXXFLAGS += -g
debug: $(EXEC)

$(EMU_BUILD_DIR)/%.o: $(EMU_SOURCE_DIR)/%.cc
	@mkdir -p $(dir $@)
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) $(EMU_INCLUDE_DIRS) -c -o $@ $<
	@echo ''

$(FE_BUILD_DIR)/%.o: $(FE_SOURCE_DIR)/%.cc
	@mkdir -p $(dir $@)
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) $(FE_INCLUDE_DIRS) -c -o $@ $< `wx-config --cxxflags`
	@echo ''

$(EXEC): $(OBJECTS)
	@echo 'Building Target: $@'
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) `wx-config --cxxflags --libs` -lasound `pkg-config --libs gtk+-3.0` -lX11
	@echo ''

.PHONY: clean deep-clean

clean:
	rm -rf $(BUILD_DIR) $(EXEC)

deep-clean:
	rm -rf $(BUILD_DIR) $(EXEC) config.txt saves

####################################################################
# Header Dependencies
####################################################################

-include $(DEPENDS)
