#ifndef state_h_INCLUDED
#define state_h_INCLUDED

#define MODE_PASSTHROUGH 147
#define MODE_NORMAL 100

#define MAX_REGISTERED_CHORDS 10
#define MAX_CHORD_LENGTH 10

#define CHORD_ERROR -1                                                                   
#define CHORD_NONE 0
#define CHORD_PARTIAL 1                                                         
#define CHORD_COMPLETE 2

#define MATCH_NONE 0
#define MATCH_FULL 1
#define MATCH_PARTIAL 2

typedef struct press_event_t
{
    int code; 
    int press_event_deferred;
    struct timeval timestamp;
    int repeats;
} press_event_t;

typedef struct chord_t
{
    int length;
    press_event_t codes[MAX_CHORD_LENGTH];
    int mapped_code;
} chord_t;

typedef struct state_t
{
    int length;
    press_event_t codes_chrono[MAX_CHORD_LENGTH];
    press_event_t codes_sorted[MAX_CHORD_LENGTH];
    int active_chord;

    int do_not_match_chords;
    int space_pressed;
    int capslock_pressed;
    int control_pressed;
    int mode;
} state_t;

#endif // state_h_INCLUDED

