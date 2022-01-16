#include <Arduino.h>
#include <arduino_homekit_server.h>

#include <Console.h>
#include <BuiltinLed.h>
#include <VictorOTA.h>
#include <VictorWifi.h>
#include <VictorWeb.h>

using namespace Victor;
using namespace Victor::Components;

extern "C" homekit_characteristic_t temperatureState;
extern "C" homekit_characteristic_t humidityState;
extern "C" homekit_characteristic_t accessoryName;
extern "C" homekit_server_config_t serverConfig;

VictorWeb webPortal(80);
String hostName;

String parsePercent(float state) {
  return String(state) + "%";
}

String parseYesNo(bool state) {
  return state == true ? F("Yes") : F("No");
}

void setup(void) {
  console.begin(115200);
  if (!LittleFS.begin()) {
    console.error()
      .bracket(F("fs"))
      .section(F("mount failed"));
  }

  builtinLed.setup();
  builtinLed.turnOn();

  // setup web
  webPortal.onRequestStart = []() { builtinLed.toggle(); };
  webPortal.onRequestEnd = []() { builtinLed.toggle(); };
  webPortal.onServiceGet = [](std::vector<KeyValueModel>& items) {
    items.push_back({ .key = F("Service"),     .value = VICTOR_ACCESSORY_SERVICE_NAME });
    items.push_back({ .key = F("Temperature"), .value = String(temperatureState.value.float_value) });
    items.push_back({ .key = F("Humidity"),    .value = parsePercent(humidityState.value.float_value) });
    items.push_back({ .key = F("Paired"),      .value = parseYesNo(homekit_is_paired()) });
    items.push_back({ .key = F("Clients"),     .value = String(arduino_homekit_connected_clients_count()) });
  };
  webPortal.onServicePost = [](const String type) {
    if (type == F("reset")) {
      homekit_server_reset();
    }
  };
  webPortal.setup();

  // setup sensor
  //

  // setup homekit server
  hostName = victorWifi.getHostName();
  accessoryName.value.string_value = const_cast<char*>(hostName.c_str());
  arduino_homekit_setup(&serverConfig);

  // setup wifi
  victorOTA.setup();
  victorWifi.setup();

  // done
  console.log()
    .bracket(F("setup"))
    .section(F("complete"));
}

void loop(void) {
  webPortal.loop();
  arduino_homekit_loop();
}
