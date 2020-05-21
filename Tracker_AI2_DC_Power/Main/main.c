#include "stm32f10x.h"
#include "core_cm3.h"
#include "os.h"
#include "cpu.h"
#include "GlobalDefine.h"
#include "GlobalOs.h"
#include "LED.h"
#include "FlashStorage.h"
#include "Init.h"
#include "ModbusFrame.h"
#include "ProtocolAnalysis.h"
#include "WorkModeManager.h"
#include "PowerManagement.h"
#include "ADCSample.h"
#include "TMPSensor.h"

GlobalVariableDef GlobalVariable = {0};
/***************************************************************************************************
                                    Task priorities
***************************************************************************************************/
#define SAMPLING_PRIO              6
#define POWER_MANAGEMENT_PRIO      8
#define MODBUS_OVER_LORA_PRIO      9
#define PARAMETER_SAVE_PRIO       15
#define LED_IWDG_PRIO             17
/***************************************************************************************************
                                    Task stack
***************************************************************************************************/
#define SAMPLING_STK_SIZE            256
#define POWER_MANAGEMENT_STK_SIZE    256
#define MODBUS_OVER_LORA_STK_SIZE    512
#define PARAMETER_SAVE_STK_SIZE      256
#define LED_IWDG_STK_SIZE            256

static CPU_STK sampling_stk[SAMPLING_STK_SIZE];
static CPU_STK power_management_stk[POWER_MANAGEMENT_STK_SIZE];
static CPU_STK modbus_over_lora_stk[MODBUS_OVER_LORA_STK_SIZE];
static CPU_STK parameter_save_stk[PARAMETER_SAVE_STK_SIZE];
static CPU_STK led_IWDG_stk[LED_IWDG_STK_SIZE];
/***************************************************************************************************
                                    Task TCB
***************************************************************************************************/
OS_TCB SamplingTCB;
OS_TCB PowerManagementTCB;
OS_TCB ModbusOverLoRaTCB;
OS_TCB ParameterSaveTCB;
OS_TCB LedIWDGTCB;
/***************************************************************************************************
                                    OS EVENT FLAG
***************************************************************************************************/
OS_FLAG_GRP	Motor_continuous_Flags;       //create a event flag
/***************************************************************************************************
                                    Task functions declaration
***************************************************************************************************/
static void Sampling(void *p_arg);
static void PowerManagement(void *p_arg);
static void ModbusOverLoRa(void *p_arg);
static void ParameterSave(void *p_arg);
static void LedIWDG(void *p_arg);
/***************************************************************************************************
                                    Main program
***************************************************************************************************/
int main(void)
{
    OS_ERR err;
	
    OSInit(&err);
    NVICInit();  
    ParaInit();
    IWDGInit();
    OSTaskCreate((OS_TCB *    )&ParameterSaveTCB,
                 (CPU_CHAR *  )"ParameterSave",
                 (OS_TASK_PTR )ParameterSave,
                 (void *      )0,
                 (OS_PRIO     )PARAMETER_SAVE_PRIO,
                 (CPU_STK *   )&parameter_save_stk,
                 (CPU_STK_SIZE)PARAMETER_SAVE_STK_SIZE/10,
                 (CPU_STK_SIZE)PARAMETER_SAVE_STK_SIZE,
                 (OS_MSG_QTY  )0u,
                 (OS_TICK     )0u,
                 (void *      )0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR),
                 (OS_ERR *    )&err);
                 
    OSTaskCreate((OS_TCB *    )&SamplingTCB,
                 (CPU_CHAR *  )"Sampling",
                 (OS_TASK_PTR )Sampling,
                 (void *      )0,
                 (OS_PRIO     )SAMPLING_PRIO,
                 (CPU_STK *   )&sampling_stk,
                 (CPU_STK_SIZE)SAMPLING_STK_SIZE/10,
                 (CPU_STK_SIZE)SAMPLING_STK_SIZE,
                 (OS_MSG_QTY  )0u,
                 (OS_TICK     )0u,
                 (void *      )0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR),
                 (OS_ERR *    )&err);              
           
    OSTaskCreate((OS_TCB *    )&PowerManagementTCB,
                 (CPU_CHAR *  )"PowerManagement",
                 (OS_TASK_PTR )PowerManagement,
                 (void *      )0,
                 (OS_PRIO     )POWER_MANAGEMENT_PRIO,
                 (CPU_STK *   )&power_management_stk,
                 (CPU_STK_SIZE)POWER_MANAGEMENT_STK_SIZE/10,
                 (CPU_STK_SIZE)POWER_MANAGEMENT_STK_SIZE,
                 (OS_MSG_QTY  )0u,
                 (OS_TICK     )0u,
                 (void *      )0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR),
                 (OS_ERR *    )&err); 
     
    OSTaskCreate((OS_TCB *    )&ModbusOverLoRaTCB,
                 (CPU_CHAR *  )"ModbusOverLoRa",
                 (OS_TASK_PTR )ModbusOverLoRa,
                 (void *      )0,
                 (OS_PRIO     )MODBUS_OVER_LORA_PRIO,
                 (CPU_STK *   )&modbus_over_lora_stk,
                 (CPU_STK_SIZE)MODBUS_OVER_LORA_STK_SIZE/10,
                 (CPU_STK_SIZE)MODBUS_OVER_LORA_STK_SIZE,
                 (OS_MSG_QTY  )0u,
                 (OS_TICK     )0u,
                 (void *      )0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR),
                 (OS_ERR *    )&err);
                                             
    OSTaskCreate((OS_TCB *    )&LedIWDGTCB,
                 (CPU_CHAR *  )"LEDIWDG",
                 (OS_TASK_PTR )LedIWDG,
                 (void *      )0,
                 (OS_PRIO     )LED_IWDG_PRIO,
                 (CPU_STK *   )&led_IWDG_stk,
                 (CPU_STK_SIZE)LED_IWDG_STK_SIZE/10,
                 (CPU_STK_SIZE)LED_IWDG_STK_SIZE,
                 (OS_MSG_QTY  )0u,
                 (OS_TICK     )0u,
                 (void *      )0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR),
                 (OS_ERR *    )&err);
                 
    OS_CPU_SysTickInit(720000);
    
    OSStart(&err);
}

/***************************************************************************************************
                                    Task functions
***************************************************************************************************/
static void Sampling(void *p_arg)
{
    OS_ERR         err;
    p_arg = p_arg;
    
    TMPSensorInit();
    while(1)
    {
        GetTMP(&GlobalVariable.Tmp.BoardTmp);
        OSTimeDly(90,OS_OPT_TIME_DLY,&err);
    }
}

static void PowerManagement(void *p_arg)
{
    OS_ERR err;
    p_arg = p_arg;
    PowerInit();
    while(1)
    {
        PowerMangement();
        OSTimeDly(10,OS_OPT_TIME_DLY,&err); 
    }
}
static void ModbusOverLoRa(void *p_arg)
{
    OS_ERR                err;
    static unsigned char  send_buffer[300],send_buffer5[300];
    static unsigned char  read_buffer[300],read_buffer5[300];
    static unsigned short reply_num_of_bytes,reply_num_of_bytes5;
    static unsigned short num_of_bytes,num_of_bytes5;
    static unsigned short delay_count = 0;
    
    ModbusPortInit(0);
	  ModbusPortInit(1);
    ProtocolInit();
    while(1)
    {
        if(ModbusRead(0,&num_of_bytes,read_buffer) == 0)
        {
            if(read_buffer[0] == GlobalVariable.ConfigPara.ComID || read_buffer[0] == 0)
            {
                delay_count = 0;
							  reply_num_of_bytes = 0;
                reply_num_of_bytes = ProtocolAnalysis(read_buffer,send_buffer,num_of_bytes);
                if((reply_num_of_bytes > 0)&&(read_buffer[0] != 0))
                {
                    ModbusSend(0,reply_num_of_bytes,send_buffer);
                }
            }
						else
						{
							if((delay_count++) >= (5 * 60 * 1000 /10))  //持续5分钟收的没有自己的ID或者广发
							{
								LoRaModulePowerOff();                     //lora模块断电
								OSTimeDly(100,OS_OPT_TIME_DLY,&err);      //延时1S
								LoRaModulePowerOn();                      //lora模块上电
								delay_count = 0;													//重新计时
							}						
						}
        }
				else
				{
				  if((delay_count++) >= (5 * 60 * 1000 /10))  //持续5分钟收的没有数据或校验错误
					{
					  LoRaModulePowerOff();                     //lora模块断电
					  OSTimeDly(100,OS_OPT_TIME_DLY,&err);      //延时1S
						LoRaModulePowerOn();                      //lora模块上电
						delay_count = 0;													//重新计时
					}						
				}
				
				if(ModbusRead(1,&num_of_bytes5,read_buffer5) == 0)
        {
            if(read_buffer5[0] == GlobalVariable.ConfigPara.ComID || read_buffer5[0] == 0)
            {
							  reply_num_of_bytes5 = 0;
                reply_num_of_bytes5 = ProtocolAnalysis(read_buffer5,send_buffer5,num_of_bytes5);
                if((reply_num_of_bytes5 > 0)&&(read_buffer5[0] != 0))
                {
                    ModbusSend(1,reply_num_of_bytes5,send_buffer5);
                }
            }
        }
        OSTimeDly(1,OS_OPT_TIME_DLY,&err);
    }
}

static void ParameterSave(void *p_arg)
{
    OS_ERR err;
    p_arg = p_arg;
    CPU_SR_ALLOC();
    if(ReadConfigParaFromFlash((unsigned short *)&GlobalVariable.ConfigPara.ComID,sizeof(GlobalVariable.ConfigPara)/2) != 0)
    {
        ParaInit();  //电源板就ComID一个参数
    }
 
    while(1)
    {
        /*System Update*/
        if(GlobalVariable.WorkMode.SystemStatus == 1)    /*Flash wipe*/
        {
            EraseNewSystemFlash();
            GlobalVariable.WorkMode.SystemStatus = 2;
        }
        else if(GlobalVariable.WorkMode.SystemStatus == 2)
        {
            GlobalVariable.IAPUpdateSys.UpdateTimeOut ++;
            if(GlobalVariable.IAPUpdateSys.UpdateTimeOut >= 30*1000/10) /*TimeOut 30s*/
            {
                GlobalVariable.WorkMode.SystemStatus = 0;
            }
            else
            {
                if(GlobalVariable.WriteFlag.UpdatePackWrite == 1)
                {
                    OS_CRITICAL_ENTER();
                    GlobalVariable.WriteFlag.UpdatePackWrite = 0;
                    WriteNewSystemToFlash((unsigned short *)GlobalVariable.IAPUpdateSys.UpdateBuffer,GlobalVariable.IAPUpdateSys.PackNumber);
                    GlobalVariable.IAPUpdateSys.PackNumber++;
                    OS_CRITICAL_EXIT();
                }
            }
        }
        else if(GlobalVariable.WorkMode.SystemStatus == 3)   /*Update finish*/
        {
            GlobalVariable.WorkMode.SystemStatus = 0;
            WriteIAPFlagToFlash(GlobalVariable.IAPUpdateSys.PackNumber * 256 / 2 + 10);
            NVIC_SystemReset();
        }
        else
        {
            GlobalVariable.WorkMode.SystemStatus = 0;
        }
        
        OSTimeDly(1,OS_OPT_TIME_DLY,&err);
    }
}

static void LedIWDG(void *p_arg)
{
    OS_ERR           err;
    unsigned char   i = 0;	
    p_arg = p_arg;
  
    LEDInit();
    while(1)
    {
			  i++;
        if(GlobalVariable.WorkMode.SystemStatus == 0)
        {
            if(i > 6)
            {
                i = 0;
                LEDFlash();
            }
        }
        else if(GlobalVariable.WorkMode.SystemStatus == 1)
        {
            LEDTurnOn();
        }
        else if(GlobalVariable.WorkMode.SystemStatus == 2)
        {
            if(i >= 2)
            {
                i = 0;
                LEDFlash();
            }
        }
        else
        {
            LEDTurnOff();
        }
        IWDG_ReloadCounter(); 
        OSTimeDly(10,OS_OPT_TIME_DLY,&err);
    }
}
