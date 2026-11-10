#include "bootloader.h"
#include "flash_if.h"
#include "ota_protocol.h"
#include "bl_log.h"
#include "bl_conf.h"
#include "jump_to_app.h"
#include <stdint.h>

OtaFrame_Type o;
static OTA_Update_t OTA = OTA_IDLE;

void BootLoader_Init(void){
    // 驱动初始化


    // ota接收初始化
    OtaFrame_TypeInit(&o);
}

void BootLoader(void){
    if(OTA == OTA_IDLE){
        app_jump(BOOTL_APPLICATION_ADDR);
    }
    static uint32_t g_download_size = 0;
    uint8_t ret = ota_protocol_start(&o);
    if(ret){
        if(o.cmd == OTA_CMD_START){
            OTA = OTA_UPDATING;
            ret = 0;
            ota_protocol_Clear();
            ota_protocol_ACK();
        }
        else if (OTA == OTA_UPDATING) {
            BootL_Flash_If_write(BOOTL_DOWNLOAD_ADDR +  g_download_size, &o.frame[8], o.len);
            g_download_size+=o.len;
            ota_protocol_Clear();
            ota_protocol_ACK();
        }
        else {
            ota_protocol_Clear();
            ota_protocol_NACK();
        }
    }
}