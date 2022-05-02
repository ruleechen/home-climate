#include "ClimateStorage.h"

namespace Victor::Components {

  ClimateStorage::ClimateStorage(const char* filePath) {
    _filePath = filePath;
    _maxSize = 512;
  }

  void ClimateStorage::_serializeTo(const ClimateModel& model, DynamicJsonDocument& doc) {
    const JsonObject reviseObj = doc.createNestedObject(F("revise"));
    reviseObj[F("h")] = model.revise.h;
    reviseObj[F("t")] = model.revise.t;
    reviseObj[F("co2")] = model.revise.co2;
    reviseObj[F("voc")] = model.revise.voc;
  }

  void ClimateStorage::_deserializeFrom(ClimateModel& model, const DynamicJsonDocument& doc) {
    const auto reviseObj = doc[F("revise")];
    model.revise = {
      .h = reviseObj[F("h")],
      .t = reviseObj[F("t")],
      .co2 = reviseObj[F("co2")],
      .voc = reviseObj[F("voc")],
    };
  }

} // namespace Victor::Components
