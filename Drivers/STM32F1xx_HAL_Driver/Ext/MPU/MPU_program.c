#include "main.h"
#include "MPU_interface.h"

#include "FreeRTOS.h"
#include "task.h"

#include <math.h>

#define MPU6050_ADDR          (0x68 << 1)

#define WHO_AM_I_REG          0x75
#define PWR_MGMT_1_REG        0x6B
#define SMPLRT_DIV_REG        0x19
#define CONFIG_REG            0x1A
#define GYRO_CONFIG_REG       0x1B
#define ACCEL_CONFIG_REG      0x1C
#define ACCEL_XOUT_H_REG      0x3B
#define GYRO_XOUT_H_REG       0x43

#define MPU6050_TIMEOUT_MS    100
#define ACCEL_SCALE           16384.0f
#define GYRO_SCALE            131.0f

#define FILTER_ALPHA          0.98f
#define RAD_TO_DEG            57.2957795f

#define CALIBRATION_SAMPLES   100
#define CALIBRATION_DELAY_MS  10

static I2C_HandleTypeDef *mpu_i2c = NULL;

static float accel_x = 0.0f;
static float accel_y = 0.0f;
static float accel_z = 0.0f;

static float gyro_x = 0.0f;
static float gyro_y = 0.0f;
static float gyro_z = 0.0f;

static float gyro_offset_x = 0.0f;
static float gyro_offset_y = 0.0f;
static float gyro_offset_z = 0.0f;

static float pitch = 0.0f;
static float roll  = 0.0f;
static float yaw   = 0.0f;

static uint32_t last_update_ms = 0;


/* Read one register */
static HAL_StatusTypeDef MPU6050_ReadReg(uint8_t reg, uint8_t *data)
{
    return HAL_I2C_Mem_Read(
        mpu_i2c,
        MPU6050_ADDR,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        1,
        MPU6050_TIMEOUT_MS
    );
}


/* Write one register */
static HAL_StatusTypeDef MPU6050_WriteReg(uint8_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(
        mpu_i2c,
        MPU6050_ADDR,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1,
        MPU6050_TIMEOUT_MS
    );
}


/* Calibrate gyroscope */
static HAL_StatusTypeDef MPU6050_Calibrate(void)
{
    uint8_t cal_data[6];

    int32_t sum_gx = 0;
    int32_t sum_gy = 0;
    int32_t sum_gz = 0;

    uint16_t successful_samples = 0;

    for (uint16_t i = 0; i < CALIBRATION_SAMPLES; i++)
    {
        HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
            mpu_i2c,
            MPU6050_ADDR,
            GYRO_XOUT_H_REG,
            I2C_MEMADD_SIZE_8BIT,
            cal_data,
            6,
            MPU6050_TIMEOUT_MS
        );

        if (status == HAL_OK)
        {
            int16_t raw_gx =
                (int16_t)((uint16_t)(cal_data[0] << 8) | cal_data[1]);

            int16_t raw_gy =
                (int16_t)((uint16_t)(cal_data[2] << 8) | cal_data[3]);

            int16_t raw_gz =
                (int16_t)((uint16_t)(cal_data[4] << 8) | cal_data[5]);

            sum_gx += raw_gx;
            sum_gy += raw_gy;
            sum_gz += raw_gz;

            successful_samples++;
        }

        vTaskDelay(pdMS_TO_TICKS(CALIBRATION_DELAY_MS));
    }

    if (successful_samples < (CALIBRATION_SAMPLES * 0.9f))
    {
        return HAL_ERROR;
    }

    gyro_offset_x =
        ((float)sum_gx / successful_samples) / GYRO_SCALE;

    gyro_offset_y =
        ((float)sum_gy / successful_samples) / GYRO_SCALE;

    gyro_offset_z =
        ((float)sum_gz / successful_samples) / GYRO_SCALE;

    return HAL_OK;
}


/* Initialize MPU6050 */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t check = 0;

    if (hi2c == NULL)
    {
        return HAL_ERROR;
    }

    mpu_i2c = hi2c;

    if (MPU6050_ReadReg(WHO_AM_I_REG, &check) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (check != 0x68)
    {
        return HAL_ERROR;
    }

    /* Wake up MPU6050 */
    if (MPU6050_WriteReg(PWR_MGMT_1_REG, 0x00) != HAL_OK)
    {
        return HAL_ERROR;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    /* Set sample rate to 100Hz */
    if (MPU6050_WriteReg(SMPLRT_DIV_REG, 0x07) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Configure digital low pass filter */
    if (MPU6050_WriteReg(CONFIG_REG, 0x03) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Set gyroscope range to +/-250 deg/s */
    if (MPU6050_WriteReg(GYRO_CONFIG_REG, 0x00) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Set accelerometer range to +/-2g */
    if (MPU6050_WriteReg(ACCEL_CONFIG_REG, 0x00) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Reset sensor values */
    accel_x = 0.0f;
    accel_y = 0.0f;
    accel_z = 0.0f;

    gyro_x = 0.0f;
    gyro_y = 0.0f;
    gyro_z = 0.0f;

    pitch = 0.0f;
    roll  = 0.0f;
    yaw   = 0.0f;

    /* Calibrate gyroscope */
    if (MPU6050_Calibrate() != HAL_OK)
    {
        return HAL_ERROR;
    }

    last_update_ms = HAL_GetTick();

    return HAL_OK;
}


/* Read sensor data and calculate angles */
HAL_StatusTypeDef MPU6050_Update(void)
{
    uint8_t data[14];

    int16_t raw_ax, raw_ay, raw_az;
    int16_t raw_gx, raw_gy, raw_gz;

    float dt;

    if (mpu_i2c == NULL)
    {
        return HAL_ERROR;
    }

    /* Read accelerometer, temperature and gyroscope data */
    if (HAL_I2C_Mem_Read(
            mpu_i2c,
            MPU6050_ADDR,
            ACCEL_XOUT_H_REG,
            I2C_MEMADD_SIZE_8BIT,
            data,
            14,
            MPU6050_TIMEOUT_MS
        ) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Convert raw accelerometer data */
    raw_ax = (int16_t)((uint16_t)(data[0] << 8) | data[1]);
    raw_ay = (int16_t)((uint16_t)(data[2] << 8) | data[3]);
    raw_az = (int16_t)((uint16_t)(data[4] << 8) | data[5]);

    /* Convert raw gyroscope data */
    raw_gx = (int16_t)((uint16_t)(data[8] << 8) | data[9]);
    raw_gy = (int16_t)((uint16_t)(data[10] << 8) | data[11]);
    raw_gz = (int16_t)((uint16_t)(data[12] << 8) | data[13]);

    /* Convert accelerometer values to g */
    accel_x = (float)raw_ax / ACCEL_SCALE;
    accel_y = (float)raw_ay / ACCEL_SCALE;
    accel_z = (float)raw_az / ACCEL_SCALE;

    /* Convert gyroscope values to deg/s and remove offsets */
    gyro_x = ((float)raw_gx / GYRO_SCALE) - gyro_offset_x;
    gyro_y = ((float)raw_gy / GYRO_SCALE) - gyro_offset_y;
    gyro_z = ((float)raw_gz / GYRO_SCALE) - gyro_offset_z;

    /* Calculate time difference */
    uint32_t current_time = HAL_GetTick();

    dt = (current_time - last_update_ms) / 1000.0f;

    last_update_ms = current_time;

    if (dt <= 0.0f)
    {
        return HAL_OK;
    }

    /* Calculate pitch and roll from accelerometer */
    float accel_pitch = atan2f(
        -accel_x,
        sqrtf((accel_y * accel_y) + (accel_z * accel_z))
    ) * RAD_TO_DEG;

    float accel_roll = atan2f(
        accel_y,
        accel_z
    ) * RAD_TO_DEG;

    /* Handle long update intervals */
    if (dt >= 0.1f)
    {
        pitch = accel_pitch;
        roll  = accel_roll;

        return HAL_OK;
    }

    /* Apply complementary filter to pitch */
    pitch = FILTER_ALPHA * (pitch + gyro_y * dt)
          + (1.0f - FILTER_ALPHA) * accel_pitch;

    /* Apply complementary filter to roll */
    roll = FILTER_ALPHA * (roll + gyro_x * dt)
         + (1.0f - FILTER_ALPHA) * accel_roll;

    /* Calculate yaw using gyroscope */
    yaw += gyro_z * dt;

    return HAL_OK;
}


/* Get accelerometer X */
float MPU6050_GetAccelX(void)
{
    return accel_x;
}

/* Get accelerometer Y */
float MPU6050_GetAccelY(void)
{
    return accel_y;
}

/* Get accelerometer Z */
float MPU6050_GetAccelZ(void)
{
    return accel_z;
}

/* Get gyroscope X */
float MPU6050_GetGyroX(void)
{
    return gyro_x;
}

/* Get gyroscope Y */
float MPU6050_GetGyroY(void)
{
    return gyro_y;
}

/* Get gyroscope Z */
float MPU6050_GetGyroZ(void)
{
    return gyro_z;
}

/* Get pitch angle */
float MPU6050_GetPitch(void)
{
    return pitch;
}

/* Get roll angle */
float MPU6050_GetRoll(void)
{
    return roll;
}

/* Get yaw angle */
float MPU6050_GetYaw(void)
{
    return yaw;
}
