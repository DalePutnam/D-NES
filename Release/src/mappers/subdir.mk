################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/mappers/cart.cc \
../src/mappers/nrom.cc 

OBJS += \
./src/mappers/cart.o \
./src/mappers/nrom.o 

CC_DEPS += \
./src/mappers/cart.d \
./src/mappers/nrom.d 


# Each subdirectory must supply rules for building sources it contributes
src/mappers/%.o: ../src/mappers/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I"C:\gtk\include\gtk-3.0" -I"C:\gtk\include\glib-2.0" -I"C:\gtk\include\cairo" -I"C:\gtk\include\atk-1.0" -I"C:\gtk\include\fontconfig" -I"C:\gtk\include\freetype2" -I"C:\gtk\include\gail-3.0" -I"C:\gtk\include\gdk-pixbuf-2.0" -I"C:\gtk\include\gio-win32-2.0" -I"C:\gtk\include\jasper" -I"C:\gtk\include\libcroco-0.6" -I"C:\gtk\include\libpng15" -I"C:\gtk\include\librsvg-2.0" -I"C:\gtk\include\libxml2" -I"C:\gtk\include\lzma" -I"C:\gtk\include\pango-1.0" -I"C:\gtk\include\pixman-1" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


