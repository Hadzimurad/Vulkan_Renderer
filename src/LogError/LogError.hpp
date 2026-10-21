#pragma once
#include <cstdio>
#include <process.h>


#define LOG_ERROR(log_text, ...) \
    do { \
        printf("[ERROR] [ %s, %d, ]: ", __FILE__, __LINE__); \
        fprintf(stderr, log_text, ##__VA_ARGS__); \
        printf("\n"); \
    } while(0)


#ifndef NDEBUG
    #define LOG(message, ...) printf(message, ##__VA_ARGS__)

    #define ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                fprintf(stderr, "[ASSERT FAILED] (%s)\n" \
                        "Message: %s\n" \
                        "File, line: %s, %d\n", \
                        #condition, message, __FILE__, __LINE__); \
                abort(); \
            } \
        } while(0)
#endif

