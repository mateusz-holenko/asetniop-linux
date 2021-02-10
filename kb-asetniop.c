#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <ctype.h>

#include "chords.h"
#include "state.h"
#include "log.h"
#include "io.h"
#include "driver.h"

#define MAX_PRESSED_KEYS 10

#define NOT_SET 0
#define USED 1
#define SET 2

#define __STRINGIFY(y) #y

#define _STRINGIFY(y) __STRINGIFY(\x##y)
#define STRINGIFY(y) _STRINGIFY(y)

#define _HEXIFY(y) 0x##y 
#define HEXIFY(y) _HEXIFY(y) 

#define LEADER_PRESSED 999

static state_t current_state;

char** current_mapping;


char* mapping[2048];
char* symbols_mapping[2048];
char* output_mapping[256];

unsigned int state = 0;
unsigned int mask  = 0;

unsigned int pressed_keys_count = 0;
unsigned int pressed_keys[MAX_PRESSED_KEYS];

int mode;
int chord_generated;

char* create_output_mapping(char code)
{
  char* result = (char*)malloc(2);
  result[0] = 1;
  result[1] = code;

  return result;
}

char* create_output_mapping2(char code1, char code2)
{
  char* result = (char*)malloc(3);
  result[0] = 2;
  result[1] = code1;
  result[2] = code2;

  return result;
}

/*
void translate_and_generate(int code)
{
    int translated_code = translate_code(code);
    if(translated_code == NO_MAPPING)
    {
        debug("Ignoring key press\n");
        return EXIT_SUCCESS;
    }

    generate_key_press(translated_code);
}
*/

void loop()
{
    struct input_event ev;
    while(1)
    {
        if(try_read_key_event(&ev) == EXIT_FAILURE)
        {
            break;
        }

        noisy("Keyboard event: type %d, value %d, code %d\n", ev.type, ev.value, ev.code);

        press_event_t internal_event;
        internal_event.code = ev.code;
        internal_event.timestamp = ev.time;

        switch(ev.value)
        {
            case EVENT_KEY_PRESSED:
            case EVENT_KEY_REPEATED:
                if(handle_key_press(&current_state, internal_event) == EXIT_FAILURE)
                {
                    goto end;
                }
                break;

            case EVENT_KEY_RELEASED:
                handle_key_release(&current_state, internal_event);
                break;
        }
    }

end:
    info("Exiting the main loop");
}

void init_default_layer();
void init_numbers_layer();

int open_input();
int open_output();

int reset_state(state_t* state)
{
    state->length = 0;
    state->active_chord = -1;
    state->mode = MODE_PASSTHROUGH;
}

int main()
{
    reset_state(&current_state);

    info("KB mapper started in the PASSTHROUGH mode\n");
    // to avoid catching the enter key
    sleep(1);

    init_default_layer();
    init_numbers_layer();

    current_mapping = mapping;

    open_input();
    open_output();
    loop();
    return 0;
}
