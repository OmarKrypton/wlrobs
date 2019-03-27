################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../proto/wlr-screencopy-unstable-v1-protocol.c \
../proto/xdg-output-unstable-v1-protocol.c 

OBJS += \
./proto/wlr-screencopy-unstable-v1-protocol.o \
./proto/xdg-output-unstable-v1-protocol.o 

C_DEPS += \
./proto/wlr-screencopy-unstable-v1-protocol.d \
./proto/xdg-output-unstable-v1-protocol.d 


# Each subdirectory must supply rules for building sources it contributes
proto/%.o: ../proto/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../inc -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -fsanitize=address -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


