#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <ctype.h>

#define MAX_PRESSED_KEYS 10
#define MAX_KEY_CODE 255
#define NO_MAPPING -1

#define NOT_SET 0
#define USED 1
#define SET 2

#define __STRINGIFY(y) #y

#define _STRINGIFY(y) __STRINGIFY(\x##y)
#define STRINGIFY(y) _STRINGIFY(y)

#define _HEXIFY(y) 0x##y 
#define HEXIFY(y) _HEXIFY(y) 

#define MODE_PASSTHROUGH 147
#define MODE_NORMAL 100

#define TRUE 1
#define FALSE 0

#define LEADER_PRESSED 999

#define KEY_SHIFT_MASK (1<<25)
#define KEY_CTRL_MASK (1<<26)

// const char* input_dev = "/dev/input/by-path/pci-0000:00:1a.0-usb-0:1.6:1.0-event-kbd";
const char* input_dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
#ifdef ALT_EXTENSION
  #define ASETNIOP_ALT (1 << 10)
#endif
const char* output_dev = "/dev/uinput";

char** current_mapping;

int default_layer[MAX_KEY_CODE + 1];
int numbers_layer[MAX_KEY_CODE + 1];

char* mapping[2048];
char* symbols_mapping[2048];
char* output_mapping[256];

unsigned int state = 0;
unsigned int mask  = 0;

unsigned int pressed_keys_count = 0;
unsigned int pressed_keys[MAX_PRESSED_KEYS];

int mode;
int space_pressed = FALSE;
int capslock_pressed = FALSE;
int chord_generated;

int sticky_shift = 0;
#ifdef ALT_EXTENSION
int sticky_alt = 0;
#endif
int operational_mode = 1;

int output_descriptor;
int input_descriptor;

#define warning(...)
#define noisy(...)
#define info printf
#define error printf

// #define DEBUG_LOG

#ifdef DEBUG_LOG
#define debug printf
#else
#define debug(...)
#endif

void define_chord2(int code1, int code2)
{
}

void define_chord3(int code1, int code2, int code3)
{
}

void init_default_layer()
{
    for(int i = 0; i <= MAX_KEY_CODE; i++)
    {
        default_layer[i] = i;
    }
}

void init_numbers_layer()
{
    for(int i = 0; i <= MAX_KEY_CODE; i++)
    {
        numbers_layer[i] = NO_MAPPING;
    }

    numbers_layer[KEY_A] = KEY_1;
    numbers_layer[KEY_S] = KEY_2;
    numbers_layer[KEY_D] = KEY_3;
    numbers_layer[KEY_F] = KEY_4;
    numbers_layer[KEY_G] = KEY_5;
    numbers_layer[KEY_H] = KEY_6;
    numbers_layer[KEY_J] = KEY_7;
    numbers_layer[KEY_K] = KEY_8;
    numbers_layer[KEY_L] = KEY_9;
    numbers_layer[KEY_SEMICOLON] = KEY_0;

    numbers_layer[KEY_Q] = KEY_1 + KEY_SHIFT_MASK;
    numbers_layer[KEY_W] = KEY_2 + KEY_SHIFT_MASK;
    numbers_layer[KEY_E] = KEY_3 + KEY_SHIFT_MASK;
    numbers_layer[KEY_R] = KEY_4 + KEY_SHIFT_MASK;
    numbers_layer[KEY_T] = KEY_5 + KEY_SHIFT_MASK;
    numbers_layer[KEY_Y] = KEY_6 + KEY_SHIFT_MASK;
    numbers_layer[KEY_U] = KEY_7 + KEY_SHIFT_MASK;
    numbers_layer[KEY_I] = KEY_8 + KEY_SHIFT_MASK;
    numbers_layer[KEY_O] = KEY_9 + KEY_SHIFT_MASK;
    numbers_layer[KEY_P] = KEY_0 + KEY_SHIFT_MASK;
}

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

static int change_grab(long operation)
{
    int result = ioctl(input_descriptor, EVIOCGRAB, (void*)operation);
    if (result == -1)
    {
        warning("Couldn't %s input from device %s: %s\n", operation ? "grab" : "ungrab", input_dev, strerror(errno));
        return EXIT_FAILURE;
    }
    debug("Input %s successfully\n", operation ? "grabbed" : "ungrabbed");
    return EXIT_SUCCESS;
}

int grab_input()
{
    return change_grab(1);
}

int release_input()
{
    return change_grab(0);
}

int open_input()
{
    input_descriptor = open(input_dev, O_RDONLY);
    if (input_descriptor == -1)
    {
        warning("Couldn't open input device %s: %s\n", input_dev, strerror(errno));
        return EXIT_FAILURE;
    }

    return grab_input();
}

int open_output()
{
    output_descriptor = open(output_dev, O_WRONLY);
    if (output_descriptor == -1)
    {
        warning("Couldn't open output device %s: %s\n", output_dev, strerror(errno)); 
        return EXIT_FAILURE;
    }
    
    int ret = ioctl(output_descriptor, UI_SET_EVBIT, EV_KEY);
    if (ret == -1)
    {
        warning("Couldn't set ioctl: UI_SET_EVBIT EV_KEY\n"); 
        return EXIT_FAILURE;
    }
 
    ret = ioctl(output_descriptor, UI_SET_EVBIT, EV_SYN);
    if (ret == -1)
    {
        warning("Couldn't set ioctl: UI_SET_EVBIT EV_SYN\n"); 
        return EXIT_FAILURE;
    }

    int i;
    for (i = 1; i < 255; i++)
    {
      ret = ioctl(output_descriptor, UI_SET_KEYBIT, i);
      if (ret == -1)
      {
          warning("Couldn't set ioctl UI_SET_KEYBIT for key %d: %s\n", i, strerror(errno)); 
          return EXIT_FAILURE;
      }
    }

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));

    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-sample");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0xfedc;
    uidev.id.version = 1;

    ret = write(output_descriptor, &uidev, sizeof(uidev));
    if (ret == -1)
    {
        warning("Couldn't write uinput device descriptor: %s\n", strerror(errno)); 
        return EXIT_FAILURE;
    }

    ret = ioctl(output_descriptor, UI_DEV_CREATE);
    if (ret == -1)
    {
        warning("Couldn't create uinput device: %s\n", strerror(errno)); 
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void generate_sync_event();

void generate_key_event(int code, int type)
{
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));

    ev.type = EV_KEY;
    ev.code = code;
    ev.value = type;

    int ret = write(output_descriptor, &ev, sizeof(ev));
    if (ret == -1)
    {
      warning("Couldn't generate key event %d for %d: %s\n", type, code, strerror(errno)); 
    }
}

void generate_key_press(int code)
{
    if(pressed_keys_count == MAX_PRESSED_KEYS)
    {
        warning("Cannot handle more pressed keys - ignoring %d\n", code);
        return;
    }
    pressed_keys[pressed_keys_count++] = code;

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

void generate_key_release(int code)
{
    if(pressed_keys_count == 0)
    {
        warning("This is strange - received key release, but no pressed key is currently registered\n");
        return;
    }

    if(code == pressed_keys[pressed_keys_count])
    {
        pressed_keys_count--;
    }
    else
    {
        int i;
        for(i = 0; i < pressed_keys_count; i++)
        {
            if(pressed_keys[i] == code)
            {
                break;
            }
        }

        if(i == pressed_keys_count)
        {
            warning("This is strange - couldn't find this key in registered list: %d\n", code);
            return;
        }

        pressed_keys[i] = pressed_keys[pressed_keys_count - 1];
        pressed_keys_count--;
    }

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

void generate_sync_event()
{ 
  debug("Generating sync event\n");

  struct input_event ev;
  memset(&ev, 0, sizeof(ev));

  ev.type = EV_SYN;
  ev.code = 0;
  ev.value = 0;

  int ret = write(output_descriptor, &ev, sizeof(ev));
  if (ret == -1)
  {
    warning("Couldn't generate SYN event: %s\n", strerror(errno)); 
  }
}

int translate_code(int code)
{
    chord_generated = TRUE;

    if(mode == MODE_PASSTHROUGH)
    {
        return code;
    }

    if(code > MAX_KEY_CODE)
    {
        return code;
    }

    int result;

    if(space_pressed == TRUE)
    {
        result = numbers_layer[code];
    }
    else
    {
        result = default_layer[code];
    }

    if(capslock_pressed)
    {
        result += KEY_CTRL_MASK;
    }
    return result;
}

int handle_key_press(int code)
{
  debug("Key Pressed: %d, current state is: %d\n", code, state);

  if(code == KEY_RIGHTMETA)
  {
      info("Detected exit key (RIGHT META) press, exiting.\n");
      return EXIT_FAILURE;
  }

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

  int translated_code = translate_code(code);
  if(translated_code == NO_MAPPING)
  {
      debug("Ignoring key press\n");
      return EXIT_SUCCESS;
  }

  generate_key_press(translated_code);
  return EXIT_SUCCESS;
}

void release_all_pressed_keys()
{
    for(int i = pressed_keys_count - 1; i >= 0; i--)
    {
      generate_key_release(pressed_keys[i]);
    }
}

void handle_key_release(int code)
{
  debug("Key Released: %d\n", code);

  if(code == KEY_SPACE)
  {
      if(chord_generated == FALSE)
      {
        space_pressed = FALSE;
        generate_key_press(KEY_SPACE);
        generate_key_release(KEY_SPACE);
      }
      else
      {
          // release all currently presed keys
          // before clearing the space_pressed flag
          release_all_pressed_keys();
          space_pressed = FALSE;
      }
      return;
  }

  if(code == KEY_CAPSLOCK)
  {
    if(chord_generated == FALSE)
    {
        capslock_pressed = FALSE;
        generate_key_press(KEY_ESC);
        generate_key_release(KEY_ESC);
    }
    else
    {
        release_all_pressed_keys();
        capslock_pressed = FALSE;
    }
  }

  int translated_code = translate_code(code);
  if(translated_code == NO_MAPPING)
  {
      debug("Ignoring key release\n");
      return;
  }

  generate_key_release(translated_code);
}

void loop()
{
  struct input_event ev;
  ssize_t n;
  while (1)
  {
    n = read(input_descriptor, &ev, sizeof(ev));
    if (n == (ssize_t)-1) 
    {
      if (errno == EINTR)
      {
        continue;
      }
      else
      {
        break;
      }
    } 
    else if (n != sizeof ev) 
    {
        errno = EIO;
        break;
    }

    noisy("Keyboard event: type %d, value %d, code %d\n", ev.type, ev.value, ev.code);
    if (ev.type == EV_KEY)
    {
      if (ev.value == 2)
      {
        // autorepeat
        // generate_chord();
      }
      else if (ev.value == 1)
      {
        if (handle_key_press(ev.code))
        {
          break;
        }
      }
      else if (ev.value == 0)
      {
        handle_key_release(ev.code);
      }
    }
  }
}

int main()
{
    info("KB mapper started in the PASSTHROUGH mode\n");
    // to avoid catching the enter key
    sleep(3);

    mode = MODE_PASSTHROUGH;

    init_default_layer();
    init_numbers_layer();

    current_mapping = mapping;

    open_input();
    open_output();
    loop();
    return 0;
}
