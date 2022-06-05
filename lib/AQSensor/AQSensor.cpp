
#include "AQSensor.h"

namespace Victor::Components {

  AQSensor::AQSensor(AQSensorType type) {
    _sgp30 = new Adafruit_SGP30();
  }

  bool AQSensor::begin() {
    const auto found = _sgp30->begin();
    if (found) {
      const auto setting = climateStorage.load();
      if (
        setting.baseline.load &&
        setting.baseline.co2 > 0 &&
        setting.baseline.voc > 0
      ) {
        _sgp30->setIAQBaseline(
          setting.baseline.co2,
          setting.baseline.voc
        );
        console.log()
          .bracket(F("load"))
          .section(F("co2"), String(setting.baseline.co2))
          .section(F("voc"), String(setting.baseline.voc));
      }
    }
    return found;
  }

  void AQSensor::reset() {
    _sgp30->softReset();
  }

  void AQSensor::setRelHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    _sgp30->setHumidity(absoluteHumidityScaled);
  }

  bool AQSensor::measure() {
    const auto read = _sgp30->IAQmeasure();
    if (read) {
      _measureCount++;
      if (_measureCount >= 12) {
        uint16_t co2, voc;
        if (_sgp30->getIAQBaseline(&co2, &voc)) {
          _measureCount = 0;
          auto setting = climateStorage.load();
          if (setting.baseline.store) {
            setting.baseline.co2 = co2;
            setting.baseline.voc = voc;
            climateStorage.save(setting);
            console.log()
              .bracket(F("store"))
              .section(F("co2"), String(co2))
              .section(F("voc"), String(voc));
          }
        }
      }
    }
    return read;
  }

  float AQSensor::getCO2() {
    return _sgp30->eCO2;
  }

  float AQSensor::getTVOC() {
    return _sgp30->TVOC;
  }

} // namespace Victor::Components
