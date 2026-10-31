#include "mpu6050.h"
#include <math.h>

/* MPU6050 registers */
#define MPU6050_ADDR          (0x68 << 1)
#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_WHO_AM_I    0x75

#define MPU6050_TIMEOUT_MS    100U

/* Conversion factors for the selected sensor ranges */
#define ACCEL_SCALE           16384.0f
#define GYRO_SCALE            131.0f

/* Complementary filter settings */
#define FILTER_ALPHA          0.98f
#define RAD_TO_DEG            57.2957795f

/* I2C handle */
static I2C_HandleTypeDef *mpu_i2c;

/* Sensor readings */
static float accel_x = 0.0f;
static float accel_y = 0.0f;
static float accel_z = 0.0f;

static float gyro_x = 0.0f;
static float gyro_y = 0.0f;
static float gyro_z = 0.0f;

/* Estimated angles */
static float pitch = 0.0f;
static float roll = 0.0f;
static float yaw = 0.0f;

/* Gyroscope offsets */
static float gyro_offset_x = 0.0f;
static float gyro_offset_y = 0.0f;
static float gyro_offset_z = 0.0f;

/* Previous update time */
static uint32_t last_update_ms = 0U;

/* Read one register */
static HAL_StatusTypeDef MPU6050_ReadReg(uint8_t reg, uint8_t *data)
{
return HAL_I2C_Mem_Read(
mpu_i2c,
MPU6050_ADDR,
reg,
I2C_MEMADD_SIZE_8BIT,
data,
1U,
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
1U,
MPU6050_TIMEOUT_MS
);
}

/* Initialize the sensor */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
uint8_t who_am_i;
HAL_StatusTypeDef status;


if (hi2c == NULL)
    return HAL_ERROR;

mpu_i2c = hi2c;

/* Check the sensor identity */
status = MPU6050_ReadReg(MPU6050_REG_WHO_AM_I, &who_am_i);

if (status != HAL_OK)
    return status;

if (who_am_i != 0x68U)
    return HAL_ERROR;

/* Wake up the sensor */
status = MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x00U);

if (status != HAL_OK)
    return status;

HAL_Delay(100U);

/* Configure sample rate */
status = MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, 0x07U);

if (status != HAL_OK)
    return status;

/* Configure the digital low-pass filter */
status = MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x03U);

if (status != HAL_OK)
    return status;

/* Set gyroscope range to +/- 250 degrees/s */
status = MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00U);

if (status != HAL_OK)
    return status;

/* Set accelerometer range to +/- 2g */
status = MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00U);

if (status != HAL_OK)
    return status;

/* Reset stored values */
accel_x = 0.0f;
accel_y = 0.0f;
accel_z = 0.0f;

gyro_x = 0.0f;
gyro_y = 0.0f;
gyro_z = 0.0f;

pitch = 0.0f;
roll = 0.0f;
yaw = 0.0f;

last_update_ms = HAL_GetTick();

return HAL_OK;


}

/* Read accelerometer and gyroscope data */
HAL_StatusTypeDef MPU6050_Update(void)
{
uint8_t data[14];
int16_t raw_ax, raw_ay, raw_az;
int16_t raw_gx, raw_gy, raw_gz;
float accel_pitch, accel_roll;
float dt;
uint32_t current_time;
HAL_StatusTypeDef status;


if (mpu_i2c == NULL)
    return HAL_ERROR;

status = HAL_I2C_Mem_Read(
    mpu_i2c,
    MPU6050_ADDR,
    MPU6050_REG_ACCEL_XOUT_H,
    I2C_MEMADD_SIZE_8BIT,
    data,
    sizeof(data),
    MPU6050_TIMEOUT_MS
);

if (status != HAL_OK)
    return status;

/* Convert raw bytes to signed values */
raw_ax = (int16_t)((data[0] << 8) | data[1]);
raw_ay = (int16_t)((data[2] << 8) | data[3]);
raw_az = (int16_t)((data[4] << 8) | data[5]);

raw_gx = (int16_t)((data[8] << 8) | data[9]);
raw_gy = (int16_t)((data[10] << 8) | data[11]);
raw_gz = (int16_t)((data[12] << 8) | data[13]);

/* Convert acceleration to g */
accel_x = (float)raw_ax / ACCEL_SCALE;
accel_y = (float)raw_ay / ACCEL_SCALE;
accel_z = (float)raw_az / ACCEL_SCALE;

/* Convert gyroscope readings to degrees per second */
gyro_x = ((float)raw_gx / GYRO_SCALE) - gyro_offset_x;
gyro_y = ((float)raw_gy / GYRO_SCALE) - gyro_offset_y;
gyro_z = ((float)raw_gz / GYRO_SCALE) - gyro_offset_z;

/* Calculate elapsed time */
current_time = HAL_GetTick();
dt = (float)(current_time - last_update_ms) / 1000.0f;
last_update_ms = current_time;

/* Calculate angles from acceleration */
accel_pitch = atan2f(
    -accel_x,
    sqrtf(accel_y * accel_y + accel_z * accel_z)
) * RAD_TO_DEG;

accel_roll = atan2f(accel_y, accel_z) * RAD_TO_DEG;

/* Combine accelerometer and gyroscope data */
if (dt > 0.0f && dt < 0.1f)
{
    pitch = FILTER_ALPHA * (pitch + gyro_y * dt)
          + (1.0f - FILTER_ALPHA) * accel_pitch;

    roll = FILTER_ALPHA * (roll + gyro_x * dt)
         + (1.0f - FILTER_ALPHA) * accel_roll;

    /* Yaw is estimated from the gyroscope only */
    yaw += gyro_z * dt;
}

return HAL_OK;


}

/* Get acceleration */
float MPU6050_GetAccelX(void) { return accel_x; }
float MPU6050_GetAccelY(void) { return accel_y; }
float MPU6050_GetAccelZ(void) { return accel_z; }

/* Get angular velocity */
float MPU6050_GetGyroX(void) { return gyro_x; }
float MPU6050_GetGyroY(void) { return gyro_y; }
float MPU6050_GetGyroZ(void) { return gyro_z; }

/* Get estimated angles */
float MPU6050_GetPitch(void) { return pitch; }
float MPU6050_GetRoll(void)  { return roll; }
float MPU6050_GetYaw(void)   { return yaw; }
