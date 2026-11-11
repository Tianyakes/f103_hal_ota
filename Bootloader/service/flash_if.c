#include "flash_if.h"
#include "flash_drv.h"
#include "bl_conf.h"
#include <stdint.h>

int BootL_Flash_If_write(uint32_t addr, const uint8_t *data, uint32_t len){
    if(addr < BOOTL_DOWNLOAD_ADDR){
        return -1;
    }

    if((addr % BOOTL_FLAG_PAGE_SIZE) == 0){
        if(BL_FlashErasePages(addr, 1)!=0){
            return -2;
        }
    }
    return BL_FlashWriteByte(addr, data, len);
}

void BootL_Flash_If_Read(uint32_t addr, const uint8_t *data, uint32_t len){
    Flash_Read_Data(addr, data, len);
}