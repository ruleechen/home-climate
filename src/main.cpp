#include <Arduino.h>
#include <Wire.h>
#include <arduino_homekit_server.h>

#include <AppMain/AppMain.h>
#include <GlobalHelpers.h>
#include <I2cStorage/I2cStorage.h>
#include <Button/DigitalInterruptButton.h>

#include "ClimateStorage.h"
#include "HTSensor.h"
#include "AQSensor.h"

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

AppMain* appMain;
DigitalInterruptButton* button = nullptr;

ClimateSetting climate;
HTSensor* ht = nullptr;
AQSensor* aq = nullptr;

String hostName;
String serialNumber;

enum AirQuality {
  AIR_QUALITY_UNKNOWN = 0,
  AIR_QUALITY_EXCELLENT = 1,
  AIR_QUALITY_GOOD = 2,
  AIR_QUALITY_FAIR = 3,
  AIR_QUALITY_INFERIOR = 4,
  AIR_QUALITY_POOR = 5,
};

String toAirQualityName(uint8_t state) {
  return (
    state == AIR_QUALITY_EXCELLENT ? F("Excellent") :
    state == AIR_QUALITY_GOOD      ? F("Good") :
    state == AIR_QUALITY_FAIR      ? F("Fair") :
    state == AIR_QUALITY_INFERIOR  ? F("Inferior") :
    state == AIR_QUALITY_POOR      ? F("Poor") : F("Unknown")
  );
}

AirQuality toAirQuality(float value) {
  // 0 ~ 49
  if (value < 49) {
    return AIR_QUALITY_EXCELLENT;
  }
  // 50 ~ 99
  if (value < 99) {
    return AIR_QUALITY_GOOD;
  }
  // 100 ~ 299
  if (value < 399) {
    return AIR_QUALITY_FAIR;
  }
  // 300 ~ 599
  if (value < 599) {
    return AIR_QUALITY_INFERIOR;
  }
  // 600 ~ 1000
  return AIR_QUALITY_POOR;
}

void measureHT(bool isPaired, bool lightSleep) {
  const auto state = ht->measure();
  if (state == MEASURE_SKIPPED) { return; }
  const auto htOk = state == MEASURE_SUCCESS;
  if (temperatureActiveState.value.bool_value != htOk) {
    temperatureActiveState.value.bool_value = htOk;
    if (isPaired) {
      homekit_characteristic_notify(&temperatureActiveState, temperatureActiveState.value);
    }
  }
  if (humidityActiveState.value.bool_value != htOk) {
    humidityActiveState.value.bool_value = htOk;
    if (isPaired) {
      homekit_characteristic_notify(&humidityActiveState, humidityActiveState.value);
    }
  }
  if (htOk) {
    auto temperature = ht->getTemperature();
    if (!isnanf(temperature)) {
      temperature += climate.revise.temperature;
      const auto temperatureFix = std::max<float>(0, std::min<float>(100, temperature)); // 0~100
      if (temperatureState.value.float_value != temperatureFix) {
        temperatureState.value.float_value = temperatureFix;
        if (isPaired) {
          homekit_characteristic_notify(&temperatureState, temperatureState.value);
        }
      }
    }
    auto humidity = ht->getHumidity();
    if (!isnanf(humidity)) {
      humidity += climate.revise.humidity;
      const auto humidityFix = std::max<float>(0, std::min<float>(100, humidity)); // 0~100
      if (humidityState.value.float_value != humidityFix) {
        humidityState.value.float_value = humidityFix;
        if (isPaired) {
          homekit_characteristic_notify(&humidityState, humidityState.value);
        }
      }
    }
    console.log()
      .bracket(F("ht"))
      .section(F("h"), String(humidity))
      .section(F("t"), String(temperature));
    // write to AQ
    if (aq) {
      aq->setRelHumidity(humidity, temperature);
    }
  }
}

void measureAQ(bool isPaired, bool lightSleep) {
  const auto state = aq->measure();
  if (state == MEASURE_SKIPPED) { return; }
  const auto aqOk = state == MEASURE_SUCCESS;
  if (airQualityActiveState.value.bool_value != aqOk) {
    airQualityActiveState.value.bool_value = aqOk;
    if (isPaired) {
      homekit_characteristic_notify(&airQualityActiveState, airQualityActiveState.value);
    }
  }
  if (aqOk) {
    auto co2 = aq->getCO2();
    if (!isnan(co2)) {
      co2 += climate.revise.co2;
      const auto co2Fix = std::max<float>(0, std::min<float>(100000, co2)); // 0~100000
      if (carbonDioxideState.value.float_value != co2Fix) {
        carbonDioxideState.value.float_value = co2Fix;
        if (isPaired) {
          homekit_characteristic_notify(&carbonDioxideState, carbonDioxideState.value);
        }
      }
    }
    auto voc = aq->getTVOC();
    if (!isnan(voc)) {
      voc += climate.revise.voc;
      const auto vocFix = std::max<float>(0, std::min<float>(1000, voc)); // 0~1000
      if (vocDensityState.value.float_value != vocFix) {
        vocDensityState.value.float_value = vocFix;
        if (isPaired) {
          homekit_characteristic_notify(&vocDensityState, vocDensityState.value);
        }
      }
      const auto quality = toAirQuality(vocFix);
      if (airQualityState.value.uint8_value != quality) {
        airQualityState.value.uint8_value = quality;
        if (isPaired) {
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
  appMain = new AppMain();
  appMain->setup();

  // setup web
  appMain->webPortal->onServiceGet = [](std::vector<TextValueModel>& states, std::vector<TextValueModel>& buttons) {
    // states
    states.push_back({ .text = F("Service"),     .value = VICTOR_ACCESSORY_SERVICE_NAME });
    states.push_back({ .text = F("Temperature"), .value = String(temperatureState.value.float_value) + F("°C") });
    states.push_back({ .text = F("Humidity"),    .value = String(humidityState.value.float_value) + F("%") });
    states.push_back({ .text = F("CO2 Level"),   .value = String(carbonDioxideState.value.float_value) + F("ppm/㎥") });
    states.push_back({ .text = F("VOC Density"), .value = String(vocDensityState.value.float_value) + F("ppb/㎥") });
    states.push_back({ .text = F("Air Quality"), .value = toAirQualityName(airQualityState.value.uint8_value) });
    states.push_back({ .text = F("Paired"),      .value = GlobalHelpers::toYesNoName(homekit_is_paired()) });
    states.push_back({ .text = F("Clients"),     .value = String(arduino_homekit_connected_clients_count()) });
    // buttons
    buttons.push_back({ .text = F("UnPair"),   .value = F("UnPair") }); // UnPair HomeKit
    if (ht) {
      buttons.push_back({ .text = F("Reset HT"), .value = F("ht") });  // Humidity/Temperature
    }
    if (aq) {
      buttons.push_back({ .text = F("Reset AQ"), .value = F("aq") });  // Air Quality
    }
  };
  appMain->webPortal->onServicePost = [](const String& value) {
    if (value == F("UnPair")) {
      homekit_server_reset();
      ESP.restart();
    } else if (value == F("ht")) {
      ht->reset();
    } else if (value == F("aq")) {
      aq->reset();
    }
  };

  // setup homekit server
  hostName = victorWifi.getHostName();
  serialNumber = String(VICTOR_ACCESSORY_INFORMATION_SERIAL_NUMBER) + "/" + victorWifi.getHostId();
  accessoryName.value.string_value = const_cast<char*>(hostName.c_str());
  accessorySerialNumber.value.string_value = const_cast<char*>(serialNumber.c_str());
  arduino_homekit_setup(&serverConfig);

  // climate
  climate = climateStorage.load();
  if (climate.buttonPin > -1) {
    button = new DigitalInterruptButton(climate.buttonPin, climate.buttonTrueValue);
    button->onAction = [](const ButtonAction action) {
      console.log()
        .bracket(F("button"))
        .section(F("action"), String(action));
      if (action == ButtonActionPressed) {
        builtinLed.flash();
      } else if (action == ButtonActionDoublePressed) {
        builtinLed.flash(500);
        const auto enable = victorWifi.isLightSleepMode();
        victorWifi.enableAP(enable); // toggle enabling ap
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
  Wire.begin(   // https://zhuanlan.zhihu.com/p/137568249
    i2c.sdaPin, // Inter-Integrated Circuit - Serial Data (I2C-SDA)
    i2c.sclPin  // Inter-Integrated Circuit - Serial Clock (I2C-SCL)
  );
  if (climate.htSensor != HT_SENSOR_OFF) {
    ht = new HTSensor(climate.htSensor, climate.htQuery);
    if (!ht->begin()) {
      console.error()
        .bracket(F("ht"))
        .section(F("notfound"));
    }
  }
  if (climate.aqSensor != AQ_SENSOR_OFF) {
    aq = new AQSensor(climate.aqSensor, climate.aqQuery);
    if (!aq->begin(climate.baseline)) {
      console.error()
        .bracket(F("aq"))
        .section(F("notfound"));
    }
  }

  // done
  console.log()
    .bracket(F("setup"))
    .section(F("complete"));
}

void loop(void) {
  arduino_homekit_loop();
  if (button != nullptr) { button->loop(); }
  // loop sensor
  const auto isPaired = arduino_homekit_get_running_server()->paired;
  const auto lightSleep = victorWifi.isLightSleepMode() && isPaired;
  if (ht != nullptr) { measureHT(isPaired, lightSleep); }
  if (aq != nullptr) { measureAQ(isPaired, lightSleep); }
  // sleep
  appMain->loop(lightSleep);
}
