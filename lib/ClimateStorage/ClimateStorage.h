#ifndef ClimateStorage_h
#define ClimateStorage_h

#include <FileStorage.h>

namespace Victor::Components {

  enum HTSensorType {
    HTSensorOFF = 0,
    HTSensorAHT10 = 1,
    HTSensorSHT30 = 2,
  };

  struct ReviseConfig {
    float humidity;
    float temperature;
    float co2;
    float voc;
  };

  struct ClimateModel {
    int8_t buttonPin = -1;
    uint8_t buttonTrueValue = 0; // LOW
    HTSensorType htSensor = HTSensorAHT10;
    ReviseConfig revise;
  };

  class ClimateStorage : public FileStorage<ClimateModel> {
   public:
    ClimateStorage(const char* filePath);

   protected:
    void _serializeTo(const ClimateModel& model, DynamicJsonDocument& doc) override;
    void _deserializeFrom(ClimateModel& model, const DynamicJsonDocument& doc) override;
  };

} // namespace Victor::Components

#endif // ClimateStorage_h
