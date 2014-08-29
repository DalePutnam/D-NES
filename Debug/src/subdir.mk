################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/cart_factory.cc \
../src/cpu.cc \
../src/main.cc \
../src/nes.cc 

OBJS += \
./src/cart_factory.o \
./src/cpu.o \
./src/main.o \
./src/nes.o 

CC_DEPS += \
./src/cart_factory.d \
./src/cpu.d \
./src/main.d \
./src/nes.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


