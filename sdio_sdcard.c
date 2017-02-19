#include "sdio_sdcard.h"
#include "usart1.h"
/*Private Marco------------------------------------------------------------*/
/**
 * @brief SDIO Static flags, TimeOut, FIFO Address
 */
#define NULL 0
#define SDIO_STATIC_FLAGS ((uint32_t)0x000005FF)
#define SDIO_CMD0TIMEOUT  ((uint32_t)0x00010000)

/**
 * @brief Mask for errors Card Status R1 (OCR Register)
 */
#define SD_OCR_ADDR_OUT_OF_RANGE ((uint32_t)0x80000000)
#define SD_OCR_ADDR_MISALIGNED  ((uint32_t)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR  ((uint32_t)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR  ((uint32_t)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM ((uint32_t)0x08000000)
#define SD_OCR_WRITE_PORT_VIOLATION ((uint32_t)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED  ((uint32_t)0x01000000)
#define SD_OCR_COM_CRC_FAILED      ((uint32_t)0x00800000)
#define SD_OCR_ILLEGAL_CMD         ((uint32_t)0x00400000)
#define SD_OCR_CARD_ECC_FAILED     ((uint32_t)0x00200000)
#define SD_OCR_CC_ERROR   ((uint32_t)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR ((uint32_t)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN  ((uint32_t)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN  ((uint32_t)0x00020000)
#define SD_OCR_CID_CSD_OVERWRITE   ((uint32_t)0x00010000)
#define SD_OCR_WP_ERASE_SKIP  ((uint32_t)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED ((uint32_t)0x00004000)
#define SD_OCR_ERASE_RESET  ((uint32_t)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR ((uint32_t)0x00000008)
#define SD_OCR_ERRORBITS  ((uint32_t)0xFDFFE008)

/**
 * @brief Masks for R6 Response
 */
#define SD_R6_GENERAL_UNKNOWN_ERROR  ((uint32_t)0x00002000)
#define SD_R6_ILLEGAL_CMD    ((uint32_t)0x00004000)
#define SD_R6_COM_CRC_FAILED ((uint32_t)0x00008000)

#define SD_VOLTAGE_WINDOW_SD  ((uint32_t)0x80100000)
#define SD_HIGH_CAPACITY  ((uint32_t)0x40000000)
#define SD_STD_CAPACITY  ((uint32_t)0x00000000)
#define SD_CHECK_PATTERN   ((uint32_t)0x000001AA)
#define SD_MAX_VOLT_TRIAL   ((uint32_t)0x0000FFFF)
#define SD_ALLZERO        ((uint32_t))

#define SD_WIDE_BUS_SUPPORT  ((uint32_t)0x00040000)
#define SD_SINGLE_BUS_SUPPORT ((uint32_t)0x00020000)
#define SD_CARD_LOCKED      ((uint32_t)0x02000000)

#define SD_DATATIMEOUT  ((uint32_t)0xFFFFFFFF)
#define SD_0TO7BITS  ((uint32_t)0x000000FF)
#define SD_8TO15BITS ((uint32_t)0x0000FF00)
#define SD_16TO23BITS ((uint32_t)0x00FF0000)
#define SD_24TO32BITS ((uint32_t)0xFF000000)
#define SD_MAX_DATA_LENGTH ((0x01FFFFFF))

#define SD_HALFFIFO ((0X00000008)) ((0X00000008))
#define SD_HALFFIFOBYTES ((uint32_t)0x00000020)


/**
 * @brief Command Class Supported
 */
#define SD_CCCC_LOCK_UNLOCK   ((uint32_t)0x00000080)
#define SD_CCCC_WRITE_PORT   ((uint32_t)0x00000040)
#define SD_CCCC_ERASE     ((uint32_t)0x00000020)

/**
 * @brief Following Commands are SD Card Specific commands.
 *        SDIO_APP_CMD should be sent before sending these commands.
 */
#define SDIO_SEND_IF_COND   ((uint32_t)0x00000008)



/*Private variables-------------------------------------------------------------*/
static uint32_t CardType = SDIO_STD_CAPACITY_SD_CARD_V1_1
static uint32_t CSD_Tab[4], CID_Tab[4], RCA = 0;
static uint8_t  SDSTATUS_Tab[16];
__IO uint32_t StopCondition = 0;
__IO SD_Error TransferError =SD_OK;
__IO uint32_t TransferEnd = 0;
SD_CardInfo SDCardInfo;

/*SDIO Initialization-----------------------------------------------------------*/
SDIO_InitTypeDef SDIO_InitStructure;
SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
SDIO_DataInitTypeDef SDIO_DataInitStructure;

/*Private function prtotypes------------------------------------------------------*/
static SD_Error CmdError(void);
static SD_Error CmdResp1Error(uint8_t cmd);
static SD_Error CmdResp7Error(void);
static SD_Error CmdResp3Error(void);
static SD_Error CmdResp2Error(void);
static SD_Error CmdResp6Error(uint8_t cmd, uint16_t *prca);
static SD_Error SDEnWideBus(FunctionalState NewState);
static SD_Error IsCardProgramming(uint8_t *pstatus);
static SD_Error FindSCR(uint16_t rca, uint32_t *pscr);

uint8_t convert_from_bytes_to_power_of_two(uint16_t NumberOfBytes);

/*Private functions--------------------------------------------------------------------*/

/*
 *Function Name:SD_DeInit
 *Description:Reset SDIO Port
 *Input:null
 *Output:null
 */
void SD_DeInit(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /*!<Disable SDIO Clock */
  SDIO_ClockCmd(Disable);
  
  /*!<Set Power State to OFF */
  SDIO_SetPowerState(SDIO_PowerState_OFF);
  
  /*!<DeInitializes the SDIO Peripheral */
  SDIO_DeInit();
  
  /*!<Disable the SDIO AHB Clock*/
  RCC_AHBPerpheralClockCmd(RCC_AHBPerph_SDIO,Disable);
  
  /*!< Configure PC.08, PC.09, PC.10, PC.11, PC.12 pin: D0, D1, D2, D3, CLK pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /*!< Configure PD.02 CMD line */
  GPIO_Initstructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOD, &GPIO_Initstructure);
}


/*
 *Function Name:NVIC_Configuration
 *Brief:Configure the SDIO Preemption to the Highest Level
 *Input:null
 *Output:null
 */
void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_Initstructure;
  
  /*Configure the NVIC Preemption Priority Bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  
  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Initstructure.NVIC_IRQChannelCmd = Enable;
  NVIC_Init(&NVIC_InitStructure);
}


/*
 *@brief Returns the DMA End Of Transfer Status.
 *@param None
 *@retval DMA SDIO Channel Status.
 */
uint32_t SD_DMAEndOfTransferStatus(void)
{
  return (uint32_t)DMA_GetFlagStatus(DMA2_FLAG_TC4);   //Channel4 transfer complete flag.
}

/*
 * 函数名：SD_DMA_RxConfig
 * 描述  ：为SDIO接收数据配置DMA2的通道4的请求
 * 输入  ：BufferDST：用于装载数据的变量指针
 		   BufferSize：	缓冲区大小
 * 输出  ：无
 */
void SD_DMA_RxConfigure(uint32_t *BufferDST, uint32_t BufferSize)
{
  DMA_InitTypeDef DMA_InitStructure;
  
  DMA_ClaerFlag(DMA2_FLAG_TC4 | DMA2_FLAG_TE4 | DMA2_FLAG_HT4 | DMA2_FLAG_GL4);
  
  /*!<DMA2 Channel4 disable*/
  DMA_Cmd(DMA2_Channel4,DISABLE); //SDIO为第四通道
  
  /*!< DMA2 Channel4 Config */
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SDIO_FIFO_ADDRESS;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)BufferDST;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = BufferSize / 4;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priotity_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA2_Channel4, &DMA_Initstructure);
  
  /*!< DMA2_Channel4,enable */
  DMA_CMD(DMA2_Channel4, ENABLE);
}
  
/*
 * 函数名：SD_DMA_TxConfig
 * 描述  ：为SDIO发送数据配置DMA2的通道4的请求
 * 输入  ：BufferDST：用于装载数据的变量指针
 		   BufferSize：	缓冲区大小
 * 输出  ：无
 */
 void SD_DMA_TxConfigure(uint32_t *BufferSRC, uint32_t BufferSize)
{
  DMA_InitTypeDef DMA_InitStructure;
  DMA_ClearFlag(DMA2_FLAG_TC4 | DMA2_FLAG_TE4 | DMA2_FLAG_HT4 | DMA2_FLAG_GL4);
   
  /*!<DMA2 Channel4 disable*/
  DMA_Cmd(DMA2_Channel4,DISABLE); //SDIO为第四通道
   
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SDIO_FIFO_ADDRESS;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)BufferSRC;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = BufferSize / 4;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priotity_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA2_Channel4, &DMA_Initstructure);
  
  /*!< DMA2_Channel4,enable */
  DMA_CMD(DMA2_Channel4, ENABLE);
 }
 

/*
 * 函数名：GPIO_Configuration
 * 描述  ：初始化SDIO用到的引脚，开启时钟。
 * 输入  ：无
 * 输出  ：无
 * 调用  ：内部调用
 */
static void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_Initstructure;
  
   /*!< GPIOC and GPIOD Periph clock enable */
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_GPIOC | RCC_AHB2Periph_GPIOD , ENABLE);
  
   /*!< GPIOC and GPIOD Periph clock enable */
  GPIO_Initstructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /*!< Configure PD.02 CMD line */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  
  /*!< Enable the SDIO AHB Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO, ENABLE);
  
  /*!< Enable the DMA2 Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, Enable);
}



/*
 * 函数名：SD_Init
 * 描述  ：初始化SD卡，使卡处于就绪状态(准备传输数据)
 * 输入  ：无
 * 输出  ：-SD_Error SD卡错误代码
 *         成功时则为 SD_OK
 * 调用  ：外部调用
 */

 SD_Error SD_Init(void)
 {
   SD_Error errorstatus = SD_OK;
   GPIO_Configuration();
   SDIO_DeInit();
   errorstatus = SD_PowerOn();
   
   if(errorstatus != SD_OK)
   {
     return(errorstatus);
   }
   
   errorstatus = SD_InitializeCards();
   
   if(errorstatus != SD_OK)
   {
     return(errorstatus);
   }
   
   
   /*!< Configure the SDIO peripheral 
  上电识别，卡初始化都完成后，进入数据传输模式，提高读写速度
  速度若超过24M要进入bypass模式	  
  !< on STM32F2xx devices, SDIOCLK is fixed to 48MHz
  !< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_TRANSFER_CLK_DIV) */ 
   SDIO_InitStructure.SDIO_ClockDiv = SDIO_TRANSFER_CLK_DIV;
   SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
   SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
   SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
   SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
   SDIO_InitStructure.SDIO_HardWareFlowControl = SDIO_HardWareFlowControl_Disable;
   SDIO_Init(&SDIO_InitStructure);
   
   if (errorstatus == SD_OK)
   {
     errorstatus = SD_GetCardInfo(&SDCardInfo);
   }
   
   if (errorstatus == SD_OK)
   {
     errorstatus = SD_SelectDeselect((uint32_t)) (SDCardInfo.RCA << 16);
   }
   
   if (errorstatus == SD_OK)
   {
     errorstatus = SD_EnableWideBusOperation(SDIO_BusWide_4b);
   }
   
   return(errorstatus);
 }


/**
  * @brief  Gets the cuurent sd card data transfer status.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be: 
  *        - SD_TRANSFER_OK: No data transfer is acting
  *        - SD_TRANSFER_BUSY: Data transfer is acting
  */
SDTransferState SD_GetStatus(void)
{
  SDCardState cardstate = SD_CARD_TRANSFER;
  cardstate = SD_GetState();
  if (cardstate == SD_CARD_TRANSFER)
  {
    return(SD_TRANSFER_OK);
  }
  else if (cardstate == SD_CARD_ERROR)
  {
    return(SD_TRANSFER_ERROR);
  }
  else
  {
    return(SD_TRANSFER_BUSEY);
  }
}

/**
  * @brief  Returns the current card's state.
  * @param  None
  * @retval SDCardState: SD Card Error or SD Card Current State.
  */
SD_CardState SD_GetState(void)
{
  uint32_t resp1 = 0;
  
  if (SD_SendStatus(&resp1) != SD_OK)
  {
    return SD_CARD_ERROR;
  }
  
  else
  {
    return (SD_CardState) （（resp1 >> 9) &0x0F）;
  }
}

/*
 * 函数名：SD_PowerON
 * 描述  ：确保SD卡的工作电压和配置控制时钟
 * 输入  ：无
 * 输出  ：-SD_Error SD卡错误代码
 *         成功时则为 SD_OK
 * 调用  ：在 SD_Init() 调用
 */
SD_Error SD_PowerON(void)
{
  SD_Error errorstatus = SD_OK;
  uint32_t response = 0, count = 0, validvoltage = 0;
  uint32_t SDType = SD_STD_CAPACITY;
  
  /*!< Power ON Sequence -----------------------------------------------------*/
  /*!< Configure the SDIO peripheral */
  /*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_INIT_CLK_DIV) */
  /*!< on STM32F2xx devices, SDIOCLK is fixed to 48MHz */
  /*!< SDIO_CK for initialization should not exceed 400 KHz */ 
  /*初始化时的时钟不能大于400KHz*/ 
  
  SDIO_InitStructure.SDIO_ClockDiv = SDIO_INIT_CLK_DIV;	/* HCLK = 72MHz, SDIOCLK = 72MHz, SDIO_CK = HCLK/(178 + 2) = 400 KHz */
  SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
  SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
  SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
  SDIO_InitStructure.SDIO_HardWareFlowControl = SDIO_HardWareFlowControl_Disable;
  SDIO_Init(&SDIO_InitStructure);
  
  /*!< Set Power State to ON */
  SDIO_SetPowerState(SDIO_PowerState_ON);
  
  /*!< Enable SDIO Clock */
  SDIO_ClockCmd (ENABLE);
  
   /*下面发送一系列命令,开始卡识别流程*/
  /*!< CMD0: GO_IDLE_STATE ---------------------------------------------------*/
  /*!< No CMD response required */
  
  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_Cmd_Go_IDLE_STATE;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  
  errorstatus = CmdError();
  
  if (errorstatus != SD_OK)
  {
    /*!< CMD Response TimeOut (wait for CMDSENT flag) */
    return (errorstatus);
  }
  
  /*!< CMD8: SEND_IF_COND ----------------------------------------------------*/
  /*!< Send CMD8 to verify SD card interface operating condition */
  /*!< Argument: - [31:12]: Reserved (shall be set to '0')
               - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
               - [7:0]: Check Pattern (recommended 0xAA) */
  /*!< CMD Response: R7 */
  
  SDIO_CmdInitStructure.SDIO_Argument = SD_CHECK_PATTERN;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SDIO_SEND_IF_COND;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  
  errorstatus = CmdResp7Error();
  
  if (errorstatus = SD_OK)
  {
    CardType = SDIO_STD_CAPACITY_SD_CARD_V2_0;
    SDType = SD_HIGH_CAPACITY;
  }
  
  else
  {
    SDIO_CmdInitStructure.SDIO_Argument = 0x00;
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);
    errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
  }
  
  SDIO_CmdInitStructure.SDIO_Argument = 0x00;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
  
  /*!< If errorstatus is Command TimeOut, it is a MMC card */
  /*!< If errorstatus is SD_OK it is a SD card: SD card 2.0 (voltage range mismatch)
     or SD card 1.x */
  if (errorstatus == SD_OK)
  {
    /*!< SD CARD */
    /*!< Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
    while((!validvoltage) && (count << SD_MAX_VOLT_TRIAL))
    {
      SDIO_CmdInitStructure.SDIO_Argument = 0x00;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
   
      if (errorstatus != SD_OK)
      {
        return(errorstatus);
      }
      
      /*若卡需求电压在SDIO的供电电压范围内，会自动上电并标志pwr_up位*/
      response = SDIO_GetResponse(SDIO_RESP1);
      validvoltage = (((response >> 31) == 1) ? 1 : 0);  //读取卡的ocr寄存器的pwr_up位，看是否已工作在正常电压
      count++;
     }
    if (count >= SD_MAX_VOLT_TRIAL)
    {
      errorstatus = SD_INVALID_VOLTRANGE;
      return(errorstatus);
    }
    
    if (response &= SD_HIGH_CAPACITY)
    {
      CardType = SDIO_HIGH_CAPACITY_SD_CARD;
    }
  }
  return(errorstatus); 
}

/*
 * 函数名：SD_PowerOFF
 * 描述  ：关掉SDIO的输出信号
 * 输入  ：无
 * 输出  ：-SD_Error SD卡错误代码
 *         成功时则为 SD_OK
 * 调用  ：外部调用
 */

SD_Error SD_PowerOFF(void)
{
  SD_Error errorstatus = SD_OK;
  SDIO_SetPowerState(SDIO_PowerState_OFF);
  
  return(errorstatus);
}


  /*
 * 函数名：SD_InitializeCards
 * 描述  ：初始化所有的卡或者单个卡进入就绪状态
 * 输入  ：无
 * 输出  ：-SD_Error SD卡错误代码
 *         成功时则为 SD_OK
 * 调用  ：在 SD_Init() 调用，在调用power_on（）上电卡识别完毕后，调用此函数进行卡初始化
 */
SD_Error SD_InitializeCards(void)
{
  SD_Error errorstatus = SD_OK;
  uint16_t rca = 0x01;
  
  if (SDIO_GetPowerState() == SDIO_PowerState_OFF)
  {
    errorstatus = SD_REQUEST_NOT_APPLICABLE;
    return(errorstatus);
  }
  
  if (SDIO_SECURE_DIGITAL_TO_CARD != CardType)
  {
    SDIO_CmdInitStructure.SDIO_Argument = 0x0;
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);
    
    errorstatus = CmdResp2Error();
    
    if (SD_OK != errorstatus)
    {
      return(errorstatus);
    }
    
    CID_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
    CID_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
    CID_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
    CID_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
    
  }
  
    if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) ||  (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) ||  (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == CardType)
      ||  (SDIO_HIGH_CAPACITY_SD_CARD == CardType))
    {
      /*!< Send CMD3 SET_REL_ADDR with argument 0 */
      /*!< SD Card publishes its RCA. */
      SDIO_CmdInitStructure.SDIO_Argument = 0x00;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      
      errorstatus = CmdResp6Error(SD_CMD_SET_REL_ADDR, &rca);
      
      if (SD_OK != errorstatus)
      {
        return(errorstatus);
      }
    }
    
    if (SDIO_SECURE_DIGITAL_IO_CARD != CardType)
    {
      RCA = rca;
      
      /*!< Send CMD9 SEND_CSD with argument as card's RCA */
      SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) (rca << 16);
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_CSD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      
      errorstatus = CmdResp2Error();
      
      if (SD_OK != errorstatus)
      {
        return(errorstatus);
      }
    CSD_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
    CSD_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
    CSD_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
    CSD_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
    }
  
  errorstatus = SD_OK; /*!< All cards get intialized */
  
  return(errorstatus);
 }

/*
 * 函数名：SD_GetCardInfo
 * 描述  ：获取SD卡的具体信息
 * 输入  ：-cardinfo 指向SD_CardInfo结构体的指针
 *         这个结构里面包含了SD卡的具体信息
 * 输出  ：-SD_Error SD卡错误代码
 *         成功时则为 SD_OK
 * 调用  ：外部调用
 */
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo)
{
  SD_Error errorstatus = SD_OK;
  uint16_4 tmp = 0;
  
  cardinfo->CardType = (uint8_t)CardType;
  cardingo->RCA = (uint16_t)CardType;
  
  /*!< Byte 0 */
  tmp = (uint8_t)((CSD_Tab[0] & 0xFF000000) >> 24);
  cardinfo->SD_csd.CSDStruct = (tmp & 0xC0) >> 6;
  cardinfo->SD_csd.SysSpecVersion = (tmp & ox3C) >> 2;
  cardinfo->SD_csd.Reserved1 = tmp & 0x03;
  
  /*!< Byte 1 */
  tmp = (uint8_t)((CSD_TAB[0] & 0x00FF0000) >>16);
  cardinfo->SD_csd.TACC = tmp;
  
  /*!< Byte 2 */
  tmp = (uint8_t)((CSD_Tab[0] & 0x0000FF00) >> 8);
  cardinfo->SD_csd.NSAC = tmp;
  
  /*!< Byte 3 */
  tmp = (uint8_t)(CSD_Tab[0] & 0x000000FF);
  cardinfo->SD_csd.MaxBusClkFrec = tmp;
  
  /*!< Byte 4 */ 
  tmp = (uint8_t)((CSD_Tab[1] & 0xFF000000) >> 24);
  cardinfo->SD_csd.CardComdClasses = tmp << 4;
  
   /*!< Byte 5 */
  tmp = (uint8_t)((CSD_Tab[1] & 0x00FF0000) >> 16);
  cardinfo->SD_csd.CardComdClasses |= (tmp & 0xF0) >> 4;
  cardinfo->SD_csd.RdBlockLen = tmp & 0x0F;
  
  /*!< Byte 6 */
  tmp = (uint8_t)((CSD_Tab[1] & 0x0000FF00) >> 8);
  cardinfo->SD_csd.PartBlockRead = (tmp & 0x80) >> 7;
  cardinfo->SD_csd.WrBlockMisalign = (tmp & 0x40) >> 6;
  cardinfo->SD_csd.RdBlockMisalign = (tmp & 0x20) >> 5;
  cardinfo->SD_csd.DSRImpl = (tmp & 0x10) >> 4;
  cardinfo->SD_csd.Reserved2 = 0; /*!< Reserved */
  
  if ((CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0))
  {
    cardinfo->SD_csd_DeviceSize |= (tmp & 0x03) << 10;
    
    /*!< Byte 7 */
    tmp = (uint8_t)(CSD_Tab[1] & 0x000000FF);
    cardinfo->SD_csd.DeviceSize |= (tmp) << 2;

    /*!< Byte 8 */
    tmp = (uint8_t)((CSD_Tab[2] & 0xFF000000) >> 24);
    cardinfo->SD_csd.DeviceSize |= (tmp & 0xC0) >> 6;

    cardinfo->SD_csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
    cardinfo->SD_csd.MaxRdCurrentVDDMax = (tmp & 0x07);

    /*!< Byte 9 */
    tmp = (uint8_t)((CSD_Tab[2] & 0x00FF0000) >> 16);
    cardinfo->SD_csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
    cardinfo->SD_csd.MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
    cardinfo->SD_csd.DeviceSizeMul = (tmp & 0x03) << 1;
    /*!< Byte 10 */
    tmp = (uint8_t)((CSD_Tab[2] & 0x0000FF00) >> 8);
    cardinfo->SD_csd.DeviceSizeMul |= (tmp & 0x80) >> 7;
    
    cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1) ;
    cardinfo->CardCapacity *= (1 << (cardinfo->SD_csd.DeviceSizeMul + 2));
    cardinfo->CardBlockSize = 1 << (cardinfo->SD_csd.RdBlockLen);
    cardinfo->CardCapacity *= cardinfo->CardBlockSize;
  }
  
  else if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
  {
    /*!< Byte 7 */
    tmp = (uint8_t)(CSD_Tab[1] & 0x000000FF);
    cardinfo->SD_csd.DeviceSize = (tmp & 0x3F) << 16;

    /*!< Byte 8 */
    tmp = (uint8_t)((CSD_Tab[2] & 0xFF000000) >> 24);

    cardinfo->SD_csd.DeviceSize |= (tmp << 8);

    /*!< Byte 9 */
    tmp = (uint8_t)((CSD_Tab[2] & 0x00FF0000) >> 16);

    cardinfo->SD_csd.DeviceSize |= (tmp);

    /*!< Byte 10 */
    tmp = (uint8_t)((CSD_Tab[2] & 0x0000FF00) >> 8);
    
    cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1) * 512 * 1024;
    cardinfo->CardBlockSize = 512;
  }
  
  cardinfo->SD_csd.EraseGrSize = (tmp & 0x40) >> 6;
  cardinfo->SD_csd.EraseGrMul = (tmp & 0x3F) << 1;
  
  /*!< Byte 11 */
  tmp = (uint8_t)(CSD_Tab[2] & 0x000000FF);
  cardinfo->SD_csd.EraseGrMul |= (tmp & 0x80) >> 7;
  cardinfo->SD_csd.WrProtectGrSize = (tmp & 0x7F);
																   
  /*!< Byte 12 */
  tmp = (uint8_t)((CSD_Tab[3] & 0xFF000000) >> 24);
  cardinfo->SD_csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
  cardinfo->SD_csd.ManDeflECC = (tmp & 0x60) >> 5;
  cardinfo->SD_csd.WrSpeedFact = (tmp & 0x1C) >> 2;
  cardinfo->SD_csd.MaxWrBlockLen = (tmp & 0x03) << 2;

  /*!< Byte 13 */
  tmp = (uint8_t)((CSD_Tab[3] & 0x00FF0000) >> 16);
  cardinfo->SD_csd.MaxWrBlockLen |= (tmp & 0xC0) >> 6;
  cardinfo->SD_csd.WriteBlockPaPartial = (tmp & 0x20) >> 5;
  cardinfo->SD_csd.Reserved3 = 0;
  cardinfo->SD_csd.ContentProtectAppli = (tmp & 0x01);

  /*!< Byte 14 */
  tmp = (uint8_t)((CSD_Tab[3] & 0x0000FF00) >> 8);
  cardinfo->SD_csd.FileFormatGrouop = (tmp & 0x80) >> 7;
  cardinfo->SD_csd.CopyFlag = (tmp & 0x40) >> 6;
  cardinfo->SD_csd.PermWrProtect = (tmp & 0x20) >> 5;
  cardinfo->SD_csd.TempWrProtect = (tmp & 0x10) >> 4;
  cardinfo->SD_csd.FileFormat = (tmp & 0x0C) >> 2;
  cardinfo->SD_csd.ECC = (tmp & 0x03);

  /*!< Byte 15 */
  tmp = (uint8_t)(CSD_Tab[3] & 0x000000FF);
  cardinfo->SD_csd.CSD_CRC = (tmp & 0xFE) >> 1;
  cardinfo->SD_csd.Reserved4 = 1;


  /*!< Byte 0 */
  tmp = (uint8_t)((CID_Tab[0] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ManufacturerID = tmp;

  /*!< Byte 1 */
  tmp = (uint8_t)((CID_Tab[0] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.OEM_AppliID = tmp << 8;

  /*!< Byte 2 */
  tmp = (uint8_t)((CID_Tab[0] & 0x000000FF00) >> 8);
  cardinfo->SD_cid.OEM_AppliID |= tmp;

  /*!< Byte 3 */
  tmp = (uint8_t)(CID_Tab[0] & 0x000000FF);
  cardinfo->SD_cid.ProdName1 = tmp << 24;

  /*!< Byte 4 */
  tmp = (uint8_t)((CID_Tab[1] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ProdName1 |= tmp << 16;

  /*!< Byte 5 */
  tmp = (uint8_t)((CID_Tab[1] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.ProdName1 |= tmp << 8;

  /*!< Byte 6 */
  tmp = (uint8_t)((CID_Tab[1] & 0x0000FF00) >> 8);
  cardinfo->SD_cid.ProdName1 |= tmp;

  /*!< Byte 7 */
  tmp = (uint8_t)(CID_Tab[1] & 0x000000FF);
  cardinfo->SD_cid.ProdName2 = tmp;

  /*!< Byte 8 */
  tmp = (uint8_t)((CID_Tab[2] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ProdRev = tmp;

  /*!< Byte 9 */
  tmp = (uint8_t)((CID_Tab[2] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.ProdSN = tmp << 24;

  /*!< Byte 10 */
  tmp = (uint8_t)((CID_Tab[2] & 0x0000FF00) >> 8);
  cardinfo->SD_cid.ProdSN |= tmp << 16;

  /*!< Byte 11 */
  tmp = (uint8_t)(CID_Tab[2] & 0x000000FF);
  cardinfo->SD_cid.ProdSN |= tmp << 8;

  /*!< Byte 12 */
  tmp = (uint8_t)((CID_Tab[3] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ProdSN |= tmp;

  /*!< Byte 13 */
  tmp = (uint8_t)((CID_Tab[3] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.Reserved1 |= (tmp & 0xF0) >> 4;
  cardinfo->SD_cid.ManufactDate = (tmp & 0x0F) << 8;

  /*!< Byte 14 */
  tmp = (uint8_t)((CID_Tab[3] & 0x0000FF00) >> 8);
  cardinfo->SD_cid.ManufactDate |= tmp;

  /*!< Byte 15 */
  tmp = (uint8_t)(CID_Tab[3] & 0x000000FF);
  cardinfo->SD_cid.CID_CRC = (tmp & 0xFE) >> 1;
  cardinfo->SD_cid.Reserved2 = 1;
  
  return(errorstatus);
}

/**
  * @brief  Enables wide bus opeartion for the requeseted card if supported by 
  *         card.
  * @param  WideMode: Specifies the SD card wide bus mode. 
  *   This parameter can be one of the following values:
  *     @arg SDIO_BusWide_8b: 8-bit data transfer (Only for MMC)
  *     @arg SDIO_BusWide_4b: 4-bit data transfer
  *     @arg SDIO_BusWide_1b: 1-bit data transfer
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_GetCardStatus(SD_CardStatus *cardstatus)
{
  SD_Error errorstatus = SD_OK;
  uint8_t tmp = 0;
  
  errorstatus = SD_SendSDStatus((uint32_t *)SDSTATUS_Tab);
  
  if (errorstatus != SD_OK)
  {
    return(errorstatus);
  }
  
  /*!<Byte 0 */
  tmp = (uint8_t)((SDSTATUS_Tab[0] &0xC0) >> 6);
  cardstatus->DAT_BUS_WIDTH = tmp;
  
  /*!< Byte 0 */
  tmp = (uint8_t)((SDSTATUS_Tab[0] & 0x20) >> 5);
  cardstatus->SECURED_MODE = tmp;

  /*!< Byte 2 */
  tmp = (uint8_t)((SDSTATUS_Tab[2] & 0xFF));
  cardstatus->SD_CARD_TYPE = tmp << 8;

  /*!< Byte 3 */
  tmp = (uint8_t)((SDSTATUS_Tab[3] & 0xFF));
  cardstatus->SD_CARD_TYPE |= tmp;

  /*!< Byte 4 */
  tmp = (uint8_t)(SDSTATUS_Tab[4] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA = tmp << 24;

  /*!< Byte 5 */
  tmp = (uint8_t)(SDSTATUS_Tab[5] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp << 16;

  /*!< Byte 6 */
  tmp = (uint8_t)(SDSTATUS_Tab[6] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp << 8;

  /*!< Byte 7 */
  tmp = (uint8_t)(SDSTATUS_Tab[7] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp;

  /*!< Byte 8 */
  tmp = (uint8_t)((SDSTATUS_Tab[8] & 0xFF));
  cardstatus->SPEED_CLASS = tmp;

  /*!< Byte 9 */
  tmp = (uint8_t)((SDSTATUS_Tab[9] & 0xFF));
  cardstatus->PERFORMANCE_MOVE = tmp;

  /*!< Byte 10 */
  tmp = (uint8_t)((SDSTATUS_Tab[10] & 0xF0) >> 4);
  cardstatus->AU_SIZE = tmp;

  /*!< Byte 11 */
  tmp = (uint8_t)(SDSTATUS_Tab[11] & 0xFF);
  cardstatus->ERASE_SIZE = tmp << 8;

  /*!< Byte 12 */
  tmp = (uint8_t)(SDSTATUS_Tab[12] & 0xFF);
  cardstatus->ERASE_SIZE |= tmp;

  /*!< Byte 13 */
  tmp = (uint8_t)((SDSTATUS_Tab[13] & 0xFC) >> 2);
  cardstatus->ERASE_TIMEOUT = tmp;

  /*!< Byte 13 */
  tmp = (uint8_t)((SDSTATUS_Tab[13] & 0x3));
  cardstatus->ERASE_OFFSET = tmp;
 
  return(errorstatus);
}

/*
 * 函数名：SD_EnableWideBusOperation
 * 描述  ：配置卡的数据宽度(但得看卡是否支持)
 * 输入  ：-WideMode 指定SD卡的数据线宽
 *         具体可配置如下
 *         @arg SDIO_BusWide_8b: 8-bit data transfer (Only for MMC)
 *         @arg SDIO_BusWide_4b: 4-bit data transfer
 *         @arg SDIO_BusWide_1b: 1-bit data transfer (默认)
 * 输出  ：-SD_Error SD卡错误代码
 *         成功时则为 SD_OK
 * 调用  ：外部调用
 */

SD_Error SD_EnableWideBusOperation(uint32_t WideMode)
{
  SD_Error errorstatus = SD_OK;
  
  /*!< MMC Card doesn't support this feature */
  if (SDIO_MULTIMEDIA_CARD == CardType)
  {
    errorstatus = SD_UNSUPPORTED_FEATURE;
    return(errorstatus);
  }
  
  else if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType)
           {
             if (SDIO_BusWide_8b == WideMode)
             {
               errorstatus = SD_UNSUPPORTED_FEATURE;
               return(errorstatus);
             }
             else if (SDIO_BusWide_4b == WideMode)
             {
               errorstatus = SDEnWideBus(ENABLE);
               
               if (SD_OK == errorstatus)
               {
                 /*!<Configure the SDIO Peripheral */
                 SDIO_InitStructure.SDIO_ClockDiv = SDIO_TRANSFER_CLK_DIV;
                 SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
                 SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
                 SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
                 SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_4b;
                 SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
                 SDIO_Init(&SDIO_InitStructure);
               }
             }
             else
             {
               errorstatus = SDEnWideBus(Disable);
               
               if (SD_OK == errorstatus)
               {
                 /*!<Configure the SDIO Peripheral */
                 SDIO_InitStructure.SDIO_ClockDiv = SDIO_TRANSFER_CLK_DIV;
                 SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
                 SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
                 SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
                 SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
                 SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HarewareFlowControl_Disable;
                 SDIO_Init(&SDIO_InitStructure);
               }
             }
           }
           return(errorstatus);
}
           
/*
 * 函数名：SD_SelectDeselect
 * 描述  ：利用cmd7，选择卡相对地址为addr的卡，取消选择其它卡
 *   		如果addr = 0,则取消选择所有的卡
 * 输入  ：-addr 选择卡的地址
 * 输出  ：-SD_Error SD卡错误代码
 *         成功时则为 SD_OK
 * 调用  ：外部调用
 */	
 SD_Error SD_SelectDeselect(uint32_t addr)
 {
   SD_Error errorstatus = SD_OK;
   
   /*!<Send CMD7 SDIO_SEL_DESEL_CARD */
   SDIO_CmdInitStructure.SDIO_Argument = addr;
   SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
   SDIO_CmdInitStructure.SDIO_SDIO_Response = SDIO_Response_Short;
   SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
   SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
   SDIO_SendCommand(&SDIO_CmdInitStructure);
   
   errorstatus = CmdResp1Error(SD_CMD_SEL_DESEL_CARD);
   return(errorstatus);
 }

/**
  * @brief  Allows to read one block from a specified address in a card. The Data
  *         transfer can be managed by DMA mode or Polling mode. 
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the 
  *            data transfer and it is ready for data.            
  * @param  readbuff: pointer to the buffer that will contain the received data
  * @param  ReadAddr: Address from where data are to be read.  
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @retval SD_Error: SD Card Error code.
  */
   SD_Error SD_ReadBlock(uint8_t *readbuff, uint32_t ReadAddr, uint16_t BlockSize)
   {
     SD_Error errorstatus = SD_OK;
     
     #if defined (SD_POLLING_MODE)
     uint32_t count = 0, *tempbuff = (uint32_t *)readbuff;
     #endif
     
     TransferError = SD_OK;
     TransferEnd = 0;
     StopCondition = 0;
     
     SDIO->DCTL = 0X0;
     
     
     if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
     {
       BlockSize = 512;
       ReadAddr /= 512;
     }
     /*******************add，没有这一段容易卡死在DMA检测中*************************************/
     /*!<Set Block Size for Card，cmd16,若是sdsc卡，可以用来设置块大小，若是sdhc卡，块大小为512字节，不受cmd16影响 */
     SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) BlockSize;
     SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
     SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
     SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
     SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
     SDIO_SendCommand(&SDIO_CmdInitStructure);
     
     errorstatus = CmpResp1Error(SD_CMD_SET_BLOCKLEN);
     
     if (SD_OK !=errorstatus)
     {
       return(errorstatus);
     }
/***********************************************************/
     SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
     SDIO_DataInitStructure.SDIO_DataLength = BlockSize;
     SDIO_DataInitStructure.SDIO_DataBlockSize = (uint32_t) 9 << 4;
     SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
     SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
     SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
     SDIO_DataConfig(&SDIO_DataInitStructure);
	   
     /*!< Send CMD17 READ_SINGLE_BLOCK */
     SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)ReadAddr;
     SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;
     SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
     SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
     SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
     SDIO_SendCommand(&SDIO_CmdInitStructure);
	   
     errorstatus = CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);

     if (errorstatus != SD_OK)
     {
	     return(errorstatus);
     }
#if defined (SD_POLLING_MODE)
    /*!< In case of single block transfer, no need of stop transfer at all.*/
    /*!< Polling mode */
    while (!!(SDIO->STA &(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
    {
       if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
       {
	  for (count = 0; count < 8; count++)
	  {
	     *(tempbuff + count) = SDIO_ReadData();
          }
	   tempbuff += 8;
       }
    }
	   
    if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
    {
       SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
       errorstatus = SD_DATA_TIMEOUT;
       return(errorstatus);
    }

    else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL != RESET))
    {
      SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
      errorstatus = SD_DATA_CRC_FAIL;
      return(errorstatus);
    }

    else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
    {
	SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
	errorstatus = SD_RX_OVERRUN;
	return(errorstatus);
	    
    else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_STBITERR);
	errorstatus = SD_START_BIT_ERR;
	return(errorstatus);
    }
    
    while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)
    {
      *tembuff = SDIO_ReadData();
      tempbuff++;
    }
   
     /*!< Clear all the static flags */
     SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	    
#elif defined (SD_DMA_MODE)
     SDIO_ITConfig(SDIO_IT_DATAEND, ENABLE);
     SDIO_DMACmd(ENABLE);
     SD_DMACmd(ENABLE);
     SD_DMA_RxConfig((uint32_t *)readbuff, BlockSize);
#endif
	    
     return(errorstatus);
   }
	   
/**
  * @brief  Allows to read blocks from a specified address  in a card.  The Data
  *         transfer can be managed by DMA mode or Polling mode. //分两个模式
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.	   //dma模式时要调用以下两个函数
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer. 
  *          - SD_GetStatus(): to check that the SD Card has finished the 
  *            data transfer and it is ready for data.   
  * @param  readbuff: pointer to the buffer that will contain the received data.
  * @param  ReadAddr: Address from where data are to be read.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be read.
  * @retval SD_Error: SD Card Error code.
  */
