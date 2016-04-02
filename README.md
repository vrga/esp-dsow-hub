# ESP8266 Based 1-wire temp sensor hub that sends to a mqtt broker.
Hi and welcome to yet another home temperature monitoring project.
This is my code to read potentially many Dallas 1-wire temp sensors across potentially many "branches" and the code was developed while using ds18b20 chips.
Should be compatible with others as well, but you never know.

Code is licensed under GPLv2.

This uses the ESP-201 board since it was the easiest one to get started with for me.
Developed using platform-io and actively uses the following libs:
 * [PubSubClient](http://pubsubclient.knolleary.net/)
 * [OneWire](http://www.pjrc.com/teensy/td_libs_OneWire.html)
 * [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
 * [Platform-io espressif integration](http://docs.platformio.org/en/latest/platforms/espressif.html)
 * \+ whatever other libraries are deeper in these dependencies.

Also, with this, in esp-201-board.fzz you can find a [Fritzing](http://fritzing.org/home/) file with the accompanying board and connections.

If you wish to use more buses/branches/pins, change in ds18.cpp int busLength to however many entries are in the busPins array.
All other configurables are in constants.h
And yes, all changes require a recompile and upload to board. Thats why the board is built to support the whole shebang with the very specific "program" and "normal operation" pull-up/pull-down combination on GPIO15.

To change MQTT topics, take a look at ds18.cpp, mqtt_topic and mqtt_status_topic as well as

The following are mandatory prerequisites:
* 3.3V usb-to-serial adapter for programming
* decent power supply. i've built the circuit around using a 2A 5V USB supply.
* pc running an MQTT broker. i'm personally using [Mosquitto](http://mosquitto.org/)
* working WIFI of some description. in my case using OpenWRT on an Asus RT-N14U with WPA2-PSK works just fine.
  + any solution using preshared keys should work without drama.

Core steps of operation:
* Initialization
 + Connects to wifi
 + Checks all the pins defined as "bus" via the busPins array for devices on said bus
 + Allocates dynamically the required structures for each device found.
 + Connects to the MQTT broker of choice
 + makes sure connections are established
* Normal operation - arduino-style main loop
 + re-checks if wifi and mqtt connections are up. If not, re-establishes them.
 + reads temperature sensors
 + runs the mqtt loop if needed
 + waits 200 ms before next iteration
    + this increased stability in my case, with 4 different esp-201 boards. May be a quirk of the board, but i found the ESP8266 chip to get _*very*_ hot otherwise.
* Temperature reader operation
  + for each 1-wire bus we iterate through all devices on it and either initiate (for each individual device) a read, wait for conversion, read the converted temperature or fail it
  + this is done asynchronously due to the fact that to get a temperature reading, a ds18b20 sensor needs around 750ms. Multiply that by number of devices and desire to have multiple readings within a minute and we arrive at the fun conclusion that reading them 1 by 1 would not work.
  + once a read is finished, we submit the temperature to mqtt.
    + topic is defined as "sensors/temperature/\< board identifier \>/\< sensor identifier \>"
      + board identifier is an unsigned 32-bit integer, provided by the chip itself
      + sensor identifier is an 8 character hexadecimal string.

If you find this of use or derive anything from it, i'd be grateful if you let me know :)
