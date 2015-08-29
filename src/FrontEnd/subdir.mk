SOURCES += \
src/FrontEnd/app.cc \
src/FrontEnd/main_window.cc \
src/FrontEnd/game_window.cc \
src/FrontEnd/settings_window.cc \
src/FrontEnd/ppu_debug_window.cc \
src/FrontEnd/nes_thread.cc \
src/FrontEnd/game_list.cc \
src/FrontEnd/pattern_table_display.cc

OBJECTS += \
bld/FrontEnd/app.o \
bld/FrontEnd/main_window.o \
bld/FrontEnd/game_window.o \
bld/FrontEnd/settings_window.o \
bld/FrontEnd/ppu_debug_window.o \
bld/FrontEnd/nes_thread.o \
src/FrontEnd/game_list.o \
src/FrontEnd/pattern_table_display.o

DEPENDS += \
bld/FrontEnd/app.d \
bld/FrontEnd/main_window.d \
bld/FrontEnd/game_window.d \
bld/FrontEnd/settings_window.d \
bld/FrontEnd/ppu_debug_window.d \
bld/FrontEnd/nes_thread.d \
src/FrontEnd/game_list.d \
src/FrontEnd/pattern_table_display.d

INCLUDES=-Isrc/Emulator

bld/FrontEnd/%.o: src/FrontEnd/%.cc
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $< `wx-config --cxxflags` `wx-config --libs`
	@echo ''

