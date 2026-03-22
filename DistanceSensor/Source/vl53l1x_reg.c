#include "vl53l1x_reg.h"

#include "hal_i2c.h"


uint8 VL53_ReadReg8(uint16 reg)
{
  return I2C_ReadReg8(VL53_DEVICE_ADDRESS, reg);
}

uint16 VL53_ReadReg16(uint16 reg)
{
  return I2C_ReadReg16(VL53_DEVICE_ADDRESS, reg);
}

uint32 VL53_ReadReg32(uint16 reg)
{
  return I2C_ReadReg32(VL53_DEVICE_ADDRESS, reg);
}

uint8 VL53_WriteReg8(uint16 reg, uint8 value)
{
  return I2C_WriteReg8(VL53_DEVICE_ADDRESS, reg, value);
}

uint8 VL53_WriteReg16(uint16 reg, uint16 value)
{
  return I2C_WriteReg16(VL53_DEVICE_ADDRESS, reg, value);
}

uint8 VL53_WriteReg32(uint16 reg, uint32 value)
{
  return I2C_WriteReg32(VL53_DEVICE_ADDRESS, reg, value);
}
