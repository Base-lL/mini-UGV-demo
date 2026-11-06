################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/STM32F1xx_HAL_Driver/Ext/MPU/MPU_program.c 

OBJS += \
./Drivers/STM32F1xx_HAL_Driver/Ext/MPU/MPU_program.o 

C_DEPS += \
./Drivers/STM32F1xx_HAL_Driver/Ext/MPU/MPU_program.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/STM32F1xx_HAL_Driver/Ext/MPU/%.o Drivers/STM32F1xx_HAL_Driver/Ext/MPU/%.su Drivers/STM32F1xx_HAL_Driver/Ext/MPU/%.cyclo: ../Drivers/STM32F1xx_HAL_Driver/Ext/MPU/%.c Drivers/STM32F1xx_HAL_Driver/Ext/MPU/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-STM32F1xx_HAL_Driver-2f-Ext-2f-MPU

clean-Drivers-2f-STM32F1xx_HAL_Driver-2f-Ext-2f-MPU:
	-$(RM) ./Drivers/STM32F1xx_HAL_Driver/Ext/MPU/MPU_program.cyclo ./Drivers/STM32F1xx_HAL_Driver/Ext/MPU/MPU_program.d ./Drivers/STM32F1xx_HAL_Driver/Ext/MPU/MPU_program.o ./Drivers/STM32F1xx_HAL_Driver/Ext/MPU/MPU_program.su

.PHONY: clean-Drivers-2f-STM32F1xx_HAL_Driver-2f-Ext-2f-MPU

