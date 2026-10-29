#include "flag_manager.h"
#include "flash_drv.h"
#include "bl_conf.h"
#include <stdint.h>

uint32_t BL_Flag_Get(void){
    uint32_t flag;
    Flash_Read_Data(BOOTL_FLAG_PAGE_ADDR, (uint8_t *)&flag, 4);
    return flag;
}

void Flag_Set_Updating(void) {
    uint32_t flag = BOOTL_UPDATING;
    BL_FlashErasePages(BOOTL_FLAG_PAGE_ADDR, 1); // 擦除整页
    BL_FlashWriteByte(BOOTL_FLAG_PAGE_ADDR, (uint8_t *)&flag, 4);
}

void Flag_Clear(void) {
    BL_FlashErasePages(BOOTL_FLAG_PAGE_ADDR, 1); // 擦除就代表清零（变回 0xFFFFFFFF）
}