#ifndef ClimateStorage_h
#define ClimateStorage_h

#include <FileStorage.h>

namespace Victor::Components {

  struct ReviseConfig {
    float h;
    float t;
    float co2;
    float voc;
  };

  struct ClimateModel {
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
