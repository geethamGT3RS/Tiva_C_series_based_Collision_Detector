#include <stdint.h>
#include "tm4c123gh6pm.h"

#define MPU6050_ADDR 0x68
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43
#define THRESHOLD 10000
#define GRAVITY 16384
#define COLLISION_THRESHOLD 12

void I2C_Init(void)
{
    SYSCTL_RCGCI2C_R |= 0x01;  // Enable clock to I2C0
    SYSCTL_RCGCGPIO_R |= 0x02; // Enable clock to Port B

    GPIO_PORTB_AFSEL_R |= 0x0C;                                        // Enable alternate function on PB2 and PB3
    GPIO_PORTB_ODR_R |= 0x08;                                          // Enable open drain on PB3 (SDA)
    GPIO_PORTB_DEN_R |= 0x0C;                                          // Enable digital I/O on PB2 and PB3
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFF00FF) | 0x00003300; // Assign I2C signals

    I2C0_MCR_R = 0x10;  // Initialize I2C master
    I2C0_MTPR_R = 0x09; // Set clock speed
}

void UART0_Init(void)
{
    SYSCTL_RCGCUART_R |= 0x01; // Enable UART0
    SYSCTL_RCGCGPIO_R |= 0x01; // Enable PORTA clock
    UART0_CTL_R = 0;           // Disable UART0
    UART0_IBRD_R = 104;        // Set baud rate (9600 for 16 MHz)
    UART0_FBRD_R = 11;
    UART0_LCRH_R = 0x60; // 8-bit, no parity
    UART0_CTL_R = 0x301; // Enable UART0, TX, and RX

    GPIO_PORTA_AFSEL_R |= 0x03; // Enable alternate function on PA0 and PA1
    GPIO_PORTA_DEN_R |= 0x03;   // Enable digital on PA0 and PA1
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) | 0x00000011;
}

void PortF_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x20; // Enable clock for Port F
    GPIO_PORTF_DIR_R |= 0x0E;  // Set PF1, PF2, PF3 as output
    GPIO_PORTF_DEN_R |= 0x0E;  // Enable digital functionality on PF1, PF2, PF3
}

void UART0_WriteChar(char c)
{
    while ((UART0_FR_R & 0x20) != 0)
        ;
    UART0_DR_R = c;
}

void UART0_WriteString(char *str)
{
    while (*str)
    {
        UART0_WriteChar(*(str++));
    }
}

void UART0_WriteInt(int num)
{
    if (num == 0)
    {
        UART0_WriteChar('0');
        return;
    }
    char buffer[10];
    int index = 0;
    if (num < 0)
    {
        UART0_WriteChar('-');
        num = -num;
    }
    while (num > 0)
    {
        buffer[index++] = '0' + (num % 10);
        num /= 10;
    }
    while (--index >= 0)
    {
        UART0_WriteChar(buffer[index]);
    }
}

void MPU6050_Init(void)
{
    I2C0_MSA_R = (MPU6050_ADDR << 1);
    I2C0_MDR_R = 0x6B;
    I2C0_MCS_R = 0x03;
    while (I2C0_MCS_R & 0x01)
        ;

    if (I2C0_MCS_R & 0x02)
    {
        I2C0_MCS_R = 0x04;
        return;
    }
    I2C0_MDR_R = 0x00;
    I2C0_MCS_R = 0x01;
    while (I2C0_MCS_R & 0x01)
        ;
    if (I2C0_MCS_R & 0x02)
    {
        I2C0_MCS_R = 0x04;
        return;
    }
}

int16_t MPU6050_ReadAxis(uint8_t reg)
{
    int16_t axisData;
    I2C0_MSA_R = (MPU6050_ADDR << 1); // Set address for write
    I2C0_MDR_R = reg;                 // Register address
    I2C0_MCS_R = 0x03;                // START and RUN
    while (I2C0_MCS_R & 0x01)
        ;
    I2C0_MSA_R = (MPU6050_ADDR << 1) | 1; // Set address for read
    I2C0_MCS_R = 0x0B;                    // START, RUN, and ACK
    while (I2C0_MCS_R & 0x01)
        ;
    axisData = I2C0_MDR_R << 8; // High byte
    I2C0_MCS_R = 0x05;          // RUN and STOP
    while (I2C0_MCS_R & 0x01)
        ;
    axisData |= I2C0_MDR_R; // Low byte
    return axisData;
}

void Delay(int time)
{
    volatile int i;
    for (i = 0; i < time; i++)
        ;
}

int main(void)
{
    I2C_Init();
    UART0_Init();
    MPU6050_Init();
    PortF_Init();
    int16_t accelX, accelY, accelZ;
    int16_t gyroX, gyroY, gyroZ;

    while (1)
    {
        // Read Accelerometer Data
        accelX = MPU6050_ReadAxis(ACCEL_XOUT_H);
        accelY = MPU6050_ReadAxis(ACCEL_XOUT_H + 2);
        accelZ = MPU6050_ReadAxis(ACCEL_XOUT_H + 4);

        // Read Gyroscope Data
        gyroX = MPU6050_ReadAxis(GYRO_XOUT_H);
        gyroY = MPU6050_ReadAxis(GYRO_XOUT_H + 2);
        gyroZ = MPU6050_ReadAxis(GYRO_XOUT_H + 4);

        // Send Accelerometer Data
        UART0_WriteString("X: ");
        UART0_WriteInt(accelX);
        UART0_WriteString(" ");
        UART0_WriteString("Y: ");
        UART0_WriteInt(accelY);
        UART0_WriteString(" ");
        UART0_WriteString("Z: ");
        UART0_WriteInt(accelZ);
        UART0_WriteString(" ");

        // Send Gyroscope Data
        UART0_WriteString("PSSI: ");
        UART0_WriteInt(gyroX);
        UART0_WriteString(" ");
        UART0_WriteString("PHI: ");
        UART0_WriteInt(gyroY);
        UART0_WriteString(" ");
        UART0_WriteString("RHO: ");
        UART0_WriteInt(gyroZ);
        UART0_WriteString("\n");

        if (abs(accelX) > GRAVITY * 0.8 && abs(accelX) < GRAVITY * 1.1 &&
            abs(accelY) < GRAVITY * 0.8 && abs(accelZ) < GRAVITY * 0.8) {
            GPIO_PORTF_DATA_R = 0x08; // Green LED (PF3) for X-axis
        } else if (abs(accelY) > GRAVITY * 0.8 && abs(accelY) < GRAVITY * 1.1 &&
                   abs(accelX) < GRAVITY * 0.8 && abs(accelZ) < GRAVITY * 0.8) {
            GPIO_PORTF_DATA_R = 0x02; // Red LED (PF1) for Y-axis
        } else if (abs(accelZ) > GRAVITY * 0.8 && abs(accelZ) < GRAVITY * 1.1 &&
                   abs(accelX) < GRAVITY * 0.8 && abs(accelY) < GRAVITY * 0.8) {
            GPIO_PORTF_DATA_R = 0x04; // Blue LED (PF2) for Z-axis
        } else {
            GPIO_PORTF_DATA_R = 0x00; // Turn off all LEDs
        }
        if (abs(accelX) > GRAVITY * 0.15 * COLLISION_THRESHOLD || abs(accelY) > GRAVITY *  0.15 * COLLISION_THRESHOLD || abs(accelZ) > GRAVITY *  0.15 * COLLISION_THRESHOLD) {
            int i;
            for (i = 0; i < 5; i++) {
                GPIO_PORTF_DATA_R = 0x06; // Turn on Red LED (PF1)
                Delay(500000);
                GPIO_PORTF_DATA_R = 0x00; // Turn off Red LED
                Delay(500000);
            }
        }
        Delay(10000);
    }
}
