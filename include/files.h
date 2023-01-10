#ifndef __BEDROCK_FILES_H__
#define __BEDROCK_FILES_H__

// Note that the error codes must match the array in upload.html.
#define VERIFY_OK 0
#define VERIFY_SIGNATURE_MISSING -1
#define VERIFY_CONTENTS_MISSING -2
#define VERIFY_SIGNATURE_LENGTH -3
#define VERIFY_SIGNATURE -4
#define VERIFY_MV_FAILED -5
#define VERIFY_FILE_MISSING -6

void filesInitialiseFilesystem(const char* const files[][2]);
void filesDeleteEverything();
void filesListContents();
void filesOnUpload(const String& filename, size_t index, uint8_t* data, size_t len, bool final);
void filesApplyFirmware();
int filesValidate(String filename, char* signature, int length);
void filesGetKeyValue(const char* const key, char* value, int size);
bool filesSetKeyValue(const char* const key, const char* const value);
#endif  // __BEDROCK_FILES_H__