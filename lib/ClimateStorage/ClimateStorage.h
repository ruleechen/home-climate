#ifndef ClimateStorage_h
#define ClimateStorage_h

#include <FileStorage.h>

namespace Victor::Components {

  enum MeasureState {
    MEASURE_FAILED  = 0,
    MEASURE_SUCCESS = 1,
    MEASURE_SKIPPED = 2,
  };

  enum HTSensorType {
    HT_SENSOR_OFF   = 0,
    HT_SENSOR_AHT10 = 1,
    HT_SENSOR_SHT30 = 2,
  };

  enum AQSensorType {
    AQ_SENSOR_OFF   = 0,
    AQ_SENSOR_SGP30 = 1,
  };

  struct QueryConfig {
    // seconds to read devices on i2c bus
    // 0 = disabled
    uint8_t loopSeconds = 0; // (0~255)
    // hours to soft reset devices on i2c bus
    // 0 = disabled
    uint8_t resetHours = 0;  // (0~255)
  };

  struct ReviseConfig {
    float humidity    = 0;
    float temperature = 0;
    float co2 = 0;
    float voc = 0;
  };

  struct AQBaseline {
    // load stored baseline on startup or not
    bool load = false;

    // hours to store baseline
    // 0 = disabled
    uint8_t storeHours = 0; // (0~255)

    // stored co2 baseline
    uint16_t co2 = 0; // (0~65535)

    // stored voc baseline
    uint16_t voc = 0; // (0~65535)
  };

  struct ClimateSetting {
    // button input pin
    // 0~127 = gpio
    //    -1 = disabled
    int8_t buttonPin = -1; // (-128~127)
    // 0 = LOW
    // 1 = HIGH
    uint8_t buttonTrueValue = 0; // (0~255)
    HTSensorType htSensor = HT_SENSOR_AHT10;
    AQSensorType aqSensor = AQ_SENSOR_SGP30;
    QueryConfig* htQuery = nullptr;
    QueryConfig* aqQuery = nullptr;
    ReviseConfig* revise = nullptr;
    AQBaseline* baseline = nullptr;
  };

  class ClimateStorage : public FileStorage<ClimateSetting> {
   public:
    ClimateStorage(const char* filePath = "/climate.json");

   protected:
    void _serializeTo(const ClimateSetting* model, DynamicJsonDocument& doc) override;
    void _deserializeFrom(ClimateSetting* model, const DynamicJsonDocument& doc) override;
  };

  // global
  extern ClimateStorage climateStorage;

} // namespace Victor::Components

#endif // ClimateStorage_h
