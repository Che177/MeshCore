#include <Arduino.h>

#if defined(ENABLE_MCP23017_CONTACT_INPUTS) && ENABLE_MCP23017_CONTACT_INPUTS == 1

#include "Mcp23017Inputs.h"

#include <Wire.h>

namespace {

using namespace Mcp23017Gpio;

static constexpr uint8_t MCP23017_IODIRA = 0x00;
static constexpr uint8_t MCP23017_IODIRB = 0x01;
static constexpr uint8_t MCP23017_GPPUA = 0x0C;
static constexpr uint8_t MCP23017_GPPUB = 0x0D;
static constexpr uint8_t MCP23017_GPIOA = 0x12;
static constexpr uint8_t MCP23017_GPIOB = 0x13;

} // namespace

bool Mcp23017Inputs::readRegister(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom(I2C_ADDRESS, static_cast<uint8_t>(1)) != 1) return false;
  value = Wire.read();
  return true;
}

bool Mcp23017Inputs::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool Mcp23017Inputs::setRegisterBits(uint8_t reg, uint8_t bits) {
  uint8_t value;
  return readRegister(reg, value) && writeRegister(reg, value | bits);
}

bool Mcp23017Inputs::configureInputs(uint16_t input_mask) {
  const uint8_t port_a_mask = input_mask & 0xFF;
  const uint8_t port_b_mask = input_mask >> 8;

  return setRegisterBits(MCP23017_IODIRA, port_a_mask) &&
         setRegisterBits(MCP23017_IODIRB, port_b_mask) &&
         setRegisterBits(MCP23017_GPPUA, port_a_mask) &&
         setRegisterBits(MCP23017_GPPUB, port_b_mask);
}

bool Mcp23017Inputs::readSnapshot(uint16_t& snapshot) {
  uint8_t gpio_a;
  uint8_t gpio_b;
  if (!readRegister(MCP23017_GPIOA, gpio_a) ||
      !readRegister(MCP23017_GPIOB, gpio_b)) {
    return false;
  }

  snapshot = static_cast<uint16_t>(gpio_a) |
             (static_cast<uint16_t>(gpio_b) << 8);
  return true;
}

bool Mcp23017Inputs::isContactActive(uint16_t snapshot, uint8_t pin) {
  return (snapshot & (static_cast<uint16_t>(1) << pin)) == 0;
}

#endif
