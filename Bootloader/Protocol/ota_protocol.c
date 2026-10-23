#include "bl_conf.h"
#include <stdint.h>
#include "ota_protocol.h"

static OtaFrameHead_Type head;
static uint8_t data_buffer[OTA_MAX_DATA_LEN];
static uint16_t rx_index = 0;
static uint8_t rx_state = 0; // 0:找帧头, 1:收结构体, 2:收数据, 3:收CRC

void OTA_Receive_Data(){
    
}