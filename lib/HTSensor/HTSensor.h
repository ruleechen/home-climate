#ifndef HTSensor_h
#define HTSensor_h

#include <AHT10.h>
#include <SHT31.h>
#include <IntervalOver.h>
#include "ClimateStorage.h"

namespace Victor::Components {

  class HTSensor {
   public:
    HTSensor(HTSensorType type, QueryConfig query);
    bool begin();
    void reset();
    MeasureState measure();
    float getHumidity();
    float getTemperature();

   private:
    IntervalOver* _measureInterval = nullptr;
    IntervalOver* _resetInterval = nullptr;
    AHT10* _aht10 = nullptr;
    SHT31* _sht30 = nullptr;
  };

} // namespace Victor::Components

#endif // HTSensor_h
