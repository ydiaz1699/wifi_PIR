#include <ArduinoOTA.h>
#include "ota.h"
#include "logger.h"

void setupOTA() {
    ArduinoOTA.setHostname("bocina-esp");
    ArduinoOTA.onStart([]() { LOG_INFO("OTA Start"); });
    ArduinoOTA.onEnd([]() { LOG_INFO("OTA End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (total > 0) {
            LOG_DEBUG("OTA Progress: %u%%", (progress * 100) / total);
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        LOG_ERROR("OTA Error[%u]", error);
    });
    ArduinoOTA.begin();
    LOG_INFO("OTA listo");
}

void handleOTA() {
    ArduinoOTA.handle();
}
