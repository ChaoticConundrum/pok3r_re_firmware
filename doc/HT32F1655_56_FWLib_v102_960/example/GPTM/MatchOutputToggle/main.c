/*********************************************************************************************************//**
 * @file    GPTM/MatchOutputToggle/main.c
 * @version $Rev:: 929          $
 * @date    $Date:: 2015-09-16 #$
 * @brief   Main program.
 *************************************************************************************************************
 * @attention
 *
 * Firmware Disclaimer Information
 *
 * 1. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, which is supplied by Holtek Semiconductor Inc., (hereinafter referred to as "HOLTEK") is the
 *    proprietary and confidential intellectual property of HOLTEK, and is protected by copyright law and
 *    other intellectual property laws.
 *
 * 2. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, is confidential information belonging to HOLTEK, and must not be disclosed to any third parties
 *    other than HOLTEK and the customer.
 *
 * 3. The program technical documentation, including the code, is provided "as is" and for customer reference
 *    only. After delivery by HOLTEK, the customer shall use the program technical documentation, including
 *    the code, at their own risk. HOLTEK disclaims any expressed, implied or statutory warranties, including
 *    the warranties of merchantability, satisfactory quality and fitness for a particular purpose.
 *
 * <h2><center>Copyright (C) 2014 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Includes ------------------------------------------------------------------------------------------------*/
#include "ht32.h"
#include "ht32_board.h"

/** @addtogroup HT32_Series_Peripheral_Examples HT32 Peripheral Examples
  * @{
  */

/** @addtogroup GPTM_Examples GPTM
  * @{
  */

/** @addtogroup MatchOutputToggle
  * @{
  */


/* Private variables ---------------------------------------------------------------------------------------*/
u32 CH0_Frequency;
u32 CH1_Frequency;
u32 CH2_Frequency;
u32 CH3_Frequency;
u32 CH0_DeltaCCR;
u32 CH1_DeltaCCR;
u32 CH2_DeltaCCR;
u32 CH3_DeltaCCR;

/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
  * @brief  Main program.
  * @retval None
  * @note At this stage the microcontroller clock setting is already configured,
  *       this is done through SystemInit() function which is called from startup
  *       file (startup_ht32fxxxx.s) before branching to application main.
  *       To reconfigure the default setting of SystemInit() function, refer to
  *       system_ht32fxxxx.c file
  * @details Main program as following
  *  - Enable peripheral's clock of AFIO and GPTM.
  *  - Configure GPIO's AFIO mode as GT0_CH0~GT0_CH3 function.
  *  - GPTM0 Time base configuration:
  *     - GPTM0_PCLK = 72 MHz
  *     - Prescaler = 0
  *     - CounterReload = 65535
  *     - GPTM0_CounterClock = (GPTM0_PCLK / (Prescaler + 1)) = 72 MHz
  *  - Compute the channels compare match delta value.
  *     - Channel0 frequency = 1KHz
  *     - Channel1 frequency = 2KHz
  *     - Channel2 frequency = 3KHz
  *     - Channel3 frequency = 4KHz
  *  - GPTM0 Match Output Toggle Mode configuration: Channel0 ~ Channel3.
  *  - Enable GPTM0 channel0 ~ channel3 compare match interrupts.
  *    The interrupt service routine will update the CHxCCR value for next compare match.
  *    The next compare match value = CHxCCR + (72MHz / (Channelx_frequency * 2 * (Prescaler + 1))
  *  - Enable GPTM0.
  *
  ***********************************************************************************************************/
int main(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  GPTM_TimeBaseInitTypeDef TimeBaseInit;
  GPTM_OutputInitTypeDef OutInit;

  /* Enable peripheral's clock of AFIO and GPTM                                                             */
  CKCUClock.Bit.AFIO       = 1;
  CKCUClock.Bit.GPTM0      = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  /* Configure GPIO's AFIO mode as GT0_CH0~GT0_CH3 function.                                                */
  AFIO_GPAConfig(AFIO_PIN_4 | AFIO_PIN_5 | AFIO_PIN_6 | AFIO_PIN_7, AFIO_MODE_4);

  /* Time base configuration                                                                                */
  TimeBaseInit.CounterMode = GPTM_CNT_MODE_UP;
  TimeBaseInit.CounterReload = 65535;
  TimeBaseInit.Prescaler = 0;
  TimeBaseInit.PSCReloadTime = GPTM_PSC_RLD_IMMEDIATE;
  GPTM_TimeBaseInit(HT_GPTM0, &TimeBaseInit);

  /* Compute the channels compare match delta value.
     Channel0 frequency = 1KHz
     Channel1 frequency = 2KHz
     Channel2 frequency = 3KHz
     Channel3 frequency = 4KHz
  */
  CH0_Frequency = 1000;
  CH1_Frequency = 2000;
  CH2_Frequency = 3000;
  CH3_Frequency = 4000;
  CH0_DeltaCCR = SystemCoreClock / (CH0_Frequency * 2 * (TimeBaseInit.Prescaler + 1));
  CH1_DeltaCCR = SystemCoreClock / (CH1_Frequency * 2 * (TimeBaseInit.Prescaler + 1));
  CH2_DeltaCCR = SystemCoreClock / (CH2_Frequency * 2 * (TimeBaseInit.Prescaler + 1));
  CH3_DeltaCCR = SystemCoreClock / (CH3_Frequency * 2 * (TimeBaseInit.Prescaler + 1));

  /* GPTM0 Match Output Toggle Mode configuration: Channel0 ~ 3                                             */
  OutInit.Channel = GPTM_CH_0;
  OutInit.OutputMode = GPTM_OM_MATCH_TOGGLE;
  OutInit.Control = GPTM_CHCTL_ENABLE;
  OutInit.Polarity = GPTM_CHP_NONINVERTED;
  OutInit.Compare = CH0_DeltaCCR;
  GPTM_OutputInit(HT_GPTM0, &OutInit);

  OutInit.Channel = GPTM_CH_1;
  OutInit.Compare = CH1_DeltaCCR;
  GPTM_OutputInit(HT_GPTM0, &OutInit);

  OutInit.Channel = GPTM_CH_2;
  OutInit.Compare = CH2_DeltaCCR;
  GPTM_OutputInit(HT_GPTM0, &OutInit);

  OutInit.Channel = GPTM_CH_3;
  OutInit.Compare = CH3_DeltaCCR;
  GPTM_OutputInit(HT_GPTM0, &OutInit);

  /* Enable GPTM0 channel0 ~ channel3 compare match interrupts                                              */
  GPTM_IntConfig(HT_GPTM0, GPTM_INT_CH0CC | GPTM_INT_CH1CC | GPTM_INT_CH2CC | GPTM_INT_CH3CC, ENABLE);
  NVIC_EnableIRQ(GPTM0_IRQn);

  /* Enable GPTM                                                                                            */
  GPTM_Cmd(HT_GPTM0, ENABLE);

  while (1);
}

#if (HT32_LIB_DEBUG == 1)
/*********************************************************************************************************//**
  * @brief  Report both the error name of the source file and the source line number.
  * @param  filename: pointer to the source file name.
  * @param  uline: error line source number.
  * @retval None
  ***********************************************************************************************************/
void assert_error(u8* filename, u32 uline)
{
  /*
     This function is called by IP library that the invalid parameters has been passed to the library API.
     Debug message can be added here.
  */

  while (1)
  {
  }
}
#endif


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
