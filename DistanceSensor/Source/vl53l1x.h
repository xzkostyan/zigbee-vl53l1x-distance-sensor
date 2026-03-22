#ifndef VL53L1X_H
#define VL53L1X_H

#include "hal_types.h"


#define VL53_DISTANCE_MODE_SHORT        0x01
#define VL53_DISTANCE_MODE_MEDIUM       0x02
#define VL53_DISTANCE_MODE_LONG         0x03

// value used in measurement timing budget calculations
// assumes PresetMode is LOWPOWER_AUTONOMOUS
//
// vhv = LOWPOWER_AUTO_VHV_LOOP_DURATION_US + LOWPOWERAUTO_VHV_LOOP_BOUND
//       (tuning parm default) * LOWPOWER_AUTO_VHV_LOOP_DURATION_US
//     = 245 + 3 * 245 = 980
// TimingGuard = LOWPOWER_AUTO_OVERHEAD_BEFORE_A_RANGING +
//               LOWPOWER_AUTO_OVERHEAD_BETWEEN_A_B_RANGING + vhv
//             = 1448 + 2100 + 980 = 4528
#define TIMING_GUARD  (uint32)4528

// value in DSS_CONFIG__TARGET_TOTAL_RATE_MCPS register, used in DSS
// calculations
#define TARGET_RATE 0x0A00


uint8 VL53_Init(void);
bool VL53_SetDistanceMode(uint8 mode);

void VL53_StartContinuous(uint32 period_ms);
void VL53_StopContinuous(void);

int16 VL53_ReadSingle(void);

void VL53_SetROISize(uint8 width, uint8 height);
void VL53_GetROISize(uint8 * width, uint8 * height);

uint8 VL53_GetROICenter(void);
void VL53_SetROICenter(uint8 spadNumber);

uint32 VL53_GetMeasurementTimingBudget(void);
uint8 VL53_SetMeasurementTimingBudget(uint32 budget_us);

#endif