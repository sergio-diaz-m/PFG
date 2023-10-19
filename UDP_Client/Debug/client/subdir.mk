################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../client/Client.c 

C_DEPS += \
./client/Client.d 

OBJS += \
./client/Client.o 


# Each subdirectory must supply rules for building sources it contributes
client/%.o: ../client/%.c client/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-buildroot-linux-gnueabihf-gcc -I/home/ubuntu/Desktop/buildroot-2023.08.1/output/host/usr/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-client

clean-client:
	-$(RM) ./client/Client.d ./client/Client.o

.PHONY: clean-client

