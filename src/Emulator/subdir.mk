SOURCES += \
src/Emulator/cpu.cc \
src/Emulator/ppu.cc \
src/Emulator/nes.cc

OBJECTS += \
bld/Emulator/cpu.o \
bld/Emulator/ppu.o \
bld/Emulator/nes.o

DEPENDS += \
bld/Emulator/cpu.d \
bld/Emulator/ppu.d \
bld/Emulator/nes.d

bld/Emulator/%.o: src/Emulator/%.cc
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo ''

