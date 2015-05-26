SOURCES += \
src/FrontEnd/utilities/app_settings.cc

OBJECTS += \
bld/FrontEnd/utilities/app_settings.o

DEPENDS += \
bld/FrontEnd/utilities/app_settings.d

bld/FrontEnd/utilities/%.o: src/FrontEnd/utilities/%.cc
	@echo 'Building file: $<'
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo ''
