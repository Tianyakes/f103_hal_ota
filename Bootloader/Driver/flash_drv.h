#ifndef BOOTL_FLASH_DRV_H
#define BOOTL_FLASH_DRV_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>

#define BOOTL_FLASH_PAGE_SIZE 1024
int BL_FlashErasePages(uint32_t start_addr, uint32_t num_pages);
int BL_FlashWriteByte(uint32_t addr, const uint8_t *data, uint32_t len);
void Flash_Read_Data(uint32_t addr, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif //BOOTL_FLASH_DRV_H