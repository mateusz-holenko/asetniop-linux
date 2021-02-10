#include <stdlib.h>
#include <time.h>
#include <linux/uinput.h>

#include "log.h"
#include "common.h"
#include "state.h"
#include "sorted_array.h"
#include "translation.h"
#include "chords.h"
#include "io.h"

int deferr_key_press(state_t* state, press_event_t code)
{
    if(state->control_pressed)
    {
        return FALSE;
    }

    if(   code.code == KEY_Q || code.code == KEY_W || code.code == KEY_E || code.code == KEY_R || code.code == KEY_T
       || code.code == KEY_Y || code.code == KEY_U || code.code == KEY_I || code.code == KEY_O || code.code == KEY_P
       || code.code == KEY_A || code.code == KEY_S || code.code == KEY_D || code.code == KEY_F || code.code == KEY_G
       || code.code == KEY_H || code.code == KEY_J || code.code == KEY_K || code.code == KEY_L || code.code == KEY_Z
       || code.code == KEY_X || code.code == KEY_C || code.code == KEY_V || code.code == KEY_B || code.code == KEY_N
       || code.code == KEY_M
       || code.code == KEY_0 || code.code == KEY_1 || code.code == KEY_2 || code.code == KEY_3 || code.code == KEY_4
       || code.code == KEY_5 || code.code == KEY_6 || code.code == KEY_7 || code.code == KEY_8 || code.code == KEY_9
       || code.code == KEY_SPACE
       )
    {
        return TRUE;
    }

    return FALSE;
}

int register_press(state_t* state, press_event_t code)
{
    // TODO: do better
    int flag = 0;
    for(int i = 0; i < state->length; i++)
    {
        if(state->codes_chrono[i].code == code.code)
        {
            state->codes_chrono[i].repeats++;
            flag = 1;
        }

        if(state->codes_sorted[i].code == code.code)
        {
            state->codes_sorted[i].repeats++;
            flag = 1;
        }
    }

    if(flag)
    {
        return EXIT_SUCCESS;
    }

    if(state->length == MAX_CHORD_LENGTH)
    {
        return EXIT_FAILURE;
    }

    state->codes_chrono[state->length] = code;
    insert_sorted(state->codes_sorted, state->length, code);
    state->length++;

    if(code.code == KEY_LEFTCTRL || code.code == KEY_RIGHTCTRL)
    {
        state->control_pressed = TRUE;
    }
    
    return EXIT_SUCCESS;
}

int register_release(state_t* state, press_event_t code, press_event_t* corresponding_press)
{
    if(state->length == 0)
    {
        return EXIT_FAILURE;
    }

    if(remove_sorted(state->codes_sorted, state->length, code, corresponding_press) == EXIT_FAILURE ||
       remove_sorted(state->codes_chrono, state->length, code, corresponding_press) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    if(code.code == KEY_LEFTCTRL || code.code == KEY_RIGHTCTRL)
    {
        state->control_pressed = FALSE;
    }

    state->length--;
    return EXIT_SUCCESS;
}

void generate_key_press(state_t* state, int code)
{
    int raw_code = code & ~(KEY_SHIFT_MASK | KEY_CTRL_MASK);
    debug("Generating key press event for key: %d (with modifiers: %d)\n", raw_code);

    if((code & KEY_SHIFT_MASK) != 0)
    {
        generate_key_event(KEY_LEFTSHIFT, 1);
    }

    if((code & KEY_CTRL_MASK) != 0)
    {
        generate_key_event(KEY_LEFTCTRL, 1);
    }

    generate_key_event(raw_code, 1);

    generate_sync_event();
}

void generate_key_release(state_t* state, int code)
{
    int raw_code = code & ~(KEY_SHIFT_MASK | KEY_CTRL_MASK);
    debug("Generating key release event for key: %d (with modifiers: %d)\n", raw_code, code);

    generate_key_event(raw_code, 0);

    if((code & KEY_SHIFT_MASK) != 0)
    {
        generate_key_event(KEY_LEFTSHIFT, 0);
    }

    if((code & KEY_CTRL_MASK) != 0)
    {
        generate_key_event(KEY_LEFTCTRL, 0);
    }

    generate_sync_event();
}



/*
void generate_all_registered_keys(state_t* state)
{
    state->do_not_match_chords = TRUE;
    for(int i = 0; i < state->length; i++)
    {
        translate_and_generate(state->codes_chrono[i]);
    }
}
*/

/*
void release_all_pressed_keys(state_t* state)
{
    for(int i = pressed_keys_count - 1; i >= 0; i--)
    {
      generate_key_release(state, pressed_keys[i]);
    }
}
*/

int handle_key_press(state_t* state, press_event_t code)
{
    debug("Key Pressed: %d\n", code);

    if(code.code == KEY_RIGHTMETA)
    {
        info("Detected exit key (RIGHT META) press, exiting.\n");
        return EXIT_FAILURE;
    }
/*
    if(code == KEY_SCROLLLOCK)
    {
        if(mode == MODE_PASSTHROUGH)
        {
            info("Switching mode to NORMAL\n");
            mode = MODE_NORMAL;
        }
        else if(mode == MODE_NORMAL)
        {
            info("Switching mode to PASSTHROUGH\n");
            mode = MODE_PASSTHROUGH;
        }
        else
        {
            error("Unsupported mode: 0x%x\n", mode);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
  
    if(code == KEY_SPACE && pressed_keys_count == 0)
    {
        // this one can work as a chord leader key
        space_pressed = TRUE;
        chord_generated = FALSE;
        return EXIT_SUCCESS;
    }

    if(code == KEY_CAPSLOCK)
    {
        capslock_pressed = TRUE;
        chord_generated = FALSE;
        return EXIT_SUCCESS;
    }
*/

    press_event_t translated_code = translate_code(state, code);
    if(translated_code.code == NO_MAPPING)
    {
        debug("Ignoring key press\n");
        return EXIT_SUCCESS;
    }

    int deferred = deferr_key_press(state, translated_code);
    if(deferred)
    {
        code.press_event_deferred = TRUE;
    }

    if(register_press(state, code) == EXIT_FAILURE)
    {
        warning("Cannot handle more pressed keys - ignoring %d\n", code);
        return EXIT_FAILURE;
    }

    if(!deferred)
    {
        generate_key_press(state, translated_code.code);
    }

/*
    if(state->do_not_match_chords)
    {
        translate_and_generate(state->codes_chrono[i]);
    }
    else
    {
        int result = find_chord(state);
        int chord_type = (result && 0x3);
        int chord_id = chord_type >> 2;

        switch(chord_type)
        {
            case CHORD_NONE:
                state->do_not_match_chords = TRUE;
                send_pressed_keys(state);
                break;

            case CHORD_PARTIAL:
                // do nothing - wait for more
                break;

            case CHORD_COMPLETE:
                enter_chord(state, chord_id);
                break;

            case CHORD_ERROR:
                return EXIT_FAILURE;
        }
    }
*/

    return EXIT_SUCCESS;
}

int handle_key_release(state_t* state, press_event_t code)
{
    press_event_t corresponding_press;

    debug("Key Released: %d\n", code);
    if(register_release(state, code, &corresponding_press) == EXIT_FAILURE)
    {
        error("There was an error when registering release");
        return EXIT_FAILURE;
    }

/*
    if(code == KEY_SPACE)
    {
        if(chord_generated == FALSE)
        {
            space_pressed = FALSE;
            generate_key_press(state, KEY_SPACE);
            generate_key_release(state, KEY_SPACE);
        }
        else
        {
            // release all currently presed keys
            // before clearing the space_pressed flag
            release_all_pressed_keys();
            space_pressed = FALSE;
        }
        return EXIT_SUCCESS;
    }

    if(code == KEY_CAPSLOCK)
    {
        if(chord_generated == FALSE)
        {
            capslock_pressed = FALSE;
            generate_key_press(state, KEY_ESC);
            generate_key_release(state, KEY_ESC);
        }
        else
        {
            release_all_pressed_keys();
            capslock_pressed = FALSE;
        }
    }
*/

    press_event_t translated_code = translate_code(state, code);
    if(translated_code.code == NO_MAPPING)
    {
        debug("Ignoring key release\n");
        return EXIT_SUCCESS;
    }

    int autoshift = FALSE;

    if(corresponding_press.press_event_deferred)
    {
        struct timeval result;
        timersub(&code.timestamp, &corresponding_press.timestamp, &result);
        if(result.tv_sec == 0 && result.tv_usec > 250000)
        {
            autoshift = TRUE;
        }

        if(autoshift)
        {
            generate_key_press(state, KEY_LEFTSHIFT);
        }

        generate_key_press(state, translated_code.code);
    }

    generate_key_release(state, translated_code.code);

    if(autoshift)
    {
        generate_key_release(state, KEY_LEFTSHIFT);
    }
    return EXIT_SUCCESS;
}

