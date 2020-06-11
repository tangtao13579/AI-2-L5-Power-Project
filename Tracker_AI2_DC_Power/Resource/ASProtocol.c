#include "GlobalDefine.h"
#include "ASProtocol.h"
#include "stm32f10x.h"
#include "core_cm3.h"
#include "BackupRegister.h"
#include "stm32f10x_rtc.h"
#include "RtcLocalModule.h"
#include "os.h"
#include "stm32f10x_rcc.h"

extern GlobalVariableDef GlobalVariable;
unsigned short us_tick = 0;
unsigned char  uc_rtcFlag = 0;
unsigned char test_rtc = 0;
/***************************************************************************************************
                                     Private Variable
***************************************************************************************************/
/*R-0, RW-1*/
static void (* RWRegister[114])(unsigned char R_or_RW, unsigned short *value)={(void *)0};
/***************************************************************************************************
                                Jump function
***************************************************************************************************/

static void RDeviceEdition(unsigned char R_or_RW, unsigned short *value );
static void RWWorkMode1(unsigned char R_or_RW, unsigned short *value );
static void RWWorkMode2(unsigned char R_or_RW, unsigned short *value );
static void RWarningAndFault1(unsigned char R_or_RW, unsigned short *value );
static void RWarningAndFault2(unsigned char R_or_RW, unsigned short *value );
static void RDeviceType(unsigned char R_or_RW, unsigned short *value);
static void RTemp(unsigned char R_or_RW, unsigned short *value);
static void RMotorCurrent(unsigned char R_or_RW, unsigned short *value);
static void RPVBuckerVoltage(unsigned char R_or_RW, unsigned short *value);
static void RBatState(unsigned char R_or_RW, unsigned short *value);
static void RBatSOCTemp(unsigned char R_or_RW, unsigned short *value);
static void RBatVoltage(unsigned char R_or_RW, unsigned short *value);
static void RBatCurrent(unsigned char R_or_RW, unsigned short *value);
static void RWComID(unsigned char R_or_RW, unsigned short *value);

static void RDeviceEdition(unsigned char R_or_RW, unsigned short *value )
{
    if(R_or_RW == 0)
    {
        *value = GlobalVariable.FixePara.DeviceEdition;
    }
}

static void RWWorkMode1(unsigned char R_or_RW, unsigned short *value )
{
    if(R_or_RW == 0)
    {
        *value = *(((unsigned short *)&GlobalVariable.WorkMode.WorkMode)+1);
    }
    else if(R_or_RW == 1)
    {
        *(((unsigned short *)&GlobalVariable.WorkModeBuffer.WorkMode)+1) = *value;
    }
}

static void RWWorkMode2(unsigned char R_or_RW, unsigned short *value )
{
    if(R_or_RW == 0)
    {
        *value = *(((unsigned short *)&GlobalVariable.WorkMode.WorkMode));
    }
    else if(R_or_RW == 1)
    {
        *(((unsigned short *)&GlobalVariable.WorkModeBuffer.WorkMode)) = *value;
        if((GlobalVariable.WorkModeBuffer.WorkMode & 0xFF) == 0xFF)
        {
            NVIC_SystemReset();
        }
				else if((GlobalVariable.WorkModeBuffer.WorkMode & 0xFF) == 0xFE)//ͣ��ģʽ
        {
            GlobalVariable.WorkModeBuffer.WorkMode = AUTO_TRACKER_MODE; 
					  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);	//ʹ��PWR����ʱ��
	          
					  EXTI_InitTypeDef EXTI_InitStructure;
 	          NVIC_InitTypeDef NVIC_InitStructure;
  	        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);	//ʹ�ܸ��ù���ʱ��

						//GPIOE.2 �ж����Լ��жϳ�ʼ������   �½��ش���
						GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource10);

						EXTI_InitStructure.EXTI_Line=EXTI_Line10;	//PA10
						EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
						EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
						EXTI_InitStructure.EXTI_LineCmd = ENABLE;
						EXTI_Init(&EXTI_InitStructure);	 	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

  	        NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//ʹ�ܰ���WK_UP���ڵ��ⲿ�ж�ͨ��
						NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;	//��ռ���ȼ�2�� 
						NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;					//�����ȼ�3
						NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
						NVIC_Init(&NVIC_InitStructure); 
            
						IWDG_ReloadCounter(); 
						IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
						IWDG_SetPrescaler(IWDG_Prescaler_256);
						IWDG_SetReload(0xFFF); 
						IWDG_ReloadCounter();
						
            RTC_Local_Init();
						
						us_tick = SysTick->CTRL;
			      SysTick->CTRL  = 0; 
	          PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);//STOPģʽ
						OS_ERR err; 
						OSSchedLock(&err);
						while(1)
						{
							if(uc_rtcFlag==1)
							{
								uc_rtcFlag = 0;
								SysTick->CTRL = us_tick;
								SystemInit();
								OS_CPU_SysTickInit(720000); 
								IWDG_ReloadCounter();
								RCC_LSICmd(ENABLE);
								RTC_WaitForSynchro();	
								RTC_Local_Init();
								us_tick = SysTick->CTRL;
			          SysTick->CTRL  = 0; 
	              PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);//STOPģʽ
							}
						} 
        }
        else if ((GlobalVariable.WorkModeBuffer.WorkMode & 0xFF) == AUTO_AI_MODE)
        {
            if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_TRACKER_MODE)
            {
                GlobalVariable.WorkMode.WorkMode = GlobalVariable.WorkModeBuffer.WorkMode;
				        GlobalVariable.AIPara.AIRemoteAngle=GlobalVariable.Motor[0].ActualAngle;
            }
        }
        else
        {
            GlobalVariable.WorkMode.WorkMode = GlobalVariable.WorkModeBuffer.WorkMode;
        }
    }
}

void EXTI15_10_IRQHandler(void)
{
	  NVIC_SystemReset();  
}
 

static void RWarningAndFault1(unsigned char R_or_RW, unsigned short *value )
{
    if(R_or_RW == 0)
    {
        *value = *(((unsigned short *)&GlobalVariable.WarningAndFault)+1);
    }
}
static void RWarningAndFault2(unsigned char R_or_RW, unsigned short *value )
{
    if(R_or_RW == 0)
    {
        *value = *(((unsigned short *)&GlobalVariable.WarningAndFault));
    }
}
static void RDeviceType(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = GlobalVariable.FixePara.DeviceType;
    }
}
static void RWNull(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = 0;
    }
    else if(R_or_RW == 1)
    {
    }
}

static void RTemp(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = 0;
        *value = *(unsigned short *)&GlobalVariable.Tmp.BoardTmp;
    }
}
static void RMotorCurrent(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = (short)(GlobalVariable.PowerPara.Iin * 10);
    }
}

static void RPVBuckerVoltage(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = (unsigned short)(GlobalVariable.PowerPara.PVBuckerVoltage * 10);
    }
}
static void RBatState(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = 0;
        *value |= *(unsigned char*)(&GlobalVariable.PowerPara.ChargeState);
    }
}
static void RBatSOCTemp(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = 0x00FF & (unsigned char)(GlobalVariable.PowerPara.BatterySOC*100);
        *value = *value << 8;
        *value &= 0xFF00;
        *value |= ((signed char)(GlobalVariable.PowerPara.BatteryTemperature) & 0x00FF);
    }
}
static void RBatVoltage(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = (unsigned short)(GlobalVariable.PowerPara.BatteryVoltage*10);
    }
}
static void RBatCurrent(unsigned char R_or_RW, unsigned short *value)
{
    signed short temp = 0;
    if(R_or_RW == 0)
    {
        temp = (signed short)(GlobalVariable.PowerPara.BatteryCurrent*100);
        *value = *((unsigned short *)&temp);
    }
}
static void RWComID(unsigned char R_or_RW, unsigned short *value)
{
    if(R_or_RW == 0)
    {
        *value = *(((unsigned short *)&GlobalVariable.ConfigPara.ComID));
    }
    else if(R_or_RW == 1)
    {        
        if(*value >= 1 && *value < 255)
        {
            GlobalVariable.ConfigPara.ComID =(unsigned char) *value;
            GlobalVariable.WriteFlag.ConfigParaWrite = 1;
        }
    }
}

/***************************************************************************************************
                                Jump table
***************************************************************************************************/
void ASProtocolInit()
{
    RWRegister[0]   = RDeviceEdition;
    RWRegister[1]   = RWWorkMode1;
    RWRegister[2]   = RWWorkMode2;
    RWRegister[3]   = RWarningAndFault1;
    RWRegister[4]   = RWarningAndFault2;
    RWRegister[5]   = RDeviceType;
    RWRegister[6]   = RWNull;
    RWRegister[7]   = RWNull;
    RWRegister[8]   = RWNull;
    RWRegister[9]   = RWNull;
    RWRegister[10]  = RWNull;
    
    RWRegister[11]  = RWNull;
    RWRegister[12]  = RWNull;
    RWRegister[13]  = RWNull;
    RWRegister[14]  = RWNull;
    
    RWRegister[23]  = RWNull;
    RWRegister[24]  = RWNull;
    RWRegister[25]  = RWNull;
    RWRegister[26]  = RWNull;
    RWRegister[27]  = RWNull;
    RWRegister[28]  = RWNull;
    
    RWRegister[33]  = RWNull;
    RWRegister[34]  = RWNull;
    
    RWRegister[38]  = RWNull;
    RWRegister[39]  = RTemp;
    RWRegister[40]  = RMotorCurrent;
    
    RWRegister[43]  = RWNull;
    RWRegister[44]  = RWNull;
    
    RWRegister[45]  = RWNull;
    RWRegister[46]  = RWNull;
    RWRegister[47]  = RWNull;
    RWRegister[48]  = RWNull;
    RWRegister[49]  = RWNull;
    RWRegister[50]  = RWNull;
    
    RWRegister[51]  = RWNull;
    RWRegister[52]  = RWNull;
    
    RWRegister[55]  = RPVBuckerVoltage;
    RWRegister[56]  = RBatState;
    RWRegister[57]  = RBatSOCTemp;
    RWRegister[58]  = RBatVoltage;
    RWRegister[59]  = RBatCurrent;
    
    RWRegister[81]  = RWComID;
    
    RWRegister[82]  = RWNull;
    RWRegister[84]  = RWNull;
		
		RWRegister[85]  = RWNull;
    
    RWRegister[89]  = RWNull;
    RWRegister[90]  = RWNull;
    RWRegister[91]  = RWNull;
    RWRegister[92]  = RWNull;
    RWRegister[93]  = RWNull;
    RWRegister[94]  = RWNull;

    RWRegister[99]  = RWNull;
    RWRegister[100] = RWNull;
    RWRegister[101] = RWNull;
    RWRegister[102] = RWNull;
    
    RWRegister[103] = RWNull;
    RWRegister[104] = RWNull;
    RWRegister[105] = RWNull;
    RWRegister[106] = RWNull;
    
    RWRegister[111] = RWNull;
    RWRegister[112] = RWNull;
    RWRegister[113] = RWNull;

}
void *GetASProtocl()
{
    return RWRegister;
}






