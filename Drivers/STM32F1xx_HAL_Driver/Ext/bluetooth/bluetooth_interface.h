#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "stm32f1xx_hal.h"

/* Buffer size */
#define BT_BUFFER_SIZE 64U

/* Initialize Bluetooth UART */
HAL_StatusTypeDef Bluetooth_Init(UART_HandleTypeDef *huart);

/* Check if data is available */
uint8_t Bluetooth_Available(void);

/* Read one byte */
uint8_t Bluetooth_Read(uint8_t *data);

/* Read multiple bytes */
uint16_t Bluetooth_ReadBuffer(uint8_t *buffer, uint16_t size);

/* Check and clear UART errors */
uint8_t Bluetooth_HasError(void);
void Bluetooth_ClearError(void);

#endif
