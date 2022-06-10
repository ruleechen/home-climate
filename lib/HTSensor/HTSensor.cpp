#include "HTSensor.h"

namespace Victor::Components {

  HTSensor::HTSensor(HTSensorType type, QueryConfig query) {
    if (type == HTSensorAHT10) {
      _aht10 = new AHT10();
    } else if (type == HTSensorSHT30) {
      _sht30 = new SHT31();
    }
    if (query.loopSeconds > 0) {
      _measureInterval = new IntervalOver(query.loopSeconds * 1000);
    }
    if (query.resetHours > 0) {
      _resetInterval = new IntervalOver(query.resetHours * 60 * 60 * 1000);
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

  MeasureState HTSensor::measure() {
    if (_measureInterval == nullptr || !_measureInterval->isOver()) {
      return MeasureSkipped;
    }
    if (_resetInterval != nullptr && _resetInterval->isOver()) {
      reset();
      return MeasureSkipped;
    }
    auto readSuccess = false;
    if (_aht10 != nullptr) {
      readSuccess = _aht10->readRawData() != AHT10_ERROR;
    } else if (_sht30 != nullptr) {
      readSuccess = _sht30->read();
    }
    return readSuccess ? MeasureSuccess : MeasureFailed;
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
