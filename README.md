# TaaraESP-SHT21-EmonCMS

[TaaraESP-SHT21 WebSite](https://taaralabs.eu/es1)

Arduino sketch for sending TaaraESP-SHT21 (ESP8266+SHT21) temperature and humidity sensor data to EmonCMS over HTTPS or HTTP

The basic idea of this program is to start the board up, take the measurements as soon as possible before the board gets a chance to heat up, then establish the Wifi connection, connect to the EmonCMS server and send the data over HTTPS or HTTP in JSON format. After that the board goes into deep sleep (shuts down) for five minutes,  only the timer keeps going. In this mode the board consumes very little power and generates almost no heat which could affect the accuracy of the temperature and humidity measurements. Waking up from deep sleep reboots the board and the program starts from the beginning.

The on-board LED on the TaaraESP-SHT21 is used for diagnostics indicating the state of the device.

Configuration

First, the EmonCMS account is needed either on https://emoncms.org or on a private EmonCMS server.
For entering the WiFi and EmonCMS settings go to config mode - press config button in one second AFTER powering on.
The status LED should stay on.
On your WiFi enabled mobile device or laptop search for WiFi AP-s with the name starting with ESP and following few numbers (module ID) - connect to it.
In most cases the device is automatically redirected to the configuration web page on http://192.168.4.1
Choose the WiFi network from the list or enter it manually, enter WiFi password, EmonCMS server name and APIKEY and save settings.
The board should restart and enter the normal operational mode.

Normal operation

In normal operation the status LED should turn on for few seconds after every five minutes.

Diagnostics

* LED stays on longer than 5 seconds - problem with WiFi connection or the module is in config mode. WiFi is not configured properly or not in range.
* LED blinks once at a time - could not connect to server. Server is not configured or not accessible.
* LED blinks twice at a time - problem with the request. Server did not answer with "ok". Wrong apikey, error in data format or server/app is broken.
* LED keeps blinking - unknown error.
After blinking for five minutes the board restarts.
