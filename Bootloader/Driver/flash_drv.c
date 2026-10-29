#include "flash_drv.h"
#include "bl_log.h"
#include "bl_conf.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

static int BL_IsFlashAddr_Valid(uint32_t addr){
    return (addr>=0x08000000 && addr<=0x08010000);
}

int BL_FlashErasePages(uint32_t start_addr, uint32_t num_pages){
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0;

    if(!BL_IsFlashAddr_Valid(start_addr)){
        BootL_LOG_E("Erase err:start address is not valid!");
        return -1;
    }

    HAL_FLASH_Unlock();
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = start_addr;
    erase.NbPages = num_pages;
    if(HAL_FLASHEx_Erase(&erase, &page_error)!= HAL_OK){
        HAL_FLASH_Lock();
        BootL_LOG_E("Erase failed at page addr: 0x%08X", page_error);
        return -1;
    }
    HAL_FLASH_Lock();
    return 0;
}

int BL_FlashWriteByte(uint32_t addr, const uint8_t *data, uint32_t len){
    if(!BL_IsFlashAddr_Valid(addr) || !BL_IsFlashAddr_Valid(addr + len -1)){
        BootL_LOG_E("Write err:address is not vaild");
        return -1;
    }
    //写入以半字16字节写入，判断目标地址是否为奇数，是则报错
    if ((addr % 2) != 0) {
        BootL_LOG_E("Write err: addr not aligned to halfword!");
        return -1;
    }
    HAL_FLASH_Unlock();
    for(uint16_t i = 0;i<len;i+=2){
        uint16_t halfword = 0;
        halfword = data[i];
        if(i+1<len){
            halfword |= ((uint16_t)data[i+1] << 8);
        }else{
            halfword |= 0xff00;
        }
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, halfword) != HAL_OK) {
            HAL_FLASH_Lock();
            BootL_LOG_E("Write failed at addr: 0x%08X", addr + i);
            return -1;
        }
    }
    HAL_FLASH_Lock();
    return 0;
}

void Flash_Read_Data(uint32_t addr, uint8_t *buf, uint32_t len)
{
    // 读 Flash 不需要解锁，直接按字节拷贝即可
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = *(__IO uint8_t *)(addr + i);
    }
}