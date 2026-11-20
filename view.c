/* view.c -- view module for clock project (the V in MVC)
 *
 * Darren Provine, 17 July 2009
 *
 * Copyright (C) Darren Provine, 2009-2026, All Rights Reserved
 */

#include "clock.h"
#include "view.h"

/* see "view.h" for list of bits that set properties */

int view_props = 0x00; // default is 24-hour mode, plain text

// returns old properties so you can save them if needed
void set_view_properties(int viewbits)
{
    view_props = viewbits;
}

int get_view_properties()
{
    return view_props;
}

void do_test(struct tm *dateinfo)
{
    LED_memory *clockface = get_display_location();

    // test all LEDs somehow

    clockface->repaint();
}

#define MAX_TIMESTR 40 // big enough for any valid data

// make_timestring
// returns a string formatted from the "dateinfo" object.
char * make_timestring (struct tm *dateinfo)
{
    const char *timeformat;
    int led_mode = view_props & LED_MODE;

    if ( view_props & DATE_MODE ) {
        timeformat = led_mode ? "%m%d%y" : "%_m/%d/%Y dt";
    } else if ( view_props & AMPM_MODE ) {
        timeformat = led_mode ? "(no format)" : "%l:%M:%S %P";
    } else {
        timeformat = led_mode ? "%H%M%S 24" : "%H:%M:%S 24";
    }

    // make the timestring and return it
    static char timestring[MAX_TIMESTR];
    strftime(timestring, MAX_TIMESTR, timeformat, dateinfo);
    return timestring;
}

/* We get a pointer to a "struct tm" object, put it in a string, and
 * then send it to the screen.
 */
void show_led(struct tm *dateinfo)
{

    LED_memory *clockface = get_display_location();
    int i;
    digit  bitvalues = 0;

    if ( view_props & TEST_MODE ) {
        do_test(dateinfo);
        return;
    }

    char *timestring = make_timestring(dateinfo);
    // printf("%s", timestring);
    for (i = 0; i < 6; i++) {
        // TODO:
        // Putting the call in the switch is inefficient and
        // makes debugging harder.  We'll fix this in class.
        // *INDENT-OFF*
        switch ( timestring[i] ) {
            case ' ': bitvalues = 0x00; break;
            case '1': bitvalues = 0x30; break;
            case '2': bitvalues = 0x6d; break;
            case '3': bitvalues = 0x79; break;
            case '4': bitvalues = 0x04; break;
            case '5': bitvalues = 0x05; break;
            case '6': bitvalues = 0x06; break;
            case '7': bitvalues = 0x07; break;
            case '8': bitvalues = 0x08; break;
            case '9': bitvalues = 0x09; break;
            case '0': bitvalues = 0xff; break;
        }
        // *INDENT-ON*
        clockface->digit_data[i] = bitvalues;
    }

    // turn on 24HR indicator
    clockface->extras = 0xf8;
    
    clockface->repaint();
}


void show_text(struct tm *dateinfo)
{
    printf("\r%s ", make_timestring(dateinfo));
    fflush(stdout);
}

void show(struct tm *dateinfo)
{
    if ( view_props & LED_MODE ) {
        show_led(dateinfo);
    } else {
        show_text(dateinfo);
    }
}
