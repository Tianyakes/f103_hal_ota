#ifndef FLAG_MANAGER_H
#define FLAG_MANAGER_H

#ifdef __cplusplus
extern "C"{
#endif
#include <stdint.h>

uint32_t BL_Flag_Get(void);
uint32_t bl_get_version(void);
void Flag_Set_Updating(void);
void Flag_Set_Updated(void);
void Flag_Clear(void);

#ifdef __cplusplus
}
#endif

#endif //FLAG_MANAGER_H