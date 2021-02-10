#ifndef translation_h_INCLUDED
#define translation_h_INCLUDED

#define KEY_SHIFT_MASK (1<<25)
#define KEY_CTRL_MASK (1<<26)

#define NO_MAPPING -1

#include "state.h"

press_event_t translate_code(state_t* state, press_event_t code);

#endif // translation_h_INCLUDED

