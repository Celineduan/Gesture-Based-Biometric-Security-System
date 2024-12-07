Overview
This project implements a biometric security system using an embedded microcontroller and gyroscope sensor. Users can record specific hand movements as their "password" and later replicate these movements for authentication. The system processes the gesture data in real-time and compares it against stored patterns to grant or deny access.
Features

Real-time gesture recording and verification
High-precision gyroscope data processing (100Hz sampling rate)
LED-based user feedback system
Configurable sensitivity thresholds
Optimized memory usage for gesture storage
Simple one-button interface for mode switching

Hardware Requirements

STM32 Development Board
Integrated Gyroscope Sensor
LED Indicators (Green and Red)
User Button

Pin Configuration

MOSI: PF_9
MISO: PF_8
SCLK: PF_7
CS: PC_1
USER_BUTTON: PA_0
GREEN_LED: PG_13
RED_LED: PG_14

Software Dependencies

PlatformIO
Mbed Framework
C++ Compiler
