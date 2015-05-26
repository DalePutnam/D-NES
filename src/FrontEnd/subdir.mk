SOURCES += \
src/FrontEnd/app.cc \
src/FrontEnd/main_window.cc \
src/FrontEnd/game_window.cc \
src/FrontEnd/settings_window.cc \
src/FrontEnd/ppu_debug_window.cc \
src/FrontEnd/nes_thread.cc

OBJECTS += \
bld/FrontEnd/app.o \
bld/FrontEnd/main_window.o \
bld/FrontEnd/game_window.o \
bld/FrontEnd/settings_window.o \
bld/FrontEnd/ppu_debug_window.o \
bld/FrontEnd/nes_thread.o

DEPENDS += \
bld/FrontEnd/app.d \
bld/FrontEnd/main_window.d \
bld/FrontEnd/game_window.d \
bld/FrontEnd/settings_window.d \
bld/FrontEnd/ppu_debug_window.d \
bld/FrontEnd/nes_thread.d

INCLUDES=-Isrc/Emulator

bld/FrontEnd/%.o: src/FrontEnd/%.cc
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $< `wx-config --cxxflags` `wx-config --libs`
	@echo ''

