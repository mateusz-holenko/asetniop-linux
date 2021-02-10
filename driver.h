#ifndef driver_h_INCLUDED
#define driver_h_INCLUDED

#include "state.h"

int handle_key_press(state_t* state, press_event_t code);
int handle_key_release(state_t* state, press_event_t code);

void generate_key_press(state_t* state, int code);
void generate_key_release(state_t* state, int code);

#endif // driver_h_INCLUDED

