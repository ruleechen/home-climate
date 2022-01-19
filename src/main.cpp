#include <Arduino.h>
#include <Wire.h>
#include <AHT10.h>
#include <SGP30.h>
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

// temperature
extern "C" homekit_characteristic_t temperatureState;
extern "C" homekit_characteristic_t temperatureActiveState;
// humidity
extern "C" homekit_characteristic_t humidityState;
extern "C" homekit_characteristic_t humidityActiveState;
// air quality
extern "C" homekit_characteristic_t carbonDioxideState;
extern "C" homekit_characteristic_t vocDensityState;
extern "C" homekit_characteristic_t airQualityState;
extern "C" homekit_characteristic_t airQualityActiveState;
// others
extern "C" homekit_characteristic_t accessoryName;
extern "C" homekit_server_config_t serverConfig;

AHT10 aht;
SGP30 sgp;
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

String parseYesNo(bool state) {
  return state == true ? F("Yes") : F("No");
}

AirQuality parseAirQuality(float value) {
  // 0 ~ 49
  if (value < 49) {
    return AirQualityExcellent;
  }
  // 50 ~ 99
  if (value < 99) {
    return AirQualityGood;
  }
  // 100 ~ 299
  if (value < 399) {
    return AirQualityFair;
  }
  // 300 ~ 599
  if (value < 599) {
    return AirQualityInferior;
  }
  // 600 ~ 1000
  return AirQualityPoor;
}

void measure(bool notify) {
  builtinLed.flash();
  // aht
  const auto ahtOk = aht.readRawData() != AHT10_ERROR;
  if (temperatureActiveState.value.bool_value != ahtOk) {
    temperatureActiveState.value.bool_value = ahtOk;
    if (notify) {
      homekit_characteristic_notify(&temperatureActiveState, temperatureActiveState.value);
    }
  }
  if (humidityActiveState.value.bool_value != ahtOk) {
    humidityActiveState.value.bool_value = ahtOk;
    if (notify) {
      homekit_characteristic_notify(&humidityActiveState, humidityActiveState.value);
    }
  }
  if (ahtOk) {
    const auto temperature = aht.readTemperature(AHT10_USE_READ_DATA);
    if (temperatureState.value.float_value != temperature) {
      temperatureState.value.float_value = temperature;
      if (notify) {
        homekit_characteristic_notify(&temperatureState, temperatureState.value);
      }
    }
    const auto humidity = aht.readHumidity(AHT10_USE_READ_DATA);
    if (humidityState.value.float_value != humidity) {
      humidityState.value.float_value = humidity;
      if (notify) {
        homekit_characteristic_notify(&humidityState, humidityState.value);
      }
    }
    console.log()
      .bracket(F("aht10"))
      .section(F("temperature"), String(temperature))
      .section(F("humidity"), String(humidity));
    // write to sgp
    sgp.setRelHumidity(temperature, humidity);
  }
  // sgp
  const auto sgpOk = sgp.measure(true);;
  if (airQualityActiveState.value.bool_value != sgpOk) {
    airQualityActiveState.value.bool_value = sgpOk;
    if (notify) {
      homekit_characteristic_notify(&airQualityActiveState, airQualityActiveState.value);
    }
  }
  if (sgpOk) {
    const auto co2 = sgp.getCO2();
    if (carbonDioxideState.value.float_value != co2) {
      carbonDioxideState.value.float_value = co2;
      if (notify) {
        homekit_characteristic_notify(&carbonDioxideState, carbonDioxideState.value);
      }
    }
    const auto tvoc = sgp.getTVOC();
    if (vocDensityState.value.float_value != tvoc) {
      vocDensityState.value.float_value = tvoc;
      if (notify) {
        homekit_characteristic_notify(&vocDensityState, vocDensityState.value);
      }
    }
    const auto quality = parseAirQuality(tvoc);
    if (airQualityState.value.uint8_value != quality) {
      airQualityState.value.uint8_value = quality;
      if (notify) {
        homekit_characteristic_notify(&airQualityState, airQualityState.value);
      }
    }
    console.log()
      .bracket(F("sgp30"))
      .section(F("TVOC"), String(tvoc))
      .section(F("eCO2"), String(co2));
  }
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
    items.push_back({ .key = F("CO2 Level"),   .value = String(carbonDioxideState.value.float_value) });
    items.push_back({ .key = F("VOC Density"), .value = String(vocDensityState.value.float_value) });
    items.push_back({ .key = F("Air Quality"), .value = parseStateName(airQualityState.value.uint8_value) });
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
