#ifndef ClimateStorage_h
#define ClimateStorage_h

#include <FileStorage.h>

namespace Victor::Components {

  enum HTSensorType {
    HTSensorOFF = 0,
    HTSensorAHT10 = 1,
    HTSensorSHT30 = 2,
  };

  enum AQSensorType {
    AQSensorOFF = 0,
    AQSensorSGP30 = 1,
  };

  struct ReviseConfig {
    float humidity = 0;
    float temperature = 0;
    float co2 = 0;
    float voc = 0;
  };

  struct AQBaseline {
    bool load = false;
    uint8_t storeHours = 0; // 0 ~ 256
    uint16_t co2 = 0; // 0 ~ 65535
    uint16_t voc = 0; // 0 ~ 65535
  };

  struct ClimateSetting {
    int8_t buttonPin = -1; // -127 ~ 128
    uint8_t buttonTrueValue = 0; // LOW
    HTSensorType htSensor = HTSensorAHT10;
    AQSensorType aqSensor = AQSensorSGP30;
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
