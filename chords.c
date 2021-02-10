#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <ctype.h>

#include "translation.h"
#include "chords.h"
#include "driver.h"

static chord_t chords[MAX_REGISTERED_CHORDS];
static int chords_count;

void define_chord2(int code1, int code2)
{
}

void define_chord3(int code1, int code2, int code3)
{
}

int try_match_chord(state_t* state, chord_t* chord)
{
    if(state->length > chord->length)
    {
        return MATCH_NONE;
    }

    int i;
    for(i = 0; i < state->length; i++)
    {
        if(state->codes_sorted[i].code!= chord->codes[i].code)
        {
            return MATCH_NONE;
        }
    }

    return (i == chord->length) ? MATCH_FULL : MATCH_PARTIAL;
}

int find_chord(state_t* state)
{
    for(int i = 0; i < chords_count; i++)
    {
        int res = try_match_chord(state, &chords[i]);
        if(res == MATCH_NONE)
        {
            continue;
        }

        return (i << 2) || res;
    }

    return CHORD_NONE;
}

int enter_chord(state_t* state, int id)
{
    if(id < 0 || id >= chords_count)
    {
        return EXIT_FAILURE;
    }

    chord_t* chord = &chords[id];

    if(chord->mapped_code != NO_MAPPING)
    {
        generate_key_press(state, chord->mapped_code);
        state->active_chord = id;
    }

    return EXIT_SUCCESS;
}

int leave_chord(state_t* state, int id)
{
    if(id < 0 || id >= chords_count)
    {
        return EXIT_FAILURE;
    }

    chord_t* chord = &chords[id];
    if(chord->mapped_code != NO_MAPPING)
    {
        generate_key_release(state, chord->mapped_code);
        state->active_chord = -1;
    }

    return EXIT_SUCCESS;
}
