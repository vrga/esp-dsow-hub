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

#include "constants.h"
#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <vector>
#include "string_utils/helpers.h"
#include "ds18.h"
#include "user_interface.h"

WiFiClient espClient;
PubSubClient client(espClient);

// number of "buses" in use and the associated GPIO pins to use for the 1-wire
// bus.
const int busLength = 4;
int busPins[busLength] = {5, 12, 13, 14};

const uint32 boardIdentifier = ESP.getChipId();

String mqtt_topic = "sensors/temperature/";
String mqtt_status_topic = "sensors/status";

void printDbg(String str, bool newline) {
  if (SERIAL_ENABLED) {
    if (newline) {
      Serial.println(str);
    } else {
      Serial.print(str);
    }
  }
}

sensorMapCollection globalBusMap;

void setup() {
  if (SERIAL_ENABLED) {
    Serial.begin(115200);
  }
  setupWifi();
  initDevices();
  client.setServer(mqtt_server, 1883);
  mqtt_topic += String(boardIdentifier) + String("/");
  checkWifiAndMqtt();
  publishMessage(mqtt_status_topic,
                 String("Init finished. Found: ") + globalBusMap.size());
}

void initDevices() {
  printDbg("Initializing devices", true);
  // pre-allocate vector
  globalBusMap.reserve(busLength);
  for (int x = 0; x < busLength; x++) {
    printDbg("Initializing pin: ");
    printDbg(String(busPins[x]), true);

    // initialize with 1-wire set up
    busSensorMapping busMap = {OneWire(busPins[x])};

    // create DallasTempereature device
    busMap.deviceBus = DallasTemperature(&busMap.owBus);
    busMap.pin = busPins[x];

    // dont block while converting
    busMap.deviceBus.setWaitForConversion(false);
    busMap.deviceBus.setCheckForConversion(true);
    busMap.deviceBus.begin();

    busMap.deviceBus.setResolution(SENSOR_RESOLUTION);

    // autodiscovery of chips attached to bus
    busMap.owBus.reset_search();
    tempSensor s;
    while (busMap.owBus.search(s.address)) {
      String message = "Address: " + formatAddress(s.address) + " enumerated.";
      printDbg(message);
      s.lastTouch = millis();

      busMap.devices.push_back(s);
    }

    busMap.lastTouch = millis();

    // push to global state
    globalBusMap.push_back(busMap);
  }
}

bool shouldAbort = false;

void loop() {
  checkWifiAndMqtt();
  readTempSensors();
  mqttLoop();

  if (shouldAbort) {
    publishMessage(mqtt_status_topic, String("too many read fails, aborting"));
    client.loop();
    ESP.reset();
  }

  delay(200);
}

void checkWifiAndMqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
}

int mqttLastTime = 0;
void mqttLoop() {
  if ((millis() - mqttLastTime) >= MQTT_LOOP_DELAY) {
    client.loop();
    mqttLastTime = millis();
  }
}

void publishMessage(String channel, String message) {
  client.publish(channel.c_str(), message.c_str());
}

void readTempSensors() {

  unsigned long currentTime = millis();

  // for each available 1-wire bus
  for (auto busMap = globalBusMap.begin(); busMap < globalBusMap.end();
       ++busMap) {

    String message = "Poking pin: " + String(busMap->pin) + "\n";
    printDbg(message, true);

    // reset pointer to 1-wire object
    // FIXME: There's very definitely a prettier way to do this, but idk.
    // std::auto_ptr/unique_ptr and friends maybe?
    busMap->deviceBus.setOneWire(&busMap->owBus);

    // Iterate over recorded 1-wire devices
    for (auto sensor = busMap->devices.begin(); sensor < busMap->devices.end();
         ++sensor) {
      message = " on device address: " + formatAddress(sensor->address);
      printDbg(message, true);

      message = "Logging times: \t\tCurrent: " + String(currentTime) +
                "\tCurrent - delay: " +
                String(currentTime - SENSOR_READ_DELAY) + "\tLast touch: " +
                String(sensor->lastTouch);
      printDbg(message, true);
      // timer check. if current time - delay is greater than the last time we
      // touched this, continue.
      if ((currentTime - sensor->lastTouch) >= SENSOR_READ_DELAY) {
        // if not converting, request temperature conversion.
        if (!sensor->converting) {
          message = "  Starting conversion";
          if (busMap->deviceBus.requestTemperaturesByAddress(sensor->address)) {
            sensor->converting = true;
            sensor->lastStart = millis();
          }
          sensor->readAttempts++;
        }

        // if converting and a conversion is available from sensors, start
        // reading
        // if temperature is equal to DEVICE_DISCONNECTED_C wait some more and
        // repeat
        // TODO: handle "chip went splat/away" somehow
        if (sensor->converting &&
            busMap->deviceBus.isConversionAvailable(sensor->address)) {
          float temp = busMap->deviceBus.getTempC(sensor->address);
          String topic = mqtt_topic + formatAddress(sensor->address);
          char outTemp[15];
          dtostrf(temp, 7, 4, outTemp);
          if (outTemp != "85.0000") {
            publishMessage(topic, String(outTemp));
          }
          message = "  Getting temperatures";
          sensor->converting = false;
          sensor->lastTouch = millis();
          sensor->lastStart = sensor->lastStart;
          sensor->readAttempts = 0;
        }
        // if sensor is converting and time since reading was started is larger
        // than read timeout, abort.
        if (sensor->converting &&
            ((currentTime - sensor->lastStart) >= SENSOR_READ_TIMEOUT)) {
          message = "  Reading failed, reset.";
          publishMessage(mqtt_status_topic, String("Reading failed on: ") +
                                                formatAddress(sensor->address));
          sensor->converting = false;
        }

        // if a sensor failed more than the maximum number of fails, restart the
        // device.
        if (sensor->readAttempts >= SENSOR_READ_TOTAL_FAIL_MAX) {
          shouldAbort = true;
        }

        printDbg(message, true);
      }
    }
    // update last manipulated time...
    busMap->lastTouch = millis();
  }
  yield();
}

void setupWifi() {
  delay(10);
  // We start by connecting to a WiFi network
  String message = "Connecting to " + String(wifi_ssid);
  printDbg(message, true);

  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printDbg(".");
  }
  message = "WiFi connected\nIP address: " + String(WiFi.localIP());
  printDbg(message, true);
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    printDbg(String("Attempting MQTT connection..."), true);
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client")) {
      printDbg(String("connected"), true);
    } else {
      String message =
          "failed, rc=" + String(client.state()) + " try again in 0.5 seconds";
      printDbg(message, true);
      // Wait 0.5 seconds before retrying
      delay(500);
    }
  }
}
