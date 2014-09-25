################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/resources/subdir.mk
-include src/FrontEnd/utilities/subdir.mk
-include src/FrontEnd/subdir.mk
-include src/Emulator/mappers/subdir.mk
-include src/Emulator/log/subdir.mk
-include src/Emulator/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: pre-build main-build

# Main-build Target
main-build: D-NES.exe

# Tool invocations
D-NES.exe: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MinGW C++ Linker'
	g++ -L"C:\libraries\boost\bin.v2\libs\filesystem" -L"C:\libraries\boost\bin.v2\libs\system" -L"C:\libraries\boost\bin.v2\libs\thread" -o "D-NES.exe" $(OBJS) $(USER_OBJS) $(LIBS)  `pkg-config --cflags --libs gtkmm-3.0`
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C++_DEPS)$(OBJS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) D-NES.exe
	-@echo ' '

pre-build:
	-cp -f ../src/config/config.xml config.xml
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: main-build pre-build

-include ../makefile.targets
