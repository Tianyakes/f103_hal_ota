#ifndef BL_CRC_H
#define BL_CRC_H
#include <stdint.h>
#define CRC_SIZE 16 //8/16/32/64
#define CRC_USED CRC_16_XMODEM

#define CRC_8_NORMAL 0x07
#define CRC_16_IBM 0x8005
#define CRC_16_XMODEM 0x1021
#define CRC_32_NORMAL 0x04C11DB7
#define CRC_32_C 0x1EDC6F41

/* 按位宽选多项式（按你实际协议替换） */
#if CRC_SIZE == 8
    typedef uint8_t  crc_t;
    #define CRC_POLY   0x07u
    #define CRC_TOPBIT 0x80u
#elif CRC_SIZE == 16
    typedef uint16_t crc_t;
    #define CRC_POLY   0x8005u
    #define CRC_TOPBIT 0x8000u
#elif CRC_SIZE == 32
    typedef uint32_t crc_t;
    #define CRC_POLY   0x04C11DB7u
    #define CRC_TOPBIT 0x80000000u
#elif CRC_SIZE == 64
    typedef uint64_t crc_t;
    #define CRC_POLY   0x42F0E1EBA9EA3693ULL
    #define CRC_TOPBIT 0x8000000000000000ULL
#else
    #error "size not support! only 8/16/32/64 was supported"
#endif

#ifdef __cplusplus
extern "C" {
#endif

crc_t crc_check(crc_t crc, uint8_t dat);
crc_t crc_stream(const uint8_t *buf, uint32_t len, crc_t init);

#ifdef __cplusplus
}
#endif

#endif