#pragma once

// Optional MCP23017 contact-input monitoring for the simple room server.
//
// A maintained service-mode contact and a configurable set of active-low
// contacts are debounced from a shared MCP23017 GPIO snapshot. Configured
// transition messages are submitted through MyMesh::addSystemPost(), reusing
// the same system-post functionality used by `room.post <message>`.
//
// The contact-monitoring logic assigns no physical meaning to a contact.
// Physical meanings exist only in the configured human-readable messages.

#include <Arduino.h>

#include "Mcp23017Inputs.h"

class MyMesh;

class RoomServerContactInputs {
  struct DebouncedInput {
    bool stable_state;
    bool candidate_state;
    uint32_t candidate_since;
  };

  struct ContactInput {
    uint8_t pin;
    const char* active_message;
    const char* inactive_message;
    DebouncedInput state;
  };

  static ContactInput contact_inputs[];
  static size_t contactInputCount();

  Mcp23017Inputs gpio_expander;
  bool initialized;
  bool service_mode_enabled;
  uint32_t next_initialize_attempt;
  uint32_t next_poll;
  uint32_t next_diagnostic;
  DebouncedInput service_switch;

  bool initialize(uint32_t now);
  void reportFailure(const char* message, uint32_t now);
  void initializeInput(bool active, DebouncedInput& input, uint32_t now);
  bool updateInput(bool active, DebouncedInput& input, uint32_t now);

public:
  RoomServerContactInputs();

  bool begin();
  void loop(MyMesh& mesh);
};
