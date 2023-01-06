#include <Arduino.h>
#include <app.h>
#include <bedrock.h>

#define BEDROCK_LOG
#include <bedrock_config.h>
#include <log.h>

// Last time the watchdog was kicked.
long watchdogTimeMs = 0L;

// Watchdog expires if more than this between kicks.
long watchdogTimeoutMs = 1500;

// Time between watchdog kicks.
int watchdogPeriodMs = 500;

bool watchdogSetup() {
    BEDROCK_DEBUG("Setting up watchdog.");
    watchdogTimeMs = millis();
    return true;
}

void watchdogLoop() {
    long currentTimeMs = millis();
    long timeDiff = currentTimeMs - watchdogTimeMs;
    if (timeDiff > watchdogTimeoutMs) {
        BEDROCK_WARN("Watchdog timeout: %ul ***************************", timeDiff);
    }
    watchdogTimeMs = currentTimeMs;
}

void setup() {
    bedrockInitialise(config_key_values, html_files, BEDROCK_CONFIG_KEY_VALUE_MAX_LENGTH);
    bedrockRegisterTask(watchdogSetup, watchdogLoop, watchdogPeriodMs);
    bedrockRegisterTask(appSetup, appLoop, appPeriodMs);
    bedrockSetup();
}

void loop() {
    bedrockLoop();
}
