#include <base64.h>
#include <inttypes.h>

const char b64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

/* 'Private' declarations */
inline void a3_to_a4(unsigned char *a4, unsigned char *a3);
inline void a4_to_a3(unsigned char *a3, unsigned char *a4);
inline int32_t b64_lookup(char c);

/**
 * decoded should contain 0x00NNNNNN, encoded will be 0xMMMMMMMM
 */
uint32_t b64_word_encode(uint32_t decoded) {
    return 0;
}

/**
 * encoded should contain 0xMMMMMMMM, encoded will be 0x00NNNNNN
 */
uint32_t b64_word_decode(uint32_t encoded) {
    uint32_t decoded = 0;
    char index;
    index = (char)((encoded & 0xFF000000) >> 24);
    uint32_t value = b64_lookup(index);
    decoded |= value;
    index = (char)((encoded & 0x00FF0000) >> 16);
    value = b64_lookup(index);
    decoded |= value << 6;
    index = (char)((encoded & 0x0000FF00) >> 8);
    value = b64_lookup(index);
    decoded |= value << 12;
    index = (char)(encoded & 0x000000FF);
    value = b64_lookup(index);
    decoded |= value << 18;

    return decoded;
}

uint32_t b64_encode(char *output, const char *const input, int inputLen) {
    int i = 0, j = 0;
    int encLen = 0;
    unsigned char a3[3];
    unsigned char a4[4];
    int inputIndex = 0;

    while (inputLen--) {
        a3[i++] = input[inputIndex++];
        if (i == 3) {
            a3_to_a4(a4, a3);

            for (i = 0; i < 4; i++) {
                output[encLen++] = b64_alphabet[a4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            a3[j] = '\0';
        }

        a3_to_a4(a4, a3);

        for (j = 0; j < i + 1; j++) {
            output[encLen++] = b64_alphabet[a4[j]];
        }

        while ((i++ < 3)) {
            output[encLen++] = '=';
        }
    }
    output[encLen] = '\0';
    return encLen;
}

uint32_t b64_decode(char *output, const char *const input, int inputLen) {
    int i = 0, j = 0;
    int decLen = 0;
    unsigned char a3[3];
    unsigned char a4[4];
    int inputIndex = 0;

    while (inputLen--) {
        if (input[inputIndex] == '=') {
            break;
        }
        if (input[inputIndex] == '\n') {
            inputIndex++;
            continue;
        }

        a4[i++] = input[inputIndex++];
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                a4[i] = b64_lookup(a4[i]);
            }

            a4_to_a3(a3, a4);

            for (i = 0; i < 3; i++) {
                output[decLen++] = a3[i];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++) {
            a4[j] = '\0';
        }

        for (j = 0; j < 4; j++) {
            a4[j] = b64_lookup(a4[j]);
        }

        a4_to_a3(a3, a4);

        for (j = 0; j < i - 1; j++) {
            output[decLen++] = a3[j];
        }
    }
    output[decLen] = '\0';
    return decLen;
}

int b64_enc_len(int plainLen) {
    int n = plainLen;
    return (n + 2 - ((n + 2) % 3)) / 3 * 4;
}

int b64_dec_len(char *input, int inputLen) {
    int i = 0;
    int numEq = 0;
    for (i = inputLen - 1; input[i] == '='; i--) {
        numEq++;
    }

    return ((6 * inputLen) / 8) - numEq;
}

inline void a3_to_a4(unsigned char *a4, unsigned char *a3) {
    a4[0] = (a3[0] & 0xfc) >> 2;
    a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
    a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
    a4[3] = (a3[2] & 0x3f);
}

inline void a4_to_a3(unsigned char *a3, unsigned char *a4) {
    a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
    a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
    a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
}

inline int32_t b64_lookup(char c) {
    int32_t i;
    for (i = 0; i < 64; i++) {
        if (b64_alphabet[i] == c) {
            return i;
        }
    }

    if (c == '=') {
        return 0;
    }

    return -1;
}
