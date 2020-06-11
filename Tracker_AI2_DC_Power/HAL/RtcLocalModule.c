/**********************************************************************************************
�������Ų�����Դ�Ƽ��ɷ����޹�˾�Ϻ��ӹ�˾ 2004-2020 ��Ȩ����

�ļ�����      RtcLocalModule.c
���ߣ�        ����    
�汾��        V0.1
���ڣ�        2020-06-08
������        ʹ��STM32F1ϵ�����õ�RTCģ�飬��Ҫ����ʹ��RTC�������ж�������STM32��ֹͣ
              ģʽ��Ȼ�����ж��н���ι������Ϊ��ֹͣģʽ�ж������Ź������ڼ�ʱ�������
							ι������и�λ��������Ҫ��һ��RTC����
������        ����
��ʷ��        ����
�޶��汾��    V0.1
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
		NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;		//RTCȫ���ж�
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�1λ,�����ȼ�3λ
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	//��ռ���ȼ�0λ,�����ȼ�4λ
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//ʹ�ܸ�ͨ���ж�
		NVIC_Init(&NVIC_InitStructure);		//����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
}

/***********************************************
*�� �� ����    RTC_Local_Init
*��    ����    RTC��ʼ����ÿ���ϵ��ֱ���������ã���Ϊֻʹ��RTC�ж�����νʱ��׼��׼
*��    �룺    ����
*�� �� ֵ��    �½��ɹ�����0��ʧ�ܷ���1
*��    �ڣ�    2020-06-08
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
	
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//ʹ��PWR��BKP����ʱ��   
		PWR_BackupAccessCmd(ENABLE);	//ʹ�ܺ󱸼Ĵ�������  
	  //LSI����ǰ����ʹ�ܣ�������������
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET&&temp<250)	//���ָ����RCC��־λ�������,�ȴ����پ������
		{
				temp++;
		}
		if(temp>=250)return 1;																				//��ʼ��ʱ��ʧ��,ʹ�õľ���������	    
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);												//����RTCʱ��(RTCCLK),ѡ��LSI��ΪRTCʱ��    
		RCC_RTCCLKCmd(ENABLE);																				//ʹ��RTCʱ��  
		RTC_WaitForLastTask();																				//�ȴ����һ�ζ�RTC�Ĵ�����д�������
		RTC_WaitForSynchro();																					//�ȴ�RTC�Ĵ���ͬ�� 
    RTC_ClearITPendingBit(RTC_IT_ALR);		//�������ж�	  	
		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);		//�������ж�		
		RTC_WaitForLastTask();																				//�ȴ����һ�ζ�RTC�Ĵ�����д�������
		RTC_EnterConfigMode();																				//��������	
		RTC_SetPrescaler(40000-1); 																	  //����RTCԤ��Ƶ��ֵ
		RTC_WaitForLastTask();																				//�ȴ����һ�ζ�RTC�Ĵ�����д�������
//	RTC_Set(2020,6,8,9,0,0);  																	  //����ʱ��2020-06-08 9:0:0	
		
		RTC_ExitConfigMode(); 																				//�˳�����ģʽ  	
		RTC_NVIC_Config();																						//RCT�жϷ�������		 
    	
		RTC_ITConfig(RTC_IT_ALR, ENABLE);													    //�Ȳ�ʹ��RTC�����ж�
		RTC_WaitForLastTask();
		RTC_SetAlarm(RTC_GetCounter() + 10);                            //�������ӣ�10S��һ��
	  RTC_WaitForLastTask();				
		return 0; 																										//ok
}	

//void RTC_IRQHandler(void)
//{		 
//		if(RTC_GetITStatus(RTC_IT_ALR)!= RESET)//�����ж�
//		{
//		    RTC_ClearITPendingBit(RTC_IT_ALR);		//�������ж�	  	
//		} 	
//		RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);		//�������ж�
//		RTC_WaitForLastTask();
//		RTC_SetAlarm(RTC_GetCounter()+10);                            //�������ӣ�10S��һ��		
////		IWDG_ReloadCounter(); 
//    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);//STOPģʽ		
//}

void RTCAlarm_IRQHandler(void)
{
	  if (RTC_GetITStatus(RTC_IT_ALR) != RESET)
    {
		    RTC_ClearFlag(RTC_FLAG_ALR);
	      EXTI_ClearITPendingBit(EXTI_Line17);// ��EXTI_Line17����λ
        if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)// ��黽�ѱ�־�Ƿ�����
        {
            PWR_ClearFlag(PWR_FLAG_WU);// ������ѱ�־
        }			
        uc_rtcFlag = 1;				
		}			
}
