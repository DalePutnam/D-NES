FE_DIR = $(SOURCE_DIR)/FrontEnd
FE_SOURCES = $(shell find $(FE_DIR) -type f -name '*.cc')
FE_OBJECTS = $(subst src,$(BUILD_DIR),$(FE_SOURCES:.cc=.o))
OBJECTS += $(FE_OBJECTS)

FE_INCLUDE_DIRS =  $(addprefix -I,$(shell find $(FE_DIR) -type d))
FE_INCLUDE_DIRS += $(addprefix -I,$(shell find src/Emulator -type d))

$(BUILD_DIR)/FrontEnd/%.o: src/FrontEnd/%.cc
	@mkdir -p $(dir $@)
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) $(FE_INCLUDE_DIRS) -c -o $@ $< `wx-config --cxxflags`
	@echo ''
