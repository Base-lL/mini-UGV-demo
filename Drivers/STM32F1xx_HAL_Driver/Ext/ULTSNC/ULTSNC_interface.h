#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "main.h"

/* Read distance in centimeters */
HAL_StatusTypeDef ULTSNC_readCM(uint16_t *distance);

#endif
