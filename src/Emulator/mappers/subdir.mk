SOURCES += \
src/Emulator/mappers/cart.cc \
src/Emulator/mappers/nrom.cc \
src/Emulator/mappers/sxrom.cc

OBJECTS += \
bld/Emulator/mappers/cart.o \
bld/Emulator/mappers/nrom.o \
bld/Emulator/mappers/sxrom.o

DEPENDS += \
bld/Emulator/mappers/cart.d \
bld/Emulator/mappers/nrom.d \
bld/Emulator/mappers/sxrom.d

bld/Emulator/mappers/%.o: src/Emulator/mappers/%.cc
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo ''

