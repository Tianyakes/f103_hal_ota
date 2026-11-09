#ifndef BOOTL_CONF_H
#define BOOTL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define BOOTL_VERSION_0 0
#define BOOTL_VERSION_1 0
#define BOOTL_VERSION_2 1
#define STRING_BL_BL(x) #x
#define BOOTLOADER_VERSION STRING_BL(BOOTL_VERSION_0)"."STRING_BL(BOOTL_VERSION_1)"."STRING_BL(BOOTL_VERSION_2)

#define BOOTLOADER_VERSION_NUM \
    ((BOOTL_VERSION_0 << 16) | (BOOTL_VERSION_1 << 8) | (BOOTL_VERSION_2))

// 1. Bootloader 区 (起始: 0x08000000, 大小: 12KB)
#define BOOTL_BOOTLOADER_ADDR   0x08000000
#define BOOTL_BOOTLOADER_SIZE   (12 * 1024)      // 12KB

// 2. Application 区 (起始: 0x08003000, 大小: 28KB)
#define BOOTL_APPLICATION_ADDR  0x08003000
#define BOOTL_APPLICATION_SIZE  (28 * 1024)      // 28KB

// 3. Download 区 (起始: 0x0800A000, 大小: 23KB)
#define BOOTL_DOWNLOAD_ADDR     0x0800A000
#define BOOTL_DOWNLOAD_SIZE     (23 * 1024)      // 23KB

// 4. 标志位页 (位于 Flash 最后一页, 大小: 1KB)
#define BOOTL_FLAG_PAGE_ADDR    0x0800FC00
#define BOOTL_FLAG_PAGE_SIZE    (1 * 1024)       // 1KB
#define BOOTL_UPDATING 0X55AA55AA
#define BOOTL_UPDATED 0xAA55AA55

#define OTA_FRAME_HEAD  0xA5A5  // 帧头：用于对齐数据
#define OTA_MAX_DATA_LEN 512    // 单包最大携带的数据长度

#define BOOTL_MSP_TOP 0x20000000

#define BOOLT_LOG_LEVE 0

#define BOOTL_LOG_ERROR 1
#define BOOTL_LOG_WARRING 2
#define BOOTL_LOG_INFO 3


#ifdef __cplusplus
}
#endif

#endif
