/**************************************************************************************************
  Filename:       zcl_sampletemperaturesensor_data.c
  Revised:        $Date: 2014-09-25 13:20:41 -0700 (Thu, 25 Sep 2014) $
  Revision:       $Revision: 40295 $


  Description:    Zigbee Cluster Library - sample device application.


  Copyright 2013-2014 Texas Instruments Incorporated. All rights reserved.

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
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ms.h"

#include "zcl_distancesensor.h"

/*********************************************************************
 * CONSTANTS
 */

#define DISTANCE_SENSOR_DEVICE_VERSION     1
#define DISTANCE_SENSOR_FLAGS              0

#define DISTANCE_SENSOR_HWVERSION          1
#define DISTANCE_SENSOR_ZCLVERSION         1
#define DISTANCE_SENSOR_APPVERSION         1
#define DISTANCE_SENSOR_STACKVERSION       3

#define DISTANCE_MAX_MEASURED_VALUE  4000  // 4m
#define DISTANCE_MIN_MEASURED_VALUE  0     // 0m

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Global attributes
const uint16 zclDistanceSensor_clusterRevision_all = 0x0001; 

// Basic Cluster
const uint8 zclDistanceSensor_HWRevision = DISTANCE_SENSOR_HWVERSION;
const uint8 zclDistanceSensor_ZCLVersion = DISTANCE_SENSOR_ZCLVERSION;
const uint8 zclDistanceSensor_AppVersion = DISTANCE_SENSOR_APPVERSION;
const uint8 zclDistanceSensor_StackVersion = DISTANCE_SENSOR_STACKVERSION;
const uint8 zclDistanceSensor_ManufacturerName[] = { 9, 'x','z','k','o','s','t','y','a','n' };
const uint8 zclDistanceSensor_ModelId[] = { 15, 'd','i','s','t','a','n','c','e','-','s','e','n','s','o','r' };
const uint8 zclDistanceSensor_DateCode[] = { 8, '2','0','2','6','0','3','0','8' };
const uint8 zclDistanceSensor_PowerSource = POWER_SOURCE_DC;

uint8 zclDistanceSensor_LocationDescription[17];
uint8 zclDistanceSensor_PhysicalEnvironment;
uint8 zclDistanceSensor_DeviceEnable;

// Identify Cluster
uint16 zclDistanceSensor_IdentifyTime;

float zclDistanceSensor_MeasuredValue = 0;

const float zclDistanceSensor_MinMeasuredValue = DISTANCE_MIN_MEASURED_VALUE; 
const float zclDistanceSensor_MaxMeasuredValue = DISTANCE_MAX_MEASURED_VALUE;

application_config_t zclDistanceSensor_Config = {
    .ReadingInterval = DEFAULT_READING_INTERVAL
};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */

// NOTE: The attributes listed in the AttrRec must be in ascending order 
// per cluster to allow right function of the Foundation discovery commands

CONST zclAttrRec_t zclDistanceSensor_Attrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclDistanceSensor_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_APPL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclDistanceSensor_AppVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_STACK_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclDistanceSensor_StackVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclDistanceSensor_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclDistanceSensor_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclDistanceSensor_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclDistanceSensor_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&zclDistanceSensor_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclDistanceSensor_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclDistanceSensor_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclDistanceSensor_DeviceEnable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclDistanceSensor_clusterRevision_all
    }
  },
  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclDistanceSensor_IdentifyTime
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ | ACCESS_GLOBAL,
      (void *)&zclDistanceSensor_clusterRevision_all
    }
  },

//// Distance attributes
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { ATTRID_IOV_BASIC_PRESENT_VALUE,
      ZCL_DATATYPE_SINGLE_PREC,
      ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
      (void *)&zclDistanceSensor_MeasuredValue
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { // Attribute record
      ATTRID_IOV_BASIC_MIN_PRESENT_VALUE,
      ZCL_DATATYPE_SINGLE_PREC,
      ACCESS_CONTROL_READ,
      (void *)&zclDistanceSensor_MinMeasuredValue
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { // Attribute record
      ATTRID_IOV_BASIC_MAX_PRESENT_VALUE,
      ZCL_DATATYPE_SINGLE_PREC,
      ACCESS_CONTROL_READ,
      (void *)&zclDistanceSensor_MaxMeasuredValue
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { // Attribute record
      ATTRID_READING_INTERVAL,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclDistanceSensor_Config.ReadingInterval
    }
  },
////  
};

uint8 CONST zclDistanceSensor_NumAttributes = ( sizeof(zclDistanceSensor_Attrs) / sizeof(zclDistanceSensor_Attrs[0]) );

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
#define ZCLDISTANCE_SENSOR_MAX_INCLUSTERS       3
const cId_t zclDistanceSensor_InClusterList[ZCLDISTANCE_SENSOR_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC
};

SimpleDescriptionFormat_t zclDistanceSensor_SimpleDesc =
{
  DISTANCE_SENSOR_ENDPOINT,                          //  int Endpoint;
  ZCL_HA_PROFILE_ID,                                 //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_TEMPERATURE_SENSOR,                //  uint16 AppDeviceId[2];
  DISTANCE_SENSOR_DEVICE_VERSION,                    //  int   AppDevVer:4;
  DISTANCE_SENSOR_FLAGS,                             //  int   AppFlags:4;
  ZCLDISTANCE_SENSOR_MAX_INCLUSTERS,                 //  byte  AppNumInClusters;
  (cId_t *)zclDistanceSensor_InClusterList,          //  byte *pAppInClusterList;
  0,                                                 //  byte  AppNumInClusters;
  NULL                                               //  byte *pAppInClusterList;
};
 
/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zclDistanceSensor_ResetAttributesToDefaultValues
 *
 * @brief   Reset all writable attributes to their default values.
 *
 * @param   none
 *
 * @return  none
 */
void zclDistanceSensor_ResetAttributesToDefaultValues(void)
{
  int i;
  
  zclDistanceSensor_LocationDescription[0] = 16;
  for (i = 1; i <= 16; i++)
  {
    zclDistanceSensor_LocationDescription[i] = ' ';
  }
  
  zclDistanceSensor_PhysicalEnvironment = PHY_UNSPECIFIED_ENV;
  zclDistanceSensor_DeviceEnable = DEVICE_ENABLED;
  
#ifdef ZCL_IDENTIFY
  zclDistanceSensor_IdentifyTime = 0;
#endif
  
  zclDistanceSensor_MeasuredValue = 0;
  zclDistanceSensor_Config.ReadingInterval = DEFAULT_READING_INTERVAL;
}
