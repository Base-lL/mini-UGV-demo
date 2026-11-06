#include "main.h"
#include <math.h>
#include "../MPU/MPU_interface.h"
#include "DCMTR_interface.h"

/* Motor Configuration */

#define MAX_SPEED           1000
#define FORWARD_SPEED       500

#define TURN_TIMEOUT        5000U
#define ANGLE_TOLERANCE     1.5f

#define TURN_SPEED_HIGH     500
#define TURN_SPEED_MEDIUM   350
#define TURN_SPEED_LOW      200

#define ANGLE_RIGHT 		90.0f

extern TIM_HandleTypeDef htim1;


/* Internal Variables*/

static float target_yaw = 0.0f;
static uint8_t is_turning = 0;
static uint32_t turn_start_time = 0;


/* Static Functions */

/* Limit speed */
static int16_t CLAMP_SPEED(int16_t speed)
{
    if (speed > MAX_SPEED)
        return MAX_SPEED;

    if (speed < -MAX_SPEED)
        return -MAX_SPEED;

    return speed;
}


/* Calculate angle error */
static float GET_ANGLE_ERROR(float target, float current)
{
    float error = target - current;

    while (error > 180.0f)
        error -= 360.0f;

    while (error < -180.0f)
        error += 360.0f;

    return error;
}


/* Set one motor */
static void SET_MOTOR(GPIO_TypeDef *port,
                      uint16_t pin,
                      uint32_t channel,
                      int16_t speed)
{
    uint32_t arr;
    uint32_t duty;
    uint32_t magnitude;

    speed = CLAMP_SPEED(speed);

    if (speed > 0)
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        magnitude = (uint32_t)speed;
    }
    else if (speed < 0)
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
        magnitude = (uint32_t)(-(int32_t)speed);
    }
    else
    {
        magnitude = 0U;
    }

    arr = __HAL_TIM_GET_AUTORELOAD(&htim1);

    duty = (arr * magnitude) / MAX_SPEED;

    __HAL_TIM_SET_COMPARE(&htim1, channel, duty);
}


/* Get turning speed */
static int16_t GET_TURN_SPEED(float error)
{
    float abs_error = fabsf(error);

    if (abs_error > 30.0f)
        return TURN_SPEED_HIGH;

    if (abs_error > 10.0f)
        return TURN_SPEED_MEDIUM;

    return TURN_SPEED_LOW;
}


/*Motor Initialization */

HAL_StatusTypeDef MOTOR_INIT(void)
{
    HAL_StatusTypeDef status;

    status = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    if (status != HAL_OK)
        return status;
    status = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

    if (status != HAL_OK)
    {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        return status;
    }

    SET_SPEED(0, 0);

    is_turning = 0;
    target_yaw = 0.0f;

    return HAL_OK;
}


/*Motor Speed Control*/

void SET_SPEED(int16_t left_speed, int16_t right_speed)
{
    SET_MOTOR(MTR_DIR_LEFT_GPIO_Port,
              MTR_DIR_LEFT_Pin,
              TIM_CHANNEL_1,
              left_speed);

    SET_MOTOR(MTR_DIR_RIGHT_GPIO_Port,
              MTR_DIR_RIGHT_Pin,
              TIM_CHANNEL_2,
              right_speed);
}


/*Basic Movement */

/* Move forward */
void FORWARD(void)
{
    is_turning = 0;
    SET_SPEED(FORWARD_SPEED, FORWARD_SPEED);
}


/* Move backward */
void BACKWARD(void)
{
    is_turning = 0;
    SET_SPEED(-FORWARD_SPEED, -FORWARD_SPEED);
}


/* Stop motors */
void STOP(void)
{
    is_turning = 0;
    SET_SPEED(0, 0);
}
//MOTOR_UPDATE_HEADING_LOGIC

/*Turning Control*/

/* Start turn to target angle */
void MOTOR_TURN_TO_ANGLE(float angle)
{
    if (is_turning == 0)
    {
        target_yaw = MPU6050_GetYaw() + angle;
        turn_start_time = HAL_GetTick();
        is_turning = 1;
    }
}


/* Turn right */
void TURN_RIGHT(void)
{
    MOTOR_TURN_TO_ANGLE(90.0f);
}


/* Turn left */
void TURN_LEFT(void)
{
    MOTOR_TURN_TO_ANGLE(-90.0f);
}


/*Heading Control */

void MOTOR_UPDATE_HEADING_LOGIC(void)
{
    float current_yaw;
    float angle_error;
    int16_t speed;

    if (is_turning == 0)
        return;

    /* Check timeout */
    if ((HAL_GetTick() - turn_start_time) >= TURN_TIMEOUT)
    {
        STOP();
        return;
    }

    current_yaw = MPU6050_GetYaw();

    angle_error = GET_ANGLE_ERROR(target_yaw, current_yaw);

    /* Check target angle */
    if (fabsf(angle_error) <= ANGLE_TOLERANCE)
    {
        STOP();
        return;
    }

    speed = GET_TURN_SPEED(angle_error);

    /* Turn right */
    if (angle_error > 0)
    {
        SET_SPEED(speed, -speed);
    }
    else
    {
        /* Turn left */
        SET_SPEED(-speed, speed);
    }
}


/* Motor Status*/

uint8_t MOTOR_IS_TURNING(void)
{
    return is_turning;
}
