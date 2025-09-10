Smart City Monitoring System
A comprehensive environmental monitoring and automation system

🌟 Overview
The Smart City Monitoring System is an IoT-based solution that automates maintenance while providing real-time environmental monitoring. The system integrates multiple sensors to create an intelligent ecosystem that responds to changing conditions without human intervention.

Key Features:

📦 Smart Bin Monitoring: Ultrasonic detection of trash bin capacity

💡 Lamp Lighting Monitoring: Automatic detection of outdoor light failures

🌱 Intelligent Irrigation: Soil moisture-based watering with weather awareness

🌧️ Weather Adaptation: Rain detection to prevent unnecessary watering

⏰ Time-based Automation: RTC-controlled scheduling for day/night operations

📊 Real-time Monitoring: Serial interface for system status and alerts

Design Choices
Non-blocking Architecture: Uses ticker interrupts for periodic measurements instead of blocking delays

Sensor Fusion: Combines multiple environmental factors for intelligent decision making

State Management: Tracks previous states to detect transitions and trigger appropriate actions

Calibration System: Automatic sensor calibration for accurate measurements

Debounced Inputs: Proper button handling with software debouncing

🔧 Hardware Requirements
Essential Components
Microcontroller: ARM Cortex-M based board (Nucleo-F401RE series recommended)

Sensors:

HC-SR04 Ultrasonic Distance Sensor

YL-69 Soil Moisture Sensor

Photoresistor (LDR)

Rain Sensor Module

DS1302 Real-Time Clock module

Actuators: 5V Water pump with motor driver (L298N required)

Interface: Push button, Serial-to-USB converter

Wiring Diagram

Microcontroller   ────   Peripheral
─────────────────────────────────────
A4               ────   Motor+
A5               ────   Motor-
D6               ────   RTC CLK
D3               ────   RTC DAT
D8               ────   RTC RST
D2               ────   Rain Sensor DO
A0               ────   Light Sensor
PB_8             ────   Ultrasonic Trig
PB_9             ────   Ultrasonic Echo
PC_13            ────   Calibration Button
A1               ────   Soil Sensor AO
GND              ────   All GND pins
3.3V/5V          ────   Sensor VCC pins

Once powered, the system will:

Initialize all sensors and perform calibration

Monitor environment every 5 seconds

Display status information via serial

Automate watering based on conditions

Send alerts when attention is needed

Serial Interface
Connect at 115200 baud to see:


CodeCraft system started
Measurements every 5 seconds
=== Starting Calibration ===
Calibration 1: 45 cm
Calibration 2: 46 cm  
Calibration 3: 44 cm
=== Calibration Complete ===
HCS-04 reference value: 45 cm
============================
Current Time is [21:00:00]
EMPTY BIN - Bin is full and needs emptying [21:00:05]
Change bulb - light is off during nighttime [21:00:10]

Manual Controls
Button Press: Toggles between day (10:00) and night (21:00) modes for testing
