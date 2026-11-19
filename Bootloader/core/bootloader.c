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
// #include <stdint.h>

OtaFrame_Type o;
static OTA_Update_t OTA = OTA_IDLE;

void BootLoader_Init(void){
    // 驱动初始化

    UART_Transport_Init();
    // ota接收初始化
    OtaFrame_TypeInit(&o);
}

void BootLoader(void){
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET){
        OTA = OTA_UPDATING;
        uint32_t ver = bl_get_version();
        BootL_LOG_I("Bootloader version: %d.%d.%d", 
            (ver >> 0)  & 0xFF,   // 第一段 (bit0-7)
            (ver >> 8)  & 0xFF,   // 第二段 (bit8-15)
            (ver >> 16) & 0xFF);  // 第三段 (bit16-23)
    }
    // 外部触发开始升级 中断识别标识页
    if(BL_Flag_Get() == BOOTL_UPDATED && OTA == OTA_IDLE){
        app_jump(BOOTL_APPLICATION_ADDR);
    }else if (BL_Flag_Get() == BOOTL_UPDATING) {
        BootL_LOG_I("The last upgrade failed. Please reflash the firmware");
        OTA = OTA_UPDATING;
    }else if(BL_Flag_Get()!= BOOTL_UPDATED && BL_Flag_Get()!=BOOTL_UPDATING){
        Flag_Set_Updating();
    }
    static uint32_t g_download_size_total = 0;
    static uint32_t g_download_size = 0;
    static uint32_t g_app_size = 0;
    while (1) {
        uint8_t ret = ota_protocol_start(&o);
        if(ret){
            if(o.cmd == OTA_CMD_START && OTA == OTA_UPDATING){
                BootL_LOG_I("Upcoming upgrade...");
                Flag_Set_Updating();
                ota_protocol_Clear();
                ota_protocol_ACK();
            }
            else if (o.cmd == OTA_CMD_DATA && OTA == OTA_UPDATING) {

                ret = BootL_Flash_If_write(BOOTL_DOWNLOAD_ADDR +  g_download_size, &o.frame[7], o.len);
                if(ret == -1) BootL_LOG_I("Eownload addr error");
                if(ret == -2) BootL_LOG_I("Erase error");
                g_download_size+=o.len;
                g_download_size_total+=o.len;
                BootL_LOG_I("Recviced:%d/%d remaining, Pack size:%d Bytes", o.index, o.total, g_download_size_total);
                if((BOOTL_DOWNLOAD_SIZE - g_download_size) < OTA_MAX_DATA_LEN){
                    OTA = OTA_CARRYING;
                    BootL_LOG_I("Carry start");
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
        }else{
            BootL_LOG_I("Frame error: Request the frame again");
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
                // if(ret == -1) BootL_LOG_I("download addr error");
                // if(ret == -2) BootL_LOG_I("erase error");
                g_app_size+=o.len;
                g_download_size+=o.len;
            }
            BootL_LOG_I("Carry end");
            //搬运复用结束再开启接收
            if(OTA == OTA_FINISH){
                g_download_size_total = 0;
                g_app_size = 0;
                OTA = OTA_IDLE;
                Flag_Set_Updated();
                BootL_LOG_I("Updated,rebooting...");
                ota_protocol_Clear();
                ota_protocol_ACK();
                app_jump(BOOTL_APPLICATION_ADDR);
            }else {
                OTA = OTA_UPDATING;
            }
            g_download_size = 0;
            ota_protocol_Clear();
            ota_protocol_ACK();
        }
    }
}