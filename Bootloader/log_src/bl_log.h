#ifndef BOOTL_LOG_H
#define BOOTL_LOG_H
#ifdef __cplusplus
extern "C" {
#endif

#include "bl_conf.h"

void BootLoader_LOG(const char *format, ...);

#if BOOLT_LOG_LEVE >= BOOTL_LOG_ERROR
    #define BootL_LOG_E(fmt, ...) BootLoader_LOG("[E] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define BootL_LOG_E(fmt, ...)
#endif

#if BOOLT_LOG_LEVE >= BOOTL_LOG_WARRING
    #define BootL_LOG_W(fmt, ...) BootLoader_LOG("[W] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define BootL_LOG_W(fmt, ...)
#endif

#if BOOLT_LOG_LEVE >= BOOTL_LOG_INFO
    #define BootL_LOG_I(fmt, ...) BootLoader_LOG("[I] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define BootL_LOG_I(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif