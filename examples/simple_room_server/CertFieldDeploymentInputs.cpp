#include <Arduino.h>

#if defined(CERT_FIELD_DEPLOYMENT) && CERT_FIELD_DEPLOYMENT == 1

#include "CertFieldDeploymentInputs.h"
#include <Wire.h>

#include "CertFieldDeploymentHardware.h"
#include "MyMesh.h"

namespace {

using namespace CertFieldDeploymentHardware;

static constexpr uint8_t MCP23017_IODIRA = 0x00;
static constexpr uint8_t MCP23017_IODIRB = 0x01;
static constexpr uint8_t MCP23017_GPPUA = 0x0C;
static constexpr uint8_t MCP23017_GPPUB = 0x0D;
static constexpr uint8_t MCP23017_GPIOA = 0x12;
static constexpr uint8_t MCP23017_GPIOB = 0x13;

static constexpr uint8_t SERVICE_SWITCH_PIN = GPIO_PA0;
static constexpr uint32_t DEBOUNCE_INTERVAL_MS = 50;

struct DoorInputConfig {
  uint8_t pin;
  const char* name;
};

static constexpr DoorInputConfig DOOR_INPUTS[] = {
  {GPIO_PB1, "Side"},
  {GPIO_PB2, "Rear"},
};

static constexpr size_t DOOR_INPUT_COUNT = sizeof(DOOR_INPUTS) / sizeof(DOOR_INPUTS[0]);

bool readRegister(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(GPIO_EXPANDER_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom(GPIO_EXPANDER_ADDRESS, static_cast<uint8_t>(1)) != 1) return false;
  value = Wire.read();
  return true;
}

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(GPIO_EXPANDER_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool setRegisterBits(uint8_t reg, uint8_t bits) {
  uint8_t value;
  return readRegister(reg, value) && writeRegister(reg, value | bits);
}

} // namespace

CertFieldDeploymentInputs::CertFieldDeploymentInputs()
  : initialized(false), service_mode_enabled(false), service_switch{}, door_states{} {}

bool CertFieldDeploymentInputs::configureExpander() {
  const uint8_t port_a_inputs = 1U << (SERVICE_SWITCH_PIN & 7);
  const uint8_t port_b_inputs =
    (1U << (DOOR_INPUTS[0].pin & 7)) |
    (1U << (DOOR_INPUTS[1].pin & 7));

  return setRegisterBits(MCP23017_IODIRA, port_a_inputs) &&
         setRegisterBits(MCP23017_IODIRB, port_b_inputs) &&
         setRegisterBits(MCP23017_GPPUA, port_a_inputs) &&
         setRegisterBits(MCP23017_GPPUB, port_b_inputs);
}

bool CertFieldDeploymentInputs::readLogicalActive(uint8_t pin, bool& active) {
  const uint8_t gpio_register = pin < 8 ? MCP23017_GPIOA : MCP23017_GPIOB;
  uint8_t port_state;
  if (!readRegister(gpio_register, port_state)) return false;

  const bool electrical_high = (port_state & (1U << (pin & 7))) != 0;
  active = !electrical_high;
  return true;
}

bool CertFieldDeploymentInputs::initializeInput(uint8_t pin, DebouncedInput& input) {
  bool active;
  if (!readLogicalActive(pin, active)) return false;

  input.stable_state = active;
  input.candidate_state = active;
  input.candidate_since = millis();
  return true;
}

bool CertFieldDeploymentInputs::updateInput(
  uint8_t pin, DebouncedInput& input, uint32_t now, bool& changed) {
  changed = false;

  bool active;
  if (!readLogicalActive(pin, active)) return false;

  if (active != input.candidate_state) {
    input.candidate_state = active;
    input.candidate_since = now;
  }

  if (input.candidate_state != input.stable_state &&
      static_cast<uint32_t>(now - input.candidate_since) >= DEBOUNCE_INTERVAL_MS) {
    input.stable_state = input.candidate_state;
    changed = true;
  }

  return true;
}

void CertFieldDeploymentInputs::initializeDoorStates() {
  for (size_t i = 0; i < DOOR_INPUT_COUNT; i++) {
    initializeInput(DOOR_INPUTS[i].pin, door_states[i]);
  }
}

bool CertFieldDeploymentInputs::begin() {
  if (!configureExpander() ||
      !initializeInput(SERVICE_SWITCH_PIN, service_switch)) {
    Serial.println("MCP23017 field inputs unavailable");
    return false;
  }

  for (size_t i = 0; i < DOOR_INPUT_COUNT; i++) {
    if (!initializeInput(DOOR_INPUTS[i].pin, door_states[i])) {
      Serial.println("MCP23017 field inputs unavailable");
      return false;
    }
  }

  service_mode_enabled = service_switch.stable_state;
  initialized = true;
  Serial.printf("MCP23017 field inputs ready at 0x%02X\n", GPIO_EXPANDER_ADDRESS);
  return true;
}

void CertFieldDeploymentInputs::loop(MyMesh& mesh) {
  if (!initialized) return;

  const uint32_t now = millis();
  bool service_changed;
  if (!updateInput(SERVICE_SWITCH_PIN, service_switch, now, service_changed)) return;

  if (service_changed) {
    service_mode_enabled = service_switch.stable_state;
    mesh.addSystemPost(service_mode_enabled ? "Service mode enabled" : "Service mode disabled");

    if (!service_mode_enabled) {
      // Resynchronize at service exit so transitions made during service cannot
      // appear later as stale door events.
      initializeDoorStates();
    }
  }

  for (size_t i = 0; i < DOOR_INPUT_COUNT; i++) {
    bool door_changed;
    if (!updateInput(DOOR_INPUTS[i].pin, door_states[i], now, door_changed)) continue;
    if (!door_changed || service_mode_enabled || service_changed) continue;

    char message[32];
    snprintf(message, sizeof(message), "%s door %s",
             DOOR_INPUTS[i].name,
             door_states[i].stable_state ? "opened" : "closed");
    mesh.addSystemPost(message);
  }
}

#endif
