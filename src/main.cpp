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
DigitalInterruptButton* button;

HTSensor* ht;
AQSensor* aq;

String hostName;
String serialNumber;

ClimateModel climate;
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
  const auto htOk = ht->read();
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
    auto temperature = ht->getTemperature();
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
    auto humidity = ht->getHumidity();
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
    if (aq) {
      aq->setRelHumidity(temperature, humidity);
    }
  }
}

void measureAQ(bool notify) {
  const auto aqOk = aq->measure();
  if (airQualityActiveState.value.bool_value != aqOk) {
    airQualityActiveState.value.bool_value = aqOk;
    if (notify) {
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
        if (notify) {
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
      buttons.push_back({ .text = F("Reset-HT"), .value = F("ht") });  // Humidity/Temperature
    }
    if (aq) {
      buttons.push_back({ .text = F("Reset-AQ"), .value = F("aq") });  // Air Quality
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
  readInterval = i2c.loopSeconds * 1000;
  resetInterval = i2c.resetHours * 60 * 60 * 1000;
  Wire.begin(   // https://zhuanlan.zhihu.com/p/137568249
    i2c.sdaPin, // Inter-Integrated Circuit - Serial Data (I2C-SDA)
    i2c.sclPin  // Inter-Integrated Circuit - Serial Clock (I2C-SCL)
  );
  if (climate.htSensor != HTSensorOFF) {
    ht = new HTSensor(climate.htSensor);
    if (!ht->begin()) {
      console.error()
        .bracket(F("ht"))
        .section(F("notfound"));
    }
  }
  if (climate.aqSensor != AQSensorOFF) {
    aq = new AQSensor(climate.aqSensor);
    if (!aq->begin()) {
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
  if (button) {
    button->loop();
  }
  const auto isPaired = arduino_homekit_get_running_server()->paired;
  const auto lightSleep = victorWifi.isLightSleepMode() && isPaired;
  // loop sensor
  const auto now = millis();
  if (
    readInterval > 0 &&
    now - lastRead > readInterval
  ) {
    lastRead = now;
    if (!lightSleep) {
      builtinLed.turnOn();
    }
    ESP.wdtFeed();
    if (ht) {
      measureHT(isPaired);
    }
    if (aq) {
      measureAQ(isPaired);
    }
    if (!lightSleep) {
      builtinLed.turnOff();
    }
  }
  // reset sensor
  if (
    resetInterval > 0 &&
    now - lastReset > resetInterval
  ) {
    lastReset = now;
    if (ht) {
      ht->reset();
    }
    if (aq) {
      aq->reset();
    }
  }
  // sleep
  appMain->loop(lightSleep);
}
