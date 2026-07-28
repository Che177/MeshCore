#pragma once

// MCP23017 GPIO-expander definitions for the simple room-server example.
//
// The MCP23017 shares the existing I2C bus and uses the unshifted 7-bit
// address 0x20. This address corresponds to the MCP23017 address pins A0,
// A1, and A2 being connected to GND.
//
// All 16 GPIO identifiers are declared here to establish one consistent
// numbering scheme for current and future inputs. Declaring a pin does not
// configure or enable it. RoomServerAccessInputs configures only the pins
// currently used by the example.

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
