#ifndef OTA_PROTOCOL_H
#define OTA_PROTOCOL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif

typedef enum{
    OTA_CMD_START = 0X01,
    OTA_CMD_DATA = 0x02,
    OTA_CMD_FINISH = 0X03,
    OTA_CMD_ACK = 0X81,
    OTA_CMD_NACK = 0X82,
} OtaCmd_Type;

typedef struct{
    uint16_t haed;
    OtaCmd_Type cmd;
    uint16_t len;
    uint8_t index;
    uint8_t total;
    uint8_t* frame;
} OtaFrame_Type;

void OtaFrame_TypeInit(OtaFrame_Type *o);
OtaFrame_Type* ota_protocol_start(void);
void ota_protocol_Clear(void);
#ifdef __cplusplus
}
#endif
#endif