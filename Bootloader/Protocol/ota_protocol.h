#ifndef OTA_PROTOCOL_H
#define OTA_PROTOCOL_H
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
    uint8_t cmd;
    uint16_t seq;
    uint16_t data_len;
} OtaFrameHead_Type;

#ifdef __cplusplus
}
#endif
#endif