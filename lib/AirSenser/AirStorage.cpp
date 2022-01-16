#include "AirStorage.h"

namespace Victor::Components {

  AirStorage::AirStorage() {
    _filePath = F("/air.json");
    _maxSize = 512;
  }

  void AirStorage::_serializeTo(const AirSetting& model, DynamicJsonDocument& doc) {
    doc[F("sda")] = model.sdaPin;
    doc[F("scl")] = model.sclPin;
    doc[F("repeat")] = model.repeat;
  }

  void AirStorage::_deserializeFrom(AirSetting& model, const DynamicJsonDocument& doc) {
    model.sdaPin = doc[F("sda")];
    model.sclPin = doc[F("scl")];
    model.repeat = doc[F("repeat")];
  }

  // global
  AirStorage airStorage;

} // namespace Victor::Components
