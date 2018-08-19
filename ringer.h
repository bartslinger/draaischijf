#ifndef RINGER_H
#define RINGER_H

#include "events.h"

typedef void (*Ringer_State) (Event);
extern Ringer_State ringer_state;

void Ringer_state_transition(Ringer_State new_state);

void Ringer_off(Event e);
void Ringer_ring(Event e);
void Ringer_pause(Event e);

#endif //Ringer_H
