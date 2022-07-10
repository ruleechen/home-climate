#ifndef AQSensor_h
#define AQSensor_h

#include <SparkFun_SGP30_Arduino_Library.h>
#include <Timer/IntervalOverAuto.h>
#include <Timer/IntervalOver.h>
#include "ClimateStorage.h"

namespace Victor::Components {

  class AQSensor {
   public:
    AQSensor(AQSensorType type, QueryConfig* query);
    bool begin(AQBaseline* baseline);
    void reset();
    MeasureState measure();
    float getCO2();
    float getTVOC();
    void setRelHumidity(float relHumidity, float temperature);
    static double getAbsoluteHumidity(float relativeHumidity, float temperature);
    static uint16_t doubleToFixedPoint(double number);

   private:
    IntervalOverAuto* _measureInterval = nullptr;
    IntervalOverAuto* _resetInterval = nullptr;
    IntervalOver* _storeInterval = nullptr;
    SGP30* _sgp30 = nullptr;
  };

} // namespace Victor::Components

#endif // AQSensor_h
