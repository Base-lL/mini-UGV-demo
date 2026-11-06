#include "APP_interface.h"
#include "main.h"
#include "cmsis_os.h"
#include "../Ext/DCMTR/DCMTR_interface.h"
#include "../Ext/MPU/MPU_interface.h"
#include "../Ext/ULTSNC/ULTSNC_interface.h"
#include "../Ext/bluetooth/bluetooth_interface.h"


/* ========== CONFIGURATION MACROS ========== */
/* Emergency stop trigger distance */
#define OBSTACLE_THRESHOLD_CM       (15U)

#define BT_QUEUE_SIZE               (10U)
#define BT_QUEUE_ITEM_SIZE          sizeof(uint8_t)

/* task delays in ms */
#define ULTRASONIC_TASK_PERIOD_MS   (50U)
#define BLUETOOTH_TASK_PERIOD_MS    (20U)
#define VEHICLE_TASK_PERIOD_MS      (20U)
#define MPU_TASK_PERIOD_MS          (50U)

/* bluetooth app input bytes */
#define BLUETOOTH_INPUT_FORWARD		'f'
#define BLUETOOTH_INPUT_BRAKE		's'
#define BLUETOOTH_INPUT_BACKWARD	'b'

/* ========== SHARED VARIABLES & QUEUE HANDLES ========== */
static volatile uint8_t eStopActive = 0;
//1 = Emergency Stop Engaged, 0 = Normal


/* ========== TASKS CONFIGURATION ========= */
/* --- 1. Ultrasonic Emergency Task --- */
osThreadId_t ultrasonicTaskHandle;
const osThreadAttr_t ultrasonicTaskAttr = {
    .name       = "Ultrasonic",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t) osPriorityRealtime1,
};
void vTaskUltrasonic(void *argument);

/* --- Bluetooth task --- */
osThreadId_t bluetoothTaskHandle;
const osThreadAttr_t bluetoothTaskAttr = {
    .name       = "Bluetooth",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t) osPriorityAboveNormal1,
};
void vTaskBluetooth(void *argument);

/* --- Vehicle control --- */
osThreadId_t vehicleControlTaskHandle;
const osThreadAttr_t vehicleControlTaskAttr = {
    .name       = "VehicleControl",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t) osPriorityNormal1,
};
void vTaskVehicleControl(void *argument);

/* --- MPU6050 sensor --- */
osThreadId_t mpuSensorTaskHandle;
const osThreadAttr_t mpuSensorTaskAttr = {
    .name       = "MPUSensor",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t) osPriorityBelowNormal1,
};
void vTaskMPUSensor(void *argument);

/*========== QUEUES ==========*/
/* Definitions for BluetoothQueue */
osMessageQueueId_t BluetoothQueueHandle;
const osMessageQueueAttr_t BluetoothQueue_attributes = {
  .name = "BluetoothQueue"
};

/*========== VARIABLES ==========*/
static volatile uint8_t eStopActive = 0;

void APP_init(void)
{
    MOTOR_INIT();

    BluetoothQueueHandle = osMessageQueueNew(BT_QUEUE_SIZE, BT_QUEUE_ITEM_SIZE, &BluetoothQueue_attr);

    ultrasonicTaskHandle     = osThreadNew(vTaskUltrasonic,     NULL, &ultrasonicTaskAttr);
    bluetoothTaskHandle      = osThreadNew(vTaskBluetooth,      NULL, &bluetoothTaskAttr);
    vehicleControlTaskHandle = osThreadNew(vTaskVehicleControl, NULL, &vehicleControlTaskAttr);
    mpuSensorTaskHandle      = osThreadNew(vTaskMPUSensor,      NULL, &mpuSensorTaskAttr);
}

void APP_loop(void)
{
    osDelay(20);
}


void vTaskUltrasonic(void *argument)
{
    uint16_t distance = 0;

    for (;;)
    {
        ULTSNC_readCM(&distance);

        if (distance > 0 && distance < OBSTACLE_THRESHOLD_CM)
        {
            eStopActive = 1;
            STOP();
        }
        else
        {
            eStopActive = 0;
        }

        osDelay(ULTRASONIC_TASK_PERIOD_MS);
    }
}

void vTaskBluetooth(void *argument)
{
	uint8_t receivedValue = 0;

	    for (;;)
	    {
	        if (Bluetooth_Available())
	        {
	            Bluetooth_Read(&receivedValue);

	            if (receivedValue == '\r' || receivedValue == '\n')
	            {
	                continue;
	            }
	            if (eStopActive && (receivedValue == 'F'))
	            {
	                STOP();
	                continue;
	            }

	            /* Execute Command */
	            if (receivedValue == BLUETOOTH_INPUT_FORWARD)
	            {
	                FORWARD();
	            }
	            else if (receivedValue == BLUETOOTH_INPUT_DOWNWARD)
	            {
	                BACKWARD();
	            }
	            else if (receivedValue == 'S')
	            {
	                STOP();
	            }
	            else if (receivedValue <= 180)
	            {
	                MOTOR_TURN_TO_ANGLE((float)receivedValue);
	            }
	        }

	        osDelay(BLUETOOTH_TASK_PERIOD_MS);
	    }
}


void vTaskMPUSensor(void *argument)
{
    /* Infinite loop */

    for (;;)
    {
        osDelay(MPU_TASK_PERIOD_MS);
    }
}
