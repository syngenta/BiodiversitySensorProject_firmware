Biodiversity Sensor Project
============================

Overview
--------
This project involves a custom-built biodiversity sensor designed for ultra-low-power operation, using a **nRF5340DK** board integrated with a **GSM EC200** module and an **ArduCAM 3MP 3.3V SPI** camera.
The sensor captures images at 20-second intervals, making it ideal for long-term outdoor deployments with minimal power consumption.

Key Features
------------
- **ArduCAM 3MP 3.3V SPI Camera**: Captures high-quality images of the environment at regular intervals.
- **nRF5340DK Custom Board**: Provides low-power operation and seamless integration with the camera and GSM module.
- **GSM EC200 Module**: Enables remote data transmission via cellular networks.
- **Ultra-low power consumption**: The device consumes only 1 watt of power, making it efficient for extended field operations.
- **Image Capture Interval**: Configured to capture images every 20 seconds.
- **Remote Data Upload**: Captured images are transmitted via the GSM module to a remote server or cloud service.

System Requirements
-------------------
- **nRF5340DK Custom Board**: With integrated GSM EC200 module.
- **ArduCAM 3MP 3.3V SPI Camera**.
- **1W Power Supply**: The device operates on an ultra-low power budget of 1 watt.

Installation
------------
1. Clone the project repository to your local machine:
   .. code-block:: bash

      git clone https://github.com/SumanKumar891/Biodiversity_sensor.git

2. Set up the environment and dependencies:
   - Ensure Zephyr RTOS is installed and configured for **nRF5340DK**.
   - Set up cellular network configurations for the **GSM EC200** module.
   - Integrate the **ArduCAM** drivers and configure SPI communication.

3. Build and flash the firmware onto the nRF5340DK custom board:
   .. code-block:: bash

      west build -b nrf5340dk_nrf5340_cpuapp
      west flash

Usage
-----
Once the sensor is powered and running, it will automatically:
1. Capture a photo every 20 seconds using the **ArduCAM**.
2. Transmit the captured image via the **GSM EC200** module.
3. Enter a low-power state between capture intervals to minimize energy consumption.

Contributing
------------
Feel free to submit issues or pull requests. Contributions are welcome to improve the functionality and performance of the project.
