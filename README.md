# Brewing_temp_controller
This project adds PID temperature control to existing devices. It is intended to control an SSR, which will control a heater (e.g. kettle). It is intended to run with an MQTT server. The current temperature from up to 3 sensors will be continuously sent to the server and the control parameter P, I, and D as well as the setpoint and the update interval can be set from the server. All parameters can be set through a web server. Additionally the setpoint can be modified with two buttons, where a short press will result in a change by one and a continuous press will result in a change in steps of 5°C. The project is made for an ESP8266.
## MQTT
**Submit**
- temp1
- temp2
- temp3
- setpoint (if changed)
- updateInterval (if changed through web interface)
- P (if changed through web interface)
- I (if changed through web interface)
- D (if changed through web interface)

**Subscribed**
- setpoint
- updateInterval
- P
- I
- D

## Libraries
The following libraries are required:
- DallasTemperature
- ESP8266_and_ESP32_OLED_driver_for_SSD1306_displays
- ESP8266WiFi
- ESP8266WebServer
- PID
- PubSubClient
- WiFiManager
