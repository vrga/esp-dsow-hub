/*
ESP8266-based Dallas 1-wire temperature sensors to mqtt broker interface
Copyright (C) 2016  Fran Viktor Pavelić <fv.pavelic@egzodus.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// dumps a lot of data on the serial output. useful for debugging. use the
// included printDbg function for these.
#define SERIAL_ENABLED false

// wifi info.
#define wifi_ssid "ssid"
#define wifi_password "123412341234"

// mqtt server ip.
#define mqtt_server "192.168.1.2"

// Dallas 1 wire temp sensor resolution. 9-12
#define SENSOR_RESOLUTION 12

// How often the sensors are read? default is every 5 seconds
#define SENSOR_READ_DELAY 5000

#define MQTT_MAX_PACKET_SIZE 256

// How often the MQTT loop is evaluated. I've been getting away with once a
// second
#define MQTT_LOOP_DELAY 1000

// Timeout for sensor reading, if we didnt read out a sensor within 2 seconds by
// default, assume failure.
#define SENSOR_READ_TIMEOUT 2000
