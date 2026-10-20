################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Radio/CRSF.c \
../Drivers/Radio/crc8.c \
../Drivers/Radio/crsf2.c \
../Drivers/Radio/radio.c \
../Drivers/Radio/sx126x-board.c \
../Drivers/Radio/timer.c 

OBJS += \
./Drivers/Radio/CRSF.o \
./Drivers/Radio/crc8.o \
./Drivers/Radio/crsf2.o \
./Drivers/Radio/radio.o \
./Drivers/Radio/sx126x-board.o \
./Drivers/Radio/timer.o 

C_DEPS += \
./Drivers/Radio/CRSF.d \
./Drivers/Radio/crc8.d \
./Drivers/Radio/crsf2.d \
./Drivers/Radio/radio.d \
./Drivers/Radio/sx126x-board.d \
./Drivers/Radio/timer.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Radio/%.o Drivers/Radio/%.su Drivers/Radio/%.cyclo: ../Drivers/Radio/%.c Drivers/Radio/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_BAND_915 -DUSE_MODEM_LORA -DUSE_HAL_DRIVER -DSTM32F405xx -DTX_INCLUDE_USER_DEFINE_FILE -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"M:/STM32CubeIDE/workspace/F405-Joystick-Screen/App" -I"M:/STM32CubeIDE/workspace/F405-Joystick-Screen/Drivers/Board" -I"M:/STM32CubeIDE/workspace/F405-Joystick-Screen/Drivers/Radio" -I../AZURE_RTOS/App -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/usbx_host_classes/inc/ -I../Middlewares/ST/usbx/common/core/inc/ -I../Middlewares/ST/usbx/ports/generic/inc/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/ -I../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Radio

clean-Drivers-2f-Radio:
	-$(RM) ./Drivers/Radio/CRSF.cyclo ./Drivers/Radio/CRSF.d ./Drivers/Radio/CRSF.o ./Drivers/Radio/CRSF.su ./Drivers/Radio/crc8.cyclo ./Drivers/Radio/crc8.d ./Drivers/Radio/crc8.o ./Drivers/Radio/crc8.su ./Drivers/Radio/crsf2.cyclo ./Drivers/Radio/crsf2.d ./Drivers/Radio/crsf2.o ./Drivers/Radio/crsf2.su ./Drivers/Radio/radio.cyclo ./Drivers/Radio/radio.d ./Drivers/Radio/radio.o ./Drivers/Radio/radio.su ./Drivers/Radio/sx126x-board.cyclo ./Drivers/Radio/sx126x-board.d ./Drivers/Radio/sx126x-board.o ./Drivers/Radio/sx126x-board.su ./Drivers/Radio/timer.cyclo ./Drivers/Radio/timer.d ./Drivers/Radio/timer.o ./Drivers/Radio/timer.su

.PHONY: clean-Drivers-2f-Radio

