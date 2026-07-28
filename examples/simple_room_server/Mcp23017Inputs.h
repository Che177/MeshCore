#pragma once

// Minimal MCP23017 input support for the simple room-server contact example.
//
// The module configures selected pins as inputs with pull-ups and returns one
// 16-bit electrical-level snapshot containing GPIOA and GPIOB. It intentionally
// implements only the MCP23017 operations required by this example.

#include <Arduino.h>

namespace Mcp23017Gpio {

static constexpr uint8_t I2C_ADDRESS = 0x20;

static constexpr uint8_t GPIO_PA0 = 0;
static constexpr uint8_t GPIO_PA1 = 1;
static constexpr uint8_t GPIO_PA2 = 2;
static constexpr uint8_t GPIO_PA3 = 3;
static constexpr uint8_t GPIO_PA4 = 4;
static constexpr uint8_t GPIO_PA5 = 5;
static constexpr uint8_t GPIO_PA6 = 6;
static constexpr uint8_t GPIO_PA7 = 7;

static constexpr uint8_t GPIO_PB0 = 8;
static constexpr uint8_t GPIO_PB1 = 9;
static constexpr uint8_t GPIO_PB2 = 10;
static constexpr uint8_t GPIO_PB3 = 11;
static constexpr uint8_t GPIO_PB4 = 12;
static constexpr uint8_t GPIO_PB5 = 13;
static constexpr uint8_t GPIO_PB6 = 14;
static constexpr uint8_t GPIO_PB7 = 15;

} // namespace Mcp23017Gpio

class Mcp23017Inputs {
  bool readRegister(uint8_t reg, uint8_t& value);
  bool writeRegister(uint8_t reg, uint8_t value);
  bool setRegisterBits(uint8_t reg, uint8_t bits);

public:
  bool configureInputs(uint16_t input_mask);
  bool readSnapshot(uint16_t& snapshot);

  static bool isContactActive(uint16_t snapshot, uint8_t pin);
};
