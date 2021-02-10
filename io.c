#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <ctype.h>

#include "log.h"

// const char* input_dev = "/dev/input/by-path/pci-0000:00:1a.0-usb-0:1.6:1.0-event-kbd";
const char* input_dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
const char* output_dev = "/dev/uinput";

int output_descriptor;
int input_descriptor;

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

int try_read_key_event(struct input_event* ev)
{
    ssize_t n;

    while(1)
    {
        n = read(input_descriptor, ev, sizeof(struct input_event));
        if(n == (ssize_t)-1) 
        {
            if(errno == EINTR)
            {
                continue;
            }

            return EXIT_FAILURE;
        } 
        else if(n != sizeof(struct input_event)) 
        {
            errno = EIO;
            return EXIT_FAILURE;
        }

        if(ev->type == EV_KEY)
        {
            return EXIT_SUCCESS;
        }
    }
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
      warning("Couldn't generate key event %d for %d: %s\n", type, code, strerror(errno)); 
    }
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
