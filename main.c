#include <stdint.h>
#include "tm4c123gh6pm.h"

// MPU6050 I2C address and accelerometer register
#define MPU6050_ADDR 0x68
#define ACCEL_XOUT_H 0x3B
#define THRESHOLD 10000

// Function to initialize I2C
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

// Function to initialize UART0
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

// Function to write a single character to UART0
void UART0_WriteChar(char c) {
    while ((UART0_FR_R & 0x20) != 0);   // Wait for TX buffer to be empty
    UART0_DR_R = c;
}

// Function to write a string to UART0
void UART0_WriteString(char* str) {
    while (*str) {
        UART0_WriteChar(*(str++));
    }
}

// Function to write an integer to UART as string (base 10)
void UART0_WriteInt(int num) {
    if (num == 0) {
        UART0_WriteChar('0');
        return;
    }
    char buffer[10];  // Buffer for the number
    int index = 0;

    // Handle negative numbers
    if (num < 0) {
        UART0_WriteChar('-');
        num = -num;
    }

    // Convert the integer to a string (reverse order)
    while (num > 0) {
        buffer[index++] = '0' + (num % 10);
        num /= 10;
    }

    // Send the number in the correct order
    while (--index >= 0) {
        UART0_WriteChar(buffer[index]);
    }
}

// Function to initialize the MPU6050 sensor
void MPU6050_Init(void) {
    I2C0_MSA_R = (MPU6050_ADDR << 1);   // Set slave address
    I2C0_MDR_R = 0x6B;                  // Power management register
    I2C0_MCS_R = 0x03;                  // Start and Run I2C transaction (Start + Run)

    // Wait until done
    while (I2C0_MCS_R & 0x01);

    // Check for errors
    if (I2C0_MCS_R & 0x02) {
        I2C0_MCS_R = 0x04;  // Send STOP to recover from error
        // Handle error (could be timeout or communication failure)
        return;
    }

    // Wake up MPU6050 (write to power management register)
    I2C0_MDR_R = 0x00;
    I2C0_MCS_R = 0x01;  // Run I2C transaction for wake up

    // Wait until done
    while (I2C0_MCS_R & 0x01);

    // Check for errors
    if (I2C0_MCS_R & 0x02) {
        I2C0_MCS_R = 0x04;  // Send STOP to recover from error
        // Handle error (could be timeout or communication failure)
        return;
    }
}

void LED_Init(void) {
    SYSCTL_RCGCGPIO_R |= 0x20;
    GPIO_PORTF_DIR_R |= 0x0E;
    GPIO_PORTF_DEN_R |= 0x0E;
}

// Function to read axis data from MPU6050
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

void Delay()
{
    volatile int i;
    for (i = 0; i < 100000; i++);
}

void UpdateLEDs(int16_t x, int16_t y, int16_t z) {
    GPIO_PORTF_DATA_R &= ~0x0E;
    if (x > THRESHOLD || x < -THRESHOLD ) {
        GPIO_PORTF_DATA_R = 0x02;
    }
    else if (y > THRESHOLD || y < -THRESHOLD ) {
        GPIO_PORTF_DATA_R = 0x08;
    }
    else if (z > THRESHOLD  || z < -THRESHOLD ) {
        GPIO_PORTF_DATA_R = 0x04;
    }
}

#define COLLISION_THRESHOLD 2000  // Adjust this based on sensitivity requirements
#define BLINK_COUNT 5  // Number of blinks for each detected collision

// Variables to store previous acceleration values
int16_t prevX = 0, prevY = 0, prevZ = 0;

void DelayShort() {
    volatile int i;
    for (i = 0; i < 50000; i++);  // Short delay for LED blink
}

void BlinkLED(uint8_t ledMask) {
    int i;
    for (i = 0; i < BLINK_COUNT; i++) {
        GPIO_PORTF_DATA_R |= ledMask;  // Turn on LED
        DelayShort();
        GPIO_PORTF_DATA_R &= ~ledMask; // Turn off LED
        DelayShort();
    }
}

void DetectCollision(int16_t x, int16_t y, int16_t z) {
    // Clear all LEDs initially
    GPIO_PORTF_DATA_R &= ~0x0E;

    // Check for sudden increase in X-axis acceleration
    if ((x - prevX > COLLISION_THRESHOLD) || (prevX - x > COLLISION_THRESHOLD)) {
        BlinkLED(0x02);  // Blink red LED 5 times for X-axis collision
    }

    // Check for sudden increase in Y-axis acceleration
    if ((y - prevY > COLLISION_THRESHOLD) || (prevY - y > COLLISION_THRESHOLD)) {
        BlinkLED(0x08);  // Blink green LED 5 times for Y-axis collision
    }

    // Check for sudden increase in Z-axis acceleration
    if ((z - prevZ > COLLISION_THRESHOLD) || (prevZ - z > COLLISION_THRESHOLD)) {
        BlinkLED(0x04);  // Blink blue LED 5 times for Z-axis collision
    }

    // Update previous values for the next detection cycle
    prevX = x;
    prevY = y;
    prevZ = z;
}



int main(void) {
    I2C_Init();
    UART0_Init();
    MPU6050_Init();
    LED_Init();
    int16_t accelX, accelY, accelZ;

    while (1) {
        accelX = MPU6050_ReadAxis(ACCEL_XOUT_H);
        accelY = MPU6050_ReadAxis(ACCEL_XOUT_H + 2);
        accelZ = MPU6050_ReadAxis(ACCEL_XOUT_H + 4);

        UpdateLEDs(accelX, accelY, accelZ);
        //DetectCollision(accelX, accelY, accelZ);

        // Send X value
        UART0_WriteString("X: ");
        UART0_WriteInt(accelX);
        UART0_WriteString(" ");

        // Send Y value
        UART0_WriteString("Y: ");
        UART0_WriteInt(accelY);
        UART0_WriteString(" ");

        // Send Z value
        UART0_WriteString("Z: ");
        UART0_WriteInt(accelZ);
        UART0_WriteString("\n");
        Delay();
    }
}
