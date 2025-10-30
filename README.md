# IoT-Based Weather Station
> *by Zahraa – MSCS Year 1, IoT Class, Al-Nahrain University*
> 
> **Date:** 2025-10-01

---

## Project Overview

This project is a **real-time IoT-based Weather Monitoring Station** built using the Arduino UNO R4 WiFi board, featuring a web dashboard accessible over your local network. The station collects environmental data such as temperature, humidity, atmospheric pressure, rainfall, and air quality using common sensors. All readings are displayed live on a modern web dashboard, offering both raw values and graphical trends.

## Table of Contents

- [Project Overview](#project-overview)
- [Block Diagram](#block-diagram)
- [Circuit Diagram](#circuit-diagram)
- [Components Used](#components-used)
- [Sensor Pin Mapping](#sensor-pin-mapping)
- [How It Works](#how-it-works)
- [How To Build](#how-to-build)
- [Web Dashboard Preview](#web-dashboard-preview)
- [References](#references)

---

## Block Diagram

This diagram summarizes the communication architecture between your sensors, Arduino UNO R4 WiFi, and the web dashboard:

![Block Diagram](https://circuitdigest.com/sites/default/files/inlineimages/u5/Block-Diagram-Mini-Weather-Monitoring-System.jpg)

---

## Circuit Diagram

Use this wiring to connect all your sensors to the Arduino UNO R4 WiFi, as shown below:

![Circuit Diagram](https://circuitdigest.com/sites/default/files/circuitdiagram_mic/Circuit-Diagram-Arduino-Based-Weather-Monitoring-System.png)

Or use the Fritzing/breadboard-style diagram for exact pin-outs (example shown based on your attached layout):

![Breadboard Layout](https://circuitdigest.com/sites/default/files/inlineimages/u5/Hardware-Setup-Weather-Monitoring-System-using-Arduino.jpg)

---

## Components Used

| Component                                | Quantity | Notes                                             |
|-------------------------------------------|----------|---------------------------------------------------|
| Arduino UNO R4 WiFi                      | 1        | Main microcontroller & WiFi server                |
| DHT11 Sensor (Temperature & Humidity)     | 1        | Data, VCC, GND (Digital)                          |
| BMP180 Sensor (Atmospheric Pressure)      | 1        | I2C communication                                 |
| MQ135 Sensor (Air Quality/CO₂ Sensor)     | 1        | Analog output (calibration required for accuracy) |
| Rain Sensor (“YL-83”)                     | 1        | Analog or digital output                          |
| Breadboard & Jumper Wires                 | –        | Standard                                            |
| Power Supply                              | 1        | For Arduino board                                 |

---

## Sensor Pin Mapping

**Refer to this table to correctly wire your sensors:**

| Sensor Pinout       | Arduino UNO R4 Pin                |
|---------------------|-----------------------------------|
| **DHT11**           | Signal: Pin 2, VCC: 5V, GND: GND  |
| **BMP180**          | SDA: A4, SCL: A5, VCC: 3.3V/5V, GND: GND |
| **MQ135**           | AO: A0, VCC: 5V, GND: GND         |
| **Rain Sensor**     | AO: A1, VCC: 5V, GND: GND         |

*(For details, see [CircuitDigest’s Pin Mapping Tables](https://circuitdigest.com/microcontroller-projects/how-to-build-an-iot-based-weather-monitoring-system-using-arduino#:~:text=MQ135%20Sensor%20and,Digital%20Pin%20(D3))).*

---

## How It Works

1. **Sensors collect weather data:**  
   - Temperature and humidity from DHT11  
   - Atmospheric pressure from BMP180  
   - Air quality from MQ135  
   - Rain detection from rain sensor

2. **Arduino UNO R4 WiFi reads all sensor data** and serves it via HTTP over WiFi.

3. **Web dashboard:**  
   - Accessed on any device connected to the same WiFi network.
   - Built-in AJAX updates and real-time charting (using Chart.js).
   - Displays both current and historical sensor readings.

4. **No cloud needed:**  
   - All data stays local; no third-party services.

---

## How To Build

### 1. Hardware Assembly

- Carefully connect each sensor according to the diagrams above.
- Supply the Arduino UNO R4 WiFi with the appropriate power.
- Double-check all connections against the [Circuit Diagram](#circuit-diagram).

### 2. Software Setup

- Install necessary Arduino libraries:
  - Adafruit BMP085 Library
  - DHT Sensor Library
  - Arduino_LED_Matrix
  - WiFiS3 (for R4 WiFi networking)
- Flash the provided `.ino` file (`IOT_WeatherStation.ino`) to your board.
- Open the Serial Monitor to see status messages.

### 3. Accessing the Dashboard

- Connect your computer/smartphone to the same WiFi network as the Arduino.
- The Arduino will print its IP address to the Serial Monitor upon boot.
- Enter the IP (e.g. `http://192.168.0.101/`) into any browser to view your local IoT Weather Dashboard.

---

## 🌐 Web Dashboard Preview

Your dashboard features responsive, modern styling and real-time data visualization:

![Web Dashboard Sample (Chart.js)](https://c.top4top.io/p_3590akziq1.png)
![Web Dashboard Sample (Chart.js)](https://k.top4top.io/p_3590l9eoy1.png)

- **Live Sensor Cards:** For temperature, humidity, pressure, AQI, and rain.
- **Trends Graph:** Interactive chart of sensor readings (auto-refreshes every second).

---

## References & Further Reading

- Main Project Resource:  
  [CircuitDigest: How to Build an IoT-based Weather Monitoring System Using Arduino](https://circuitdigest.com/microcontroller-projects/how-to-build-an-iot-based-weather-monitoring-system-using-arduino)


---

## Contact
For questions, reach out via Al-Nahrain University, Computer Science Department.
