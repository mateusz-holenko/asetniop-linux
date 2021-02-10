#ifndef log_h_INCLUDED
#define log_h_INCLUDED

#include <stdio.h>

// #define _AY(y) #y
// #define B(y) _A(y)
// #define S_LINE B(__LINE__)
#define S_LINE "0"

#define NOISY_LOG
#define DEBUG_LOG

#define warning(...) printf(__FILE__ ":" S_LINE " :: "  __VA_ARGS__)
#define info(...) printf(__FILE__ ":" S_LINE " :: " __VA_ARGS__)
#define error(...) printf(__FILE__ ":" S_LINE " :: " __VA_ARGS__)

#ifdef NOISY_LOG
    #define noisy(...) printf(__FILE__ ":" S_LINE " :: " __VA_ARGS__)
#else
    #define noisy(...)
#endif

#if defined(NOISY_LOG) || defined(DEBUG_LOG)
  #define debug(...) printf(__FILE__ ":" S_LINE " :: " __VA_ARGS__)
#else
  #define debug(...)
#endif

#endif // log_h_INCLUDED

