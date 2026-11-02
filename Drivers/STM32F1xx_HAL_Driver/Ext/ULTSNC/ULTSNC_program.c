#include "main.h"

#include "FreeRTOS.h"
#include "task.h"

/* Timer settings */
#define TRIGGER_PULSE_US 10U
#define ECHO_TIMEOUT_MS  40U

/* Timer handle */
extern TIM_HandleTypeDef htim2;

/* Measurement states */
#define ULTSNC_IDLE          0U
#define ULTSNC_WAIT_RISING   1U
#define ULTSNC_WAIT_FALLING  2U
#define ULTSNC_COMPLETE      3U
#define ULTSNC_TIMEOUT       4U

/* Measurement variables */
static volatile uint8_t capture_state = ULTSNC_IDLE;

static volatile uint32_t echo_start = 0U;
static volatile uint32_t echo_duration = 0U;

static volatile uint8_t measurement_ready = 0U;
static volatile uint8_t measurement_error = 0U;

static uint32_t measurement_start_ms = 0U;


/* Read distance in centimeters */
HAL_StatusTypeDef ULTSNC_readCM(uint16_t *distance)
{
    uint32_t start_time;
    uint32_t timer_max;
    uint32_t captured_duration;
    uint8_t current_state;

    if (distance == NULL)
    {
        return HAL_ERROR;
    }

    taskENTER_CRITICAL();
    current_state = capture_state;
    taskEXIT_CRITICAL();

    /* Check if a measurement is already running */
    if (current_state == ULTSNC_WAIT_RISING ||
        current_state == ULTSNC_WAIT_FALLING)
    {
        if ((HAL_GetTick() - measurement_start_ms) >= ECHO_TIMEOUT_MS)
        {
            taskENTER_CRITICAL();

            __HAL_TIM_SET_CAPTUREPOLARITY(
                &htim2,
                TIM_CHANNEL_1,
                TIM_INPUTCHANNELPOLARITY_RISING
            );

            capture_state = ULTSNC_TIMEOUT;
            measurement_ready = 0U;
            measurement_error = 1U;

            taskEXIT_CRITICAL();

            return HAL_TIMEOUT;
        }

        return HAL_BUSY;
    }

    /* Return the completed measurement */
    if (current_state == ULTSNC_COMPLETE)
    {
        taskENTER_CRITICAL();

        captured_duration = echo_duration;

        capture_state = ULTSNC_IDLE;
        measurement_ready = 0U;
        measurement_error = 0U;

        taskEXIT_CRITICAL();

        *distance = (uint16_t)(captured_duration / 58U);

        return HAL_OK;
    }

    /* Handle timeout */
    if (current_state == ULTSNC_TIMEOUT)
    {
        taskENTER_CRITICAL();

        capture_state = ULTSNC_IDLE;
        measurement_error = 0U;

        taskEXIT_CRITICAL();

        return HAL_TIMEOUT;
    }

    /* Start a new measurement */
    taskENTER_CRITICAL();

    measurement_ready = 0U;
    measurement_error = 0U;

    __HAL_TIM_SET_CAPTUREPOLARITY(
        &htim2,
        TIM_CHANNEL_1,
        TIM_INPUTCHANNELPOLARITY_RISING
    );

    __HAL_TIM_SET_COUNTER(&htim2, 0U);

    measurement_start_ms = HAL_GetTick();
    capture_state = ULTSNC_WAIT_RISING;

    taskEXIT_CRITICAL();

    /* Send trigger pulse */
    HAL_GPIO_WritePin(
        ULTSNC_TRIG_GPIO_Port,
        ULTSNC_TRIG_Pin,
        GPIO_PIN_SET
    );

    start_time = __HAL_TIM_GET_COUNTER(&htim2);
    timer_max = __HAL_TIM_GET_AUTORELOAD(&htim2);

    while (((__HAL_TIM_GET_COUNTER(&htim2) + timer_max + 1U
             - start_time) % (timer_max + 1U)) < TRIGGER_PULSE_US)
    {
    }

    HAL_GPIO_WritePin(
        ULTSNC_TRIG_GPIO_Port,
        ULTSNC_TRIG_Pin,
        GPIO_PIN_RESET
    );

    return HAL_BUSY;
}


/* Timer input capture callback */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint32_t echo_end;
    uint32_t timer_max;

    if (htim->Instance != TIM2 ||
        htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
    {
        return;
    }

    /* Capture rising edge */
    if (capture_state == ULTSNC_WAIT_RISING)
    {
        echo_start = HAL_TIM_ReadCapturedValue(
            htim,
            TIM_CHANNEL_1
        );

        capture_state = ULTSNC_WAIT_FALLING;

        __HAL_TIM_SET_CAPTUREPOLARITY(
            &htim2,
            TIM_CHANNEL_1,
            TIM_INPUTCHANNELPOLARITY_FALLING
        );
    }

    /* Capture falling edge */
    else if (capture_state == ULTSNC_WAIT_FALLING)
    {
        echo_end = HAL_TIM_ReadCapturedValue(
            htim,
            TIM_CHANNEL_1
        );

        timer_max = __HAL_TIM_GET_AUTORELOAD(&htim2);

        if (echo_end >= echo_start)
        {
            echo_duration = echo_end - echo_start;
        }
        else
        {
            echo_duration = (timer_max - echo_start)
                          + echo_end + 1U;
        }

        capture_state = ULTSNC_COMPLETE;
        measurement_ready = 1U;

        __HAL_TIM_SET_CAPTUREPOLARITY(
            &htim2,
            TIM_CHANNEL_1,
            TIM_INPUTCHANNELPOLARITY_RISING
        );
    }
}
