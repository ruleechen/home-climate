#ifndef ClimateStorage_h
#define ClimateStorage_h

#include <FileStorage.h>

namespace Victor::Components {

  enum MeasureState {
    MEASURE_FAILED = 0,
    MEASURE_SUCCESS = 1,
    MEASURE_SKIPPED = 2,
  };

  enum HTSensorType {
    HT_SENSOR_OFF = 0,
    HT_SENSOR_AHT10 = 1,
    HT_SENSOR_SHT30 = 2,
  };

  enum AQSensorType {
    AQ_SENSOR_OFF = 0,
    AQ_SENSOR_SGP30 = 1,
  };

  struct QueryConfig {
    uint8_t loopSeconds = 0; // (0 ~ 256) How many seconds to read devices on i2c bus.
    uint8_t resetHours = 0;  // (0 ~ 256) How many hours to soft reset devices on i2c bus.
  };

  struct ReviseConfig {
    float humidity = 0;
    float temperature = 0;
    float co2 = 0;
    float voc = 0;
  };

  struct AQBaseline {
    bool load = false;
    uint8_t storeHours = 0; // (0 ~ 256)
    uint16_t co2 = 0; // (0 ~ 65535)
    uint16_t voc = 0; // (0 ~ 65535)
  };

  struct ClimateSetting {
    int8_t buttonPin = -1; // (-127 ~ 128)
    uint8_t buttonTrueValue = 0; // (0 ~ 256) LOW
    HTSensorType htSensor = HT_SENSOR_AHT10;
    AQSensorType aqSensor = AQ_SENSOR_SGP30;
    QueryConfig htQuery;
    QueryConfig aqQuery;
    ReviseConfig revise;
    AQBaseline baseline;
  };

  class ClimateStorage : public FileStorage<ClimateSetting> {
   public:
    ClimateStorage(const char* filePath = "/climate.json");

   protected:
    void _serializeTo(const ClimateSetting& model, DynamicJsonDocument& doc) override;
    void _deserializeFrom(ClimateSetting& model, const DynamicJsonDocument& doc) override;
  };

  // global
  extern ClimateStorage climateStorage;

} // namespace Victor::Components

#endif // ClimateStorage_h
