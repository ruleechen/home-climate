#include "ClimateStorage.h"

namespace Victor::Components {

  ClimateStorage::ClimateStorage() {
    _filePath = "/climate.json";
    _maxSize = 512;
  }

  void ClimateStorage::_serializeTo(const ClimateSetting& model, DynamicJsonDocument& doc) {
    doc[F("sda")] = model.sdaPin;
    doc[F("scl")] = model.sclPin;
    doc[F("loop")] = model.loopSeconds;
    doc[F("reset")] = model.resetHours;
  }

  void ClimateStorage::_deserializeFrom(ClimateSetting& model, const DynamicJsonDocument& doc) {
    model.sdaPin = doc[F("sda")];
    model.sclPin = doc[F("scl")];
    model.loopSeconds = doc[F("loop")];
    model.resetHours = doc[F("reset")];
  }

  // global
  ClimateStorage climateStorage;

} // namespace Victor::Components
