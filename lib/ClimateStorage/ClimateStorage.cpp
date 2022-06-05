#include "ClimateStorage.h"

namespace Victor::Components {

  ClimateStorage::ClimateStorage(const char* filePath) : FileStorage(filePath) {
    _maxSize = 512;
  }

  void ClimateStorage::_serializeTo(const ClimateSetting& model, DynamicJsonDocument& doc) {
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
    // baseline
    const JsonObject baselineObj = doc.createNestedObject(F("baseline"));
    baselineObj[F("load")] = model.baseline.load ? 1 : 0;
    baselineObj[F("store")] = model.baseline.storeHours;
    baselineObj[F("co2")] = model.baseline.co2;
    baselineObj[F("voc")] = model.baseline.voc;
  }

  void ClimateStorage::_deserializeFrom(ClimateSetting& model, const DynamicJsonDocument& doc) {
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
    // baseline
    const auto baselineObj = doc[F("baseline")];
    model.baseline = {
      .load = baselineObj[F("load")] == 1,
      .storeHours = baselineObj[F("store")],
      .co2 = baselineObj[F("co2")],
      .voc = baselineObj[F("voc")],
    };
  }

  // global
  ClimateStorage climateStorage;

} // namespace Victor::Components
