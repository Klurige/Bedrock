#ifndef __BEDROCK_BASE64_H__
#define __BEDROCK_BASE64_H__
#include <inttypes.h>

uint32_t b64_encode(char *output, const char *const input, int inputLen);
uint32_t b64_decode(char *output, const char *const input, int inputLen);

/**
 * decoded should contain 0x00NNNNNN, encoded will be 0xMMMMMMMM
 */
uint32_t b64_word_encode(uint32_t decoded);

/**
 * decoded should contain 0xMMMMMMMM, encoded will be 0x00NNNNNN
 */
uint32_t b64_word_decode(uint32_t encoded);

#endif  // __BEDROCK_BASE64_H__