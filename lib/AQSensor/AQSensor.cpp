
#include "AQSensor.h"

namespace Victor::Components {

  AQSensor::AQSensor(AQSensorType type) {
    _sgp30 = new SGP30();
  }

  bool AQSensor::begin() {
    return _sgp30->begin();
  }

  void AQSensor::reset() {
    _sgp30->GenericReset();
  }

  void AQSensor::setRelHumidity(float T, float H) {
    _sgp30->setRelHumidity(T, H);
  }

  bool AQSensor::measure() {
    return _sgp30->measure(true);
  }

  float AQSensor::getCO2() {
    return _sgp30->getCO2();
  }

  float AQSensor::getTVOC() {
    return _sgp30->getTVOC();
  }

} // namespace Victor::Components
