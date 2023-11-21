################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/gpio.c \
../src/rtc.c \
../src/test_multitask.c 

C_DEPS += \
./src/gpio.d \
./src/rtc.d \
./src/test_multitask.d 

OBJS += \
./src/gpio.o \
./src/rtc.o \
./src/test_multitask.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-buildroot-linux-gnueabihf-gcc -I/home/ubuntu/Desktop/buildroot-2023.08.1/output/host/usr/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/gpio.d ./src/gpio.o ./src/rtc.d ./src/rtc.o ./src/test_multitask.d ./src/test_multitask.o

.PHONY: clean-src

