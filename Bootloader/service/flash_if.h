#ifndef FLASH_IF_H
#define FLASH_IF_H
#include <stdint.h>

int BootL_Flash_If_write(uint32_t addr, const uint8_t *data, uint32_t len);
#endif