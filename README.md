# MeterMonitor ESP32

## Project Overview
**MeterMonitor** is a project designed to monitor water usage by capturing images of a water meter at regular intervals using an ESP32-Cam module. The captured images are transmitted via MQTT to a server for further processing and classification, enabling real-time tracking and monitoring of water consumption.

This repository contains the firmware code for the ESP32-Cam, which controls the image capture, deep sleep management, and MQTT transmission. The ESP32-Cam is mounted in a custom-designed and 3D-printed enclosure to protect the hardware and ensure consistent image quality. The project is optimized for low power consumption, allowing it to be powered by batteries for extended periods.

## Features
- **Configurable Image Capture Interval**: The ESP32-Cam captures images at a user-defined interval (in minutes) to monitor the water meter.
- **Deep Sleep Mode**: The ESP32-Cam enters Deep Sleep mode between captures to significantly reduce power consumption, making it suitable for battery-powered operation.
- **MQTT Integration**: Captured images are sent to a specified MQTT broker, where they can be processed and classified.
- **Custom Enclosure**: The ESP32-Cam is housed in a custom-designed, 3D-printed enclosure to ensure durability and optimal performance.
- **Remote Configuration**: Adjust the capture interval and other settings via the firmware to suit your monitoring needs.

## Hardware Requirements
- **ESP32-Cam**: The main hardware used for image capture.
- **Custom 3D-Printed Enclosure**: Designed to fit the ESP32-Cam and securely attach it to a water meter.
- **Power Supply**: A battery pack or stable 5V power source. The low power consumption via Deep Sleep allows the device to run on batteries for extended periods.

## Software Requirements
- **ESP-IDF** or **Arduino IDE**: For compiling and uploading the code to the ESP32-Cam.
- **MQTT Broker**: A server to receive and handle the images sent by the ESP32-Cam.
- **Wi-Fi Network**: The ESP32-Cam requires a stable Wi-Fi connection to send data to the MQTT broker.

## Getting Started

### 1. Clone the Repository
```bash
git clone https://gitlab.com/yourusername/metermonitor.git
cd metermonitor
```

### 2. Configure the Firmware
Before uploading the firmware to the ESP32-Cam, configure the config.h file with your specific settings:

- Wi-Fi Credentials: Enter your Wi-Fi SSID and password.
- MQTT Broker Settings: Set the MQTT broker address, port, and topic.
- Capture Interval: Define the interval (in minutes) for the ESP32-Cam to capture images and enter Deep Sleep mode.

### 3. Upload the Code
Using the Arduino IDE or ESP-IDF:

- Select the appropriate board (ESP32-Cam).
- Connect your ESP32-Cam to your computer via USB.
- Compile and upload the code.
### 4. Mount the ESP32-Cam
   Print the custom-designed enclosure using the provided STL files. Assemble the enclosure and securely mount the ESP32-Cam to your water meter.

### 5. Start Monitoring
   Once everything is set up, power on the ESP32-Cam. It will begin capturing images at the specified interval, entering Deep Sleep mode between captures to conserve battery life, and sending the images to your MQTT broker.
   
## Repository Structure
```bash
MeterMonitor/
├── src/
│   ├── main.cpp               # Main firmware code
│   └── ...
├── enclosure/
│   └── meter_monitor_case.stl # 3D model for the enclosure
└── README.md
```

Happy Monitoring! 🚰