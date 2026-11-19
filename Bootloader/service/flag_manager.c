#include "flag_manager.h"
#include "flash_drv.h"
#include "bl_conf.h"
#include <stdint.h>

uint32_t BL_Flag_Get(void){
    uint32_t flag;
    Flash_Read_Data(BOOTL_FLAG_PAGE_ADDR, (uint8_t *)&flag, 4);
    return flag;
}

static uint32_t bl_format_version(void){
    uint32_t n = 0;
    n |= (uint32_t)BOOTL_VERSION_0;        /* bit0-7  */
    n |= (uint32_t)BOOTL_VERSION_1 << 8;   /* bit8-15 */
    n |= (uint32_t)BOOTL_VERSION_2 << 16;  /* bit16-23 */
    return n;
}

static void bl_set_version(void){
    uint32_t flag = bl_format_version();
    BL_FlashWriteByte(BOOTL_FLAG_PAGE_ADDR + 4, (uint8_t *)&flag, 4);
}

uint32_t bl_get_version(void){
    uint32_t v;
    Flash_Read_Data(BOOTL_FLAG_PAGE_ADDR + 4, (uint8_t *)&v, 4);
    return v;
}

void Flag_Set_Updating(void) {
    uint32_t flag = BOOTL_UPDATING;
    BL_FlashErasePages(BOOTL_FLAG_PAGE_ADDR, 1); // 擦除整页
    BL_FlashWriteByte(BOOTL_FLAG_PAGE_ADDR, (uint8_t *)&flag, 4);
    bl_set_version();
}

void Flag_Set_Updated(void) {
    uint32_t flag = BOOTL_UPDATED;
    BL_FlashErasePages(BOOTL_FLAG_PAGE_ADDR, 1); // 擦除整页
    BL_FlashWriteByte(BOOTL_FLAG_PAGE_ADDR, (uint8_t *)&flag, 4);
    bl_set_version();
}

void Flag_Clear(void) {
    BL_FlashErasePages(BOOTL_FLAG_PAGE_ADDR, 1); // 擦除就代表清零（变回 0xFFFFFFFF）
}