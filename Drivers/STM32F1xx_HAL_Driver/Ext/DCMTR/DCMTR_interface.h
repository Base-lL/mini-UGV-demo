#ifndef DC_MOTOR_H
#define DC_MOTOR_H

#include "stm32f1xx_hal.h"

/* Initialize DC motor driver */
HAL_StatusTypeDef DC_Motor_Init(void);

/* Set left and right motor speeds */
void DC_Motor_SetSpeed(int16_t left_speed, int16_t right_speed);

/* Movement functions */
void DC_Motor_Forward(void);
void DC_Motor_Backward(void);
void DC_Motor_Left(void);
void DC_Motor_Right(void);
void DC_Motor_Stop(void);

#endif
