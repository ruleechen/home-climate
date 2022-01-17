#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_SGP30.h>
#include <arduino_homekit_server.h>

#include <Console.h>
#include <BuiltinLed.h>
#include <VictorOTA.h>
#include <VictorWifi.h>
#include <VictorWeb.h>

#include "AirModels.h"
#include "AirStorage.h"

using namespace Victor;
using namespace Victor::Components;

extern "C" homekit_characteristic_t temperatureState;
extern "C" homekit_characteristic_t humidityState;
extern "C" homekit_characteristic_t airQualityState;
extern "C" homekit_characteristic_t vocDensityState;
extern "C" homekit_characteristic_t accessoryName;
extern "C" homekit_server_config_t serverConfig;

Adafruit_AHTX0 aht;
Adafruit_SGP30 sgp;
VictorWeb webPortal(80);
String hostName;
unsigned long lastRead;
unsigned long readInterval;

String parseStateName(uint8_t state) {
  return (
    state == AirQualityExcellent ? F("Excellent") :
    state == AirQualityGood      ? F("Good") :
    state == AirQualityFair      ? F("Fair") :
    state == AirQualityInferior  ? F("Inferior") :
    state == AirQualityPoor      ? F("Poor") : F("Unknown")
  );
}

AirQuality parseAirQuality(float value) {
  return AirQualityGood;
}

String parseYesNo(bool state) {
  return state == true ? F("Yes") : F("No");
}

uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
  return absoluteHumidityScaled;
}

void measure(bool notify) {
  builtinLed.flash();
  // aht
  sensors_event_t humidity, temp;
  if (aht.getEvent(&humidity, &temp)) {
    temperatureState.value.float_value = temp.temperature;
    humidityState.value.float_value = humidity.relative_humidity;
    sgp.setHumidity(getAbsoluteHumidity(temp.temperature, humidity.relative_humidity));
    if (notify) {
      homekit_characteristic_notify(&temperatureState, temperatureState.value);
      homekit_characteristic_notify(&humidityState, humidityState.value);
    }
    console.log()
      .bracket(F("aht10"))
      .section(F("temperature"), String(temp.temperature))
      .section(F("humidity"), String(humidity.relative_humidity));
  }
  // sgp
  if (sgp.IAQmeasure()) {
    vocDensityState.value.float_value = sgp.TVOC;
    airQualityState.value.uint8_value = parseAirQuality(sgp.TVOC);
    if (notify) {
      homekit_characteristic_notify(&vocDensityState, vocDensityState.value);
      homekit_characteristic_notify(&airQualityState, airQualityState.value);
    }
    console.log()
      .bracket(F("sgp30"))
      .section(F("TVOC"), String(sgp.TVOC))
      .section(F("eCO2"), String(sgp.eCO2));
  }
  // if (sgp.IAQmeasureRaw()) {
  //   console.log()
  //     .bracket(F("sgp30"))
  //     .section(F("RawH2"), String(sgp.rawH2))
  //     .section(F("RawEthanol"), String(sgp.rawEthanol));
  // }
}

void setup(void) {
  console.begin(115200);
  if (!LittleFS.begin()) {
    console.error()
      .bracket(F("fs"))
      .section(F("mount failed"));
  }

  builtinLed.setup();
  builtinLed.turnOn();

  // setup web
  webPortal.onRequestStart = []() { builtinLed.toggle(); };
  webPortal.onRequestEnd = []() { builtinLed.toggle(); };
  webPortal.onServiceGet = [](std::vector<KeyValueModel>& items) {
    items.push_back({ .key = F("Service"),     .value = VICTOR_ACCESSORY_SERVICE_NAME });
    items.push_back({ .key = F("Temperature"), .value = String(temperatureState.value.float_value) + F("C") });
    items.push_back({ .key = F("Humidity"),    .value = String(humidityState.value.float_value) + F("%") });
    items.push_back({ .key = F("Air Quality"), .value = parseStateName(airQualityState.value.uint8_value) });
    items.push_back({ .key = F("VOC Density"), .value = String(vocDensityState.value.float_value) });
    items.push_back({ .key = F("Paired"),      .value = parseYesNo(homekit_is_paired()) });
    items.push_back({ .key = F("Clients"),     .value = String(arduino_homekit_connected_clients_count()) });
  };
  webPortal.onServicePost = [](const String type) {
    if (type == F("reset")) {
      homekit_server_reset();
    }
  };
  webPortal.setup();

  // setup homekit server
  hostName = victorWifi.getHostName();
  accessoryName.value.string_value = const_cast<char*>(hostName.c_str());
  arduino_homekit_setup(&serverConfig);

  // setup sensor
  const auto model = airStorage.load();
  readInterval = model.repeat * 1000;
  Wire.begin(     // https://zhuanlan.zhihu.com/p/137568249
    model.sdaPin, // Inter-Integrated Circuit - Serial Data (I2C-SDA)
    model.sclPin  // Inter-Integrated Circuit - Serial Clock (I2C-SCL)
  );
  if (aht.begin()) {
    console.log()
      .bracket(F("aht10"))
      .section(F("begin"));
  }
  if (sgp.begin()) {
    console.log()
      .bracket(F("sgp30"))
      .section(F("begin"));
  }

  // setup wifi
  victorOTA.setup();
  victorWifi.setup();

  // done
  console.log()
    .bracket(F("setup"))
    .section(F("complete"));
}

void loop(void) {
  webPortal.loop();
  arduino_homekit_loop();
  // loop sensor
  const auto now = millis();
  if (now - lastRead > readInterval) {
    lastRead = now;
    measure(homekit_is_paired());
  }
}
