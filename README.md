# MeterMonitor ESP32

## Project Overview
**MeterMonitor** is a project
designed to monitor water usage by capturing images of a water meter at regular intervals using an ESP32-Cam module.
The captured images are transmitted via MQTT to a server for further processing and classification,
enabling real-time tracking and monitoring of water consumption.

This repository contains the firmware code for the ESP32-Cam,
which controls the image capture, deep sleep management, and MQTT transmission.
The ESP32-Cam is mounted in a custom-designed and 3D-printed enclosure to protect the hardware
and ensure consistent image quality.
The project is optimized for low power consumption, allowing it to be powered by batteries for extended periods.

## Features
- **Configurable Image Capture Interval**: The ESP32-Cam captures images at a user-defined interval (in minutes) to monitor the water meter.
- **Deep Sleep Mode**: The ESP32-Cam enters Deep Sleep mode between captures to significantly reduce power consumption, making it suitable for battery-powered operation.
- **MQTT Integration**: Captured images are sent to a specified MQTT broker, where they can be processed and classified.
- **External LED-Strip**: It is possible to use a WS2812B LED-Strip to light up the picture as precisely as possible.
- **Custom Enclosure**: The ESP32-Cam is housed in a custom-designed, 3D-printed enclosure to ensure durability and optimal performance.
- **Custom Configuration**: Adjust the capture interval and other settings via the 'menuconfig' to suit your monitoring needs.

## Hardware Requirements
- **ESP32-Cam**: The main hardware used for image capture. (Tested with AiThinker ESP32-Cam)
- **Custom 3D-Printed Enclosure**: Designed to fit the ESP32-Cam and securely attach it to a water meter.
- **Power Supply**: A battery pack or stable 5V power source. The low power consumption via Deep Sleep allows the device to run on batteries for extended periods.

## Software Requirements
- **ESP-IDF**: For compiling and uploading the code to the ESP32-Cam.
- **MQTT Broker**: A server to receive and handle the images sent by the ESP32-Cam.
- **Wi-Fi Network**: The ESP32-Cam requires a stable Wi-Fi connection to send data to the MQTT broker.

## Getting Started

### 1. Clone the Repository
```bash
git clone https://gitlab.cs.hs-rm.de/metermonitor/metermonitor-esp.git
cd metermonitor-esp
```

### 2. Configure the Firmware
Before uploading the firmware to the ESP32-Cam, configure the application settings with your specific settings:
```bash
idf.py set-target esp32
idf.py menuconfig
```
Switch into the Subsection "MeterMonitor config" and set at least the following settings:
- Wi-Fi Credentials: Enter your Wi-Fi SSID and password.
- MQTT Broker Settings: Set the MQTT broker address and port. (Username and password if needed)
- Capture Interval: Define the interval (in minutes) for the ESP32-Cam to capture images and enter Deep Sleep mode.

### 3. Upload the Code
Using ESP-IDF:

First, you will need to compile the ESP-IDF project via the following command:
```bash
idf.py build
```
After a successful compilation, you can flash the ESP32-Cam with the software via the following command:

*Hint: To enter the Flash-Mode with the AiThinker ESP32-Cam the GPIO-Pin 0 needs to be bridged with Ground at startup!*
```bash
idf.py -p /dev/ttyUSB0 flash
```

To see, if everything is working as expected, you can check the log-Output via the following command:
```bash
idf.py -p /dev/ttyUSB0 monitor
```

### 4. Mount the ESP32-Cam
   Print the custom-designed enclosure using the provided STL files.
   Assemble the enclosure and securely mount the ESP32-Cam to your meter.

### 5. Start Monitoring
   Once everything is set up, power on the ESP32-Cam. It will begin capturing images at the specified interval, entering Deep Sleep mode between captures to conserve battery life, and sending the images to your MQTT broker.
   
## Repository Structure
```bash
metermonitor-esp/
├── src/
│   ├── camera.c            # ESP32-Cam camera - Handling
│   ├── CMakeLists.txt
│   ├── esp_ws28xx.c        # External LED-Strip - Handling
│   ├── main.c              # Main firmware code
│   ├── mqtt.c              # MQTT - Handling
│   ├── sntp.c              # SNTP Time synchronisation
│   ├── wifi.c              # WiFI - Handling
│   └── ...
├── .gitignore
├── CMakeLists.txt
├── README.md
└── sdkconfig.defaults      # Default settings to be set when compiling firmware
```

Happy Monitoring! 🚰