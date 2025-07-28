#ifndef MPU6050_H
#define MPU6050_H

void mpu6050_reset();
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp);
void mpu6050_compute_angles(int16_t accel[3], float *roll, float *pitch);

#endif