#include <Arduino.h>
#include <bedrock.h>
#include <string.h>

#define BEDROCK_DEBUG
#include <log.h>

/**
 * @brief Testing mqtt and Home Assistant
 * http:
 * Set count direction with "GET count_dir?state=".
 * Get count direction with "GET count_dir".
 * sse:
 * "measurement" publishes an int every few seconds. The value increases or decreases according to count_dir.
 * "led_state" publishes changes to led. on or off.
 *
 * See app.html for mqtt publish and subscribe strings, as well as sse examples.
 *
 */

int appPeriodMs = 5000;
static int sample = 0;
static bool isCountUp = false;

static uint8_t payload[32];

/*
  Things coming in and going out.
  The string stored in memory starts with a slash, because
  it's needed when registering sse event receivers.
*/
// Measurement, an int.
static const char *const measurement = "/measurement";
static const char *const event_measurement = &measurement[1];
static const char *const mqtt_publish_measurement = &measurement[1];

// Led, on or off.
static const char *const led_state = "/led_state";
static const char *const event_led = &led_state[1];
static const char *const mqtt_publish_led_state = &led_state[1];
static const char *const mqtt_subscribe_led_set = "led_set";
static const char *const payload_led_on = "on";
static const int payload_led_on_length = 2;
static const char *const payload_led_off = "off";
static const int payload_led_off_length = 3;

// Count direction (measurement), up or down.
static const char *const count_dir_state = "/count_direction";
static const char *const event_count_dir = &count_dir_state[1];
static const char *const mqtt_publish_count_dir = &count_dir_state[1];
static const char *const mqtt_subscribe_count_dir = "count_dir_set";
static const char *const payload_count_up = "up";
static const int payload_count_up_length = 2;
static const char *const payload_count_down = "down";
static const int payload_count_down_length = 4;

bool isLedOn() {
    int isHigh = digitalRead(LED_BUILTIN);
    return (isHigh) ? false : true;
}

/**
 * @brief Toggle the LED
 *
 * @param payload on or off
 * @param length only passed in from mqtt. Do not use.
 * @return const char* const New state. only used by sse.
 */
const char *const led_toggle(const uint8_t *const payload, unsigned int length) {
    BEDROCK_DEBUG("Toggle the LED: {%s}", (char *)payload);
    if (strncmp(payload_led_on, (char *)payload, payload_led_on_length) == 0) {
        digitalWrite(LED_BUILTIN, LOW);
        bedrockSendEvent(event_led, payload_led_on);
        bedrockPublish(mqtt_publish_led_state, (uint8_t *)payload_led_on, payload_led_on_length, true);
        return payload_led_on;
    } else if (strncmp(payload_led_off, (char *)payload, payload_led_off_length) == 0) {
        digitalWrite(LED_BUILTIN, HIGH);
        bedrockSendEvent(event_led, payload_led_off);
        bedrockPublish(mqtt_publish_led_state, (uint8_t *)payload_led_off, payload_led_off_length, true);
        return payload_led_off;
    } else {
        BEDROCK_ERROR("unknown payload.");
        bool isOn = isLedOn();
        return (isOn) ? payload_led_on : payload_led_off;
    }
}

void mqtt_led_toggle(const uint8_t *const payload, unsigned int length) {
    led_toggle(payload, length);
}

const char *const sse_led_toggle(const char *const param) {
    return led_toggle((uint8_t *)param, -1);
}

/**
 * @brief Change direction of counter.
 *
 * @param payload up or down
 * @param length only passed in from mqtt. Do not use.
 * @return const char* const New state. only used by sse.
 */
const char *const count_dir_toggle(const uint8_t *const payload, unsigned int length) {
    BEDROCK_DEBUG("Toggle the Direction: {%s}", (char *)payload);
    if (strncmp(payload_count_up, (char *)payload, payload_count_up_length) == 0) {
        isCountUp = true;
        bedrockSendEvent(event_count_dir, payload_count_up);
        bedrockPublish(mqtt_publish_count_dir, (uint8_t *)payload_count_up, payload_count_up_length, true);
        return payload_count_up;
    } else if (strncmp(payload_count_down, (char *)payload, payload_count_down_length) == 0) {
        isCountUp = false;
        bedrockSendEvent(event_count_dir, payload_count_down);
        bedrockPublish(mqtt_publish_count_dir, (uint8_t *)payload_count_down, payload_count_down_length, true);
        return payload_count_down;
    } else {
        BEDROCK_ERROR("unknown payload.");
        return (isCountUp) ? payload_count_up : payload_count_down;
    }
}

void mqtt_count_dir_toggle(const uint8_t *const payload, unsigned int length) {
    uint8_t param[32];
    memcpy(param, payload, length);
    count_dir_toggle(param, length);
}

const char *const sse_count_dir_toggle(const char *const param) {
    return count_dir_toggle((uint8_t *)param, -1);
}

static char measurementPayload[32];

const char *const getMeasurement(const char *const param) {
    snprintf((char *)measurementPayload, 32, "%d", sample);
    return measurementPayload;
}

void appLoop() {
    if (isCountUp) {
        if (sample < 1100) {
            sample++;
        }
    } else {
        if (sample > 900) {
            sample--;
        }
    }

    const char *const measurement = getMeasurement(NULL);
    BEDROCK_DEBUG("Sending event: [%s] -> {%s}", event_measurement, measurement);
    bedrockSendEvent(event_measurement, measurement);

    int length = strlen(measurement);
    BEDROCK_DEBUG("Publishing: [%s] -> {%s} %d bytes", mqtt_publish_measurement, measurement, length);
    bedrockPublish(mqtt_publish_measurement, (const uint8_t *)measurement, length, false);
}

bool appSetup() {
    pinMode(LED_BUILTIN, OUTPUT);

    BEDROCK_DEBUG("Register server events and mqtt subscriptions.");
    bedrockRegisterEvent(measurement, NULL, getMeasurement);
    bedrockRegisterEvent(led_state, "value", sse_led_toggle);
    bedrockRegisterEvent(count_dir_state, "value", sse_count_dir_toggle);

    bedrockSubscribe(mqtt_subscribe_led_set, mqtt_led_toggle);
    bedrockSubscribe(mqtt_subscribe_count_dir, mqtt_count_dir_toggle);

    // Initial states
    led_toggle((uint8_t *)payload_led_off, payload_led_off_length);
    count_dir_toggle((uint8_t *)payload_count_up, payload_count_up_length);
    sample = 1000;

    return true;
}
