################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../server/Server.c 

C_DEPS += \
./server/Server.d 

OBJS += \
./server/Server.o 


# Each subdirectory must supply rules for building sources it contributes
server/%.o: ../server/%.c server/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-server

clean-server:
	-$(RM) ./server/Server.d ./server/Server.o

.PHONY: clean-server

