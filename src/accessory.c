#include <homekit/homekit.h>
#include <homekit/characteristics.h>

void onAccessoryIdentify(homekit_value_t value) {
  printf("accessory identify\n");
}

// format: string; max length 64
homekit_characteristic_t accessoryManufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER, VICTOR_ACCESSORY_INFORMATION_MANUFACTURER);
homekit_characteristic_t accessorySerialNumber = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, VICTOR_ACCESSORY_INFORMATION_SERIAL_NUMBER);
homekit_characteristic_t accessoryModel        = HOMEKIT_CHARACTERISTIC_(MODEL, VICTOR_ACCESSORY_INFORMATION_MODEL);
homekit_characteristic_t accessoryVersion      = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, VICTOR_FIRMWARE_VERSION);
homekit_characteristic_t accessoryIdentify     = HOMEKIT_CHARACTERISTIC_(IDENTIFY, onAccessoryIdentify);
homekit_characteristic_t accessoryName         = HOMEKIT_CHARACTERISTIC_(NAME, VICTOR_ACCESSORY_SERVICE_NAME); // change on setup
// info service
homekit_service_t informationService = HOMEKIT_SERVICE_(
  ACCESSORY_INFORMATION,
  .primary = true,
  .characteristics = (homekit_characteristic_t*[]) {
    &accessoryManufacturer,
    &accessorySerialNumber,
    &accessoryModel,
    &accessoryVersion,
    &accessoryIdentify,
    &accessoryName,
    NULL,
  },
);

// format: float; min 0, max 100, step 0.1, unit celsius
homekit_characteristic_t temperatureState = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
// format: bool; HAP section 9.96; true or false
homekit_characteristic_t temperatureActiveState = HOMEKIT_CHARACTERISTIC_(STATUS_ACTIVE, false);
// format: string; HAP section 9.62; maximum length 64
homekit_characteristic_t temperatureNameState = HOMEKIT_CHARACTERISTIC_(NAME, "Temperature");
// service
homekit_service_t temperatureService = HOMEKIT_SERVICE_(
  TEMPERATURE_SENSOR,
  .primary = true,
  .characteristics = (homekit_characteristic_t*[]) {
    &temperatureState,
    &temperatureActiveState,
    &temperatureNameState,
    NULL,
  },
);
// info service
homekit_service_t temperatureInformationService = HOMEKIT_SERVICE_(
  ACCESSORY_INFORMATION,
  .characteristics = (homekit_characteristic_t*[]) {
    HOMEKIT_CHARACTERISTIC(NAME, "Temperature"),
    HOMEKIT_CHARACTERISTIC(IDENTIFY, onAccessoryIdentify),
    NULL,
  },
);

// format: float; min 0, max 100, step 1
homekit_characteristic_t humidityState = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
// format: bool; HAP section 9.96; true or false
homekit_characteristic_t humidityActiveState = HOMEKIT_CHARACTERISTIC_(STATUS_ACTIVE, false);
// format: string; HAP section 9.62; maximum length 64
homekit_characteristic_t humidityNameState = HOMEKIT_CHARACTERISTIC_(NAME, "Humidity");
// service
homekit_service_t humidityService = HOMEKIT_SERVICE_(
  HUMIDITY_SENSOR,
  .primary = true,
  .characteristics = (homekit_characteristic_t*[]) {
    &humidityState,
    &humidityActiveState,
    &humidityNameState,
    NULL,
  },
);
// info service
homekit_service_t humidityInformationService = HOMEKIT_SERVICE_(
  ACCESSORY_INFORMATION,
  .characteristics = (homekit_characteristic_t*[]) {
    HOMEKIT_CHARACTERISTIC(NAME, "Humidity"),
    HOMEKIT_CHARACTERISTIC(IDENTIFY, onAccessoryIdentify),
    NULL,
  },
);

// format: float; HAP section 9.16; min 0, max 100000
homekit_characteristic_t carbonDioxideState = HOMEKIT_CHARACTERISTIC_(CARBON_DIOXIDE_LEVEL, 0);
// format: float; HAP section 9.126; min 0, max 1000, step 1
homekit_characteristic_t vocDensityState = HOMEKIT_CHARACTERISTIC_(VOC_DENSITY, 0);
// format: uint8; HAP section 9.118; 0 = Unknown, 1 = Excellent, 2 = Good, 3 = Fair, 4 = Inferior, 5 = Poor
homekit_characteristic_t airQualityState = HOMEKIT_CHARACTERISTIC_(AIR_QUALITY, 0);
// format: bool; HAP section 9.96; true or false
homekit_characteristic_t airQualityActiveState = HOMEKIT_CHARACTERISTIC_(STATUS_ACTIVE, false);
// format: string; HAP section 9.62; maximum length 64
homekit_characteristic_t airQualityNameState = HOMEKIT_CHARACTERISTIC_(NAME, "Air Quality");
// service
homekit_service_t airQualityService = HOMEKIT_SERVICE_(
  AIR_QUALITY_SENSOR,
  .primary = true,
  .characteristics = (homekit_characteristic_t*[]) {
    &carbonDioxideState,
    &vocDensityState,
    &airQualityState,
    &airQualityActiveState,
    &airQualityNameState,
    NULL,
  },
);
// info service
homekit_service_t airQualityInformationService = HOMEKIT_SERVICE_(
  ACCESSORY_INFORMATION,
  .characteristics = (homekit_characteristic_t*[]) {
    HOMEKIT_CHARACTERISTIC(NAME, "Air Quality"),
    HOMEKIT_CHARACTERISTIC(IDENTIFY, onAccessoryIdentify),
    NULL,
  },
);

homekit_accessory_t* accessories[] = {
  HOMEKIT_ACCESSORY(
    .id = 1,
    .category = homekit_accessory_category_bridge,
    .services = (homekit_service_t*[]) {
      &informationService,
      NULL,
    },
  ),
  HOMEKIT_ACCESSORY(
    .id = 2,
    .category = homekit_accessory_category_sensor,
    .services = (homekit_service_t*[]) {
      &temperatureInformationService,
      &temperatureService,
      NULL,
    },
  ),
  HOMEKIT_ACCESSORY(
    .id = 3,
    .category = homekit_accessory_category_sensor,
    .services = (homekit_service_t*[]) {
      &humidityInformationService,
      &humidityService,
      NULL,
    },
  ),
  HOMEKIT_ACCESSORY(
    .id = 4,
    .category = homekit_accessory_category_sensor,
    .services = (homekit_service_t*[]) {
      &airQualityInformationService,
      &airQualityService,
      NULL,
    },
  ),
  NULL,
};

homekit_server_config_t serverConfig = {
  .accessories = accessories,
  .password = VICTOR_ACCESSORY_SERVER_PASSWORD,
};
