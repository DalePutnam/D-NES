################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/main.cc 

OBJS += \
./src/main.o 

CC_DEPS += \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I"C:\gtkmm3\include\atkmm-1.6" -I"C:\gtkmm3\include\cairomm-1.0" -I"C:\gtkmm3\include\gdkmm-3.0" -I"C:\gtkmm3\include\giomm-2.4" -I"C:\gtkmm3\include\glibmm-2.4" -I"C:\gtkmm3\include\gtkmm-3.0" -I"C:\gtkmm3\include\pangomm-1.4" -I"C:\gtkmm3\include\sigc++-2.0" -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags --libs gtkmm-3.0` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


