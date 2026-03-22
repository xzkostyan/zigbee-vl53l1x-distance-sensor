/**************************************************************************************************
  Filename:       hal_i2c.h
  Revised:        $Date: 2009-01-20 06:41:30 -0800 (Tue, 20 Jan 2009) $
  Revision:       $Revision: 18809 $

  Description:    Interface for I2C driver.


  Copyright 2006 - 2008 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED ?AS IS? WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

#ifndef HAL_I2C_H
#define HAL_I2C_H

#include "hal_types.h"
#define STATIC static

/* I2C clock rate */
#define I2C_CLOCK_123KHZ    0x00
#define I2C_CLOCK_144KHZ    0x01
#define I2C_CLOCK_165KHZ    0x02
#define I2C_CLOCK_197KHZ    0x03
#define I2C_CLOCK_33KHZ     0x80
#define I2C_CLOCK_267KHZ    0x81
#define I2C_CLOCK_533KHZ    0x82

/*********************************************************************
 * @fn      HalI2CInit
 * @brief   Initializes two-wire serial I/O bus
 * @param   void
 * @return  void
 */
void  HalI2CInit( uint8 clockRate );

/*********************************************************************
 * @fn      HALI2CReceive
 * @brief   Receives data into a buffer from an I2C slave device
 * @param   address: address of the slave device
 * @param   buf: target array for read characters
 * @param   len: max number of characters to read
 * @return  zero when successful.
 */
int8   HalI2CReceive(uint8 address, uint8 *buf, uint16 len);
uint8  I2C_ReadMultByte( uint8 address, uint16 reg, uint8 *buffer, uint16 len );

/*********************************************************************
 * @fn      HALI2CSend
 * @brief   Sends buffer contents to an I2C slave device
 * @param   address: address of the slave device
 * @param   buf - ptr to buffered data to send
 * @param   len - number of bytes in buffer
 * @return  zero when successful.
 */
int8   HalI2CSend(uint8 address, uint8 *buf, uint16 len);
STATIC void   hali2cSend( uint8 *buffer, uint16 len, uint8 sendStart, uint8 sendStop );
STATIC _Bool  hali2cSendByte( uint8 dByte );
STATIC void   hali2cWrite( bool dBit );
STATIC void   hali2cClock( bool dir );
uint8   hali2cStart( void );
void   hali2cStop( void );
STATIC void   hali2cReceive( uint8 address, uint8 *buffer, uint16 len );
uint8  I2C_WriteMultByte( uint8 address, uint16 reg, uint8 *buffer, uint16 len );
STATIC uint8  hali2cReceiveByte( void );
STATIC _Bool  hali2cRead( void );
STATIC void   hali2cSendDeviceAddress(uint8 address);
void WaitUs(uint16 microSecs);
void WaitMs(uint16 mSecs);

uint8 I2C_WriteReg8(uint8 deviceAddr, uint16 reg, uint8 value);
uint8 I2C_WriteReg16(uint8 deviceAddr, uint16 reg, uint16 value);
uint8 I2C_WriteReg32(uint8 deviceAddr, uint16 reg, uint32 value);

uint8 I2C_ReadReg8(uint8 deviceAddr, uint16 reg);
uint16 I2C_ReadReg16(uint8 deviceAddr, uint16 reg);
uint32 I2C_ReadReg32(uint8 deviceAddr, uint16 reg);


STATIC __near_func void   hali2cWait( uint8 );

STATIC uint8 s_xmemIsInit;

#endif
