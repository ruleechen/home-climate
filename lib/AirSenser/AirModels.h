#ifndef AirModels_h
#define AirModels_h

#include <Arduino.h>

namespace Victor {

  enum AirQuality {
    AirQualityUnknown = 0,
    AirQualityExcellent = 1, // 0 ~ 99
    AirQualityGood = 2,      // 100 ~ 199
    AirQualityFair = 3,      // 200 ~ 399
    AirQualityInferior = 4,  // 400 ~ 599
    AirQualityPoor = 5,      // 600 ~ 1000
  };

  struct AirSetting {
    uint8_t sdaPin = 4;
    uint8_t sclPin = 5;
    uint8_t repeat = 5;
  };

} // namespace Victor

#endif // AirModels_h
