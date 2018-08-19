#include <Arduino.h>
#include "dial.h"

char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

// The global dial_state
Dial_State dial_state;
Dial_Output dial_output;
Dial_Input dial_input;

void Dial_state_transition(Dial_State new_state) {
  // execute exit of old state
  dial_state(EXIT);
  dial_state = new_state;
  // and entry of the new state
  dial_state(ENTRY);
}

struct DialVars {
  uint32_t state_presence = 0;
  char number[20];
  uint8_t number_idx = 0;
  uint8_t pulse_counter = 0;
} dial_vars;

void Dial_waiting_for_number(Event e) {
  switch(e) {
    
    case ENTRY:
      memset(dial_vars.number, 0, sizeof(dial_vars.number));
      dial_vars.number_idx = 0;
      dial_vars.pulse_counter = 0;
      dial_vars.state_presence = 0;
      break;
    
    case EXIT:
      break;
    
    case PERIODIC:
      if (dial_input.dial_pin == LOW) {
        Dial_state_transition(&Dial_low);
      }
      break;
  }
}

void Dial_low(Event e) {
    switch(e) {
    
    case ENTRY:
      dial_vars.state_presence = 1;
      break;
    
    case EXIT:
      break;
    
    case PERIODIC:
      dial_vars.state_presence++;
      if (dial_input.dial_pin == HIGH && dial_vars.state_presence >= 3 && dial_vars.state_presence < 50) {
        dial_vars.pulse_counter++;
        Dial_state_transition(&Dial_high);
      }
      break;
  }
}

void Dial_high(Event e) {
    switch(e) {
    
    case ENTRY:
      dial_vars.state_presence = 1;
      break;
    
    case EXIT:
      break;
    
    case PERIODIC:
      dial_vars.state_presence++;
      if (dial_input.dial_pin == LOW && dial_vars.state_presence >= 2 && dial_vars.state_presence < 20) {
        Dial_state_transition(&Dial_low);
      }
      if (dial_vars.state_presence >= 20) {
        // digit ready
        dial_vars.number[dial_vars.number_idx++] = digits[dial_vars.pulse_counter % 10];
        Dial_state_transition(&Dial_wait_next_digit);
      }
      break;
  }
}

void Dial_wait_next_digit(Event e) {
    switch(e) {
    
    case ENTRY:
      dial_vars.pulse_counter = 0;
      dial_vars.state_presence = 0;
      break;
    
    case EXIT:
      break;
    
    case PERIODIC:
      if (dial_input.dial_pin == LOW) {
        Dial_state_transition(&Dial_low);
      }
    
      dial_vars.state_presence++;
      if (dial_vars.state_presence >= 200) {
        // number ready
        memcpy(dial_output.number, dial_vars.number, sizeof(dial_vars.number));
        dial_output.ready = true;
        Dial_state_transition(&Dial_waiting_for_number);
      }
      break;
  }
}

