#include "crc16.h"

crc_t crc_check(crc_t crc, uint8_t dat)
{
    /* 把新字节放到余数的高 8 位再异或 */
    crc ^= (crc_t)dat << (CRC_SIZE - 8);

    for (uint8_t i = 0; i < 8; i++) {        /* 固定 8 次 */
        if (crc & CRC_TOPBIT)
            crc = (crc_t)((crc << 1) ^ CRC_POLY);
        else
            crc = (crc_t)(crc << 1);
    }
    return crc;
}

crc_t crc_stream(const uint8_t *buf, uint32_t len, crc_t init){
    crc_t crc = init;
    for(uint32_t i=0;i<len;i++)
        crc = crc_check(crc, buf[i]);
    return crc;
}