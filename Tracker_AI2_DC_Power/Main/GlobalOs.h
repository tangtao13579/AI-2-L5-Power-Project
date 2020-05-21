#ifndef _GLOBAL_OS_H_ 
#define _GLOBAL_OS_H_

#include "os.h"
/***************************************************************************************************
                                    OSSEM
***************************************************************************************************/
#define Motor_continuous_FLAGS_VALUE   0X00		
#define Motor_continuous_step1_FLAG    0x01
#define Motor_continuous_step2_FLAG    0x02
#define Motor_continuous_step3_FLAG    0x04
#define Motor_continuous_clear_FLAG    0x07
extern OS_FLAG_GRP	Motor_continuous_Flags;       //create a event flag
#endif
