#include <stdint.h>
#include "tm4c123gh6pm.h"
#include <stdlib.h>
#include <stdio.h>

#define MPU6050_ADDR 0x68     // MPU6050 I2C address
#define ACCEL_XOUT_H 0x3B     // Accelerometer data register

void I2C_Init(void) {
    SYSCTL_RCGCI2C_R |= 0x01;           // Enable clock to I2C0
    SYSCTL_RCGCGPIO_R |= 0x02;          // Enable clock to Port B

    GPIO_PORTB_AFSEL_R |= 0x0C;         // Enable alternate function on PB2 and PB3
    GPIO_PORTB_ODR_R |= 0x08;           // Enable open drain on PB3 (SDA)
    GPIO_PORTB_DEN_R |= 0x0C;           // Enable digital I/O on PB2 and PB3
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFF00FF) | 0x00003300; // Assign I2C signals

    I2C0_MCR_R = 0x10;                  // Initialize I2C master
    I2C0_MTPR_R = 0x09;                 // Set clock speed
}

void UART0_Init(void) {
    SYSCTL_RCGCUART_R |= 0x01;          // Enable UART0
    SYSCTL_RCGCGPIO_R |= 0x01;          // Enable PORTA clock
    UART0_CTL_R = 0;                    // Disable UART0
    UART0_IBRD_R = 104;                 // Set baud rate (9600 for 16 MHz)
    UART0_FBRD_R = 11;
    UART0_LCRH_R = 0x60;                // 8-bit, no parity
    UART0_CTL_R = 0x301;                // Enable UART0, TX, and RX

    GPIO_PORTA_AFSEL_R |= 0x03;         // Enable alternate function on PA0 and PA1
    GPIO_PORTA_DEN_R |= 0x03;           // Enable digital on PA0 and PA1
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) | 0x00000011;
}

void UART0_WriteChar(char c) {
    while ((UART0_FR_R & 0x20) != 0);   // Wait for TX buffer to be empty
    UART0_DR_R = c;
}

void UART0_WriteString(char* str) {
    while (*str) {
        UART0_WriteChar(*(str++));
    }
}

void MPU6050_Init(void) {
    I2C0_MSA_R = (MPU6050_ADDR << 1);   // Set slave address and indicate write
    I2C0_MDR_R = 0x6B;                  // Register address to power on the MPU6050
    I2C0_MCS_R = 0x03;                  // START and RUN
    while (I2C0_MCS_R & 0x01);          // Wait until done
    I2C0_MDR_R = 0x00;                  // Set power management register to 0
    I2C0_MCS_R = 0x05;                  // RUN and STOP
    while (I2C0_MCS_R & 0x01);          // Wait until done
}

int16_t MPU6050_ReadAxis(uint8_t reg) {
    int16_t axisData;

    // Set register to read from
    I2C0_MSA_R = (MPU6050_ADDR << 1);   // Set address for write
    I2C0_MDR_R = reg;                   // Register address
    I2C0_MCS_R = 0x03;                  // START and RUN
    while (I2C0_MCS_R & 0x01);          // Wait until done

    // Read high byte
    I2C0_MSA_R = (MPU6050_ADDR << 1) | 1; // Set address for read
    I2C0_MCS_R = 0x0B;                  // START, RUN, and ACK
    while (I2C0_MCS_R & 0x01);          // Wait until done
    axisData = I2C0_MDR_R << 8;         // High byte

    // Read low byte
    I2C0_MCS_R = 0x05;                  // RUN and STOP
    while (I2C0_MCS_R & 0x01);          // Wait until done
    axisData |= I2C0_MDR_R;             // Low byte

    return axisData;
}

int main(void) {
    I2C_Init();
    UART0_Init();
    MPU6050_Init();

    int16_t accelX, accelY, accelZ;
    char buffer[32];

    while (1) {
        accelX = MPU6050_ReadAxis(ACCEL_XOUT_H);
        accelY = MPU6050_ReadAxis(ACCEL_XOUT_H + 2);
        accelZ = MPU6050_ReadAxis(ACCEL_XOUT_H + 4);
        sprintf(buffer, "X:%d Y:%d Z:%d\n", accelX, accelY, accelZ);
        UART0_WriteString(buffer);
    }
}
