#include <Arduino.h>
#include <Wire.h>
#include <AHT10.h>
#include <SGP30.h>
#include <arduino_homekit_server.h>

#include <GlobalHelpers.h>
#include <Console.h>
#include <BuiltinLed.h>
#include <VictorOTA.h>
#include <VictorWifi.h>
#include <VictorWeb.h>

#include <I2cStorage/I2cStorage.h>
#include <Button/DigitalInputButton.h>
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
extern "C" homekit_characteristic_t accessorySerialNumber;
extern "C" homekit_server_config_t serverConfig;

AHT10 aht10;
SGP30 sgp30;
VictorWeb webPortal(80);

String hostName;
String serialNumber;

ClimateModel climate;
DigitalInputButton* button;
bool ledIndicator = false;
unsigned long lastRead;
unsigned long lastReset;
unsigned long readInterval;
unsigned long resetInterval;

enum AirQuality {
  AirQualityUnknown = 0,
  AirQualityExcellent = 1,
  AirQualityGood = 2,
  AirQualityFair = 3,
  AirQualityInferior = 4,
  AirQualityPoor = 5,
};

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
    auto temperature = aht10.readTemperature(AHT10_USE_READ_DATA);
    if (!isnanf(temperature)) {
      temperature += climate.revise.temperature;
      const auto temperatureFix = std::max<float>(0, std::min<float>(100, temperature)); // 0~100
      if (temperatureState.value.float_value != temperatureFix) {
        temperatureState.value.float_value = temperatureFix;
        if (notify) {
          homekit_characteristic_notify(&temperatureState, temperatureState.value);
        }
      }
    }
    auto humidity = aht10.readHumidity(AHT10_USE_READ_DATA);
    if (!isnanf(humidity)) {
      humidity += climate.revise.humidity;
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
    // write to AQ
    sgp30.setRelHumidity(temperature, humidity);
  }
}

void measureAQ(bool notify) {
  const auto aqOk = sgp30.measure(true);;
  if (airQualityActiveState.value.bool_value != aqOk) {
    airQualityActiveState.value.bool_value = aqOk;
    if (notify) {
      homekit_characteristic_notify(&airQualityActiveState, airQualityActiveState.value);
    }
  }
  if (aqOk) {
    auto co2 = sgp30.getCO2();
    if (!isnan(co2)) {
      co2 += climate.revise.co2;
      const auto co2Fix = std::max<float>(0, std::min<float>(100000, co2)); // 0~100000
      if (carbonDioxideState.value.float_value != co2Fix) {
        carbonDioxideState.value.float_value = co2Fix;
        if (notify) {
          homekit_characteristic_notify(&carbonDioxideState, carbonDioxideState.value);
        }
      }
    }
    auto voc = sgp30.getTVOC();
    if (!isnan(voc)) {
      voc += climate.revise.voc;
      const auto vocFix = std::max<float>(0, std::min<float>(1000, voc)); // 0~1000
      if (vocDensityState.value.float_value != vocFix) {
        vocDensityState.value.float_value = vocFix;
        if (notify) {
          homekit_characteristic_notify(&vocDensityState, vocDensityState.value);
        }
      }
      const auto quality = toAirQuality(vocFix);
      if (airQualityState.value.uint8_value != quality) {
        airQualityState.value.uint8_value = quality;
        if (notify) {
          homekit_characteristic_notify(&airQualityState, airQualityState.value);
        }
      }
    }
    console.log()
      .bracket(F("aq"))
      .section(F("voc"), String(voc))
      .section(F("co2"), String(co2));
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
  victorOTA.setup();
  victorWifi.setup();

  // setup web
  webPortal.onRequestStart = []() { builtinLed.toggle(); };
  webPortal.onRequestEnd = []() { builtinLed.toggle(); };
  webPortal.onServiceGet = [](std::vector<TextValueModel>& states, std::vector<TextValueModel>& buttons) {
    // states
    states.push_back({ .text = F("Service"),     .value = VICTOR_ACCESSORY_SERVICE_NAME });
    states.push_back({ .text = F("Temperature"), .value = String(temperatureState.value.float_value) + F("°C") });
    states.push_back({ .text = F("Humidity"),    .value = String(humidityState.value.float_value) + F("%") });
    states.push_back({ .text = F("CO2 Level"),   .value = String(carbonDioxideState.value.float_value) });
    states.push_back({ .text = F("VOC Density"), .value = String(vocDensityState.value.float_value) });
    states.push_back({ .text = F("Air Quality"), .value = toAirQualityName(airQualityState.value.uint8_value) });
    states.push_back({ .text = F("Paired"),      .value = GlobalHelpers::toYesNoName(homekit_is_paired()) });
    states.push_back({ .text = F("Clients"),     .value = String(arduino_homekit_connected_clients_count()) });
    // buttons
    buttons.push_back({ .text = F("UnPair"),   .value = F("UnPair") }); // UnPair HomeKit
    buttons.push_back({ .text = F("Reset-HT"), .value = F("ht") });  // Humidity/Temperature
    buttons.push_back({ .text = F("Reset-AQ"), .value = F("aq") });  // Air Quality
  };
  webPortal.onServicePost = [](const String& value) {
    if (value == F("UnPair")) {
      homekit_server_reset();
      ESP.restart();
    } else if (value == F("ht")) {
      aht10.softReset();
    } else if (value == F("aq")) {
      sgp30.GenericReset();
    }
  };
  webPortal.setup();

  // setup homekit server
  hostName = victorWifi.getHostName();
  serialNumber = String(VICTOR_ACCESSORY_INFORMATION_SERIAL_NUMBER) + "/" + victorWifi.getHostId();
  accessoryName.value.string_value = const_cast<char*>(hostName.c_str());
  accessorySerialNumber.value.string_value = const_cast<char*>(serialNumber.c_str());
  arduino_homekit_setup(&serverConfig);

  // climate
  const auto climateStorage = new ClimateStorage("/climate.json");
  climate = climateStorage->load();
  if (climate.buttonPin > -1) {
    button = new DigitalInputButton(climate.buttonPin, climate.buttonTrueValue);
    button->onAction = [](const ButtonAction action) {
      if (action == ButtonActionPressed) {
        builtinLed.flash();
      } else if (action == ButtonActionDoublePressed) {
        builtinLed.flash(200);
        ledIndicator = !ledIndicator;
      } else if (action == ButtonActionRestart) {
        ESP.restart();
      } else if (action == ButtonActionRestore) {
        homekit_server_reset();
        ESP.restart();
      }
    };
  }

  // setup sensor
  const auto i2cStorage = new I2cStorage("/i2c.json");
  const auto i2c = i2cStorage->load();
  readInterval = (i2c.loopSeconds > 0 ? i2c.loopSeconds : 10) * 1000;
  resetInterval = (i2c.resetHours > 0 ? i2c.resetHours : 24) * 60 * 60 * 1000;
  Wire.begin(   // https://zhuanlan.zhihu.com/p/137568249
    i2c.sdaPin, // Inter-Integrated Circuit - Serial Data (I2C-SDA)
    i2c.sclPin  // Inter-Integrated Circuit - Serial Clock (I2C-SCL)
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

  // done
  console.log()
    .bracket(F("setup"))
    .section(F("complete"));
}

void loop(void) {
  arduino_homekit_loop();
  webPortal.loop();
  if (button) {
    button->loop();
  }
  // loop sensor
  const auto now = millis();
  if (now - lastRead > readInterval) {
    lastRead = now;
    if (ledIndicator) {
      builtinLed.turnOn();
    }
    const auto notify = homekit_is_paired();
    ESP.wdtFeed();
    measureHT(notify);
    measureAQ(notify);
    if (ledIndicator) {
      builtinLed.turnOff();
    }
  }
  // reset sensor
  if (now - lastReset > resetInterval) {
    lastReset = now;
    aht10.softReset();
    sgp30.GenericReset();
  }
}
