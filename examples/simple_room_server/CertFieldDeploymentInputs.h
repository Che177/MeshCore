#pragma once

#include <Arduino.h>

class MyMesh;

class CertFieldDeploymentInputs {
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
  void initializeDoorStates();

public:
  CertFieldDeploymentInputs();

  bool begin();
  void loop(MyMesh& mesh);
};
