#include "AQSensor.h"

namespace Victor::Components {

  AQSensor::AQSensor(AQSensorType type, QueryConfig* query) {
    _sgp30 = new SGP30();
    if (query->loopSeconds > 0) {
      _measureInterval = new IntervalOverAuto(query->loopSeconds * 1000);
    }
    if (query->resetHours > 0) {
      _resetInterval = new IntervalOverAuto(query->resetHours * 60 * 60 * 1000);
    }
  }

  AQSensor::~AQSensor() {
    if (_measureInterval != nullptr) {
      free(_measureInterval);
      _measureInterval = nullptr;
    }
    if (_resetInterval != nullptr) {
      free(_resetInterval);
      _resetInterval = nullptr;
    }
    if (_storeInterval != nullptr) {
      free(_storeInterval);
      _storeInterval = nullptr;
    }
    if (_sgp30 != nullptr) {
      delete _sgp30;
      _sgp30 = nullptr;
    }
  }

  bool AQSensor::begin(AQBaseline* baseline) {
    if (baseline->storeHours > 0) {
      _storeInterval = new IntervalOver(baseline->storeHours * 60 * 60 * 1000);
    }
    const auto found = _sgp30->begin();
    if (found) {
      _sgp30->initAirQuality();
      if (
        baseline->load &&
        baseline->co2 > 0 &&
        baseline->voc > 0
      ) {
        _sgp30->setBaseline(
          baseline->co2,
          baseline->voc
        );
        console.log()
          .bracket(F("load"))
          .section(F("co2"), String(baseline->co2))
          .section(F("voc"), String(baseline->voc));
      }
    }
    return found;
  }

  void AQSensor::reset() {
    _sgp30->generalCallReset();
  }

  MeasureState AQSensor::measure() {
    const auto now = millis();
    if (_measureInterval == nullptr || !_measureInterval->isOver(now)) {
      return MEASURE_SKIPPED;
    }
    if (_resetInterval != nullptr && _resetInterval->isOver(now)) {
      reset();
      return MEASURE_SKIPPED;
    }
    ESP.wdtFeed();
    const auto readSuccess = _sgp30->measureAirQuality() == SGP30_SUCCESS;
    if (
      readSuccess &&
      _storeInterval != nullptr &&
      _storeInterval->isOver(now)
    ) {
      if (_sgp30->getBaseline() == SGP30_SUCCESS) {
        _storeInterval->start(now);
        auto setting = climateStorage.load();
        setting->baseline->load = true; // once we have baseline generated, enable load for next boot
        setting->baseline->co2 = _sgp30->baselineCO2;
        setting->baseline->voc = _sgp30->baselineTVOC;
        climateStorage.save(setting);
        console.log()
          .bracket(F("store"))
          .section(F("co2"), String(_sgp30->baselineCO2))
          .section(F("voc"), String(_sgp30->baselineTVOC));
      }
    }
    return readSuccess ? MEASURE_SUCCESS : MEASURE_FAILED;
  }

  float AQSensor::getCO2() {
    return _sgp30->CO2;
  }

  float AQSensor::getTVOC() {
    return _sgp30->TVOC;
  }

  void AQSensor::setRelHumidity(float relHumidity, float temperature) {
    auto absHumidity = getAbsoluteHumidity(relHumidity, temperature);
    auto sensHumidity = doubleToFixedPoint(absHumidity);
    _sgp30->setHumidity(sensHumidity);
  }

  double AQSensor::getAbsoluteHumidity(float relativeHumidity, float temperature) {
    double eSat = 6.11 * pow(10.0, (7.5 * temperature / (237.7 + temperature)));
    double vaporPressure = (relativeHumidity * eSat) / 100; // millibars
    double absHumidity = 1000 * vaporPressure * 100 / ((temperature + 273) * 461.5); // Ideal gas law with unit conversions
    return absHumidity;
  }

  uint16_t AQSensor::doubleToFixedPoint(double number) {
    int power = 1 << 8;
    double number2 = number * power;
    uint16_t value = floor(number2 + 0.5);
    return value;
  }

} // namespace Victor::Components
