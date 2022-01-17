#ifndef AirModels_h
#define AirModels_h

#include <Arduino.h>

namespace Victor {

  enum AirQuality {
    AirQualityUnknown = 0,
    AirQualityExcellent = 1,
    AirQualityGood = 2,
    AirQualityFair = 3,
    AirQualityInferior = 4,
    AirQualityPoor = 5,
  };

  struct AirSetting {
    uint8_t sdaPin = 4;
    uint8_t sclPin = 5;
    uint8_t repeat = 5;
  };

} // namespace Victor

#endif // AirModels_h
