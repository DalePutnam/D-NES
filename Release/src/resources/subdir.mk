################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/resources/resources.c 

OBJS += \
./src/resources/resources.o 

C_DEPS += \
./src/resources/resources.d 


# Each subdirectory must supply rules for building sources it contributes
src/resources/%.o: ../src/resources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"C:\gtk\include\glib-2.0" -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags --libs gtk+-3.0` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


