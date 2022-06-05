#ifndef HTSensor_h
#define HTSensor_h

#include <AHT10.h>
#include <SHT31.h>
#include "ClimateStorage.h"

namespace Victor::Components {

  class HTSensor {
   public:
    HTSensor(HTSensorType type);
    bool begin();
    void reset();
    bool read();
    float getHumidity();
    float getTemperature();

   private:
    AHT10* _aht10 = nullptr;
    SHT31* _sht30 = nullptr;
  };

} // namespace Victor::Components

#endif // HTSensor_h
