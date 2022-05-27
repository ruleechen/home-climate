#include "ClimateStorage.h"

namespace Victor::Components {

  ClimateStorage::ClimateStorage(const char* filePath) : FileStorage(filePath) {
    _maxSize = 256;
  }

  void ClimateStorage::_serializeTo(const ClimateModel& model, DynamicJsonDocument& doc) {
    // sensors
    doc[F("hts")] = model.htSensor;
    doc[F("aqs")] = model.aqSensor;
    // button
    const JsonArray buttonArr = doc.createNestedArray(F("button"));
    buttonArr[0] = model.buttonPin;
    buttonArr[1] = model.buttonTrueValue;
    // revise
    const JsonObject reviseObj = doc.createNestedObject(F("revise"));
    reviseObj[F("h")] = model.revise.humidity;
    reviseObj[F("t")] = model.revise.temperature;
    reviseObj[F("co2")] = model.revise.co2;
    reviseObj[F("voc")] = model.revise.voc;
  }

  void ClimateStorage::_deserializeFrom(ClimateModel& model, const DynamicJsonDocument& doc) {
    // sensors
    model.htSensor = doc[F("hts")];
    model.aqSensor = doc[F("aqs")];
    // button
    const auto buttonArr = doc[F("button")];
    model.buttonPin = buttonArr[0];
    model.buttonTrueValue = buttonArr[1];
    // revise
    const auto reviseObj = doc[F("revise")];
    model.revise = {
      .humidity = reviseObj[F("h")],
      .temperature = reviseObj[F("t")],
      .co2 = reviseObj[F("co2")],
      .voc = reviseObj[F("voc")],
    };
  }

} // namespace Victor::Components
