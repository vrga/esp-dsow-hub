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

#include <OneWire.h>
#include <DallasTemperature.h>
#include <vector>

struct tempSensor {
  DeviceAddress address;
  bool converting = false;
  unsigned long lastStart;
  unsigned long lastTouch;
  unsigned int readAttempts = 0;
};

typedef std::vector<tempSensor> tempSensorAddressCollection;

struct busSensorMapping {
  OneWire owBus;
  DallasTemperature deviceBus;
  tempSensorAddressCollection devices;
  int pin;
  unsigned long lastTouch;
};

typedef std::vector<busSensorMapping> sensorMapCollection;

void setupWifi();
void initDevices();

void checkWifiAndMqtt();
void mqttLoop();
void readTempSensors();
void reconnectMQTT();
void publishMessage(String channel, String message);

#ifndef MQTT_LOOP_DELAY
#define MQTT_LOOP_DELAY 1000
#endif

#ifndef SENSOR_RESOLUTION
#define SENSOR_RESOLUTION 12
#endif

#ifndef SERIAL_ENABLED
#define SERIAL_ENABLED false
#endif

#ifndef SENSOR_READ_TIMEOUT
#define SENSOR_READ_TIMEOUT 2000
#endif

#ifndef SENSOR_READ_TOTAL_FAIL_MAX
#define SENSOR_READ_TOTAL_FAIL_MAX 4
#endif

void printDbg(String str, bool newline = false);
