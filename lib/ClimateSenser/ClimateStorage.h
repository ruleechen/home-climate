#ifndef ClimateStorage_h
#define ClimateStorage_h

#include <FileStorage.h>
#include "ClimateModels.h"

namespace Victor::Components {
  class ClimateStorage : public FileStorage<ClimateSetting> {
   public:
    ClimateStorage();

   protected:
    void _serializeTo(const ClimateSetting& model, DynamicJsonDocument& doc) override;
    void _deserializeFrom(ClimateSetting& model, const DynamicJsonDocument& doc) override;
  };

  // global
  extern ClimateStorage climateStorage;

} // namespace Victor::Components

#endif // ClimateStorage_h
