/*********************************************************************************************************//**
 * @file    I2C/EEPROM_Simulate/ht32f165x_it.c
 * @version $Rev:: 929          $
 * @date    $Date:: 2015-09-16 #$
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
#include "i2cm.h"

/** @addtogroup HT32_Series_Peripheral_Examples HT32 Peripheral Examples
  * @{
  */

/** @addtogroup I2C_Examples I2C
  * @{
  */

/** @addtogroup EEPROM_Simulate
  * @{
  */


/* Private constants ---------------------------------------------------------------------------------------*/
#define EEPROM_PAGE_WRITE_SIZE  16

/* Private variables ---------------------------------------------------------------------------------------*/
sI2CM_Transfer I2CM_Transfer;
u8 EEPROM_Memory[256];
u8 EEPROM_PageWriteBuffer[EEPROM_PAGE_WRITE_SIZE];

/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
 * @brief   This function handles NMI exception.
 * @retval  None
 ************************************************************************************************************/
void NMI_Handler(void)
{
}

/*********************************************************************************************************//**
 * @brief   This function handles Hard Fault exception.
 * @retval  None
 ************************************************************************************************************/
void HardFault_Handler(void)
{
  while (1);
}

/*********************************************************************************************************//**
 * @brief   This function handles Memory Manage exception.
 * @retval  None
 ************************************************************************************************************/
void MemManage_Handler(void)
{
  while (1);
}

/*********************************************************************************************************//**
 * @brief   This function handles Bus Fault exception.
 * @retval  None
 ************************************************************************************************************/
void BusFault_Handler(void)
{
  while (1);
}

/*********************************************************************************************************//**
 * @brief   This function handles Usage Fault exception.
 * @retval  None
 ************************************************************************************************************/
void UsageFault_Handler(void)
{
  while (1);
}

/*********************************************************************************************************//**
 * @brief   This function handles SVCall exception.
 * @retval  None
 ************************************************************************************************************/
void SVC_Handler(void)
{
}

/*********************************************************************************************************//**
 * @brief   This function handles Debug Monitor exception.
 * @retval  None
 ************************************************************************************************************/
void DebugMon_Handler(void)
{
}

/*********************************************************************************************************//**
 * @brief   This function handles PendSVC exception.
 * @retval  None
 ************************************************************************************************************/
void PendSV_Handler(void)
{
}

/*********************************************************************************************************//**
 * @brief   This function handles SysTick Handler.
 * @retval  None
 ************************************************************************************************************/
void SysTick_Handler(void)
{
}

/*********************************************************************************************************//**
 * @brief   This function handles I2C0 interrupt. (I2C Master to access EEPROM)
 * @retval  None
 ************************************************************************************************************/
void I2C0_IRQHandler(void)
{
  vu32 state;
  u32 i;
  state = HT_I2C0->SR;
  HT_I2C0->SR = state;

  /* Check BUS error condition                                                                              */
  if (state & I2C_FLAG_BUSERR)
  {
    I2CM_Transfer.Status = I2CM_BUSERR;
    goto i2c_master_transfer_error;
  }

  /* Check timeout condition                                                                                */
  if (state & I2C_FLAG_TOUTF)
  {
    I2CM_Transfer.Status = I2CM_TOUTF;
    goto i2c_master_transfer_error;
  }

  /* Check Arbitration Loss condition                                                                       */
  if (state & I2C_FLAG_ARBLOS)
  {
    I2CM_Transfer.Status = I2CM_ARBLOS;
    goto i2c_master_transfer_error;
  }

  /* Check NACK condition                                                                                   */
  if (state & I2C_FLAG_RXNACK)
  {
    if (I2CM_Transfer.RetryDownCounter)
    {
      I2CM_Transfer.RetryDownCounter--;
      I2C_TargetAddressConfig(HT_I2C0, I2CM_Transfer.DevAddr, I2C_MASTER_WRITE);
      return;
    }
    else
    {
      I2CM_Transfer.Status = I2CM_SLAVE_NO_RESPONSE;
      goto i2c_master_transfer_error;
    }
  }

  if (I2CM_Transfer.Buffer)
  {
    switch (state)
    {
      /*-----------------------------Master Transmitter ----------------------------------------------------*/
      case I2C_MASTER_SEND_START:
        break;

      case I2C_MASTER_TRANSMITTER_MODE:
        /* Send the register address to I2C                                                                 */
        HT_I2C0->DR = I2CM_Transfer.RegAddr;
        break;

      case I2C_MASTER_TX_EMPTY:
        if (I2CM_Transfer.Direction == I2CM_DIRECTION_OUT)
        {
          if (I2CM_Transfer.Counter < I2CM_Transfer.Length)
          {
            HT_I2C0->DR = I2CM_Transfer.Buffer[I2CM_Transfer.Counter++];
          }
          else
          {
            /* Send I2C STOP condition                                                                      */
            I2C_GenerateSTOP(HT_I2C0);
            I2CM_Transfer.Status = I2CM_OK;
            I2CM_Transfer.Locked = FALSE;
          }
        }
        else
        {
          /* Send Repeated Start to change the I2C transfer direction                                       */
          I2C_TargetAddressConfig(HT_I2C0, I2CM_Transfer.DevAddr, I2C_MASTER_READ);
        }
        break;

      /*-----------------------------Master Receiver -------------------------------------------------------*/
      case I2C_MASTER_RECEIVER_MODE:
        /* Enable I2CM ACK for receiving data                                                               */
        if (I2CM_Transfer.Length > 1)
        {
          I2C_AckCmd(HT_I2C0, ENABLE);
        }
        break;

      case I2C_MASTER_RX_NOT_EMPTY:
        /* Receive data sent from I2C                                                                       */
        i = I2CM_Transfer.Counter; 
        I2CM_Transfer.Buffer[i] = HT_I2C0->DR;
        I2CM_Transfer.Counter++;
        if (I2CM_Transfer.Counter == I2CM_Transfer.Length - 1)
        {
          /* Disable I2CM ACK                                                                               */
          I2C_AckCmd(HT_I2C0, DISABLE);
        }
        if (I2CM_Transfer.Counter == I2CM_Transfer.Length)
        {
          /* Generate STOP                                                                                  */
          I2C_GenerateSTOP(HT_I2C0);
          I2CM_Transfer.Locked = FALSE;
          I2CM_Transfer.Status = I2CM_OK;
        }
        break;

      default:
        //printf("Undefine State %08X\r\n", state);
        break;
    }
    return;
  }
  else
  {
    I2CM_Transfer.Status = I2CM_BUFFER_NULL;
  }

i2c_master_transfer_error:
  /* Send I2C STOP condition                                                                                */
  I2C_GenerateSTOP(HT_I2C0);
  I2CM_Transfer.Locked = FALSE;
  return;
}

/*********************************************************************************************************//**
 * @brief   This function handles I2C1 interrupt. (Simulate EEPROM slave)
 * @retval  None
 ************************************************************************************************************/
void I2C1_IRQHandler(void)
{
  static u8 MemAddr = 0;
  static s32 WriteIndex;
  u32 ProgramIndex, BaseAddr;

  switch (I2C_ReadRegister(HT_I2C1, I2C_REGISTER_SR))
  {
    /*-----------------------------Slave Receiver ----------------------------------------------------------*/
    case I2C_SLAVE_ACK_RECEIVER_ADDRESS:
      WriteIndex = -1;
      break;

    case I2C_SLAVE_RX_NOT_EMPTY:
      if (WriteIndex == -1)
      {
        MemAddr = I2C_ReceiveData(HT_I2C1);
        WriteIndex = MemAddr;
      }
      else
      {
        EEPROM_PageWriteBuffer[WriteIndex % EEPROM_PAGE_WRITE_SIZE] = I2C_ReceiveData(HT_I2C1);
        WriteIndex++;
      }
      break;

    case I2C_SLAVE_STOP_DETECTED:
      /* Start program EEPROM                                                                               */
      I2C_AckCmd(HT_I2C1, DISABLE);
      BaseAddr = MemAddr - (MemAddr % EEPROM_PAGE_WRITE_SIZE);

      for (ProgramIndex = MemAddr; ProgramIndex < WriteIndex; ProgramIndex++)
      {
        vu32 j = 3000;
        u8 PageBufferPointer;
        while (j--);

        PageBufferPointer = ProgramIndex % EEPROM_PAGE_WRITE_SIZE;
        EEPROM_Memory[BaseAddr + PageBufferPointer] = EEPROM_PageWriteBuffer[PageBufferPointer];
      }

      I2C_AckCmd(HT_I2C1, ENABLE);
      break;

    /*-----------------------------Slave Transmitter -------------------------------------------------------*/
    case I2C_SLAVE_ACK_TRANSMITTER_ADDRESS:
    case I2C_SLAVE_TX_EMPTY:
      /* Send data to master                                                                                */
      I2C_SendData(HT_I2C1, EEPROM_Memory[MemAddr++]);
      break;

    case I2C_SLAVE_RECEIVED_NACK_STOP:
      /* Clear RXNACK flag                                                                                  */
      I2C_ClearFlag(HT_I2C1, I2C_FLAG_RXNACK);
      break;

    default:
      break;
  }
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
