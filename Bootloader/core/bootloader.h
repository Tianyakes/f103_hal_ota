#ifndef BOOTLOADER_H
#define BOOTLOADER_H

typedef enum{
    OTA_IDLE = 0,
    OTA_UPDATING,
    OTA_FINISH,
    OTA_CARRYING,
    OTA_ERROR
} OTA_Update_t;

void BootLoader_Init(void);
void BootLoader(void);

#endif //BOOTLOADER_H