################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/FrontEnd/game_window.cc \
../src/FrontEnd/log_window.cc \
../src/FrontEnd/main_window.cc \
../src/FrontEnd/name_table_viewer.cc \
../src/FrontEnd/settings_window.cc 

OBJS += \
./src/FrontEnd/game_window.o \
./src/FrontEnd/log_window.o \
./src/FrontEnd/main_window.o \
./src/FrontEnd/name_table_viewer.o \
./src/FrontEnd/settings_window.o 

CC_DEPS += \
./src/FrontEnd/game_window.d \
./src/FrontEnd/log_window.d \
./src/FrontEnd/main_window.d \
./src/FrontEnd/name_table_viewer.d \
./src/FrontEnd/settings_window.d 


# Each subdirectory must supply rules for building sources it contributes
src/FrontEnd/%.o: ../src/FrontEnd/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -DDEBUG -DGLIBMM_PROPERTIES_ENABLED -I"C:\libraries\gtkmm3\include\atkmm-1.6" -I"C:\libraries\gtkmm3\include\cairomm-1.0" -I"C:\libraries\gtkmm3\include\gdkmm-3.0" -I"C:\libraries\gtkmm3\include\giomm-2.4" -I"C:\libraries\gtkmm3\include\glibmm-2.4" -I"C:\libraries\gtkmm3\include\gtkmm-3.0" -I"C:\libraries\gtkmm3\include\pangomm-1.4" -I"C:\libraries\gtkmm3\include\sigc++-2.0" -I"C:\libraries\boost" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" `pkg-config --cflags --libs gtkmm-3.0`
	@echo 'Finished building: $<'
	@echo ' '


