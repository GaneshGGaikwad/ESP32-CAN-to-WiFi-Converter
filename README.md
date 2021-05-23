# CAN-to-WiFi-Converter

It connects to CAN device and communicate. We can get the data over WiFi, so we can get easily integrate this with web application or Mobile Apllication.

# Developer View

As we look the CAN communication, we are sometimes restrict to use USB based converters. So we need to keep our systems near to CAN device in the workplace. Sometimes it's not feasible. So heres the solution as my point of view and my approch to solve this.

# Prerequisite
Knowledge of CAN Communication Protocol.

Understanding of UDS protocol.

# Installation

You have to install Arduino IDE and ESP32 toolchain to run this project.

Installation Links:

Arduino:

Link: https://www.arduino.cc/en/software

ESP32 Tool Chain

Process:

Open the Arduino IDE -----> Preferences-----> Enter "https://dl.espressif.com/dl/package_esp32_index.json" into the "Additional Board Manager URLs" field. Then, click the "OK" button.------> Open the Boards Manager. Go to Tools > Board > Boards Manager --------> Search for ESP32 and press install button for the "ESP32 by Espressif Systems".

Select your Board in Tools > Board menu (in my case it’s the DOIT ESP32 DEVKIT V1) to run the programs on ESP32 board.

# Downloads:
ESP32_CAN_Master Library Required.

Link: https://github.com/miwagner/ESP32-Arduino-CAN

To run the project you need to download the CP210x USB to UART Bridge Drivers.

Link: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

# Contribute

For Suggestions your always welcome.

Please contribute to this project to Improve it's Performance and Reliability.
