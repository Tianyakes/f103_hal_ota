#include "bootloader.h"
#include "flash_if.h"
#include "ota_protocol.h"
#include "bl_log.h"
#include "bl_conf.h"
#include "jump_to_app.h"
#include "flag_manager.h"
#include "main.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>

OtaFrame_Type o;
static OTA_Update_t OTA = OTA_IDLE;

void BootLoader_Init(void){
    // 驱动初始化


    // ota接收初始化
    OtaFrame_TypeInit(&o);
}

void BootLoader(void){
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET){
        OTA = OTA_UPDATING;
        BootL_LOG_I("waiting...");
    }
    // 外部触发开始升级 中断识别标识页
    if(BL_Flag_Get() == BOOTL_UPDATED && OTA == OTA_IDLE){
        app_jump(BOOTL_APPLICATION_ADDR);
    }else if (BL_Flag_Get() == BOOTL_UPDATING) {
    
    }else {
        Flag_Set_Updated();
    }
    static uint32_t g_download_size_total = 0;
    static uint32_t g_download_size = 0;
    static uint32_t g_app_size = 0;
    uint8_t ret = ota_protocol_start(&o);
    if(ret){
        if(o.cmd == OTA_CMD_START && OTA == OTA_UPDATING){
            BootL_LOG_I("Upcoming upgrade...");
            Flag_Set_Updating();
            ota_protocol_Clear();
            ota_protocol_ACK();
        }
        else if (o.cmd == OTA_CMD_DATA && OTA == OTA_UPDATING) {
            BootL_Flash_If_write(BOOTL_DOWNLOAD_ADDR +  g_download_size, &o.frame[7], o.len);
            g_download_size+=o.len;
            g_download_size_total+=o.len;
            BootL_LOG_I("Recviced:%d/%d remaining, Pack size:%d Bytes", o.index, o.total, g_download_size_total);
            if((BOOTL_DOWNLOAD_SIZE - g_download_size) <= OTA_MAX_DATA_LEN){
                OTA = OTA_CARRYING;
                BootL_LOG_I("Start carry");
                // g_download_size_total+=g_download_size;
            }
            if(OTA != OTA_CARRYING){
                ota_protocol_Clear();
                ota_protocol_ACK();
            }
        }else if (o.cmd == OTA_CMD_FINISH && OTA == OTA_UPDATING) {
            OTA = OTA_FINISH;
        }
        else {

        }
        ret = 0;
    }else{
        BootL_LOG_E("Frame error:Request the frame again");
        ota_protocol_Clear();
        ota_protocol_NACK();
    }
    if(OTA == OTA_CARRYING || OTA == OTA_FINISH){
        uint32_t carry_total = g_download_size;
        g_download_size = 0;
        //正在搬运或者提前搬运
        while(g_download_size < carry_total){
            // if()
            if(carry_total - g_download_size < OTA_MAX_DATA_LEN){
                o.len = carry_total - g_download_size;
            }else{
                o.len = OTA_MAX_DATA_LEN;
            }

            //节省空间复用数据帧的缓存空间
            BootL_Flash_If_Read(BOOTL_DOWNLOAD_ADDR + g_download_size, &o.frame[7], o.len);
            BootL_Flash_If_write(BOOTL_APPLICATION_ADDR + g_app_size, &o.frame[7], o.len);
            g_app_size+=o.len;
            g_download_size+=o.len;
        }
        BootL_LOG_I("Carry end")
        //搬运复用结束再开启接收
        if(OTA == OTA_FINISH){
            g_download_size_total = 0;
            g_app_size = 0;
            OTA = OTA_IDLE;
            Flag_Set_Updated();
            BootL_LOG_I("Updated,rebooting...")
        }else {
            OTA = OTA_UPDATING;
        }
        g_download_size = 0;
        ota_protocol_Clear();
        ota_protocol_ACK();
    }
}