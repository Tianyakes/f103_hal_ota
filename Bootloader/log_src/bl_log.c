#include "bl_log.h"
#include <stdio.h>
#include <stdarg.h>
#include "uart_drv.h"

void BootLoader_LOG(const char *format, ...){
    char log_buf[128];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(log_buf, sizeof(log_buf), format, args);
    va_end(args);
    if (len > 0) {
        // 发送出去 (注意 len 不能超过 128)
        BootLoader_uart_send((const uint8_t *)log_buf, (len < 128) ? len : 128);
    }
}
