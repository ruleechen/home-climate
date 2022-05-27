
#ifndef AQSensor_h
#define AQSensor_h

#include <SGP30.h>
#include "ClimateStorage.h"

namespace Victor::Components {

  class AQSensor {
   public:
    AQSensor(AQSensorType type);
    bool begin();
    void reset();
    void setRelHumidity(float T, float H);
    bool measure();
    float getCO2();
    float getTVOC();

   private:
    SGP30* _sgp30 = nullptr;
  };

} // namespace Victor::Components

#endif // AQSensor_h
