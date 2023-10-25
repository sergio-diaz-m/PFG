################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/latency_test.c 

C_DEPS += \
./src/latency_test.d 

OBJS += \
./src/latency_test.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-buildroot-linux-gnueabihf-gcc -I/home/ubuntu/Desktop/buildroot-2023.08.1/output/host/usr/lib/gcc/arm-buildroot-linux-gnueabihf/12.3.0/plugin/include -I/home/ubuntu/Desktop/buildroot-2023.08.1/output/host/usr/lib -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/latency_test.d ./src/latency_test.o

.PHONY: clean-src

