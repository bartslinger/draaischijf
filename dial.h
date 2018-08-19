#ifndef DIAL_H
#define DIAL_H

#include "events.h"

typedef void (*Dial_State) (Event);
extern Dial_State dial_state;

typedef struct Dial_Output {
  bool ready = false;
  char number[20] = {0};
};
extern Dial_Output dial_output;

typedef struct Dial_Input {
  uint8_t dial_pin = HIGH;
};
extern Dial_Input dial_input;

void Dial_state_transition(Dial_State new_state);

void Dial_waiting_for_number(Event e);
void Dial_low(Event e);
void Dial_high(Event e);
void Dial_wait_next_digit(Event e);

#endif //DIAL_H
