#ifndef DCMTR_INTERFACE_H
#define DCMTR_INTERFACE_H

#include "main.h"

HAL_StatusTypeDef MOTOR_INIT(void);

void SET_SPEED(int16_t left_speed, int16_t right_speed);

void FORWARD(void);
void BACKWARD(void);
void STOP(void);

void MOTOR_TURN_TO_ANGLE(float angle);

void TURN_RIGHT(void);
void TURN_LEFT(void);

void MOTOR_UPDATE_HEADING_LOGIC(void);


uint8_t MOTOR_IS_TURNING(void);

#endif
