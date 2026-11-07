#pragma once
#include <cstdio>
#include <cstdlib> // Для abort()

// Переводим всё на fprintf(stderr), чтобы вывод не перемешивался из-за разных буферов.
// Символ \n ставим строго в конце всей строки.
#define LOG_ERROR(log_text, ...) \
    do { \
        fprintf(stderr, "[ERROR] [%s:%d]: " log_text "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)


#ifndef NDEBUG
    // Добавляем явный \n в конец, чтобы логи не склеивались в одну строку
    #define LOG(message, ...) \
        do { \
            fprintf(stderr, "[LOG] " message "\n", ##__VA_ARGS__); \
        } while (0)

    // Форматируем assert в один понятный блок, выводящийся за один вызов
    #define ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                fprintf(stderr, "\n--- [ASSERT FAILED] ---\n" \
                                "Condition: %s\n" \
                                "Message:   %s\n" \
                                "Source:    %s:%d\n" \
                                "-----------------------\n", \
                        #condition, message, __FILE__, __LINE__); \
                std::abort(); \
            } \
        } while(0)
#else
    #define LOG(message, ...) do { } while (0)
    #define ASSERT(condition, message) do { } while (0)
#endif
