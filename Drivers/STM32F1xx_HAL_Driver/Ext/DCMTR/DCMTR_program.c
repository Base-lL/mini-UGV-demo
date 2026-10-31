
#include "dc_motor.h"

/* Motor direction pins */
#define LEFT_DIR_PORT   GPIOA
#define LEFT_DIR_PIN    GPIO_PIN_11

#define RIGHT_DIR_PORT  GPIOA
#define RIGHT_DIR_PIN   GPIO_PIN_10

/* Speed values */
#define MAX_SPEED       1000
#define FORWARD_SPEED   500
#define TURN_SPEED      300



extern TIM_HandleTypeDef htim1;


/* Check speed limits */
static int16_t clamp(int16_t speed)
{
    if (speed > MAX_SPEED)
        return MAX_SPEED;

    if (speed < -MAX_SPEED)
        return -MAX_SPEED;

    return speed;
}


/* Control one motor */
static void set_motor(GPIO_TypeDef *port, uint16_t pin,
                      uint32_t channel, int16_t speed)
{
    uint32_t arr;
    uint32_t duty;

    /* Set direction */
    if (speed >= 0)
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
        speed = -speed;
    }

    /* Calculate PWM */
    arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    duty = ((uint32_t)speed * arr) / MAX_SPEED;

    __HAL_TIM_SET_COMPARE(&htim1, channel, duty);
}


/* Initialize motors */
HAL_StatusTypeDef DC_Motor_Init(void)
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

    DC_Motor_Stop();

    return HAL_OK;
}


/* Set speed for both motors */
void DC_Motor_SetSpeed(int16_t left_speed, int16_t right_speed)
{
    left_speed = clamp(left_speed);
    right_speed = clamp(right_speed);

    set_motor(LEFT_DIR_PORT, LEFT_DIR_PIN,
              TIM_CHANNEL_1, left_speed);

    set_motor(RIGHT_DIR_PORT, RIGHT_DIR_PIN,
              TIM_CHANNEL_2, right_speed);
}


/* Move forward */
void DC_Motor_Forward(void)
{
    DC_Motor_SetSpeed(FORWARD_SPEED, FORWARD_SPEED);
}


/* Move backward */
void DC_Motor_Backward(void)
{
    DC_Motor_SetSpeed(-FORWARD_SPEED, -FORWARD_SPEED);
}


/* Turn left */
void DC_Motor_Left(void)
{
    DC_Motor_SetSpeed(-TURN_SPEED, TURN_SPEED);
}


/* Turn right */
void DC_Motor_Right(void)
{
    DC_Motor_SetSpeed(TURN_SPEED, -TURN_SPEED);
}


/* Stop motors */
void DC_Motor_Stop(void)
{
    DC_Motor_SetSpeed(0, 0);
}

