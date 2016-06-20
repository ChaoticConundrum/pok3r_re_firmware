/*********************************************************************************************************//**
 * @file    ADC/PWMTrigger_OneShotMode/main.c
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

/** @addtogroup ADC_Examples ADC
  * @{
  */

/** @addtogroup PWMTrigger_OneShotMode

  * @{
  */


/* Private variables ---------------------------------------------------------------------------------------*/
extern u32 gADC_Result[3];
extern volatile bool gADC_CycleEndOfConversion;

/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
  * @brief  Main program.
  * @retval None
  * @details Main program as following
  *  - Configure USART used to output gPotentiometerLevel.
  *  - Enable the NVIC ADC Interrupt
  *  - Enable peripherals clock of ADC, GPTM0, AFIO.
  *  - Configure GPTM0 Channel 3 as PWM output mode used to trigger ADC start of conversion
  *    every 1 second
  *  - Set ADC Divider as 64, therefore ADCLK frequency as 72MHz /64 = 1.125MHz
  *  - Configure specify GPIO's AFIO mode as ADC function.
  *  - ADC configure as One Shot Mode, Length 3, SubLength 1
  *  - Configure ADC Rank 0 to convert ADC channel n with 11.5 sampling clock.
  *  - Configure ADC Rank 1 to convert ADC channel n with 6.5 sampling clock.
  *  - Configure ADC Rank 2 to convert ADC channel n with 7.5 sampling clock.
  *  - Configure GPTM0 CH3O as ADC trigger source
  *  - Enable ADC cycle end of conversion interrupt,
  *    The ADC ISR will store the ADC result into global variable gADC_Result and
  *    set gADC_CycleEndOfConversion as TRUE.
  *  - Enable GPTM which will trigger ADC start of conversion every 1 second
  *  - Print gADC_Result in a infinite loop if needed.
  *
  ***********************************************************************************************************/
int main(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  GPTM_TimeBaseInitTypeDef TimeBaseInit;
  GPTM_OutputInitTypeDef OutInit;

  RETARGET_Configuration();           /* Retarget Related configuration                                     */

  /* Enable the ADC Interrupts                                                                              */
  NVIC_EnableIRQ(ADC_IRQn);

  /* Enable peripheral clocks of ADC, GPTM0                                                                 */
  CKCUClock.Bit.AFIO       = 1;
  CKCUClock.Bit.GPTM0      = 1;
  CKCUClock.Bit.ADC        = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  /* Configure GPTM0 Channel 3 as PWM output mode used to trigger ADC start of conversion
     every 1 second */
  TimeBaseInit.CounterMode = GPTM_CNT_MODE_UP;
  TimeBaseInit.CounterReload = 39999;
  TimeBaseInit.Prescaler = 1799;
  TimeBaseInit.PSCReloadTime = GPTM_PSC_RLD_IMMEDIATE;
  GPTM_TimeBaseInit(HT_GPTM0, &TimeBaseInit);

  OutInit.Channel = GPTM_CH_3;
  OutInit.OutputMode = GPTM_OM_PWM2;
  OutInit.Control = GPTM_CHCTL_ENABLE;
  OutInit.Polarity = GPTM_CHP_NONINVERTED;
  OutInit.Compare = 39999;
  GPTM_OutputInit(HT_GPTM0, &OutInit);

  /* ADCLK frequency is set to 72/64 MHz = 1.125MHz                                                         */
  CKCU_SetADCPrescaler(CKCU_ADCPRE_DIV64);

  /* Configure specify GPIO's AFIO mode as ADC function                                                     */
  AFIO_GPAConfig(AFIO_PIN_0 | AFIO_PIN_1 , AFIO_MODE_2);
  AFIO_GPCConfig(AFIO_PIN_11 , AFIO_MODE_2);

  /* One Shot Mode, Length 3, SubLength 1                                                                   */
  ADC_RegularGroupConfig(HT_ADC, ONE_SHOT_MODE, 3, 1);

  /* ADC Channel n, Rank 0, Sampling clock is (1.5 + 10) ADCLK
        Conversion time = (sampling clock + 12.5) / ADCLK = 21.3 uS */
  ADC_RegularChannelConfig(HT_ADC, ADC_CH_15, 0, 10);

  /*   ADC Channel 0, Rank 1, Sampling clock is (1.5 + 5) ADCLK
        Conversion time = (sampling clock + 12.5) / ADCLK = 16.9 uS */
  ADC_RegularChannelConfig(HT_ADC, ADC_CH_0, 1, 5);

  /* ADC Channel 1, Rank 2, Sampling clock is (1.5 + 6) ADCLK
        Conversion time = (sampling clock + 12.5) / ADCLK = 17.8 uS */
  ADC_RegularChannelConfig(HT_ADC, ADC_CH_1, 2, 6);

  /* Use GPTM0 CH3O as ADC trigger source                                                                   */
  ADC_RegularTrigConfig(HT_ADC, ADC_TRIG_GPTM0_CH3O);

  /* Enable ADC cycle end of conversion interrupt,
     The ADC ISR will store the ADC result into global variable gADC_Result and
     set gADC_CycleEndOfConversion as TRUE */
  ADC_IntConfig(HT_ADC, ADC_INT_CYCLE_EOC, ENABLE);

  /* Enable GPTM which will trigger ADC start of conversion every 1 second                                  */
  GPTM_Cmd(HT_GPTM0, ENABLE);

  while (1)
  {
    if (gADC_CycleEndOfConversion)
    {
      /* Output gADC_Result if needed.                                                                      */
      printf("ADC Result Rank0:%4u Rank1:%4u Rank2:%4u\r\n",
        (int)gADC_Result[0], (int)gADC_Result[1], (int)gADC_Result[2]);
      gADC_CycleEndOfConversion = FALSE;
    }
  }
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
