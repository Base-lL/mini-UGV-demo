#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "stm32f1xx_hal.h"

/* Initialize ultrasonic sensor */
void Ultrasonic_Init(void);

/* Start a new measurement */
void Ultrasonic_StartMeasurement(void);

/* Get measured distance */
float Ultrasonic_GetDistance(void);

/* Check if measurement is ready */
uint8_t Ultrasonic_IsDataReady(void);

/* Handle measurement timeout */
void Ultrasonic_Process(void);

#endif
