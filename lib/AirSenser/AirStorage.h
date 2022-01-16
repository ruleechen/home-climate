#ifndef AirStorage_h
#define AirStorage_h

#include <FileStorage.h>
#include "AirModels.h"

namespace Victor::Components {
  class AirStorage : public FileStorage<AirSetting> {
   public:
    AirStorage();

   protected:
    void _serializeTo(const AirSetting& model, DynamicJsonDocument& doc) override;
    void _deserializeFrom(AirSetting& model, const DynamicJsonDocument& doc) override;
  };

  // global
  extern AirStorage airStorage;

} // namespace Victor::Components

#endif // AirStorage_h
