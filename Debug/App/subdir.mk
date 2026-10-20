################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/AdcToChannel.c \
../App/Application.c \
../App/CRSFRadio.c 

OBJS += \
./App/AdcToChannel.o \
./App/Application.o \
./App/CRSFRadio.o 

C_DEPS += \
./App/AdcToChannel.d \
./App/Application.d \
./App/CRSFRadio.d 


# Each subdirectory must supply rules for building sources it contributes
App/%.o App/%.su App/%.cyclo: ../App/%.c App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_BAND_915 -DUSE_MODEM_LORA -DUSE_HAL_DRIVER -DSTM32F405xx -DTX_INCLUDE_USER_DEFINE_FILE -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"M:/STM32CubeIDE/workspace/F405-Joystick-Screen/App" -I"M:/STM32CubeIDE/workspace/F405-Joystick-Screen/Drivers/Board" -I"M:/STM32CubeIDE/workspace/F405-Joystick-Screen/Drivers/Radio" -I../AZURE_RTOS/App -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/usbx_host_classes/inc/ -I../Middlewares/ST/usbx/common/core/inc/ -I../Middlewares/ST/usbx/ports/generic/inc/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/ -I../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App

clean-App:
	-$(RM) ./App/AdcToChannel.cyclo ./App/AdcToChannel.d ./App/AdcToChannel.o ./App/AdcToChannel.su ./App/Application.cyclo ./App/Application.d ./App/Application.o ./App/Application.su ./App/CRSFRadio.cyclo ./App/CRSFRadio.d ./App/CRSFRadio.o ./App/CRSFRadio.su

.PHONY: clean-App

