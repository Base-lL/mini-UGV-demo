#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f1xx_hal.h"

/* Initialize the MPU6050 sensor */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);

/* Read sensor data and update calculations */
HAL_StatusTypeDef MPU6050_Update(void);

/* Get acceleration in g */
float MPU6050_GetAccelX(void);
float MPU6050_GetAccelY(void);
float MPU6050_GetAccelZ(void);

/* Get angular velocity in degrees per second */
float MPU6050_GetGyroX(void);
float MPU6050_GetGyroY(void);
float MPU6050_GetGyroZ(void);

/* Get estimated angles in degrees */
float MPU6050_GetPitch(void);
float MPU6050_GetRoll(void);
float MPU6050_GetYaw(void);

#endif
