/**********************************************************************************************
江苏中信博新能源科技股份有限公司上海子公司 2004-2020 版权所有

文件名：      RtcLocalModule.c
作者：        汤涛    
版本：        V0.1
日期：        2020-06-08
描述：        使用STM32F1系列内置的RTC模块，主要用于使用RTC的闹钟中断来唤醒STM32的停止
              模式，然后在中断中进行喂狗。因为在停止模式中独立看门狗依旧在计时，如果不
							喂狗会进行复位，所以需要加一个RTC闹钟
其他：        暂无
历史：        暂无
修订版本：    V0.1
**********************************************************************************************/
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"
#include "RtcLocalModule.h"
#include "GlobalDefine.h"
#include "os.h"

extern unsigned short us_tick;
extern unsigned char  uc_rtcFlag;
//unsigned char test_rtc = 0;


static void RTC_NVIC_Config(void)
{	
		NVIC_InitTypeDef NVIC_InitStructure;
		NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;		//RTC全局中断
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//先占优先级1位,从优先级3位
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	//先占优先级0位,从优先级4位
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//使能该通道中断
		NVIC_Init(&NVIC_InitStructure);		//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
}

/***********************************************
*函 数 名：    RTC_Local_Init
*描    述：    RTC初始化，每次上电就直接重新配置，因为只使用RTC中断无所谓时间准不准
*输    入：    暂无
*返 回 值：    新建成功返回0，失败返回1
*日    期：    2020-06-08
***********************************************/
int RTC_Local_Init(void)
{
    int temp=0; 
	
	  EXTI_InitTypeDef EXTI_InitStructure;
		EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
	
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//使能PWR和BKP外设时钟   
		PWR_BackupAccessCmd(ENABLE);	//使能后备寄存器访问  
	  //LSI晶振前面已使能，这里无需设置
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET&&temp<250)	//检查指定的RCC标志位设置与否,等待低速晶振就绪
		{
				temp++;
		}
		if(temp>=250)return 1;																				//初始化时钟失败,使用的晶振有问题	    
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);												//设置RTC时钟(RTCCLK),选择LSI作为RTC时钟    
		RCC_RTCCLKCmd(ENABLE);																				//使能RTC时钟  
		RTC_WaitForLastTask();																				//等待最近一次对RTC寄存器的写操作完成
		RTC_WaitForSynchro();																					//等待RTC寄存器同步 
    RTC_ClearITPendingBit(RTC_IT_ALR);		//清闹钟中断	  	
		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);		//清闹钟中断		
		RTC_WaitForLastTask();																				//等待最近一次对RTC寄存器的写操作完成
		RTC_EnterConfigMode();																				//允许配置	
		RTC_SetPrescaler(40000-1); 																	  //设置RTC预分频的值
		RTC_WaitForLastTask();																				//等待最近一次对RTC寄存器的写操作完成
//	RTC_Set(2020,6,8,9,0,0);  																	  //设置时间2020-06-08 9:0:0	
		
		RTC_ExitConfigMode(); 																				//退出配置模式  	
		RTC_NVIC_Config();																						//RCT中断分组设置		 
    	
		RTC_ITConfig(RTC_IT_ALR, ENABLE);													    //先不使能RTC闹钟中断
		RTC_WaitForLastTask();
		RTC_SetAlarm(RTC_GetCounter() + 10);                            //设置闹钟，10S响一次
	  RTC_WaitForLastTask();				
		return 0; 																										//ok
}	

//void RTC_IRQHandler(void)
//{		 
//		if(RTC_GetITStatus(RTC_IT_ALR)!= RESET)//闹钟中断
//		{
//		    RTC_ClearITPendingBit(RTC_IT_ALR);		//清闹钟中断	  	
//		} 	
//		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);		//清闹钟中断
//		RTC_WaitForLastTask();
//		RTC_SetAlarm(RTC_GetCounter()+10);                            //设置闹钟，10S响一次		
////		IWDG_ReloadCounter(); 
//    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);//STOP模式		
//}

void RTCAlarm_IRQHandler(void)
{
	  if (RTC_GetITStatus(RTC_IT_ALR) != RESET)
    {
		    RTC_ClearFlag(RTC_FLAG_ALR);
	      EXTI_ClearITPendingBit(EXTI_Line17);// 清EXTI_Line17挂起位
        if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)// 检查唤醒标志是否设置
        {
            PWR_ClearFlag(PWR_FLAG_WU);// 清除唤醒标志
        }			
        uc_rtcFlag = 1;				
		}			
}
