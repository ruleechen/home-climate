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

#include "ClimateModels.h"
#include "ClimateStorage.h"

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

AHT10 aht10;
SGP30 sgp30;
VictorWeb webPortal(80);

String hostName;
unsigned long lastRead;
unsigned long readInterval;

String toYesNoName(bool state) {
  return state == true ? F("Yes") : F("No");
}

String toAirQualityName(uint8_t state) {
  return (
    state == AirQualityExcellent ? F("Excellent") :
    state == AirQualityGood      ? F("Good") :
    state == AirQualityFair      ? F("Fair") :
    state == AirQualityInferior  ? F("Inferior") :
    state == AirQualityPoor      ? F("Poor") : F("Unknown")
  );
}

AirQuality toAirQuality(float value) {
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

void measureHT(bool notify) {
  // aht10
  const auto htOk = aht10.readRawData() != AHT10_ERROR;
  if (temperatureActiveState.value.bool_value != htOk) {
    temperatureActiveState.value.bool_value = htOk;
    if (notify) {
      homekit_characteristic_notify(&temperatureActiveState, temperatureActiveState.value);
    }
  }
  if (humidityActiveState.value.bool_value != htOk) {
    humidityActiveState.value.bool_value = htOk;
    if (notify) {
      homekit_characteristic_notify(&humidityActiveState, humidityActiveState.value);
    }
  }
  if (htOk) {
    const auto temperature = aht10.readTemperature(AHT10_USE_READ_DATA);
    if (!isnanf(temperature)) {
      const auto temperatureFix = std::max<float>(0, std::min<float>(100, temperature)); // 0~100
      if (temperatureState.value.float_value != temperatureFix) {
        temperatureState.value.float_value = temperatureFix;
        if (notify) {
          homekit_characteristic_notify(&temperatureState, temperatureState.value);
        }
      }
    }
    const auto humidity = aht10.readHumidity(AHT10_USE_READ_DATA);
    if (!isnanf(humidity)) {
      const auto humidityFix = std::max<float>(0, std::min<float>(100, humidity)); // 0~100
      if (humidityState.value.float_value != humidityFix) {
        humidityState.value.float_value = humidityFix;
        if (notify) {
          homekit_characteristic_notify(&humidityState, humidityState.value);
        }
      }
    }
    console.log()
      .bracket(F("ht"))
      .section(F("h"), String(humidity))
      .section(F("t"), String(temperature));
    // write to sgp30
    sgp30.setRelHumidity(temperature, humidity);
  }
}

void measureAQ(bool notify) {
  // sgp30
  const auto aqOk = sgp30.measure(true);;
  if (airQualityActiveState.value.bool_value != aqOk) {
    airQualityActiveState.value.bool_value = aqOk;
    if (notify) {
      homekit_characteristic_notify(&airQualityActiveState, airQualityActiveState.value);
    }
  }
  if (aqOk) {
    const auto co2 = sgp30.getCO2();
    if (!isnan(co2)) {
      const auto co2Fix = std::max<float>(0, std::min<float>(100000, co2)); // 0~100000
      if (carbonDioxideState.value.float_value != co2Fix) {
        carbonDioxideState.value.float_value = co2Fix;
        if (notify) {
          homekit_characteristic_notify(&carbonDioxideState, carbonDioxideState.value);
        }
      }
    }
    const auto voc = sgp30.getTVOC();
    if (!isnan(voc)) {
      const auto vocFix = std::max<float>(0, std::min<float>(1000, voc)); // 0~1000
      if (vocDensityState.value.float_value != vocFix) {
        vocDensityState.value.float_value = vocFix;
        if (notify) {
          homekit_characteristic_notify(&vocDensityState, vocDensityState.value);
        }
        const auto quality = toAirQuality(vocFix);
        if (airQualityState.value.uint8_value != quality) {
          airQualityState.value.uint8_value = quality;
          if (notify) {
            homekit_characteristic_notify(&airQualityState, airQualityState.value);
          }
        }
      }
    }
    console.log()
      .bracket(F("aq"))
      .section(F("VOC"), String(voc))
      .section(F("CO2"), String(co2));
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
    items.push_back({ .key = F("Temperature"), .value = String(temperatureState.value.float_value) + F("°C") });
    items.push_back({ .key = F("Humidity"),    .value = String(humidityState.value.float_value) + F("%") });
    items.push_back({ .key = F("CO2 Level"),   .value = String(carbonDioxideState.value.float_value) });
    items.push_back({ .key = F("VOC Density"), .value = String(vocDensityState.value.float_value) });
    items.push_back({ .key = F("Air Quality"), .value = toAirQualityName(airQualityState.value.uint8_value) });
    items.push_back({ .key = F("Paired"),      .value = toYesNoName(homekit_is_paired()) });
    items.push_back({ .key = F("Clients"),     .value = String(arduino_homekit_connected_clients_count()) });
  };
  webPortal.onServicePost = [](const String& value) {
    if (value == F("reset")) {
      homekit_server_reset();
    }
  };
  webPortal.setup();

  // setup homekit server
  hostName = victorWifi.getHostName();
  accessoryName.value.string_value = const_cast<char*>(hostName.c_str());
  arduino_homekit_setup(&serverConfig);

  // setup sensor
  const auto model = climateStorage.load();
  readInterval = model.repeat * 1000;
  Wire.begin(     // https://zhuanlan.zhihu.com/p/137568249
    model.sdaPin, // Inter-Integrated Circuit - Serial Data (I2C-SDA)
    model.sclPin  // Inter-Integrated Circuit - Serial Clock (I2C-SCL)
  );
  if (aht10.begin()) {
    console.log()
      .bracket(F("aht10"))
      .section(F("begin"));
  }
  if (sgp30.begin()) {
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
  arduino_homekit_loop();
  webPortal.loop();
  // loop sensor
  const auto now = millis();
  if (now - lastRead > readInterval) {
    lastRead = now;
    builtinLed.turnOn();
    const auto notify = homekit_is_paired();
    ESP.wdtFeed();
    measureHT(notify);
    delay(10); // cool down + feed watchdog
    measureAQ(notify);
    builtinLed.turnOff();
  }
}
