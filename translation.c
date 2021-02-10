#include <linux/uinput.h>

#include "common.h"
#include "state.h"
#include "translation.h"

#define MAX_KEY_CODE 255

press_event_t default_layer[MAX_KEY_CODE + 1];
press_event_t numbers_layer[MAX_KEY_CODE + 1];

void init_default_layer()
{
    for(int i = 0; i <= MAX_KEY_CODE; i++)
    {
        default_layer[i].code = i;
    }
}

void init_numbers_layer()
{
    for(int i = 0; i <= MAX_KEY_CODE; i++)
    {
        numbers_layer[i].code = NO_MAPPING;
    }

    numbers_layer[KEY_A].code = KEY_1;
    numbers_layer[KEY_S].code = KEY_2;
    numbers_layer[KEY_D].code = KEY_3;
    numbers_layer[KEY_F].code = KEY_4;
    numbers_layer[KEY_G].code = KEY_5;
    numbers_layer[KEY_H].code = KEY_6;
    numbers_layer[KEY_J].code = KEY_7;
    numbers_layer[KEY_K].code = KEY_8;
    numbers_layer[KEY_L].code = KEY_9;
    numbers_layer[KEY_SEMICOLON].code = KEY_0;

    numbers_layer[KEY_Q].code = KEY_1 + KEY_SHIFT_MASK;
    numbers_layer[KEY_W].code = KEY_2 + KEY_SHIFT_MASK;
    numbers_layer[KEY_E].code = KEY_3 + KEY_SHIFT_MASK;
    numbers_layer[KEY_R].code = KEY_4 + KEY_SHIFT_MASK;
    numbers_layer[KEY_T].code = KEY_5 + KEY_SHIFT_MASK;
    numbers_layer[KEY_Y].code = KEY_6 + KEY_SHIFT_MASK;
    numbers_layer[KEY_U].code = KEY_7 + KEY_SHIFT_MASK;
    numbers_layer[KEY_I].code = KEY_8 + KEY_SHIFT_MASK;
    numbers_layer[KEY_O].code = KEY_9 + KEY_SHIFT_MASK;
    numbers_layer[KEY_P].code = KEY_0 + KEY_SHIFT_MASK;
}

press_event_t translate_code(state_t* state, press_event_t code)
{
    if(state->mode == MODE_PASSTHROUGH)
    {
        return code;
    }

    if(code.code > MAX_KEY_CODE)
    {
        return code;
    }

    press_event_t result;

    if(state->space_pressed == TRUE)
    {
        result = numbers_layer[code.code];
    }
    else
    {
        result = default_layer[code.code];
    }

    if(state->capslock_pressed)
    {
        result.code += KEY_CTRL_MASK;
    }
    return result;
}       
