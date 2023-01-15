#include <Arduino.h>
#include <TaskScheduler.h>
#include <bedrock.h>
#include <files.h>
#include <mqtt.h>
#include <params.h>
#include <server.h>

#if defined(BEDROCK_DEBUG_MISC) && BEDROCK_DEBUG_MISC
#define BEDROCK_LOG
#endif
#include <log.h>

typedef struct bedrockTask {
    bool (*taskSetup)();
    void (*taskLoop)();
    int taskPeriodMs;
    struct bedrockTask* pNext;
} bedrockTask;

bedrockTask* pBedrockTasks;

Scheduler runner;

void bedrockInitialise(const char* const params[][2], const char* const files[][2], int maxKeyValueLength) {
    if (maxKeyValueLength > BEDROCK_KEY_MAX_LENGTH) {
        BEDROCK_ERROR("Longest keyValue mismatch!");
    }

    // Do not change the order of the lines in this function!
    logSetup();                        // Must be first.
    paramsInitialise(params);          // Must be as early as possible.
    filesInitialiseFilesystem(files);  // Must be after params.

    // Finally, initialise a few internal tasks.
    bedrockRegisterTask(mqttSetup, mqttLoop, 100);
    bedrockRegisterTask(serverSetup, serverLoop, 1000);
}

void bedrockRegisterTask(bool (*taskSetup)(), void (*taskLoop)(), int taskPeriodMs) {
    bedrockTask* pNewTask = (bedrockTask*)malloc(sizeof(bedrockTask));
    pNewTask->pNext = NULL;
    pNewTask->taskSetup = taskSetup;
    pNewTask->taskLoop = taskLoop;
    pNewTask->taskPeriodMs = taskPeriodMs;

    if (pBedrockTasks == NULL) {
        pBedrockTasks = pNewTask;
    } else {
        bedrockTask* pThis = pBedrockTasks;
        while (pThis->pNext != NULL) {
            pThis = pThis->pNext;
        }
        pThis->pNext = pNewTask;
    }
}
void bedrockRegisterEvent(const char* const path, const char* const arg, const char* const (*callback)(const char* const param)) {
    serverRegister(path, arg, callback);
}

void bedrockSendEvent(const char* const label, const char* const message) {
    serverSendEvent(label, message);
}

boolean bedrockSubscribe(const char* const topic, void (*funptr)(const uint8_t* const, unsigned int)) {
    return mqttSubscribe(topic, funptr);
}
bool bedrockPublish(const char* const topic, const uint8_t* const payload, unsigned int length, boolean isRetain) {
    return mqttPublish(topic, payload, length, isRetain);
}

void bedrockSetup() {
    runner.init();

    while (pBedrockTasks != NULL) {
        BEDROCK_DEBUG("Registering task.");
        Task* pTask = new Task(pBedrockTasks->taskPeriodMs, TASK_FOREVER, pBedrockTasks->taskLoop, __null, false, pBedrockTasks->taskSetup, __null);
        runner.addTask(*pTask);
        pTask->enable();
        bedrockTask* pThis = pBedrockTasks;
        pBedrockTasks = pBedrockTasks->pNext;
        free(pThis);
    }
}

void bedrockLoop() {
    runner.execute();
}

long bedrockEpochTime() {
    return serverGetEpochTime();
}
