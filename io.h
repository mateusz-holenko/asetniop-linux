#ifndef io_h_INCLUDED
#define io_h_INCLUDED

#define EVENT_KEY_RELEASED 0
#define EVENT_KEY_PRESSED 1
#define EVENT_KEY_REPEATED 2

int try_read_key_event(struct input_event*);

void generate_sync_event();
void generate_key_event(int code, int type);

#endif // io_h_INCLUDED

