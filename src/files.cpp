#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Hash.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <base64.h>
#include <files.h>
#include <params.h>

#if defined(BEDROCK_DEBUG_FILES) && BEDROCK_DEBUG_FILES
#define BEDROCK_LOG
#endif
#include <log.h>

#define FILES_BUFFER_SIZE 256  // Size used for upload, signature.

BearSSL::PublicKey* signPubKey;
BearSSL::HashSHA256 hash;
BearSSL::SigningVerifier* sign;

void filesDeleteEverything() {
    bool isOk = LittleFS.format();
    BEDROCK_DEBUG("Formatted filesystem, with result %s", isOk ? "success" : "failure");
}

void listContents(Dir dir) {
    while (dir.next()) {
        if (dir.isDirectory()) {
            BEDROCK_INFO("%s/", dir.fileName().c_str());
            listContents(LittleFS.openDir(dir.fileName()));
        } else {
            BEDROCK_INFO("%s, %d bytes.", dir.fileName().c_str(), dir.fileSize());
        }
    }
}

void filesListContents() {
    listContents(LittleFS.openDir(""));
}

void filesInitialiseFilesystem(const char* const files[][2]) {
    signPubKey = new BearSSL::PublicKey(paramPublicKey);
    sign = new BearSSL::SigningVerifier(signPubKey);

    LittleFS.begin();

#if defined(BEDROCK_WIPE_FILESYSTEM) && BEDROCK_WIPE_FILESYSTEM
    filesDeleteEverything();
#endif

    if (!LittleFS.exists("/unverified")) {
        LittleFS.mkdir("/unverified");
    }

    char buffer[FILES_BUFFER_SIZE];
    int fileIndex = 0;
    File fsUploadFile;
    while (fileIndex >= 0) {
        strncpy_P(buffer, (char*)pgm_read_dword(&(files[fileIndex][0])), FILES_BUFFER_SIZE);
        if (strlen(buffer) == 0) {
            break;
        } else {
            if (!LittleFS.exists(buffer)) {
                BEDROCK_DEBUG("Creating file %d: %s", fileIndex, buffer);
                fsUploadFile = LittleFS.open(buffer, "w");
                int encodedLength = strlen_P((char*)pgm_read_dword(&(files[fileIndex][1])));
                for (int encodedIndex = 0; encodedIndex < encodedLength; encodedIndex += 4) {
                    uint32_t enc = 0;
                    memcpy_P(&enc, (files[fileIndex][1] + encodedIndex), 4);
                    uint32_t dec = b64_word_decode(enc);
                    char* pC = (char*)&dec;
                    for (int i = 2; i >= 0; i--) {
                        if (pC[i] != '\0') {
                            fsUploadFile.print(pC[i]);
                        }
                    }
                }
                fsUploadFile.close();
            }
            fileIndex++;
        }
    }

    // Remove firmware files from any failed updates.
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        if (dir.fileName().startsWith("firmware") && dir.fileName().endsWith(".bin")) {
            BEDROCK_DEBUG("Removing old firmware file.");
            LittleFS.remove(dir.fileName());
        }
    }
    // Remove all unverified files.
    dir = LittleFS.openDir("/unverified");
    while (dir.next()) {
        BEDROCK_DEBUG("Removing from unverified: %s", dir.fileName().c_str());
        bool isRemoved = LittleFS.remove(dir.fileName());
        BEDROCK_DEBUG("Remove result: %d", isRemoved);
    }
}

File fsUploadFile;  // a File object to temporarily store the received file

void filesOnUpload(const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
    if (!index) {
        BEDROCK_DEBUG("UPLOAD_FILE_START: %s", filename.c_str());
        hash.begin();
        fsUploadFile = LittleFS.open("/unverified/" + filename, "w");
    }

    if (fsUploadFile) {
        fsUploadFile.write(data, len);
        hash.add(data, len);
    }

    if (final) {
        if (fsUploadFile) {
            BEDROCK_DEBUG("Uploaded. Real name is %s", fsUploadFile.fullName());

            fsUploadFile.close();
            BEDROCK_DEBUG("UPLOAD_FILE_END: %s -> %u bytes", filename.c_str(), index + len);
            hash.end();
        }
    }
}

/**
 * Validates file signature and moves it from /unverified if correct
 */
int filesValidate(String filename, char* signature, int length) {
    int result;
    bool isOk = sign->verify(&hash, signature, length);
    if (!isOk) {
        BEDROCK_ERROR("Signature is wrong.");
        result = VERIFY_SIGNATURE;
        // TODO: Remove file.
    } else {
        BEDROCK_DEBUG("Signature is ok.");

        isOk = LittleFS.rename("/unverified/" + filename, "/" + filename);
        if (!isOk) {
            BEDROCK_ERROR("Failed to move file.");
            result = VERIFY_MV_FAILED;
            // TODO: Remove file.
        } else {
            result = VERIFY_OK;
        }
    }
    return result;
}

void filesApplyFirmware() {
    BEDROCK_INFO("Applying firmware.");
    Dir root = LittleFS.openDir("/");
    while (root.next()) {
        if (root.fileName().startsWith("firmware") && root.fileName().endsWith(".bin")) {
            File file = root.openFile("r");
            if (!file) {
                BEDROCK_ERROR("Failed to open file for reading.");
                return;
            }

            BEDROCK_INFO("Starting update..");

            size_t fileSize = file.size();

            if (!Update.begin(fileSize)) {
                BEDROCK_ERROR("Cannot do the update");
                return;
            };

            Update.writeStream(file);

            if (Update.end()) {
                BEDROCK_INFO("Successful update");
            } else {
                BEDROCK_ERROR("Error Occurred: %d", Update.getError());
                return;
            }

            file.close();
            LittleFS.remove(root.fileName());

            BEDROCK_INFO("Reset in 4 seconds...");
            delay(4000);

            ESP.restart();
        }
    }
}

bool filesSetKeyValue(const char* const key, const char* const value) {
    if ((strlen(key) >= BEDROCK_KEY_MAX_LENGTH - 1) || (strlen(value) >= BEDROCK_KEY_MAX_LENGTH - 1)) {
        BEDROCK_ERROR("Cannot store key-value. Too long: %s = %s", key, value);
        return false;
    }
    if ((strlen(key) == 0) || (strlen(value) == 0)) {
        BEDROCK_ERROR("Cannot store empty param. [%s] [%s]", key, value);
        return false;
    }
    if ((strchr(key, '\t') != NULL) || (strchr(value, '\t') != NULL)) {
        BEDROCK_ERROR("Key or Value contains illegal char: \\t");
        return false;
    }
    char buffer[2 * BEDROCK_KEY_MAX_LENGTH];
    snprintf(buffer, 2 * BEDROCK_KEY_MAX_LENGTH, "/config/%s", key);
    File configFile = LittleFS.open(buffer, "w");
    if (configFile) {
        BEDROCK_DEBUG("Config file opened for writing.");
        snprintf(buffer, 2 * BEDROCK_KEY_MAX_LENGTH, "%s", value);
        BEDROCK_DEBUG("Storing buffer: [%s]", buffer);
        int numWritten = configFile.write(buffer, strlen(buffer) + 1);
        configFile.close();
        BEDROCK_DEBUG("Wrote %d bytes.", numWritten);
        return true;
    } else {
        int err = configFile.getWriteError();
        BEDROCK_ERROR("Failed to open config file [%s] for writing: %d", buffer, err);
        return false;
    }
}

void filesGetKeyValue(const char* const key, char* value, int size) {
    value[0] = '\0';
    char buffer[2 * BEDROCK_KEY_MAX_LENGTH];
    snprintf(buffer, 2 * BEDROCK_KEY_MAX_LENGTH, "/config/%s", key);
    File configFile = LittleFS.open(buffer, "r");
    if (configFile) {
        BEDROCK_DEBUG("Config file is opened.");
        int numRead = configFile.read((uint8_t*)buffer, BEDROCK_KEY_MAX_LENGTH);
        configFile.close();
        buffer[numRead] = '\0';
        BEDROCK_DEBUG("Read %d bytes: [%s]", numRead, buffer);
        strncpy(value, buffer, size);
        BEDROCK_DEBUG("Found key-value: [%s] = [%s]", key, value);
    } else {
        BEDROCK_ERROR("Failed to open config file.");
    }
}
