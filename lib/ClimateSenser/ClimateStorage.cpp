#include "ClimateStorage.h"

namespace Victor::Components {

  ClimateStorage::ClimateStorage() {
    _filePath = F("/climate.json");
    _maxSize = 512;
  }

  void ClimateStorage::_serializeTo(const ClimateSetting& model, DynamicJsonDocument& doc) {
    doc[F("sda")] = model.sdaPin;
    doc[F("scl")] = model.sclPin;
    doc[F("repeat")] = model.repeat;
  }

  void ClimateStorage::_deserializeFrom(ClimateSetting& model, const DynamicJsonDocument& doc) {
    model.sdaPin = doc[F("sda")];
    model.sclPin = doc[F("scl")];
    model.repeat = doc[F("repeat")];
  }

  // global
  ClimateStorage climateStorage;

} // namespace Victor::Components
