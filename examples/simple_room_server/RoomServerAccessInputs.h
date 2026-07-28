#pragma once

// Optional MCP23017-based access-input example for the simple room server.
//
// The example monitors one maintained service-mode switch and two door
// contacts. Debounced changes are posted through MyMesh::addSystemPost(),
// reusing the same system-post functionality used by the existing
// `room.post <message>` console command.
//
// Service mode suppresses door notifications. Input states are initialized
// at startup without posting messages, and door states continue to be tracked
// while service mode is enabled.

#include <Arduino.h>

class MyMesh;

class RoomServerAccessInputs {
  struct DebouncedInput {
    bool stable_state;
    bool candidate_state;
    uint32_t candidate_since;
  };

  bool initialized;
  bool service_mode_enabled;
  DebouncedInput service_switch;
  DebouncedInput door_states[2];

  bool configureExpander();
  bool readLogicalActive(uint8_t pin, bool& active);
  bool initializeInput(uint8_t pin, DebouncedInput& input);
  bool updateInput(uint8_t pin, DebouncedInput& input, uint32_t now, bool& changed);

public:
  RoomServerAccessInputs();

  bool begin();
  void loop(MyMesh& mesh);
};
