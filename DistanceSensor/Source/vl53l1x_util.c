#include "vl53l1x_util.h"
#include "vl53l1x_reg.h"
#include "vl53l1x.h"
#include "hal_i2c.h"

#include <stdio.h>
#include "uart.h"

uint16 fast_osc_frequency;
uint16 osc_calibrate_val;

uint8 calibrated = 0;
uint8 saved_vhv_init = 0;
uint8 saved_vhv_timeout = 0;

struct RangingData ranging_data;
struct ResultBuffer results;


#ifdef UART_DEBUG
static char uartBuffer[256];
#endif


// Calculate macro period in microseconds (12.12 format) with given VCSEL period
// assumes fast_osc_frequency has been read and stored
// based on VL53L1_calc_macro_period_us()
uint32 VL53_CalcMacroPeriod(uint8 vcsel_period)
{
  // from VL53L1_calc_pll_period_us()
  // fast osc frequency in 4.12 format; PLL period in 0.24 format
  uint32 pll_period_us = ((uint32)0x01 << 30) / fast_osc_frequency;

  // from VL53L1_decode_vcsel_period()
  uint8 vcsel_period_pclks = (vcsel_period + 1) << 1;

  // VL53L1_MACRO_PERIOD_VCSEL_PERIODS = 2304
  uint32 macro_period_us = (uint32)2304 * pll_period_us;
  macro_period_us >>= 6;
  macro_period_us *= vcsel_period_pclks;
  macro_period_us >>= 6;

  return macro_period_us;
}

// Decode sequence step timeout in MCLKs from register value
// based on VL53L1_decode_timeout()
uint32 VL53_DecodeTimeout(uint16 reg_val)
{
  return ((uint32)(reg_val & 0xFF) << (reg_val >> 8)) + 1;
}

// Encode sequence step timeout register value from timeout in MCLKs
// based on VL53L1_encode_timeout()
uint16 VL53_EncodeTimeout(uint32 timeout_mclks)
{
  // encoded format: "(LSByte * 2^MSByte) + 1"

  uint32 ls_byte = 0;
  uint16 ms_byte = 0;

  if (timeout_mclks > 0)
  {
    ls_byte = timeout_mclks - 1;

    while ((ls_byte & 0xFFFFFF00) > 0)
    {
      ls_byte >>= 1;
      ms_byte++;
    }

    return (ms_byte << 8) | (ls_byte & 0xFF);
  }
  else { return 0; }
}

// Convert sequence step timeout from macro periods to microseconds with given
// macro period in microseconds (12.12 format)
// based on VL53L1_calc_timeout_us()
uint32 VL53_TimeoutMclksToMicroseconds(uint32 timeout_mclks, uint32 macro_period_us)
{
  uint32 a = timeout_mclks;
  uint32 b = macro_period_us;
    
  uint32 a_lo = a & 0xFFFF;
  uint32 a_hi = a >> 16;
  uint32 b_lo = b & 0xFFFF;
  uint32 b_hi = b >> 16;

  uint32 p0 = a_lo * b_lo;
  uint32 p1 = a_lo * b_hi;
  uint32 p2 = a_hi * b_lo;
  uint32 p3 = a_hi * b_hi;

  uint32 lo = p0 + ((p1 & 0xFFFF) << 16) + ((p2 & 0xFFFF) << 16);
  uint32 carry = (lo < p0);

  uint32 hi = p3 + (p1 >> 16) + (p2 >> 16) + carry;

  lo += 0x800;
  if (lo < 0x800)
      hi++;

  return (lo >> 12) | (hi << (32 - 12));
  // We have no 64bits where. unsigned long long is 32bit. So, we can't do
  // return ((unsigned long long)timeout_mclks * macro_period_us + 0x800) >> 12;
}

// Convert sequence step timeout from microseconds to macro periods with given
// macro period in microseconds (12.12 format)
// based on VL53L1_calc_timeout_mclks()
uint32 VL53_TimeoutMicrosecondsToMclks(uint32 timeout_us, uint32 macro_period_us)
{
  return (((uint32)timeout_us << 12) + (macro_period_us >> 1)) / macro_period_us;
}

// Convert count rate from fixed point 9.7 format to float
float VL53_CountRateFixedToFloat(uint16 count_rate_fixed)
{
  return (float)count_rate_fixed / (1 << 7);
}

// "Setup ranges after the first one in low power auto mode by turning off
// FW calibration steps and programming static values"
// based on VL53L1_low_power_auto_setup_manual_calibration()
void VL53_SetupManualCalibration(void)
{
  // "save original vhv configs"
  saved_vhv_init = VL53_ReadReg8(VL53_REG_VHV_CONFIG__INIT);
  saved_vhv_timeout = VL53_ReadReg8(VL53_REG_VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND);

  // "disable VHV init"
  VL53_WriteReg8(VL53_REG_VHV_CONFIG__INIT, saved_vhv_init & 0x7F);

  // "set loop bound to tuning param"
  VL53_WriteReg8(VL53_REG_VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND,
    (saved_vhv_timeout & 0x03) + (3 << 2)); // tuning parm default (LOWPOWERAUTO_VHV_LOOP_BOUND_DEFAULT)

  // "override phasecal"
  VL53_WriteReg8(VL53_REG_PHASECAL_CONFIG__OVERRIDE, 0x01);
  VL53_WriteReg8(VL53_REG_CAL_CONFIG__VCSEL_START, VL53_ReadReg8(VL53_REG_PHASECAL_RESULT__VCSEL_START));
}


// Returns a range reading in millimeters when continuous mode is active. If
// blocking is true (the default), this function waits for a new measurement to
// be available. If blocking is false, it will try to return data immediately.
// (readSingle() also calls this function after starting a single-shot range
// measurement)
uint16 VL53_Read(uint8 blocking)
{
  const uint8 timeout = 10;
  uint8 i = 0;

  if (blocking)
  {
    while ((VL53_ReadReg8(VL53_REG_GPIO__TIO_HV_STATUS) & 0x01) == 1)
    {
      WaitMs(10);
      i++;
      if (i >= timeout) {
#ifdef UART_DEBUG
        writeUART0("Read timeout\r\n");
#endif
        break;
      }
    }
  }

  VL53_ReadResults();

  if (!calibrated)
  {
    VL53_SetupManualCalibration();
    calibrated = 1;
  }

  VL53_UpdateDSS();

  VL53_GetRangingData();

  VL53_WriteReg8(VL53_REG_SYSTEM__INTERRUPT_CLEAR, 0x01); // sys_interrupt_clear_range

  return ranging_data.range_mm;
}

// read measurement results into buffer
void VL53_ReadResults(void)
{
  uint8 data[17] = {0};
  
  uint8 readOK = I2C_ReadMultByte(VL53_DEVICE_ADDRESS, VL53_REG_RESULT__RANGE_STATUS, data, 17);

#ifdef UART_DEBUG
  sprintf(uartBuffer, "VL53_ReadResults [status=%s]: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d [range_status=%d]\r\n", readOK ? "OK" : "NOT OK", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16]);
  writeUART0(uartBuffer);
#endif

  if (!readOK) {
    return;
  }
  
  results.range_status = data[16];

  // data[15]; // report_status: not used

  results.stream_count = data[14];

  results.dss_actual_effective_spads_sd0  = (uint16)data[13] << 8; // high byte
  results.dss_actual_effective_spads_sd0 |=           data[12];      // low byte

  // data[11]; // peak_signal_count_rate_mcps_sd0: not used
  // data[10];

  results.ambient_count_rate_mcps_sd0  = (uint16)data[9] << 8; // high byte
  results.ambient_count_rate_mcps_sd0 |=           data[8];      // low byte

  // data[7]; // sigma_sd0: not used
  // data[6];

  // data[5]; // phase_sd0: not used
  // data[4]

  results.final_crosstalk_corrected_range_mm_sd0  = (uint16)data[3] << 8; // high byte
  results.final_crosstalk_corrected_range_mm_sd0 |=           data[2];      // low byte

  results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0  = (uint16)data[1] << 8; // high byte
  results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0 |=           data[0];      // low byte*/
}

// perform Dynamic SPAD Selection calculation/update
// based on VL53L1_low_power_auto_update_DSS()
void VL53_UpdateDSS(void)
{
  uint16 spadCount = results.dss_actual_effective_spads_sd0;

  if (spadCount != 0)
  {
    // "Calc total rate per spad"

    uint32 totalRatePerSpad =
      (uint32)results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0 +
      results.ambient_count_rate_mcps_sd0;

    // "clip to 16 bits"
    if (totalRatePerSpad > 0xFFFF) { totalRatePerSpad = 0xFFFF; }

    // "shift up to take advantage of 32 bits"
    totalRatePerSpad <<= 16;

    totalRatePerSpad /= spadCount;

    if (totalRatePerSpad != 0)
    {
      // "get the target rate and shift up by 16"
      uint32 requiredSpads = ((uint32)TARGET_RATE << 16) / totalRatePerSpad;

      // "clip to 16 bit"
      if (requiredSpads > 0xFFFF) { requiredSpads = 0xFFFF; }

      // "override DSS config"
      VL53_WriteReg16(VL53_REG_DSS_CONFIG__MANUAL_EFFECTIVE_SPADS_SELECT, requiredSpads);
      // DSS_CONFIG__ROI_MODE_CONTROL should already be set to REQUESTED_EFFFECTIVE_SPADS

      return;
    }
  }

  // If we reached this point, it means something above would have resulted in a
  // divide by zero.
  // "We want to gracefully set a spad target, not just exit with an error"

   // "set target to mid point"
   VL53_WriteReg16(VL53_REG_DSS_CONFIG__MANUAL_EFFECTIVE_SPADS_SELECT, 0x8000);
}

// get range, status, rates from results buffer
// based on VL53L1_GetRangingMeasurementData()
void VL53_GetRangingData(void)
{
  // VL53L1_copy_sys_and_core_results_to_range_results() begin

  uint16 range = results.final_crosstalk_corrected_range_mm_sd0;

  // "apply correction gain"
  // gain factor of 2011 is tuning parm default (VL53L1_TUNINGPARM_LITE_RANGING_GAIN_FACTOR_DEFAULT)
  // Basically, this appears to scale the result by 2011/2048, or about 98%
  // (with the 1024 added for proper rounding).
  ranging_data.range_mm = ((uint32)range * 2011 + 0x0400) / 0x0800;

  // VL53L1_copy_sys_and_core_results_to_range_results() end

  // set range_status in ranging_data based on value of RESULT__RANGE_STATUS register
  // mostly based on ConvertStatusLite()
  switch(results.range_status)
  {
    case 17: // MULTCLIPFAIL
    case 2: // VCSELWATCHDOGTESTFAILURE
    case 1: // VCSELCONTINUITYTESTFAILURE
    case 3: // NOVHVVALUEFOUND
      // from SetSimpleData()
      ranging_data.range_status = HardwareFail;
      break;

    case 13: // USERROICLIP
     // from SetSimpleData()
      ranging_data.range_status = MinRangeFail;
      break;

    case 18: // GPHSTREAMCOUNT0READY
      ranging_data.range_status = SynchronizationInt;
      break;

    case 5: // RANGEPHASECHECK
      ranging_data.range_status =  OutOfBoundsFail;
      break;

    case 4: // MSRCNOTARGET
      ranging_data.range_status = SignalFail;
      break;

    case 6: // SIGMATHRESHOLDCHECK
      ranging_data.range_status = SigmaFail;
      break;

    case 7: // PHASECONSISTENCY
      ranging_data.range_status = WrapTargetFail;
      break;

    case 12: // RANGEIGNORETHRESHOLD
      ranging_data.range_status = XtalkSignalFail;
      break;

    case 8: // MINCLIP
      ranging_data.range_status = RangeValidMinRangeClipped;
      break;

    case 9: // RANGECOMPLETE
      // from VL53L1_copy_sys_and_core_results_to_range_results()
      if (results.stream_count == 0)
      {
        ranging_data.range_status = RangeValidNoWrapCheckFail;
      }
      else
      {
        ranging_data.range_status = RangeValid;
      }
      break;

    default:
      ranging_data.range_status = None;
  }

  // from SetSimpleData()
  ranging_data.peak_signal_count_rate_MCPS =
    VL53_CountRateFixedToFloat(results.peak_signal_count_rate_crosstalk_corrected_mcps_sd0);
  ranging_data.ambient_count_rate_MCPS =
    VL53_CountRateFixedToFloat(results.ambient_count_rate_mcps_sd0);
}

void VL53_ResetCalbrated(void)
{
  calibrated = 0;
}

void VL53_RestoreVHV(void)
{
  // "restore vhv configs"
  if (saved_vhv_init != 0)
  {
    VL53_WriteReg8(VL53_REG_VHV_CONFIG__INIT, saved_vhv_init);
  }
  if (saved_vhv_timeout != 0)
  {
     VL53_WriteReg8(VL53_REG_VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, saved_vhv_timeout);
  }
}