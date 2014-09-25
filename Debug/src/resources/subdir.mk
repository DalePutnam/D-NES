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
	gcc -D__GXX_EXPERIMENTAL_CXX0X__ -I"C:\libraries\gtk\include\glib-2.0" -I"C:\libraries\gtk\include\gtk-3.0" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" `pkg-config --cflags --libs gtk+-3.0`
	@echo 'Finished building: $<'
	@echo ' '


