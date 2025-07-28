/**
 * Este codigo possui funcoes para inicializar, ler dados do acelerometro,
 * giroscopio, temperatura e calcular os angulos de roll e pitch a partir
 * dos dados do acelerometro.
 */

#include "hardware/i2c.h" // Funcoes de controle do I2C
#include <math.h>         // Funcoes matematicas (atan2f, sqrtf) para calculo de angulos

#define I2C_PORT_MPU6050 i2c0 // Usa o I2C0
#define I2C_SDA_MPU 0         // Pino GPIO 0 para a linha de dados (SDA)
#define I2C_SCL_MPU 1         // Pino GPIO 1 para a linha de clock (SCL)

// Endereco I2C padrao do MPU6050.
static int addr = 0x68;

// Esta funcao primeiro envia um comando de reset e depois tira o sensor do modo de suspensao.
void mpu6050_reset()
{
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(I2C_PORT_MPU6050, addr, buf, 2, false);
    sleep_ms(100); 

    buf[1] = 0x00;
    i2c_write_blocking(I2C_PORT_MPU6050, addr, buf, 2, false);
    sleep_ms(10);
}

// Le os dados do acelerometro, giroscopio e temperatura.
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp)
{
    uint8_t buffer[6];

    // Le aceleracao a partir do registrador 0x3B
    uint8_t val = 0x3B;
    i2c_write_blocking(I2C_PORT_MPU6050, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT_MPU6050, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        accel[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }

    // Le giroscopio a partir do registrador 0x43
    val = 0x43;
    i2c_write_blocking(I2C_PORT_MPU6050, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT_MPU6050, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        gyro[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }

    // Le temperatura a partir do registrador 0x41
    val = 0x41;
    i2c_write_blocking(I2C_PORT_MPU6050, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT_MPU6050, addr, buffer, 2, false);

    *temp = (buffer[0] << 8) | buffer[1];
}

// Calcula os angulos de roll e pitch em graus a partir dos dados do acelerometro
void mpu6050_compute_angles(int16_t accel[3], float *roll, float *pitch)
{
    // Converte os valores do acelerometro para 'g' (forca da gravidade).
    float ax = accel[0] / 16384.0f;
    float ay = accel[1] / 16384.0f;
    float az = accel[2] / 16384.0f;

    // Calcula o angulo de roll (rotacao em torno do eixo X) usando a tangente do arco.
    *roll = atan2f(ay, az) * 180.0f / M_PI;

    // Calcula o angulo de pitch (rotacao em torno do eixo Y).
    *pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / M_PI;
}