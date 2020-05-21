#include "GlobalDefine.h"
#include "Init.h"
#include "misc.h"
#include "HardwareVersion.h"

extern GlobalVariableDef GlobalVariable;

static void ConfigParaInit()
{
    GlobalVariable.ConfigPara.ComID                 = 1;
}
static void FixedParaInit()
{
    HDVInit();
    GlobalVariable.FixePara.DeviceEdition = 0x0200;                 //Fireware    : 0.02
    GlobalVariable.FixePara.DeviceEdition |= GetHardwareVersion();
	  GlobalVariable.FixePara.DeviceType    = 0x0080;                 //devic type  : 10
}


void ParaInit()
{
    ConfigParaInit();
    FixedParaInit();
}
void NVICInit()
{
    NVIC_InitTypeDef NVICInitStruc;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    /*Enable the TIM3 gloabal Interrupt*/
    NVICInitStruc.NVIC_IRQChannel = TIM3_IRQn; 
    NVICInitStruc.NVIC_IRQChannelPreemptionPriority = 0;
    NVICInitStruc.NVIC_IRQChannelSubPriority = 1;
    NVICInitStruc.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVICInitStruc);
    
    NVICInitStruc.NVIC_IRQChannel = DMA1_Channel1_IRQn; /*ADC*/
    NVICInitStruc.NVIC_IRQChannelPreemptionPriority = 0;
    NVICInitStruc.NVIC_IRQChannelSubPriority = 2;
    NVICInitStruc.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVICInitStruc); 
    
    NVICInitStruc.NVIC_IRQChannel = USART1_IRQn;
    NVICInitStruc.NVIC_IRQChannelPreemptionPriority = 1;
    NVICInitStruc.NVIC_IRQChannelSubPriority = 0;
    NVICInitStruc.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVICInitStruc);
		
		NVICInitStruc.NVIC_IRQChannel = UART5_IRQn;
    NVICInitStruc.NVIC_IRQChannelPreemptionPriority = 1;
    NVICInitStruc.NVIC_IRQChannelSubPriority = 0;
    NVICInitStruc.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVICInitStruc);
		
}

void IWDGInit()
{

    RCC_LSICmd(ENABLE);

    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY)==RESET);

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    IWDG_SetPrescaler(IWDG_Prescaler_64);

    IWDG_SetReload(4000); 

    IWDG_ReloadCounter();

    IWDG_Enable();
}

