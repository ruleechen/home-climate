#ifndef AQSensor_h
#define AQSensor_h

#include <SparkFun_SGP30_Arduino_Library.h>
#include "ClimateStorage.h"

namespace Victor::Components {

  class AQSensor {
   public:
    AQSensor(AQSensorType type);
    bool begin(AQBaseline baseline);
    void reset();
    void setRelHumidity(float relHumidity, float temperature);
    bool measure();
    float getCO2();
    float getTVOC();
    static double getAbsoluteHumidity(float relativeHumidity, float temperature);
    static uint16_t doubleToFixedPoint(double number);

   private:
    SGP30* _sgp30 = nullptr;
    unsigned long _storeInterval = 0;
    unsigned long _storeTimestamp = 0;
  };

} // namespace Victor::Components

#endif // AQSensor_h
