SOURCES += \
src/Emulator/log/logger.cc

OBJECTS += \
bld/Emulator/log/logger.o

DEPENDS += \
bld/Emulator/log/logger.d

bld/Emulator/log/%.o: src/Emulator/log/%.cc
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo ''

