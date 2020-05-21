#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_dma.h"
#include "ModbusFrame.h"

#define TX_EN_485()      GPIO_SetBits(GPIOA, GPIO_Pin_11)
#define RX_EN_485()      GPIO_ResetBits(GPIOA, GPIO_Pin_11)
#define Power_on_LoRa()  GPIO_ResetBits(GPIOE, GPIO_Pin_14)
#define Power_off_LoRa() GPIO_SetBits(GPIOE, GPIO_Pin_14)
/***************************************************************************************************
                                Private variable declaration
***************************************************************************************************/
typedef struct 
{
    unsigned short RxDataCnt;
    unsigned short TxDataCnt;
    unsigned char  TimerEn;
    unsigned char  TimeOutCnt;
    unsigned char  RxBuf[300];
    unsigned char  TxBuf[300];
    void (* SendByDMA)(unsigned short data_size);
    
}ComParaDef;

/***************************************************************************************************
                                  Private variable definition
***************************************************************************************************/
static ComParaDef ComPara[2] = {0};
static ComParaDef * PortList[2] = {(void * )0};

/***************************************************************************************************
                                    Private functions
***************************************************************************************************/

static void UART1Init(void);
static void UART5Init(void);
static void DMA2C5Send(unsigned short data_size);
static unsigned short CRC16Check(unsigned char *buf, unsigned short num_of_bytes);

static void UART1Init()
{
    GPIO_InitTypeDef   GPIOInitStruc;
    USART_InitTypeDef  USARTInitStruc;
    DMA_InitTypeDef    DMAInitStruc;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOE, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
    
    DMAInitStruc.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    DMAInitStruc.DMA_MemoryBaseAddr     = (uint32_t)&(ComPara[0].TxBuf);
    DMAInitStruc.DMA_DIR                = DMA_DIR_PeripheralDST;
    DMAInitStruc.DMA_BufferSize         = 128;
    DMAInitStruc.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMAInitStruc.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMAInitStruc.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMAInitStruc.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMAInitStruc.DMA_Mode               = DMA_Mode_Normal;
    DMAInitStruc.DMA_Priority           = DMA_Priority_Medium;
    DMAInitStruc.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMAInitStruc);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC,DISABLE);
    DMA_Cmd(DMA1_Channel4, DISABLE);
    
    /* USART1 Tx */
    GPIOInitStruc.GPIO_Pin   = GPIO_Pin_9;
    GPIOInitStruc.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIOInitStruc.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIOInitStruc);
    
    /* USART1 Rx */
    GPIOInitStruc.GPIO_Pin   = GPIO_Pin_10;
    GPIOInitStruc.GPIO_Mode  = GPIO_Mode_IPU;    //输入下拉
    GPIOInitStruc.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIOInitStruc);
    
    /* USART1 Dir */
    GPIOInitStruc.GPIO_Pin   = GPIO_Pin_11;
    GPIOInitStruc.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIOInitStruc.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIOInitStruc);	
    RX_EN_485();	                               //默认接收
		 
    /* Power PIN-Hardware power:reset*/
    GPIOInitStruc.GPIO_Pin   = GPIO_Pin_14;
    GPIOInitStruc.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIOInitStruc.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOE, &GPIOInitStruc);
    Power_on_LoRa();
		
    USARTInitStruc.USART_BaudRate            = 9600;
    USARTInitStruc.USART_WordLength          = USART_WordLength_8b;
    USARTInitStruc.USART_StopBits            = USART_StopBits_1;
    USARTInitStruc.USART_Parity              = USART_Parity_No;
    USARTInitStruc.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USARTInitStruc.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USARTInitStruc);
    
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
    USART_ClearITPendingBit(USART1, USART_IT_TC);
    USART_ITConfig(USART1,USART_IT_TC,ENABLE);
    
    USART_DMACmd(USART1,USART_DMAReq_Tx, ENABLE);
    
    USART_Cmd(USART1,ENABLE);
}

static void UART5Init()
{
    GPIO_InitTypeDef   GPIOInitStruc;
    USART_InitTypeDef  USARTInitStruc;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOE, ENABLE);
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
        
    /* UART5 Tx */
    GPIOInitStruc.GPIO_Pin   = GPIO_Pin_12;
    GPIOInitStruc.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIOInitStruc.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIOInitStruc);
    
    /* UART5 Rx */
    GPIOInitStruc.GPIO_Pin   = GPIO_Pin_2;
    GPIOInitStruc.GPIO_Mode  = GPIO_Mode_IPU;    //输入下拉
    GPIOInitStruc.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIOInitStruc);
    	
    USARTInitStruc.USART_BaudRate            = 9600;
    USARTInitStruc.USART_WordLength          = USART_WordLength_8b;
    USARTInitStruc.USART_StopBits            = USART_StopBits_1;
    USARTInitStruc.USART_Parity              = USART_Parity_No;
    USARTInitStruc.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USARTInitStruc.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UART5, &USARTInitStruc);
    
    USART_ClearITPendingBit(UART5, USART_IT_RXNE);
    USART_ITConfig(UART5,USART_IT_RXNE,ENABLE);
    USART_ClearITPendingBit(UART5, USART_IT_TC);
    USART_ITConfig(UART5,USART_IT_TC,ENABLE);
    
    USART_Cmd(UART5,ENABLE);
}



static void DMA2C5Send(unsigned short data_size)
{
    TX_EN_485();
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA1_Channel4->CNDTR = data_size;
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

static unsigned short CRC16Check(unsigned char *buf, unsigned short num_of_bytes)
{
    unsigned short i     = 0;
    unsigned char  j     = 0;
    unsigned short w_crc = 0xFFFF;
    
    for(i = 0; i < num_of_bytes; i++)
    {
        w_crc ^= buf[i];
        for(j = 0; j < 8; j++)
        {
            if(w_crc & 1)
            {
                w_crc >>= 1;
                w_crc ^= 0xA001;
            }
            else
            {
                w_crc >>= 1;
            }
        }
    }
    
    return w_crc;
}
/***************************************************************************************************
                                Public functions
***************************************************************************************************/
void ModbusPortInit(unsigned char port_number)
{
    switch (port_number)
    {
        case 0:
            UART1Init();
            PortList[0] = &ComPara[0];
            PortList[0]->SendByDMA = DMA2C5Send;
            break;
        case 1: /*Reserved for expansion*/
				    UART5Init();
            PortList[1] = &ComPara[1];
            break;
        default:
            break;
    }
        
}

void LoRaModulePowerOff(void)
{
	Power_off_LoRa();
}

void LoRaModulePowerOn(void)
{
	Power_on_LoRa();
}

void ModbusSend(unsigned char port_number, unsigned short num_of_byte, unsigned char *SendBuffer)
{
    unsigned short i, j;
    unsigned short crc_value;
	  
	  switch (port_number)
    {
        case 0:
            TX_EN_485();
						for(i=0;i<50000;i++){__nop();__nop();__nop();__nop();__nop();}
						for(i=0;i<50000;i++){__nop();__nop();__nop();__nop();__nop();}
						for(i = 0; i < num_of_byte; i++ )
						{
								PortList[port_number]->TxBuf[i] = *(SendBuffer + i);
						}
						crc_value = CRC16Check(PortList[port_number]->TxBuf,num_of_byte );
						PortList[port_number]->TxBuf[num_of_byte] = (crc_value)&0xFF;
						PortList[port_number]->TxBuf[num_of_byte+1] = (crc_value>>8)&0xFF;
						(*(PortList[port_number]->SendByDMA))(num_of_byte+2); /* Send */
            break;
        case 1: /*Reserved for expansion*/
						for(i=0;i<50000;i++){__nop();__nop();__nop();__nop();__nop();}
						for(i=0;i<50000;i++){__nop();__nop();__nop();__nop();__nop();}
						for(i = 0; i < num_of_byte; i++ )
						{
								PortList[port_number]->TxBuf[i] = *(SendBuffer + i);
						}
						crc_value = CRC16Check(PortList[port_number]->TxBuf,num_of_byte );
						PortList[port_number]->TxBuf[num_of_byte] = (crc_value)&0xFF;
						PortList[port_number]->TxBuf[num_of_byte+1] = (crc_value>>8)&0xFF;
						for(i=0;i<num_of_byte+2;i++)
						{
								USART_SendData(UART5, PortList[port_number]->TxBuf[i]);//向串口5发送数据
								for(j=0;j<5000;j++){__nop();__nop();__nop();__nop();__nop();}
				//				while(USART_GetFlagStatus(UART5,USART_FLAG_TC)!=SET);//等待发送结束
						}
            break;
        default:
            break;
    }
}

int ModbusRead(unsigned char port_number, unsigned short *num_of_byte, unsigned char *ReadBuffer)
{
    unsigned short crc_value;
    unsigned short i;
    
    if(PortList[port_number]->TimerEn == 0xAB)
    {
        if(PortList[port_number]->TimeOutCnt++ >= 6)   /* Receive a frame */
        {
            PortList[port_number]->TimerEn = 0x56;
            PortList[port_number]->TimeOutCnt = 0;
            
            *num_of_byte = PortList[port_number]->RxDataCnt;
            PortList[port_number]->RxDataCnt = 0;
            crc_value = CRC16Check(PortList[port_number]->RxBuf,*num_of_byte);
            if(crc_value == 0)
            {
                for (i = 0; i < *num_of_byte; i++)
                {
                    *(ReadBuffer + i) = PortList[port_number]->RxBuf[i];
                    PortList[port_number]->RxBuf[i] = 0xFF;
                }
                return 0;
            }
        }
    }
    else
    {
        PortList[port_number]->TimeOutCnt = 0;
    }
    return -1;
}


/***************************************************************************************************
                                    IRQHandler
***************************************************************************************************/
void USART1_IRQHandler()
{
    if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        PortList[0]->TimeOutCnt = 0;
        PortList[0]->TimerEn = 0xAB;
        if(PortList[0] ->RxDataCnt < 300)
        {
            PortList[0]->RxBuf[PortList[0] ->RxDataCnt++] = USART1->DR;
        }
        else
        {
            PortList[0]->TimerEn = 0x56;
            PortList[0]->RxDataCnt = 0;
            PortList[0]->RxBuf[PortList[0] ->RxDataCnt] = USART1->DR;
        }
    }
    else if(USART_GetITStatus(USART1,USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        PortList[0]->RxDataCnt = 0;
        PortList[0]->TimeOutCnt = 0;
        PortList[0]->TimerEn = 0x56;
        
        DMA_Cmd(DMA1_Channel4, DISABLE);
        RX_EN_485();
    }
}

void UART5_IRQHandler()
{
    if(USART_GetITStatus(UART5,USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(UART5, USART_IT_RXNE);
        PortList[1]->TimeOutCnt = 0;
        PortList[1]->TimerEn = 0xAB;
        if(PortList[1] ->RxDataCnt < 300)
        {
            PortList[1]->RxBuf[PortList[1] ->RxDataCnt++] = UART5->DR;
        }
        else
        {
            PortList[1]->TimerEn = 0x56;
            PortList[1]->RxDataCnt = 0;
            PortList[1]->RxBuf[PortList[1] ->RxDataCnt] = UART5->DR;
        }
    }
    else if(USART_GetITStatus(UART5,USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(UART5, USART_IT_TC);
        PortList[1]->RxDataCnt = 0;
        PortList[1]->TimeOutCnt = 0;
        PortList[1]->TimerEn = 0x56;
    }
}

