#include <Arduino.h>
#include "ringer.h"
#include "config.h"

// The global Ringer_state
Ringer_State ringer_state;

void Ringer_state_transition(Ringer_State new_state) {
  // execute exit of old state
  ringer_state(EXIT);
  ringer_state = new_state;
  // and entry of the new state
  ringer_state(ENTRY);
}

struct RingerVars {
  uint32_t state_presence = 0;
} ringer_vars;

void Ringer_off(Event e) {
  switch(e) {
    
    case ENTRY:
      //Serial.println("ringer off");
      ringer_vars.state_presence = 0;
      break;
    
    case EXIT:
      break;
    
    case PERIODIC:
      break;

    case START_RINGING:
      Ringer_state_transition(&Ringer_ring);
      break;
  }
}

void Ringer_ring(Event e) {
  switch(e) {
    
    case ENTRY:
      ringer_vars.state_presence = 0;
      //Serial.println("Start");
      digitalWrite(RINGER_RESET_PIN, HIGH);
      break;
    
    case EXIT:
      //Serial.println("stop");
      digitalWrite(RINGER_RESET_PIN, LOW);
      break;
    
    case PERIODIC:
      digitalWrite(RINGER_STEP_PIN, !digitalRead(RINGER_STEP_PIN));
      if (ringer_vars.state_presence >= 200) {
        Ringer_state_transition(&Ringer_pause);
        break;
      }
      ringer_vars.state_presence++;
      break;

    case START_RINGING:
      break;

    case STOP_RINGING:
      Ringer_state_transition(&Ringer_off);
      break;
  }
}

void Ringer_pause(Event e) {
  switch(e) {
    
    case ENTRY:
      ringer_vars.state_presence = 0;
      break;
    
    case EXIT:
      break;
    
    case PERIODIC:
      if (ringer_vars.state_presence >= 400) {
        Ringer_state_transition(&Ringer_ring);
        break;
      }
      ringer_vars.state_presence++;
      break;

    case START_RINGING:
      break;

    case STOP_RINGING:
      Ringer_state_transition(&Ringer_off);
      break;
  }
}

