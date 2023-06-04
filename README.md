# Datalogger

The datalogger's primary responsibility is to collect data from two CAN bus lines and write it to an SD card in MMC mode. To achieve maximum performance for read and write operations, we are using Expressif-FreeRTOS in combination with the two cores of the ESP32-PICO-D4 platform. The datalogger also has ADC inputs with filter options, extra digital inputs/outputs, and encoder reading capabilities.

In addition to collecting data, the datalogger is responsible for sending the received data from the buses to the telemetry module.
