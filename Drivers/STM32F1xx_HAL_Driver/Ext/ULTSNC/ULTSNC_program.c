#include "ultrasonic.h"

/* Trigger pin configuration */
#define TRIG_PORT GPIOB
#define TRIG_PIN  GPIO_PIN_0

/* Timer configuration */
#define TIMER_MAX_COUNT 65535U
#define TRIGGER_PULSE_US 10U
#define ECHO_TIMEOUT_MS 40U

/* Timer handle */
extern TIM_HandleTypeDef htim2;

/* Measurement variables */
static volatile uint32_t echo_start = 0;        // Timer value at ECHO rising edge
static volatile uint32_t echo_end = 0;          // Timer value at ECHO falling edge
static volatile uint32_t echo_duration = 0;     // ECHO pulse duration in microseconds

static volatile float distance_cm = 0.0f;       // Measured distance in centimeters

static volatile uint8_t measurement_ready = 0;  // 1 = Data ready
static volatile uint8_t capture_state = 0;      // 0 = Idle, 1 = Wait rising, 2 = Wait falling

static uint32_t measurement_start_ms = 0;       // Measurement start time


/* Initialize ultrasonic sensor */
void Ultrasonic_Init(void)
{
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    /* Start Timer Input Capture interrupt */
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
}


/* Start a new measurement */
void Ultrasonic_StartMeasurement(void)
{
    if (capture_state != 0)
    {
        return;
    }

    measurement_ready = 0;
    capture_state = 1;

    /* Detect rising edge first */
    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,
                                  TIM_CHANNEL_1,
                                  TIM_INPUTCHANNELPOLARITY_RISING);

    measurement_start_ms = HAL_GetTick();

    /* Send trigger pulse */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);

    /* Wait for 10 microseconds */
    uint32_t start_time = __HAL_TIM_GET_COUNTER(&htim2);

    while ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) - start_time)
           < TRIGGER_PULSE_US)
    {
    }

    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
}


/* Timer Input Capture callback */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2 &&
        htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        if (capture_state == 1)
        {
            /* Save ECHO rising-edge time */
            echo_start = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

            capture_state = 2;

            /* Wait for falling edge */
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,
                                          TIM_CHANNEL_1,
                                          TIM_INPUTCHANNELPOLARITY_FALLING);
        }
        else if (capture_state == 2)
        {
            /* Save ECHO falling-edge time */
            echo_end = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

            /* Calculate pulse duration */
            if (echo_end >= echo_start)
            {
                echo_duration = echo_end - echo_start;
            }
            else
            {
                echo_duration = (TIMER_MAX_COUNT - echo_start)
                              + echo_end + 1U;
            }

            /* Convert pulse duration to distance */
            distance_cm = (float)echo_duration / 58.0f;

            measurement_ready = 1;
            capture_state = 0;

            /* Restore rising-edge detection */
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,
                                          TIM_CHANNEL_1,
                                          TIM_INPUTCHANNELPOLARITY_RISING);
        }
    }
}


/* Handle measurement timeout */
void Ultrasonic_Process(void)
{
    if (capture_state != 0)
    {
        if ((HAL_GetTick() - measurement_start_ms) >= ECHO_TIMEOUT_MS)
        {
            /* Reset measurement state */
            capture_state = 0;
            measurement_ready = 0;

            /* Restore rising-edge detection */
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,
                                          TIM_CHANNEL_1,
                                          TIM_INPUTCHANNELPOLARITY_RISING);
        }
    }
}


/* Get measured distance */
float Ultrasonic_GetDistance(void)
{
    return distance_cm;
}


/* Check if measurement is ready */
uint8_t Ultrasonic_IsDataReady(void)
{
    return measurement_ready;
}
