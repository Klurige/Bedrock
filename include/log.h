// No guard around this file. The logging is controlled by macro before including it
// in a source file.

#include <Arduino.h>

#ifdef BEDROCK_DEBUG
#undef BEDROCK_DEBUG
#endif
#ifdef BEDROCK_INFO
#undef BEDROCK_INFO
#endif
#ifdef BEDROCK_WARN
#undef BEDROCK_WARN
#endif
#ifdef BEDROCK_ERROR
#undef BEDROCK_ERROR
#endif

#if defined BEDROCK_LOG && defined BEDROCK_LOG_ON
#define BEDROCK_DEBUG(fmt, ...) logServer(PSTR("%s:%d -> %s():" fmt), __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define BEDROCK_DEBUG(...) \
    do {                   \
    } while (0)
#endif

#if defined BEDROCK_LOG_ON
#define BEDROCK_INFO(fmt, ...) logServer(PSTR("%s:%d -> %s():" fmt), __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__)
#define BEDROCK_WARN(fmt, ...) logServer(PSTR("%s:%d -> %s():" fmt), __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__)
#define BEDROCK_ERROR(fmt, ...) logServer(PSTR("%s:%d -> %s():" fmt), __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define BEDROCK_INFO(...) \
    do {                  \
    } while (0)
#define BEDROCK_WARN(...) \
    do {                  \
    } while (0)
#define BEDROCK_ERROR(...) \
    do {                   \
    } while (0)

#endif
void logSetup();

void logServer(const char* const format, ...);

/**
 * Initialise json.
 * @param skip Skip all messages of this age and older.
 */
void logJsonStart(uint32_t skip);

/**
 * Get log as a chunk, in json format.
 *
 * Must be preceeded by logJsonStart().
 *
 * To get all chunks, call this in a loop until it returns 0.
 *
 * @param buffer fill this with the chunk.
 * @param maxLen Size of buffer.
 * @return size_t  Size of chunk.
 */
size_t logJsonNext(char* const buffer, size_t maxLen);