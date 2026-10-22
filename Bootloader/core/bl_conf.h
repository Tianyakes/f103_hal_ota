#ifndef BOOTL_CONF_H
#define BOOTL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define BOOTL_BOOTLOADER_ADDR 0X08000000 // BL地址
#define BOOTL_BOOTLOADER_SIZE (8 * 1024) // BOOTLOADER 大小

#define BOOTL_APPLICATION_ADDR 0X08002000 //APP地址
#define BOOTL_APPLICATION_SIZE (28 * 1024)

#define BOOTL_DOWNLOAD_ADDR 0x08009000
#define BOOTL_DOWNLOAD_SIZE (27 * 1024)

#define BOOTL_FLAG_PAGE_ADDR 0x0800FC00
#define BOOTL_UPDATING 0X55AA55AA

#define OTA_FRAME_HEAD  0xA5A5  // 帧头：用于对齐数据
#define OTA_MAX_DATA_LEN 512    // 单包最大携带的数据长度

#define BOOTL_MSP_TOP 0x20000000

#define BOOLT_LOG_LEVE 3

#define BOOTL_LOG_ERROR 1
#define BOOTL_LOG_WARRING 2
#define BOOTL_LOG_INFO 3


#ifdef __cplusplus
}
#endif

#endif
