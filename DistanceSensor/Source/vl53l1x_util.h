#ifndef VL53L1X_UTIL_H
#define VL53L1X_UTIL_H

#include "hal_types.h"


struct RangingData
{
  uint16 range_mm;
  uint8 range_status;
  float peak_signal_count_rate_MCPS;
  float ambient_count_rate_MCPS;
};

// for storing values read from RESULT__RANGE_STATUS (0x0089)
// through RESULT__PEAK_SIGNAL_COUNT_RATE_CROSSTALK_CORRECTED_MCPS_SD0_LOW
// (0x0099)
struct ResultBuffer
{
  uint8 range_status;
// uint8 report_status: not used
  uint8 stream_count;
  uint16 dss_actual_effective_spads_sd0;
// uint16 peak_signal_count_rate_mcps_sd0: not used
  uint16 ambient_count_rate_mcps_sd0;
// uint16 sigma_sd0: not used
// uint16 phase_sd0: not used
  uint16 final_crosstalk_corrected_range_mm_sd0;
  uint16 peak_signal_count_rate_crosstalk_corrected_mcps_sd0;
};


enum RangeStatus
{
  RangeValid                =   0,

  // "sigma estimator check is above the internal defined threshold"
  // (sigma = standard deviation of measurement)
  SigmaFail                 =   1,

  // "signal value is below the internal defined threshold"
  SignalFail                =   2,

  // "Target is below minimum detection threshold."
  RangeValidMinRangeClipped =   3,

  // "phase is out of bounds"
  // (nothing detected in range; try a longer distance mode if applicable)
  OutOfBoundsFail           =   4,

  // "HW or VCSEL failure"
  HardwareFail              =   5,

  // "The Range is valid but the wraparound check has not been done."
  RangeValidNoWrapCheckFail =   6,

  // "Wrapped target, not matching phases"
  // "no matching phase in other VCSEL period timing."
  WrapTargetFail            =   7,

  // "Internal algo underflow or overflow in lite ranging."
// ProcessingFail            =   8: not used in API

  // "Specific to lite ranging."
  // should never occur with this lib (which uses low power auto ranging,
  // as the API does)
  XtalkSignalFail           =   9,

  // "1st interrupt when starting ranging in back to back mode. Ignore
  // data."
  // should never occur with this lib
  SynchronizationInt         =  10, // (the API spells this "syncronisation")

  // "All Range ok but object is result of multiple pulses merging together.
  // Used by RQL for merged pulse detection"
// RangeValid MergedPulse    =  11: not used in API

  // "Used by RQL as different to phase fail."
// TargetPresentLackOfSignal =  12:

  // "Target is below minimum detection threshold."
  MinRangeFail              =  13,

  // "The reported range is invalid"
// RangeInvalid              =  14: can't actually be returned by API (range can never become negative, even after correction)

  // "No Update."
  None                      = 255,
};

uint32 VL53_CalcMacroPeriod(uint8 vcsel_period);

uint32 VL53_DecodeTimeout(uint16 reg_val);
uint16 VL53_EncodeTimeout(uint32 timeout_mclks);

uint32 VL53_TimeoutMclksToMicroseconds(uint32 timeout_mclks, uint32 macro_period_us);
uint32 VL53_TimeoutMicrosecondsToMclks(uint32 timeout_us, uint32 macro_period_us);

float VL53_CountRateFixedToFloat(uint16 count_rate_fixed);

void VL53_SetupManualCalibration(void);

uint16 VL53_Read(uint8 blocking);
void VL53_ReadResults(void);
void VL53_UpdateDSS(void);
void VL53_GetRangingData(void);

void VL53_ResetCalbrated(void);
void VL53_RestoreVHV(void);

#endif
