# Collision Detector

This project, **Collision Detector**, uses a **Tiva C Series TM4C123G microcontroller** to detect and visualize collision events through acceleration data in real-time. The microcontroller reads data from an accelerometer sensor and sends it to a **Python-based GUI** for visualization and analysis.

---

## Features

- **Real-time Data Visualization**: Plots X, Y, and Z acceleration data in real time on a graph.
- **Collision Detection**: Detects sudden changes in acceleration which might indicate a collision or abrupt movement.
- **Serial Communication**: Data transfer between the Tiva C microcontroller and Python GUI is achieved via serial communication.

---

## Project Structure

This project is composed of two main parts:

1. **Microcontroller Code (Tiva C TM4C123G)**: Captures acceleration data and sends it via UART to a connected computer.
2. **Python GUI**: Receives and visualizes the data using a PyQt5 and Matplotlib interface.

---
# MPU6050 to Tiva C Series TM4C123GH6PM Connections

This table describes the connections required to interface the MPU6050 with the Tiva C Series TM4C123GH6PM microcontroller using I2C on Port B.

| **MPU6050 Pin** | **TM4C123GH6PM Pin** | **Description**       |
|-----------------|----------------------|-----------------------|
| VCC             | 3.3V or 5V          | Power Supply         |
| GND             | GND                  | Ground               |
| SDA             | PB3                  | I2C Data Line        |
| SCL             | PB2                  | I2C Clock Line       |
| AD0             | GND or VCC           | I2C Address Select   |
| INT             | Not Connected        | Interrupt (optional) |

## Notes
- **I2C Address (AD0)**: 
  - Connect **AD0** to **GND** to use the I2C address `0x68`. 
  - Connect **AD0** to **VCC** to use the alternate address `0x69`.
  
- **Pull-up Resistors**:
  - Both **SDA** and **SCL** lines require pull-up resistors (typically 4.7kΩ to 3.3V) for reliable I2C communication.
  - Check if your MPU6050 module includes built-in pull-up resistors, as many modules come with them pre-installed.
  
This setup enables the Tiva C Series TM4C123GH6PM to communicate with the MPU6050 for accelerometer data collection.
