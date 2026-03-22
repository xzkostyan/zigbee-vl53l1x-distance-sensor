#include "vl53l1x.h"
#include "vl53l1x_util.h"
#include "vl53l1x_reg.h"
#include "hal_i2c.h"

#ifdef UART_DEBUG
#include "uart.h"
#endif

#include <stdio.h>


// Taken from https://github.com/pololu/vl53l1x-arduino


extern uint16 fast_osc_frequency;
extern uint16 osc_calibrate_val;
extern struct RangingData ranging_data;


#ifdef UART_DEBUG
static char uartBuffer[256];
#endif


uint8 VL53_Init(void)
{
  // wait for sensor to init
  uint8 initOK = 0;
  for (int i=0; i < 5; i++) {
    // Check sensor model id.
    if(VL53_ReadReg16(VL53_REG_IDENTIFICATION__MODEL_ID) == 0xEACC) {
      initOK = 1;
      break;
    }
  }

#ifdef UART_DEBUG
  sprintf(uartBuffer, "Sensor Init: %s\r\n", initOK ? "OK" : "NOT OK");
  writeUART0(uartBuffer);
#endif

  
  if (!initOK) {
    return 0;
  }
  
   // VL53L1_software_reset() begin
  VL53_WriteReg8(VL53_REG_SOFT_RESET, 0);
  WaitUs(100);
  VL53_WriteReg8(VL53_REG_SOFT_RESET, 1);
  
  // give it some time to boot; otherwise the sensor NACKs during the readReg()
  // call below and the Arduino 101 doesn't seem to handle that well
  WaitUs(1000);

  // VL53L1_poll_for_boot_completion() begin

  while ((VL53_ReadReg8(VL53_REG_FIRMWARE__SYSTEM_STATUS) & 0x01) == 0)
  {
#ifdef UART_DEBUG
    writeUART0("Wait sensor to reset\r\n");
#endif
    WaitUs(10);
  }
  // VL53L1_poll_for_boot_completion() end

  // VL53L1_software_reset() end

  // VL53L1_DataInit() begin

  // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
  int io_2v8 = 0;
  if (io_2v8)
  {
    VL53_WriteReg8(VL53_REG_PAD_I2C_HV__EXTSUP_CONFIG,
      VL53_ReadReg8(VL53_REG_PAD_I2C_HV__EXTSUP_CONFIG) | 0x01);
  }

  // store oscillator info for later use
  fast_osc_frequency = VL53_ReadReg16(VL53_REG_OSC_MEASURED__FAST_OSC__FREQUENCY);
  osc_calibrate_val = VL53_ReadReg16(VL53_REG_RESULT__OSC_CALIBRATE_VAL);

  // VL53L1_DataInit() end

  // VL53L1_StaticInit() begin

  // Note that the API does not actually apply the configuration settings below
  // when VL53L1_StaticInit() is called: it keeps a copy of the sensor's
  // register contents in memory and doesn't actually write them until a
  // measurement is started. Writing the configuration here means we don't have
  // to keep it all in memory and avoids a lot of redundant writes later.

  // the API sets the preset mode to LOWPOWER_AUTONOMOUS here:
  // VL53L1_set_preset_mode() begin

  // VL53L1_preset_mode_standard_ranging() begin

  // values labeled "tuning parm default" are from vl53l1_tuning_parm_defaults.h
  // (API uses these in VL53L1_init_tuning_parm_storage_struct())

  // static config
  // API resets PAD_I2C_HV__EXTSUP_CONFIG here, but maybe we don't want to do
  // that? (seems like it would disable 2V8 mode)
  VL53_WriteReg16(VL53_REG_DSS_CONFIG__TARGET_TOTAL_RATE_MCPS, TARGET_RATE); // should already be this value after reset
  VL53_WriteReg8(VL53_REG_GPIO__TIO_HV_STATUS, 0x02);
  VL53_WriteReg8(VL53_REG_SIGMA_ESTIMATOR__EFFECTIVE_PULSE_WIDTH_NS, 8); // tuning parm default
  VL53_WriteReg8(VL53_REG_SIGMA_ESTIMATOR__EFFECTIVE_AMBIENT_WIDTH_NS, 16); // tuning parm default
  VL53_WriteReg8(VL53_REG_ALGO__CROSSTALK_COMPENSATION_VALID_HEIGHT_MM, 0x01);
  VL53_WriteReg8(VL53_REG_ALGO__RANGE_IGNORE_VALID_HEIGHT_MM, 0xFF);
  VL53_WriteReg8(VL53_REG_ALGO__RANGE_MIN_CLIP, 0); // tuning parm default
  VL53_WriteReg8(VL53_REG_ALGO__CONSISTENCY_CHECK__TOLERANCE, 2); // tuning parm default

  // general config
  VL53_WriteReg16(VL53_REG_SYSTEM__THRESH_RATE_HIGH, 0x0000);
  VL53_WriteReg16(VL53_REG_SYSTEM__THRESH_RATE_LOW, 0x0000);
  VL53_WriteReg8(VL53_REG_DSS_CONFIG__APERTURE_ATTENUATION, 0x38);

  // timing config
  // most of these settings will be determined later by distance and timing
  // budget configuration
  VL53_WriteReg16(VL53_REG_RANGE_CONFIG__SIGMA_THRESH, 360); // tuning parm default
  VL53_WriteReg16(VL53_REG_RANGE_CONFIG__MIN_COUNT_RATE_RTN_LIMIT_MCPS, 192); // tuning parm default

  // dynamic config

  VL53_WriteReg8(VL53_REG_SYSTEM__GROUPED_PARAMETER_HOLD_0, 0x01);
  VL53_WriteReg8(VL53_REG_SYSTEM__GROUPED_PARAMETER_HOLD_1, 0x01);
  VL53_WriteReg8(VL53_REG_SD_CONFIG__QUANTIFIER, 2); // tuning parm default

  // VL53L1_preset_mode_standard_ranging() end

  // from VL53L1_preset_mode_timed_ranging_*
  // GPH is 0 after reset, but writing GPH0 and GPH1 above seem to set GPH to 1,
  // and things don't seem to work if we don't set GPH back to 0 (which the API
  // does here).
  VL53_WriteReg8(VL53_REG_SYSTEM__GROUPED_PARAMETER_HOLD, 0x00);
  VL53_WriteReg8(VL53_REG_SYSTEM__SEED_CONFIG, 1); // tuning parm default

  // from VL53L1_config_low_power_auto_mode
  VL53_WriteReg8(VL53_REG_SYSTEM__SEQUENCE_CONFIG, 0x8B); // VHV, PHASECAL, DSS1, RANGE
  VL53_WriteReg16(VL53_REG_DSS_CONFIG__MANUAL_EFFECTIVE_SPADS_SELECT, (uint16)200 << 8);
  VL53_WriteReg8(VL53_REG_DSS_CONFIG__ROI_MODE_CONTROL, 2); // REQUESTED_EFFFECTIVE_SPADS

  // VL53L1_set_preset_mode() end

  // default to long range, 50 ms timing budget
  // note that this is different than what the API defaults to
  VL53_SetDistanceMode(VL53_DISTANCE_MODE_LONG);
  VL53_SetMeasurementTimingBudget(50000);

  // VL53L1_StaticInit() end

  // the API triggers this change in VL53L1_init_and_start_range() once a
  // measurement is started; assumes MM1 and MM2 are disabled

  VL53_WriteReg16(VL53_REG_ALGO__PART_TO_PART_RANGE_OFFSET_MM,
    VL53_ReadReg16(VL53_REG_MM_CONFIG__OUTER_OFFSET_MM) * 4);

  return 1;
}

// set distance mode to Short, Medium, or Long
// based on VL53L1_SetDistanceMode()
uint8 VL53_SetDistanceMode(uint8 mode)
{
   // save existing timing budget
  uint32 budget_us = VL53_GetMeasurementTimingBudget();
  
  switch (mode)
  {
    case VL53_DISTANCE_MODE_SHORT:
      // from VL53L1_preset_mode_standard_ranging_short_range()

      // timing config
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_A, 0x07);
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_B, 0x05);
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VALID_PHASE_HIGH, 0x38);

      // dynamic config
      VL53_WriteReg8(VL53_REG_SD_CONFIG__WOI_SD0, 0x07);
      VL53_WriteReg8(VL53_REG_SD_CONFIG__WOI_SD1, 0x05);
      VL53_WriteReg8(VL53_REG_SD_CONFIG__INITIAL_PHASE_SD0, 6); // tuning parm default
      VL53_WriteReg8(VL53_REG_SD_CONFIG__INITIAL_PHASE_SD1, 6); // tuning parm default

      break;

    case VL53_DISTANCE_MODE_MEDIUM:
      // from VL53L1_preset_mode_standard_ranging()

      // timing config
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_A, 0x0B);
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_B, 0x09);
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VALID_PHASE_HIGH, 0x78);

      // dynamic config
      VL53_WriteReg8(VL53_REG_SD_CONFIG__WOI_SD0, 0x0B);
      VL53_WriteReg8(VL53_REG_SD_CONFIG__WOI_SD1, 0x09);
      VL53_WriteReg8(VL53_REG_SD_CONFIG__INITIAL_PHASE_SD0, 10); // tuning parm default
      VL53_WriteReg8(VL53_REG_SD_CONFIG__INITIAL_PHASE_SD1, 10); // tuning parm default

      break;

    case VL53_DISTANCE_MODE_LONG: // long
      // from VL53L1_preset_mode_standard_ranging_long_range()

      // timing config
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_A, 0x0F);
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_B, 0x0D);
      VL53_WriteReg8(VL53_REG_RANGE_CONFIG__VALID_PHASE_HIGH, 0xB8);

      // dynamic config
      VL53_WriteReg8(VL53_REG_SD_CONFIG__WOI_SD0, 0x0F);
      VL53_WriteReg8(VL53_REG_SD_CONFIG__WOI_SD1, 0x0D);
      VL53_WriteReg8(VL53_REG_SD_CONFIG__INITIAL_PHASE_SD0, 14); // tuning parm default
      VL53_WriteReg8(VL53_REG_SD_CONFIG__INITIAL_PHASE_SD1, 14); // tuning parm default

      break;

    default:
      // unrecognized mode - do nothing
      return 0;
  }

  // reapply timing budget
  VL53_SetMeasurementTimingBudget(budget_us);

  return 1;
}

// Start continuous ranging measurements, with the given inter-measurement
// period in milliseconds determining how often the sensor takes a measurement.
void VL53_StartContinuous(uint32 period_ms)
{
  // from VL53L1_set_inter_measurement_period_ms()
  VL53_WriteReg32(VL53_REG_SYSTEM__INTERMEASUREMENT_PERIOD, period_ms * osc_calibrate_val);

  VL53_WriteReg8(VL53_REG_SYSTEM__INTERRUPT_CLEAR, 0x01); // sys_interrupt_clear_range
  VL53_WriteReg8(VL53_REG_SYSTEM__MODE_START, 0x40); // mode_range__timed
}

// Stop continuous measurements
// based on VL53L1_stop_range()
void VL53_StopContinuous(void)
{
  VL53_WriteReg8(VL53_REG_SYSTEM__MODE_START, 0x80); // mode_range__abort

  // VL53L1_low_power_auto_data_stop_range() begin

  VL53_ResetCalbrated();
  VL53_RestoreVHV();

  // "remove phasecal override"
  VL53_WriteReg8(VL53_REG_PHASECAL_CONFIG__OVERRIDE, 0x00);

  // VL53L1_low_power_auto_data_stop_range() end
}

// Starts a single-shot range measurement.
// This function waits for the measurement to finish and returns the reading.
int16 VL53_ReadSingle(void)
{
  uint16 distance;
  VL53_WriteReg8(VL53_REG_SYSTEM__INTERRUPT_CLEAR, 0x01); // sys_interrupt_clear_range
  VL53_WriteReg8(VL53_REG_SYSTEM__MODE_START, 0x10); // mode_range__single_shot

  distance = VL53_Read(1);
  if (ranging_data.range_status == HardwareFail || ranging_data.range_status == None) {
    return -1;
  }
  return distance;
}

// Set the width and height of the region of interest
// based on VL53L1X_SetROI() from STSW-IMG009 Ultra Lite Driver
//
// ST user manual UM2555 explains ROI selection in detail, so we recommend
// reading that document carefully.
void VL53_SetROISize(uint8 width, uint8 height)
{
  if ( width > 16) {  width = 16; }
  if (height > 16) { height = 16; }

  // Force ROI to be centered if width or height > 10, matching what the ULD API
  // does. (This can probably be overridden by calling setROICenter()
  // afterwards.)
  if (width > 10 || height > 10)
  {
    VL53_WriteReg8(VL53_REG_ROI_CONFIG__USER_ROI_CENTRE_SPAD, 199);
  }

  VL53_WriteReg8(VL53_REG_ROI_CONFIG__USER_ROI_REQUESTED_GLOBAL_XY_SIZE,
                 (height - 1) << 4 | (width - 1));
}

// Get the width and height of the region of interest (ROI)
// based on VL53L1X_GetROI_XY() from STSW-IMG009 Ultra Lite Driver
void VL53_GetROISize(uint8 * width, uint8 * height)
{
  uint8 reg_val = VL53_ReadReg8(VL53_REG_ROI_CONFIG__USER_ROI_REQUESTED_GLOBAL_XY_SIZE);
  *width = (reg_val & 0xF) + 1;
  *height = (reg_val >> 4) + 1;
}

// Set the center SPAD of the region of interest (ROI)
// based on VL53L1X_SetROICenter() from STSW-IMG009 Ultra Lite Driver
//
// ST user manual UM2555 explains ROI selection in detail, so we recommend
// reading that document carefully. Here is a table of SPAD locations from
// UM2555 (199 is the default/center):
//
// 128,136,144,152,160,168,176,184,  192,200,208,216,224,232,240,248
// 129,137,145,153,161,169,177,185,  193,201,209,217,225,233,241,249
// 130,138,146,154,162,170,178,186,  194,202,210,218,226,234,242,250
// 131,139,147,155,163,171,179,187,  195,203,211,219,227,235,243,251
// 132,140,148,156,164,172,180,188,  196,204,212,220,228,236,244,252
// 133,141,149,157,165,173,181,189,  197,205,213,221,229,237,245,253
// 134,142,150,158,166,174,182,190,  198,206,214,222,230,238,246,254
// 135,143,151,159,167,175,183,191,  199,207,215,223,231,239,247,255
//
// 127,119,111,103, 95, 87, 79, 71,   63, 55, 47, 39, 31, 23, 15,  7
// 126,118,110,102, 94, 86, 78, 70,   62, 54, 46, 38, 30, 22, 14,  6
// 125,117,109,101, 93, 85, 77, 69,   61, 53, 45, 37, 29, 21, 13,  5
// 124,116,108,100, 92, 84, 76, 68,   60, 52, 44, 36, 28, 20, 12,  4
// 123,115,107, 99, 91, 83, 75, 67,   59, 51, 43, 35, 27, 19, 11,  3
// 122,114,106, 98, 90, 82, 74, 66,   58, 50, 42, 34, 26, 18, 10,  2
// 121,113,105, 97, 89, 81, 73, 65,   57, 49, 41, 33, 25, 17,  9,  1
// 120,112,104, 96, 88, 80, 72, 64,   56, 48, 40, 32, 24, 16,  8,  0 <- Pin 1
//
// This table is oriented as if looking into the front of the sensor (or top of
// the chip). SPAD 0 is closest to pin 1 of the VL53L1X, which is the corner
// closest to the VDD pin on the Pololu VL53L1X carrier board:
//
//   +--------------+
//   |             O| GPIO1
//   |              |
//   |             O|
//   | 128    248   |
//   |+----------+ O|
//   ||+--+  +--+|  |
//   |||  |  |  || O|
//   ||+--+  +--+|  |
//   |+----------+ O|
//   | 120      0   |
//   |             O|
//   |              |
//   |             O| VDD
//   +--------------+
//
// However, note that the lens inside the VL53L1X inverts the image it sees
// (like the way a camera works). So for example, to shift the sensor's FOV to
// sense objects toward the upper left, you should pick a center SPAD in the
// lower right.
void VL53_SetROICenter(uint8 spadNumber)
{
  VL53_WriteReg8(VL53_REG_ROI_CONFIG__USER_ROI_CENTRE_SPAD, spadNumber);
}

// Get the center SPAD of the region of interest
// based on VL53L1X_GetROICenter() from STSW-IMG009 Ultra Lite Driver
uint8 VL53_GetROICenter(void)
{
  return VL53_ReadReg8(VL53_REG_ROI_CONFIG__USER_ROI_CENTRE_SPAD);
}

// Get the measurement timing budget in microseconds
// based on VL53L1_SetMeasurementTimingBudgetMicroSeconds()
uint32 VL53_GetMeasurementTimingBudget(void)
{
  // assumes PresetMode is LOWPOWER_AUTONOMOUS and these sequence steps are
  // enabled: VHV, PHASECAL, DSS1, RANGE

  // VL53L1_get_timeouts_us() begin

  // "Update Macro Period for Range A VCSEL Period"
  uint32 macro_period_us = VL53_CalcMacroPeriod(VL53_ReadReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_A));

  // "Get Range Timing A timeout"

  uint32 range_config_timeout_us = VL53_TimeoutMclksToMicroseconds(VL53_DecodeTimeout(
    VL53_ReadReg16(VL53_REG_RANGE_CONFIG__TIMEOUT_MACROP_A)), macro_period_us);

  // VL53L1_get_timeouts_us() end

  return  2 * range_config_timeout_us + TIMING_GUARD;
}

// Set the measurement timing budget in microseconds, which is the time allowed
// for one measurement. A longer timing budget allows for more accurate
// measurements.
// based on VL53L1_SetMeasurementTimingBudgetMicroSeconds()
uint8 VL53_SetMeasurementTimingBudget(uint32 budget_us)
{
  // assumes PresetMode is LOWPOWER_AUTONOMOUS

  if (budget_us <= TIMING_GUARD) { return 0; }

  uint32 range_config_timeout_us = budget_us -= TIMING_GUARD;
  if (range_config_timeout_us > 1100000) { return 0; } // FDA_MAX_TIMING_BUDGET_US * 2

  range_config_timeout_us /= 2;

  // VL53L1_calc_timeout_register_values() begin

  uint32 macro_period_us;

  // "Update Macro Period for Range A VCSEL Period"
  macro_period_us = VL53_CalcMacroPeriod(VL53_ReadReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_A));

  // "Update Phase timeout - uses Timing A"
  // Timeout of 1000 is tuning parm default (TIMED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT)
  // via VL53L1_get_preset_mode_timing_cfg().
  uint32 phasecal_timeout_mclks = VL53_TimeoutMicrosecondsToMclks(1000, macro_period_us);
  if (phasecal_timeout_mclks > 0xFF) { phasecal_timeout_mclks = 0xFF; }
  VL53_WriteReg8(VL53_REG_PHASECAL_CONFIG__TIMEOUT_MACROP, phasecal_timeout_mclks);

  // "Update MM Timing A timeout"
  // Timeout of 1 is tuning parm default (LOWPOWERAUTO_MM_CONFIG_TIMEOUT_US_DEFAULT)
  // via VL53L1_get_preset_mode_timing_cfg(). With the API, the register
  // actually ends up with a slightly different value because it gets assigned,
  // retrieved, recalculated with a different macro period, and reassigned,
  // but it probably doesn't matter because it seems like the MM ("mode
  // mitigation"?) sequence steps are disabled in low power auto mode anyway.
  VL53_WriteReg16(VL53_REG_MM_CONFIG__TIMEOUT_MACROP_A, VL53_EncodeTimeout(
    VL53_TimeoutMicrosecondsToMclks(1, macro_period_us)));

  // "Update Range Timing A timeout"
  VL53_WriteReg16(VL53_REG_RANGE_CONFIG__TIMEOUT_MACROP_A, VL53_EncodeTimeout(
    VL53_TimeoutMicrosecondsToMclks(range_config_timeout_us, macro_period_us)));

  // "Update Macro Period for Range B VCSEL Period"
  macro_period_us = VL53_CalcMacroPeriod(VL53_ReadReg8(VL53_REG_RANGE_CONFIG__VCSEL_PERIOD_B));

  // "Update MM Timing B timeout"
  // (See earlier comment about MM Timing A timeout.)
  VL53_WriteReg16(VL53_REG_MM_CONFIG__TIMEOUT_MACROP_B, VL53_EncodeTimeout(
    VL53_TimeoutMicrosecondsToMclks(1, macro_period_us)));

  // "Update Range Timing B timeout"
  VL53_WriteReg16(VL53_REG_RANGE_CONFIG__TIMEOUT_MACROP_B, VL53_EncodeTimeout(
    VL53_TimeoutMicrosecondsToMclks(range_config_timeout_us, macro_period_us)));

  // VL53L1_calc_timeout_register_values() end

  return 1;
}
