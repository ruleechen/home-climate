#ifndef ClimateModels_h
#define ClimateModels_h

#include <Arduino.h>

namespace Victor {

  struct ClimateSetting {
    uint8_t sdaPin = 4;
    uint8_t sclPin = 5;
    uint8_t loopSeconds = 10;
    uint8_t resetHours = 24;
  };

  enum AirQuality {
    AirQualityUnknown = 0,
    AirQualityExcellent = 1,
    AirQualityGood = 2,
    AirQualityFair = 3,
    AirQualityInferior = 4,
    AirQualityPoor = 5,
  };

} // namespace Victor

#endif // ClimateModels_h
