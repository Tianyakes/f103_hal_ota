#ifndef BL_CRC_H
#define BL_CRC_H

#include <stdint.h>

/* 位宽：可在编译命令行覆盖，例如 -DCRC_SIZE=32 */
#ifndef CRC_SIZE
#define CRC_SIZE 16
#endif

#define CRC_8_NORMAL  0x07
#define CRC_16_IBM    0x8005
#define CRC_16_XMODEM 0x1021
#define CRC_32_NORMAL 0x04C11DB7
#define CRC_32_C      0x1EDC6F41

/* CRC_USED：实际使用的多项式，随位宽给默认值，也可 -DCRC_USED=... 覆盖 */
#if CRC_SIZE == 8
    typedef uint8_t  crc_t;
    #ifndef CRC_USED
    #define CRC_USED CRC_8_NORMAL      /* x^8 + x^2 + x + 1 */
    #endif
    #define CRC_POLY   ((crc_t)CRC_USED)
    #define CRC_TOPBIT ((crc_t)0x80u)
#elif CRC_SIZE == 16
    typedef uint16_t crc_t;
    #ifndef CRC_USED
    #define CRC_USED CRC_16_XMODEM     /* x^16 + x^12 + x^5 + 1，与下面的表一致 */
    #endif
    #define CRC_POLY   ((crc_t)CRC_USED)
    #define CRC_TOPBIT ((crc_t)0x8000u)
#elif CRC_SIZE == 32
    typedef uint32_t crc_t;
    #ifndef CRC_USED
    #define CRC_USED CRC_32_NORMAL     /* 改成 CRC_32_C 即 CRC-32C (Castagnoli) */
    #endif
    #define CRC_POLY   ((crc_t)CRC_USED)
    #define CRC_TOPBIT ((crc_t)0x80000000u)
#elif CRC_SIZE == 64
    typedef uint64_t crc_t;
    #ifndef CRC_USED
    #define CRC_USED 0x42F0E1EBA9EA3693ULL   /* ECMA-182 */
    #endif
    #define CRC_POLY   ((crc_t)CRC_USED)
    #define CRC_TOPBIT ((crc_t)0x8000000000000000ULL)
#else
    #error "size not support! only 8/16/32/64 was supported"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 表在 crc.c 里只定义一次；头文件里只能写 extern 声明，
   否则每个包含本头文件的 .c 都会产生一个定义 -> multiple definition */
#if CRC_SIZE == 16
extern const uint16_t crc16_table_XMODEM[256];
#endif

crc_t crc_check(crc_t crc, uint8_t dat);
crc_t crc_stream(const uint8_t *buf, uint32_t len, crc_t init);

/* 查表法只对 16 位 XMODEM 有意义 */
#if CRC_SIZE == 16
crc_t crc_check_Table_XMODEM(const uint8_t *dat, uint16_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif
