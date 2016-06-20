/*********************************************************************************************************//**
 * @file    USBD/USB_UAC_Sound/ht32f165x_it.c
 * @version $Rev:: 307          $
 * @date    $Date:: 2014-12-31 #$
 * @brief   This file provides all interrupt service routine.
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
#include "ht32_usbd_core.h"
#include "ht32_usbd_class.h"

#include "i2cm.h"

/** @addtogroup HT32_Series_Peripheral_Examples HT32 Peripheral Examples
  * @{
  */

/** @addtogroup USBD_Examples USBD
  * @{
  */

/** @addtogroup USB_UAC_Sound
  * @{
  */


/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
 * @brief   This function handles I2C interrupt.
 * @retval  None
 ************************************************************************************************************/
void I2C0_IRQHandler(void)
{
  extern I2CM_TransferType I2CM_Transfer;
  extern u8 AltBuf;
  u32 state = I2CM->SR;
  I2CM_TransferType* pTransfer = &I2CM_Transfer;

  I2CM->SR = state;

  /* Check BUS error condition                                                                              */
  if (state & I2C_FLAG_BUSERR)
  {
    pTransfer->Status = I2CM_BUSERR;
    goto i2c_master_transfer_error;
  }

  /* Check Arbitration Loss condition                                                                       */
  if (state & I2C_FLAG_ARBLOS)
  {
    pTransfer->Status = I2CM_ARBLOS;
    goto i2c_master_transfer_error;
  }

  /* Check Timeout condition                                                                                */
  if (state & I2C_FLAG_TOUTF)
  {
    pTransfer->Status = I2CM_TOUTF;
    goto i2c_master_transfer_error;
  }

  /* Check NACK condition                                                                                   */
  if (state & I2C_FLAG_RXNACK)
  {
    if (pTransfer->RetryDownCounter)
    {
      pTransfer->RetryDownCounter--;
      I2C_TargetAddressConfig(I2CM, pTransfer->DevAddr, I2C_MASTER_WRITE);
      return;
    }
    else
    {
      pTransfer->Status = I2CM_SLAVE_NO_RESPONSE;
      goto i2c_master_transfer_error;
    }
  }

  if (pTransfer->Buffer)
  {
    switch (state)
    {
      /*-----------------------------Master Transmitter ----------------------------------------------------*/
      case I2C_MASTER_SEND_START:
        break;

      case I2C_MASTER_TRANSMITTER_MODE:
          /* Send the register address to I2C                                                               */
          if (pTransfer->RegAddrMode == I2CM_WORD_ADDR_MODE_2BYTE)
          {
            I2CM->DR = pTransfer->RegAddr >> 8;
            pTransfer->IsRegAddrSent = FALSE;
          }
          else
          {
            I2CM->DR = pTransfer->RegAddr;
            pTransfer->IsRegAddrSent = TRUE;
          }
        break;

      case I2C_MASTER_TX_EMPTY:
          if (pTransfer->IsRegAddrSent)
          {
            if (pTransfer->Direction == I2CM_DIRECTION_OUT)
            {
              if (pTransfer->Counter < pTransfer->Length)
              {
                I2CM->DR = pTransfer->Buffer[pTransfer->Counter++];
              }
              else
              {
                pTransfer->Status = I2CM_OK;
                /* Send I2C STOP condition                                                                  */
                I2C_GenerateSTOP(I2CM);
                pTransfer->Buffer = NULL;
                if (pTransfer->TransferEndCallback != NULL)
                {
                  pTransfer->TransferEndCallback();
                }
              }
            }
            else
            {
              /* Send Repeated Start to change the I2C transfer direction                                   */
              I2C_TargetAddressConfig(I2CM, pTransfer->DevAddr, I2C_MASTER_READ);
            }
          }
          else
          {
            I2CM->DR = pTransfer->RegAddr & 0xFF;
            pTransfer->IsRegAddrSent = TRUE;
          }
        break;

      /*-----------------------------Master Receiver -------------------------------------------------------*/
      case I2C_MASTER_RECEIVER_MODE:
          /* Enable I2CM ACK for receiving data                                                             */
          if (pTransfer->Length > 1)
          {
            I2C_AckCmd(I2CM, ENABLE);
          }
        break;

      case I2C_MASTER_RX_NOT_EMPTY:
          /* Receive data sent from I2C                                                                     */
          pTransfer->Buffer[pTransfer->Counter++] = I2CM->DR;
          if (pTransfer->Counter == pTransfer->Length - 1)
          {
            /* Disable I2CM ACK                                                                             */
            I2C_AckCmd(I2CM, DISABLE);
          }
          if (pTransfer->Counter == pTransfer->Length)
          {
            pTransfer->Status = I2CM_OK;
            /* Send I2C STOP condition                                                                      */
            I2C_GenerateSTOP(I2CM);
            pTransfer->Buffer = NULL;
            if (pTransfer->TransferEndCallback != NULL)
            {
              pTransfer->TransferEndCallback();
            }
          }
        break;

       default:
        I2CM_DBG_PRINTF("Undefine State %08X\r\n", state);
        break;
    }
    return;
  }
  else
  {
    pTransfer->Status = I2CM_BUFFER_NULL;
  }

i2c_master_transfer_error:
  /* Send I2C STOP condition                                                                                */
  I2C_GenerateSTOP(I2CM);
  pTransfer->Buffer = NULL;
}

/*********************************************************************************************************//**
 * @brief   This function handles I2S interrupt.
 * @retval  None
 ************************************************************************************************************/
void I2S_IRQHandler(void)
{
  if (I2S_GetFlagStatus(I2S_FLAG_TXFIFO_UDF) == SET)
  {
    printf(" TU");
    while (1);
  }

  if (I2S_GetFlagStatus(I2S_FLAG_RXFIFO_OVF) == SET)
  {
    printf(" RO");
    while (1);
  }
}

/*********************************************************************************************************//**
 * @brief   This function handles PDMA_CH3 interrupt.
 * @retval  None
 ************************************************************************************************************/
void PDMA_CH3_IRQHandler(void)
{
  extern bool IsRxTrigLevelReach;
  extern u8 AltBuf;
  FlagStatus isTransferComplete = PDMA_GetFlagStatus(PDMA_CH3, PDMA_FLAG_TC);

  PDMA_ClearFlag(PDMA_CH3, PDMA_FLAG_GE);

  IsRxTrigLevelReach = TRUE;
  if (isTransferComplete == RESET)
  {
    AltBuf = 0;
  }
  else
  {
    AltBuf = 1;
  }
}

/*********************************************************************************************************//**
 * @brief   This function handles USB interrupt.
 * @retval  None
 ************************************************************************************************************/
void USB_IRQHandler(void)
{
  __ALIGN4 extern USBDCore_TypeDef gUSBCore;
  USBDCore_IRQHandler(&gUSBCore);
}


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
