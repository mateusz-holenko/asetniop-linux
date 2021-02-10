#ifndef sorted_array_h_INCLUDED
#define sorted_array_h_INCLUDED

#include "state.h"

int insert_sorted(press_event_t* array, int current_length, press_event_t value);
int remove_sorted(press_event_t* array, int current_length, press_event_t value, press_event_t* removed_element);

#endif // sorted_array_h_INCLUDED

