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
	g++ -std=c++0x -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


