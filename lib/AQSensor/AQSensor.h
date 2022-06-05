
#ifndef AQSensor_h
#define AQSensor_h

#include <Adafruit_SGP30.h>
#include "ClimateStorage.h"

namespace Victor::Components {

  class AQSensor {
   public:
    AQSensor(AQSensorType type);
    bool begin();
    void reset();
    void setRelHumidity(float temperature, float humidity);
    bool measure();
    float getCO2();
    float getTVOC();

   private:
    Adafruit_SGP30* _sgp30 = nullptr;
    uint8_t _measureCount = 0;
  };

} // namespace Victor::Components

#endif // AQSensor_h
