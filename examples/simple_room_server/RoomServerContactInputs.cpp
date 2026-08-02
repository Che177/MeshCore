#include <Arduino.h>

#if defined(ENABLE_MCP23017_CONTACT_INPUTS) && ENABLE_MCP23017_CONTACT_INPUTS == 1

#include "RoomServerContactInputs.h"

#include "MyMesh.h"
#include <Wire.h>

namespace {

using namespace Mcp23017Gpio;

static constexpr uint8_t SERVICE_SWITCH_PIN = GPIO_PA0;
static constexpr uint32_t DEBOUNCE_INTERVAL_MS = 50;
static constexpr uint32_t POLL_INTERVAL_MS = 10;
static constexpr uint32_t INITIALIZE_RETRY_INTERVAL_MS = 5000;
static constexpr uint32_t DIAGNOSTIC_INTERVAL_MS = 30000;

bool timeReached(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

#if defined(ENABLE_MCP23017_CONTACT_DEBUG) && ENABLE_MCP23017_CONTACT_DEBUG == 1

static void scanI2cBusOnce() {
  static bool already_scanned = false;
  if (already_scanned) return;
  already_scanned = true;

  Serial.println("MCP23017 debug: scanning I2C bus");

  uint8_t found = 0;

  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("I2C device found at 0x%02X\n", address);
      ++found;
    } else if (error == 4) {
      Serial.printf("Unknown I2C error at 0x%02X\n", address);
    }
  }

  Serial.printf("I2C scan complete: %u device(s)\n", found);
}

#endif

} // namespace

RoomServerContactInputs::ContactInput RoomServerContactInputs::contact_inputs[] = {
  {GPIO_PB1, "Side door opened", "Side door closed", {}},
  {GPIO_PB2, "Rear door opened", "Rear door closed", {}},
};

size_t RoomServerContactInputs::contactInputCount() {
  return sizeof(contact_inputs) / sizeof(contact_inputs[0]);
}

RoomServerContactInputs::RoomServerContactInputs()
  : initialized(false),
    service_mode_enabled(false),
    next_initialize_attempt(0),
    next_poll(0),
    next_diagnostic(0),
    service_switch{} {}

void RoomServerContactInputs::reportFailure(const char* message, uint32_t now) {
  if (!timeReached(now, next_diagnostic)) return;

  Serial.println(message);
  next_diagnostic = now + DIAGNOSTIC_INTERVAL_MS;
}

void RoomServerContactInputs::initializeInput(
  bool active, DebouncedInput& input, uint32_t now) {
  input.stable_state = active;
  input.candidate_state = active;
  input.candidate_since = now;
}

bool RoomServerContactInputs::updateInput(
  bool active, DebouncedInput& input, uint32_t now) {
  if (active != input.candidate_state) {
    input.candidate_state = active;
    input.candidate_since = now;
  }

  if (input.candidate_state == input.stable_state ||
      static_cast<uint32_t>(now - input.candidate_since) < DEBOUNCE_INTERVAL_MS) {
    return false;
  }

  input.stable_state = input.candidate_state;
  return true;
}

bool RoomServerContactInputs::initialize(uint32_t now) {
#if defined(ENABLE_MCP23017_CONTACT_DEBUG) && ENABLE_MCP23017_CONTACT_DEBUG == 1
  scanI2cBusOnce();
#endif

  uint16_t input_mask =
    static_cast<uint16_t>(1) << SERVICE_SWITCH_PIN;
  for (size_t i = 0; i < contactInputCount(); i++) {
    input_mask |= static_cast<uint16_t>(1) << contact_inputs[i].pin;
  }

  uint16_t snapshot;
  if (!gpio_expander.configureInputs(input_mask) ||
      !gpio_expander.readSnapshot(snapshot)) {
    initialized = false;
    next_initialize_attempt = now + INITIALIZE_RETRY_INTERVAL_MS;
    reportFailure("MCP23017 contact inputs unavailable; retrying", now);
    return false;
  }

  initializeInput(
    Mcp23017Inputs::isContactActive(snapshot, SERVICE_SWITCH_PIN),
    service_switch,
    now);
  service_mode_enabled = service_switch.stable_state;

  for (size_t i = 0; i < contactInputCount(); i++) {
    initializeInput(
      Mcp23017Inputs::isContactActive(snapshot, contact_inputs[i].pin),
      contact_inputs[i].state,
      now);
  }

  initialized = true;
  next_poll = now + POLL_INTERVAL_MS;
  Serial.printf("MCP23017 contact inputs ready at 0x%02X\n", I2C_ADDRESS);
  return true;
}

bool RoomServerContactInputs::begin() {
  return initialize(millis());
}

void RoomServerContactInputs::loop(MyMesh& mesh) {
  const uint32_t now = millis();

  if (!initialized) {
    if (timeReached(now, next_initialize_attempt)) initialize(now);
    return;
  }

  if (!timeReached(now, next_poll)) return;
  next_poll = now + POLL_INTERVAL_MS;

  uint16_t snapshot;
  if (!gpio_expander.readSnapshot(snapshot)) {
    reportFailure("MCP23017 contact snapshot read failed", now);
    return;
  }

  const bool service_active =
    Mcp23017Inputs::isContactActive(snapshot, SERVICE_SWITCH_PIN);
  if (updateInput(service_active, service_switch, now)) {
    service_mode_enabled = service_switch.stable_state;
    mesh.addSystemPost(
      service_mode_enabled ? "Service mode enabled" : "Service mode disabled");
  }

  for (size_t i = 0; i < contactInputCount(); i++) {
    ContactInput& contact = contact_inputs[i];
    const bool contact_active =
      Mcp23017Inputs::isContactActive(snapshot, contact.pin);
    const bool contact_changed =
      updateInput(contact_active, contact.state, now);

    if (!contact_changed || service_mode_enabled) continue;

    mesh.addSystemPost(
      contact.state.stable_state
        ? contact.active_message
        : contact.inactive_message);
  }
}

#endif
