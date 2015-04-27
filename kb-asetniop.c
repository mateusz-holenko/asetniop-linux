#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <ctype.h>

#ifdef KEYS_EXTENSION
  #define _STRINGIFY(x) #x
  #define STRINGIFY(x) _STRINGIFY(\x)
  
  #define CODE_KEY_UP 0xFF
  #define CODE_KEY_DOWN 0xFE
  #define CODE_KEY_LEFT 0xFD
  #define CODE_KEY_RIGHT 0xFC
  #define CODE_KEY_HOME 0xFB
  #define CODE_KEY_END 0xFA
  #define CODE_KEY_PAGE_UP 0xF0
  #define CODE_KEY_PAGE_DOWN 0xF9
#endif

#define ASETNIOP_A (1)
#define ASETNIOP_S (1 << 1)
#define ASETNIOP_E (1 << 2)
#define ASETNIOP_T (1 << 3)

#define ASETNIOP_N (1 << 4)
#define ASETNIOP_I (1 << 5)
#define ASETNIOP_O (1 << 6)
#define ASETNIOP_P (1 << 7)

#define ASETNIOP_SHIFT (1 << 8)
#define ASETNIOP_SPACE (1 << 9)

const char* input_dev = "/dev/input/by-path/pci-0000:00:1a.0-usb-0:1.6:1.0-event-kbd";
// const char* input_dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
const char* output_dev = "/dev/uinput";

char** current_mapping;

char* mapping[2048];
char* symbols_mapping[2048];
char* output_mapping[256];

unsigned int state = 0;
unsigned int mask  = 0;

int output_descriptor;
int input_descriptor;

void init_symbols_mapping()
{
    symbols_mapping[ASETNIOP_A] = "1";
    symbols_mapping[ASETNIOP_S] = "2";
    symbols_mapping[ASETNIOP_E] = "3";
    symbols_mapping[ASETNIOP_T] = "4";
    symbols_mapping[ASETNIOP_SHIFT] = "5";
    symbols_mapping[ASETNIOP_SPACE] = "6";
    symbols_mapping[ASETNIOP_N] = "7";
    symbols_mapping[ASETNIOP_I] = "8";
    symbols_mapping[ASETNIOP_O] = "9";
    symbols_mapping[ASETNIOP_P] = "0";

    // mapping[ASETNIOP_A | ASETNIOP_S] = "`";
    symbols_mapping[ASETNIOP_A | ASETNIOP_E] = "`";
    symbols_mapping[ASETNIOP_A | ASETNIOP_T] = "{";
    // mapping[ASETNIOP_A | ASETNIOP_N] = "q";
    symbols_mapping[ASETNIOP_A | ASETNIOP_I] = "!";
    symbols_mapping[ASETNIOP_A | ASETNIOP_O] = "(";
    symbols_mapping[ASETNIOP_A | ASETNIOP_P] = "?";

    symbols_mapping[ASETNIOP_S | ASETNIOP_E] = "-";
    // mapping[ASETNIOP_S | ASETNIOP_T] = "c";
    // mapping[ASETNIOP_S | ASETNIOP_N] = "j"; // end
    symbols_mapping[ASETNIOP_S | ASETNIOP_I] = "=";
    symbols_mapping[ASETNIOP_S | ASETNIOP_O] = ".";
    symbols_mapping[ASETNIOP_S | ASETNIOP_P] = ")";

    // mapping[ASETNIOP_E | ASETNIOP_T] = "r"; // page_up
    // mapping[ASETNIOP_E | ASETNIOP_N] = "y"; // home
    symbols_mapping[ASETNIOP_E | ASETNIOP_I] = ",";
    symbols_mapping[ASETNIOP_E | ASETNIOP_O] = "-";
    symbols_mapping[ASETNIOP_E | ASETNIOP_P] = "'";

    // mapping[ASETNIOP_T | ASETNIOP_N] = "b"; // key left
    // mapping[ASETNIOP_T | ASETNIOP_I] = "v"; // key up ?!
    // mapping[ASETNIOP_T | ASETNIOP_O] = "g"; // key right ?!
    symbols_mapping[ASETNIOP_T | ASETNIOP_P] = "\x08"; // backspace

    // mapping[ASETNIOP_N | ASETNIOP_I] = "h"; // page down
    // mapping[ASETNIOP_N | ASETNIOP_O] = "u"; // function
    symbols_mapping[ASETNIOP_N | ASETNIOP_P] = "]";
    symbols_mapping[ASETNIOP_I | ASETNIOP_O] = "=";
    symbols_mapping[ASETNIOP_I | ASETNIOP_P] = "\\";
    symbols_mapping[ASETNIOP_O | ASETNIOP_P] = ";";

    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_A] = "!";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_S] = "@";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_E] = "#";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_T] = "$";
    // symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_SHIFT] = "%";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_SPACE] = "^";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_N] = "&";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_I] = "*";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_O] = "(";
    symbols_mapping[ASETNIOP_SHIFT | ASETNIOP_P] = ")";
}

void init_standard_mapping()
{
#ifdef KEYS_EXTENSION
    mapping[ASETNIOP_E | ASETNIOP_T | ASETNIOP_N] = STRINGIFY(CODE_KEY_LEFT);
    mapping[ASETNIOP_E | ASETNIOP_T | ASETNIOP_I] = STRINGIFY(CODE_KEY_DOWN);
    mapping[ASETNIOP_E | ASETNIOP_T | ASETNIOP_O] = STRINGIFY(CODE_KEY_UP);
    mapping[ASETNIOP_E | ASETNIOP_T | ASETNIOP_P] = STRINGIFY(CODE_KEY_RIGHT);

    mapping[ASETNIOP_S | ASETNIOP_T | ASETNIOP_N] = STRINGIFY(CODE_KEY_HOME);
    mapping[ASETNIOP_S | ASETNIOP_T | ASETNIOP_I] = STRINGIFY(CODE_KEY_PAGE_DOWN);
    mapping[ASETNIOP_S | ASETNIOP_T | ASETNIOP_O] = STRINGIFY(CODE_KEY_PAGE_UP);
    mapping[ASETNIOP_S | ASETNIOP_T | ASETNIOP_P] = STRINGIFY(CODE_KEY_END);
#endif 

    // without shift
    mapping[ASETNIOP_A] = "a";
    mapping[ASETNIOP_S] = "s";
    mapping[ASETNIOP_E] = "e";
    mapping[ASETNIOP_T] = "t";
    mapping[ASETNIOP_N] = "n";
    mapping[ASETNIOP_I] = "i";
    mapping[ASETNIOP_O] = "o";
    mapping[ASETNIOP_P] = "p";
    mapping[ASETNIOP_SPACE] = " ";

    mapping[ASETNIOP_A | ASETNIOP_S] = "w";
    mapping[ASETNIOP_A | ASETNIOP_E] = "x";
    mapping[ASETNIOP_A | ASETNIOP_T] = "f";
    mapping[ASETNIOP_A | ASETNIOP_N] = "q";
    mapping[ASETNIOP_A | ASETNIOP_I] = "!";
    mapping[ASETNIOP_A | ASETNIOP_O] = "(";
    mapping[ASETNIOP_A | ASETNIOP_P] = "?";

    mapping[ASETNIOP_S | ASETNIOP_E] = "d";
    mapping[ASETNIOP_S | ASETNIOP_T] = "c";
    mapping[ASETNIOP_S | ASETNIOP_N] = "j";
    mapping[ASETNIOP_S | ASETNIOP_I] = "z";
    mapping[ASETNIOP_S | ASETNIOP_O] = ".";
    mapping[ASETNIOP_S | ASETNIOP_P] = ")";

    mapping[ASETNIOP_E | ASETNIOP_T] = "r";
    mapping[ASETNIOP_E | ASETNIOP_N] = "y";
    mapping[ASETNIOP_E | ASETNIOP_I] = ",";
    mapping[ASETNIOP_E | ASETNIOP_O] = "-";
    mapping[ASETNIOP_E | ASETNIOP_P] = "'";

    mapping[ASETNIOP_T | ASETNIOP_N] = "b";
    mapping[ASETNIOP_T | ASETNIOP_I] = "v";
    mapping[ASETNIOP_T | ASETNIOP_O] = "g";
    mapping[ASETNIOP_T | ASETNIOP_P] = "\x08"; // backspace

    mapping[ASETNIOP_N | ASETNIOP_I] = "h";
    mapping[ASETNIOP_N | ASETNIOP_O] = "u";
    mapping[ASETNIOP_N | ASETNIOP_P] = "m";

    mapping[ASETNIOP_I | ASETNIOP_O] = "l";
    mapping[ASETNIOP_I | ASETNIOP_P] = "k";

    mapping[ASETNIOP_O | ASETNIOP_P] = ";";

    // with shift
    mapping[ASETNIOP_SHIFT | ASETNIOP_A] = "A";
    mapping[ASETNIOP_SHIFT | ASETNIOP_S] = "S";
    mapping[ASETNIOP_SHIFT | ASETNIOP_E] = "E";
    mapping[ASETNIOP_SHIFT | ASETNIOP_T] = "T";
    mapping[ASETNIOP_SHIFT | ASETNIOP_N] = "N";
    mapping[ASETNIOP_SHIFT | ASETNIOP_I] = "I";
    mapping[ASETNIOP_SHIFT | ASETNIOP_O] = "O";
    mapping[ASETNIOP_SHIFT | ASETNIOP_P] = "P";
    mapping[ASETNIOP_SHIFT | ASETNIOP_SPACE] = "\n";

    mapping[ASETNIOP_SHIFT | ASETNIOP_A | ASETNIOP_S] = "W";
    mapping[ASETNIOP_SHIFT | ASETNIOP_A | ASETNIOP_E] = "X";
    mapping[ASETNIOP_SHIFT | ASETNIOP_A | ASETNIOP_T] = "F";
    mapping[ASETNIOP_SHIFT | ASETNIOP_A | ASETNIOP_N] = "Q";
    // mapping[ASETNIOP_SHIFT | ASETNIOP_A | ASETNIOP_I] = "!";
    // mapping[ASETNIOP_SHIFT | ASETNIOP_A | ASETNIOP_O] = "(";
    mapping[ASETNIOP_SHIFT | ASETNIOP_A | ASETNIOP_P] = "/";

    mapping[ASETNIOP_SHIFT | ASETNIOP_S | ASETNIOP_E] = "D";
    mapping[ASETNIOP_SHIFT | ASETNIOP_S | ASETNIOP_T] = "C";
    mapping[ASETNIOP_SHIFT | ASETNIOP_S | ASETNIOP_N] = "J";
    mapping[ASETNIOP_SHIFT | ASETNIOP_S | ASETNIOP_I] = "Z";
    mapping[ASETNIOP_SHIFT | ASETNIOP_S | ASETNIOP_O] = ">";
    // mapping[ASETNIOP_SHIFT | ASETNIOP_S | ASETNIOP_P] = ")";

    mapping[ASETNIOP_SHIFT | ASETNIOP_E | ASETNIOP_T] = "R";
    mapping[ASETNIOP_SHIFT | ASETNIOP_E | ASETNIOP_N] = "Y";
    mapping[ASETNIOP_SHIFT | ASETNIOP_E | ASETNIOP_I] = "<";
    mapping[ASETNIOP_SHIFT | ASETNIOP_E | ASETNIOP_O] = "_";
    mapping[ASETNIOP_SHIFT | ASETNIOP_E | ASETNIOP_P] = "\"";

    mapping[ASETNIOP_SHIFT | ASETNIOP_T | ASETNIOP_N] = "B";
    mapping[ASETNIOP_SHIFT | ASETNIOP_T | ASETNIOP_I] = "V";
    mapping[ASETNIOP_SHIFT | ASETNIOP_T | ASETNIOP_O] = "G";
    mapping[ASETNIOP_SHIFT | ASETNIOP_T | ASETNIOP_P] = "\x08"; // backspace

    mapping[ASETNIOP_SHIFT | ASETNIOP_N | ASETNIOP_I] = "H";
    mapping[ASETNIOP_SHIFT | ASETNIOP_N | ASETNIOP_O] = "U";
    mapping[ASETNIOP_SHIFT | ASETNIOP_N | ASETNIOP_P] = "M";

    mapping[ASETNIOP_SHIFT | ASETNIOP_I | ASETNIOP_O] = "L";
    mapping[ASETNIOP_SHIFT | ASETNIOP_I | ASETNIOP_P] = "K";

    mapping[ASETNIOP_SHIFT | ASETNIOP_O | ASETNIOP_P] = ":";
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

void init_output_mapping()
{
#ifdef KEYS_EXTENSION
  output_mapping[CODE_KEY_UP] = create_output_mapping(KEY_UP);
  output_mapping[CODE_KEY_DOWN] = create_output_mapping(KEY_DOWN);
  output_mapping[CODE_KEY_LEFT] = create_output_mapping(KEY_LEFT);
  output_mapping[CODE_KEY_RIGHT] = create_output_mapping(KEY_RIGHT);
  output_mapping[CODE_KEY_PAGE_UP] = create_output_mapping(KEY_PAGEUP);
  output_mapping[CODE_KEY_PAGE_DOWN] = create_output_mapping(KEY_PAGEDOWN);
  output_mapping[CODE_KEY_HOME] = create_output_mapping(KEY_HOME);
  output_mapping[CODE_KEY_END] = create_output_mapping(KEY_END);
#endif

  output_mapping['a'] = create_output_mapping(KEY_A);
  output_mapping['b'] = create_output_mapping(KEY_B);
  output_mapping['c'] = create_output_mapping(KEY_C);
  output_mapping['d'] = create_output_mapping(KEY_D);
  output_mapping['e'] = create_output_mapping(KEY_E);
  output_mapping['f'] = create_output_mapping(KEY_F);
  output_mapping['g'] = create_output_mapping(KEY_G);
  output_mapping['h'] = create_output_mapping(KEY_H);
  output_mapping['i'] = create_output_mapping(KEY_I);
  output_mapping['j'] = create_output_mapping(KEY_J);
  output_mapping['k'] = create_output_mapping(KEY_K);
  output_mapping['l'] = create_output_mapping(KEY_L);
  output_mapping['m'] = create_output_mapping(KEY_M);
  output_mapping['n'] = create_output_mapping(KEY_N);
  output_mapping['o'] = create_output_mapping(KEY_O);
  output_mapping['p'] = create_output_mapping(KEY_P);
  output_mapping['q'] = create_output_mapping(KEY_Q);
  output_mapping['r'] = create_output_mapping(KEY_R);
  output_mapping['s'] = create_output_mapping(KEY_S);
  output_mapping['t'] = create_output_mapping(KEY_T);
  output_mapping['u'] = create_output_mapping(KEY_U);
  output_mapping['w'] = create_output_mapping(KEY_W);
  output_mapping['v'] = create_output_mapping(KEY_V);
  output_mapping['x'] = create_output_mapping(KEY_X);
  output_mapping['y'] = create_output_mapping(KEY_Y);
  output_mapping['z'] = create_output_mapping(KEY_Z);

  char i;
  for (i = 'a'; i <= 'z'; i++)
  {
    output_mapping[toupper(i)] = create_output_mapping2(KEY_LEFTSHIFT, output_mapping[i][1]);
  }

  output_mapping[';'] = create_output_mapping(KEY_SEMICOLON);
  output_mapping['.'] = create_output_mapping(KEY_DOT);
  output_mapping[','] = create_output_mapping(KEY_COMMA);
  output_mapping['-'] = create_output_mapping(KEY_MINUS);
  output_mapping['/'] = create_output_mapping(KEY_SLASH);
  output_mapping['\''] = create_output_mapping(KEY_APOSTROPHE);
  output_mapping['\\'] = create_output_mapping(KEY_BACKSLASH);

  output_mapping['|']  = create_output_mapping2(KEY_LEFTSHIFT, KEY_BACKSLASH);
  output_mapping['\"'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_APOSTROPHE);
  output_mapping['?']  = create_output_mapping2(KEY_LEFTSHIFT, KEY_SLASH);
  output_mapping['!']  = create_output_mapping2(KEY_LEFTSHIFT, KEY_1);
  output_mapping['(']  = create_output_mapping2(KEY_LEFTSHIFT, KEY_9);
  output_mapping[')']  = create_output_mapping2(KEY_LEFTSHIFT, KEY_0);
  output_mapping['<']  = create_output_mapping2(KEY_LEFTSHIFT, KEY_COMMA);
  output_mapping['>']  = create_output_mapping2(KEY_LEFTSHIFT, KEY_DOT);

  output_mapping['\x08'] = create_output_mapping(KEY_BACKSPACE);
  output_mapping[' '] = create_output_mapping(KEY_SPACE);
  output_mapping['\n'] = create_output_mapping(KEY_ENTER);

  output_mapping['1'] = create_output_mapping(KEY_1);
  output_mapping['2'] = create_output_mapping(KEY_2);
  output_mapping['3'] = create_output_mapping(KEY_3);
  output_mapping['4'] = create_output_mapping(KEY_4);
  output_mapping['5'] = create_output_mapping(KEY_5);
  output_mapping['6'] = create_output_mapping(KEY_6);
  output_mapping['7'] = create_output_mapping(KEY_7);
  output_mapping['8'] = create_output_mapping(KEY_8);
  output_mapping['9'] = create_output_mapping(KEY_9);
  output_mapping['0'] = create_output_mapping(KEY_0);

  output_mapping['`'] = create_output_mapping(KEY_GRAVE);
  output_mapping['~'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_GRAVE);

  output_mapping['['] = create_output_mapping(KEY_LEFTBRACE);
  output_mapping[']'] = create_output_mapping(KEY_RIGHTBRACE);

  output_mapping['{'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_LEFTBRACE);
  output_mapping['}'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_RIGHTBRACE);
  output_mapping['='] = create_output_mapping(KEY_EQUAL);
  output_mapping['+'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_EQUAL);
  output_mapping['_'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_MINUS);
  output_mapping[':'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_SEMICOLON);

  output_mapping['@'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_2);
  output_mapping['#'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_3);
  output_mapping['$'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_4);
  output_mapping['%'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_5);
  output_mapping['^'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_6);
  output_mapping['&'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_7);
  output_mapping['*'] = create_output_mapping2(KEY_LEFTSHIFT, KEY_8);
}

int open_input()
{
    input_descriptor = open(input_dev, O_RDONLY);
    if (input_descriptor == -1)
    {
        printf("Couldn't open input device %s: %s\n", input_dev, strerror(errno));
        return EXIT_FAILURE;
    }

    int result = ioctl(input_descriptor, EVIOCGRAB, (void*)1);
    if (result == -1)
    {
        printf("Couldn't grab input from device %s: %s\n", input_dev, strerror(errno));
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int open_output()
{
    output_descriptor = open(output_dev, O_WRONLY);
    if (output_descriptor == -1)
    {
        printf("Couldn't open output device %s: %s\n", output_dev, strerror(errno)); 
        return EXIT_FAILURE;
    }
    
    int ret = ioctl(output_descriptor, UI_SET_EVBIT, EV_KEY);
    if (ret == -1)
    {
        printf("Couldn't set ioctl: UI_SET_EVBIT EV_KEY\n"); 
        return EXIT_FAILURE;
    }
 
    ret = ioctl(output_descriptor, UI_SET_EVBIT, EV_SYN);
    if (ret == -1)
    {
        printf("Couldn't set ioctl: UI_SET_EVBIT EV_SYN\n"); 
        return EXIT_FAILURE;
    }

    int i;
    for (i = 1; i < 255; i++)
    {
      ret = ioctl(output_descriptor, UI_SET_KEYBIT, i);
      if (ret == -1)
      {
          printf("Couldn't set ioctl UI_SET_KEYBIT for key %d: %s\n", i, strerror(errno)); 
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
        printf("Couldn't write uinput device descriptor: %s\n", strerror(errno)); 
        return EXIT_FAILURE;
    }

    ret = ioctl(output_descriptor, UI_DEV_CREATE);
    if (ret == -1)
    {
        printf("Couldn't create uinput device: %s\n", strerror(errno)); 
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

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
      printf("Couldn't generate key event %d for %d: %s\n", type, code, strerror(errno)); 
    }
}

void generate_key_press(int code)
{
  printf("Generating key press event for key: %d\n", code);
  generate_key_event(code, 1);
}

void generate_key_release(int code)
{
  printf("Generating key release event for key: %d\n", code);
  generate_key_event(code, 0);
}

void generate_sync_event()
{ 
  printf("Generating sync event\n");

  struct input_event ev;
  memset(&ev, 0, sizeof(ev));

  ev.type = EV_SYN;
  ev.code = 0;
  ev.value = 0;

  int ret = write(output_descriptor, &ev, sizeof(ev));
  if (ret == -1)
  {
    printf("Couldn't generate SYN event: %s\n", strerror(errno)); 
  }
}

int key_to_asetniop(int code)
{
  switch(code)
  {
    case KEY_A:
    case KEY_Q:
      return ASETNIOP_A;
    case KEY_S:
    case KEY_W:
      return ASETNIOP_S;
    case KEY_D:
    case KEY_E:
      return ASETNIOP_E;
    case KEY_R:
    case KEY_F:
      return ASETNIOP_T;
    case KEY_J:
    case KEY_U:
      return ASETNIOP_N;
    case KEY_K:
    case KEY_I:
      return ASETNIOP_I;
    case KEY_L:
    case KEY_O:
      return ASETNIOP_O;
    case KEY_P:
    case KEY_SEMICOLON:
      return ASETNIOP_P;
    case KEY_SPACE:
      return ASETNIOP_SPACE;
    case KEY_Z:
    case KEY_X:
    case KEY_C:
    case KEY_V:
    case KEY_B:
    case KEY_N:
    case KEY_M:
    case KEY_COMMA:
    case KEY_DOT:
    case KEY_SLASH:
    case KEY_LEFTALT:
      return ASETNIOP_SHIFT;
  }

  return -1;
}

int handle_key_press(int code)
{
  printf("Key Pressed: %d, current state is: %d\n", code, state);
  int key = key_to_asetniop(code);
  if (key == -1)
  {
    return 1;
  }

  state |= key;
  if (state == (ASETNIOP_A | ASETNIOP_T | ASETNIOP_N | ASETNIOP_P))
  {
      printf("Detected 'devils horn'\n");
      if (current_mapping == mapping)
      {
          printf("Layout changed to: symbols\n");
          current_mapping = symbols_mapping;
      }
      else
      {
          printf("Layout changed to: letters\n");
          current_mapping = mapping;
      }
      mask = 0;

      return 0;
  }

  mask = state;
  printf("State changed to: %d\n", state);
  return 0;
}

void generate_chord()
{
  char* m = current_mapping[state & mask];
  if (m == 0)
  {
      printf("Unknown mapping, ignoring...\n");
      return;
  }

  printf("Mapping for currents state %d (mask %d) is '%s'\n", state, mask, m);
  while (*m)
  {
    char key_count = output_mapping[*m][0];
    printf("This would generate %d key presses\n", key_count);
    int i;
    for (i = 1; i <= key_count; i++)
    {
      generate_key_press(output_mapping[*m][i]);
    }
    for (i = key_count; i > 0; i--)
    {
      generate_key_release(output_mapping[*m][i]);
    }
    generate_sync_event();

    m++;
  }
}

void handle_key_release(int code)
{
  printf("Key Released: %d\n", code);
  if ((state & mask) != 0)
  {
    generate_chord();
  }

  int key = key_to_asetniop(code);
  if (key != -1)
  {
      state &= (~key);
  }

  printf("State after releasing key is: %d\n", state);
  mask = 0;
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

    // printf("Keyboard event: type %d, value %d, code %d\n", ev.type, ev.value, ev.code);
    if (ev.type == EV_KEY)
    {
      if (ev.value == 2)
      {
        // autorepeat
        generate_chord();
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
  init_standard_mapping();
  init_symbols_mapping();
  init_output_mapping();

  current_mapping = mapping;

  open_input();
  open_output();
  loop();
  return 0;
}
