/**************************************************************************************************
  Filename:       zcl_distancesensor.c
  Revised:        $Date: 2014-10-24 16:04:46 -0700 (Fri, 24 Oct 2014) $
  Revision:       $Revision: 40796 $

  Description:    Zigbee Cluster Library - sample device application.


  Copyright 2013 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

/*********************************************************************
  This application implements a ZigBee Temperature Sensor, based on Z-Stack 3.0.

  This application is based on the common sample-application user interface. Please see the main
  comment in zcl_sampleapp_ui.c. The rest of this comment describes only the content specific for
  this sample applicetion.
  
  Application-specific UI peripherals being used:

  - LEDs:
    LED1 is not used in this application

  Application-specific menu system:

    <SET LOCAL TEMP> Set the temperature of the local temperature sensor
      Up/Down changes the temperature 
      This screen shows the following information:
        Line2:
          Shows the temperature of the local temperature sensor

*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "MT_SYS.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ms.h"

#include "zcl_distancesensor.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

#include "bdb_interface.h"
#include "bdb_Reporting.h"


////
#include "vl53l1x.h"
#include "hal_i2c.h"
#include "uart.h"
#include <stdio.h>
#include "osal_pwrmgr.h"
///
   
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

const uint32 distanceTimerMs = 60000;

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclDistanceSensor_TaskID;

extern int16 zdpExternalStateTaskID;
extern float zclDistanceSensor_MeasuredValue;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

#ifdef UART_DEBUG
char uartBuffer[256];
#endif

uint8 SeqNum = 0;
afAddrType_t report_DstAddr;


#ifdef BDB_REPORTING
float reportableDistanceChange = 0.0f;
#endif
  


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclDistanceSensor_BasicResetCB( void );

static void zclDistanceSensor_ProcessCommissioningStatus(bdbCommissioningModeMsg_t* bdbCommissioningModeMsg);

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclDistanceSensor_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclDistanceSensor_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclDistanceSensor_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclDistanceSensor_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclDistanceSensor_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclDistanceSensor_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclDistanceSensor_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_DISCOVER

/*********************************************************************
 * STATUS STRINGS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclDistanceSensor_CmdCallbacks =
{
  zclDistanceSensor_BasicResetCB,                 // Basic Cluster Reset command
  NULL,                                           // Identify Trigger Effect command
  NULL,             				                      // On/Off cluster command
  NULL,                                           // On/Off cluster enhanced command Off with Effect
  NULL,                                           // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                           // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
  NULL,                                           // Level Control Move to Level command
  NULL,                                           // Level Control Move command
  NULL,                                           // Level Control Step command
  NULL,                                           // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
  NULL,                                           // Group Response commands
#endif
#ifdef ZCL_SCENES
  NULL,                                           // Scene Store Request command
  NULL,                                           // Scene Recall Request command
  NULL,                                           // Scene Response command
#endif
#ifdef ZCL_ALARMS
  NULL,                                           // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
  NULL,                                           // Get Event Log command
  NULL,                                           // Publish Event Log command
#endif
  NULL,                                           // RSSI Location command
  NULL                                            // RSSI Location Response command
};

/*********************************************************************
 * @fn          zclDistanceSensor_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       none
 *
 * @return      none
 */
void zclDistanceSensor_Init( byte task_id )
{
  zclDistanceSensor_TaskID = task_id;

  // Register the Simple Descriptor for this application
  bdb_RegisterSimpleDescriptor( &zclDistanceSensor_SimpleDesc ); 
  
  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( DISTANCE_SENSOR_ENDPOINT, &zclDistanceSensor_CmdCallbacks );

  // Register the application's attribute list
  zclDistanceSensor_ResetAttributesToDefaultValues();
  zcl_registerAttrList( DISTANCE_SENSOR_ENDPOINT, zclDistanceSensor_NumAttributes, zclDistanceSensor_Attrs );   

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zclDistanceSensor_TaskID );

  bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING |
                         BDB_COMMISSIONING_MODE_FINDING_BINDING);

  bdb_RegisterCommissioningStatusCB( zclDistanceSensor_ProcessCommissioningStatus );

#ifdef BDB_REPORTING
  //Adds the default configuration values for the "distance" attribute of the ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC cluster, for endpoint DISTANCE_SENSOR_ENDPOINT
  //Default maxReportingInterval value is 3600 seconds
  //Default minReportingInterval value is 5 seconds
  //Default reportChange value is 300 (3 degrees)
  bdb_RepAddAttrCfgRecordDefaultToList(DISTANCE_SENSOR_ENDPOINT, ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC, ATTRID_IOV_BASIC_PRESENT_VALUE, 60, 3600, (uint8*)&reportableDistanceChange);
#endif
    
  zdpExternalStateTaskID = zclDistanceSensor_TaskID;

  
#ifdef UART_DEBUG
  initUART0();
  writeUART0("\n\n\n===================UART0 init===================\r\n");
#endif

  HalI2CInit(I2C_CLOCK_267KHZ);
  DistanceSensorInit();

  // Measure distance right after init.
  osal_start_timerEx(zclDistanceSensor_TaskID,
                     VL53_MEASURE_EVT,
                     100);
}

void DistanceSensorInit(void)
{
  if (VL53_Init())
  {
    VL53_SetDistanceMode(VL53_DISTANCE_MODE_LONG);
    VL53_SetMeasurementTimingBudget(50000);
  }
}


/*********************************************************************
 * @fn          zclDistanceSensor_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
uint16 zclDistanceSensor_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclDistanceSensor_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          zclDistanceSensor_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          break;

        case ZDO_STATE_CHANGE:          
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
#if ZG_BUILD_ENDDEVICE_TYPE    
  if ( events & END_DEVICE_REJOIN_EVT )
  {
    bdb_ZedAttemptRecoverNwk();
    return ( events ^ END_DEVICE_REJOIN_EVT );
  }
#endif
  
  if(events & VL53_MEASURE_EVT)
  {
    int16 readDistance = 0;

#ifdef UART_DEBUG
    HalUARTResume();
#endif

    osal_pwrmgr_task_state(zclDistanceSensor_TaskID,
                           PWRMGR_HOLD);
    
    readDistance = VL53_ReadSingle();

#ifdef UART_DEBUG
    sprintf(uartBuffer, "Measured distance %d\r\n", readDistance);
    writeUART0(uartBuffer);
#endif

    // reset sensor on error
    if (readDistance == -1)
    {
      DistanceSensorInit();
    }
    else
    {
      zclDistanceSensor_MeasuredValue = readDistance;
      ReportDistance();
    }

    osal_start_timerEx(zclDistanceSensor_TaskID,
                       VL53_MEASURE_EVT,
                       distanceTimerMs);    

    osal_pwrmgr_task_state(zclDistanceSensor_TaskID,
                           PWRMGR_CONSERVE);

#ifdef UART_DEBUG
    HalUARTSuspend();
#endif

    return events ^ VL53_MEASURE_EVT;
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zclDistanceSensor_LcdDisplayMainMode
 *
 * @brief   Called to display the main screen on the LCD.
 *
 * @param   none
 *
 * @return  none
 */
static void zclDistanceSensor_ProcessCommissioningStatus(bdbCommissioningModeMsg_t* bdbCommissioningModeMsg)
{
    switch(bdbCommissioningModeMsg->bdbCommissioningMode)
    {
      case BDB_COMMISSIONING_FORMATION:
        if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
        {
          //After formation, perform nwk steering again plus the remaining commissioning modes that has not been process yet
          bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
        }
        else
        {
          //Want to try other channels?
          //try with bdb_setChannelAttribute
        }
      break;
      case BDB_COMMISSIONING_NWK_STEERING:
        if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
        {
          //YOUR JOB:
          //We are on the nwk, what now?
          osal_stop_timerEx(zclDistanceSensor_TaskID, VL53_MEASURE_EVT);    

          osal_start_timerEx(zclDistanceSensor_TaskID,
                             VL53_MEASURE_EVT,
                             5000);    
        }
        else
        {
          //See the possible errors for nwk steering procedure
          //No suitable networks found
          //Want to try other channels?
          //try with bdb_setChannelAttribute
        }
      break;
      case BDB_COMMISSIONING_FINDING_BINDING:
        if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
        {
          //YOUR JOB:
        }
        else
        {
          //YOUR JOB:
          //retry?, wait for user interaction?
        }
      break;
      case BDB_COMMISSIONING_INITIALIZATION:
        //Initialization notification can only be successful. Failure on initialization 
        //only happens for ZED and is notified as BDB_COMMISSIONING_PARENT_LOST notification
        
        //YOUR JOB:
        //We are on a network, what now?
        
      break;
#if ZG_BUILD_ENDDEVICE_TYPE    
    case BDB_COMMISSIONING_PARENT_LOST:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED)
      {
        //We did recover from losing parent
      }
      else
      {
        //Parent not found, attempt to rejoin again after a fixed delay
        osal_start_timerEx(zclDistanceSensor_TaskID, END_DEVICE_REJOIN_EVT, END_DEVICE_REJOIN_DELAY);
      }
    break;
#endif 
    }
}

/*********************************************************************
 * @fn      zclDistanceSensor_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zclDistanceSensor_BasicResetCB( void )
{
  zclDistanceSensor_ResetAttributesToDefaultValues();
}

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zclDistanceSensor_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zclDistanceSensor_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg)
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zclDistanceSensor_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zclDistanceSensor_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
    case ZCL_CMD_CONFIG_REPORT:
      //zclDistanceSensor_ProcessInConfigReportCmd( pInMsg );
      break;
      case ZCL_CMD_READ_REPORT_CFG:
      //zclDistanceSensor_ProcessInReadReportCfgCmd( pInMsg );
      break;
    case ZCL_CMD_CONFIG_REPORT_RSP:
      //zclDistanceSensor_ProcessInConfigReportRspCmd( pInMsg );
      break;
    case ZCL_CMD_READ_REPORT_CFG_RSP:
      //zclDistanceSensor_ProcessInReadReportCfgRspCmd( pInMsg );
      break;

    case ZCL_CMD_REPORT:
      //zclDistanceSensor_ProcessInReportCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      zclDistanceSensor_ProcessInDefaultRspCmd( pInMsg );
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
      zclDistanceSensor_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
      zclDistanceSensor_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_RSP:
      zclDistanceSensor_ProcessInDiscAttrsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
      zclDistanceSensor_ProcessInDiscAttrsExtRspCmd( pInMsg );
      break;
#endif
    default:
      break;
  }

  if ( pInMsg->attrCmd )
  {
    osal_mem_free( pInMsg->attrCmd );
  }
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zclDistanceSensor_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclDistanceSensor_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < readRspCmd->numAttr; i++ )
  {
    // Notify the originator of the results of the original read attributes
    // attempt and, for each successfull request, the value of the requested
    // attribute
  }

  return ( TRUE );
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zclDistanceSensor_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclDistanceSensor_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < writeRspCmd->numAttr; i++ )
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return ( TRUE );
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zclDistanceSensor_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclDistanceSensor_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.
  (void)pInMsg;

  return ( TRUE );
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zclDistanceSensor_ProcessInDiscCmdsRspCmd
 *
 * @brief   Process the Discover Commands Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclDistanceSensor_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numCmd; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}

/*********************************************************************
 * @fn      zclDistanceSensor_ProcessInDiscAttrsRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclDistanceSensor_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}

/*********************************************************************
 * @fn      zclDistanceSensor_ProcessInDiscAttrsExtRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Extended Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclDistanceSensor_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsExtRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}
#endif // ZCL_DISCOVER


/****************************************************************************
****************************************************************************/

void ReportDistance(void)
{
  const uint8 NUM_ATTRIBUTES = 1;

  zclReportCmd_t *pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) +
                              (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;

    pReportCmd->attrList[0].attrID = ATTRID_IOV_BASIC_PRESENT_VALUE;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[0].attrData = (void *)(&zclDistanceSensor_MeasuredValue);

    report_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    report_DstAddr.addr.shortAddr = 0;
    report_DstAddr.endPoint = 1;

    zcl_SendReportCmd(DISTANCE_SENSOR_ENDPOINT, &report_DstAddr,
                      ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC, pReportCmd,
                      ZCL_FRAME_SERVER_CLIENT_DIR, false, SeqNum++);
  }

  osal_mem_free(pReportCmd);
}
