EMU_DIR = $(SOURCE_DIR)/Emulator
EMU_SOURCES = $(shell find $(EMU_DIR) -type f -name '*.cc')
EMU_OBJECTS = $(subst src,$(BUILD_DIR),$(EMU_SOURCES:.cc=.o))
OBJECTS += $(EMU_OBJECTS)

EMU_INCLUDE_DIRS = $(addprefix -I,$(shell find $(EMU_DIR) -type d))

$(BUILD_DIR)/Emulator/%.o: src/Emulator/%.cc
	@mkdir -p $(dir $@)
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) $(EMU_INCLUDE_DIRS) -c -o $@ $<
	@echo ''
