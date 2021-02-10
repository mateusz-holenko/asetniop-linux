#include <stdlib.h>
#include "sorted_array.h"

int insert_sorted(press_event_t* array, int current_length, press_event_t value)
{
    int i;
    for(i = 0; i < current_length; i++)
    {
        if(array[i].code > value.code)
        {
            break;
        }
    }
    int position_to_insert = i;
    for(int j = current_length + 1; j > position_to_insert; j--)
    {
        array[j] = array[j - 1];
    }
    array[position_to_insert] = value;
}

int remove_sorted(press_event_t* array, int current_length, press_event_t value, press_event_t* removed_element)
{
    int i;
    for(i = 0; i < current_length; i++)
    {
        if(array[i].code == value.code)
        {
            break;
        }
    }

    if(i == current_length)
    {
        return EXIT_FAILURE;
    }

    *removed_element = array[i];

    int j;
    for(j = i; j < current_length - 1; j++)
    {
        array[j] = array[j + 1];
    }
    return EXIT_SUCCESS;
}
