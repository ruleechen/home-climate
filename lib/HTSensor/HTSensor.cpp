#include "HTSensor.h"

namespace Victor::Components {

  HTSensor::HTSensor(HTSensorType type) {
    if (type == HTSensorAHT10) {
      _aht10 = new AHT10();
    } else if (type == HTSensorSHT30) {
      _sht30 = new SHT31();
    }
  }

  bool HTSensor::begin() {
    auto found = false;
    if (_aht10 != nullptr) {
      found = _aht10->begin();
    } else if (_sht30 != nullptr) {
      found = _sht30->begin();
    }
    return found;
  }

  void HTSensor::reset() {
    if (_aht10 != nullptr) {
      _aht10->softReset();
    } else if (_sht30 != nullptr) {
      _sht30->reset();
    }
  }

  bool HTSensor::read() {
    if (_aht10 != nullptr) {
      return _aht10->readRawData() != AHT10_ERROR;
    } else if (_sht30 != nullptr) {
      return _sht30->read();
    }
    return false;
  }

  float HTSensor::getHumidity() {
    if (_aht10 != nullptr) {
      return _aht10->readHumidity(AHT10_USE_READ_DATA);
    } else if (_sht30 != nullptr) {
      return _sht30->getHumidity();
    }
    return 0;
  }

  float HTSensor::getTemperature() {
    if (_aht10 != nullptr) {
      return _aht10->readTemperature(AHT10_USE_READ_DATA);
    } else if (_sht30 != nullptr) {
      return _sht30->getTemperature();
    }
    return 0;
  }

} // namespace Victor::Components
