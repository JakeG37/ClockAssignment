/* LEDisplay.h -- header file for 7segment clock library file
 *
 * Darren Provine, 14 August 2009
 *
 * Copyright (C) Darren Provine, 2009-2023, All Rights Reserved
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <ncurses.h>
#include <term.h>
#include <stdlib.h>

typedef unsigned char digit;

typedef struct {
    // digits are the time: 12:34:56
    digit digit_data[6];

    // 24hr, am, pm, date
    digit extras;

    // sets status bar (useful for debugging)
    void (*set_status)(char *);
    
    // redraws display
    void (*repaint)(void);
} LED_memory;

LED_memory *get_display_location(void);

void start_display(void);
void end_display(void);

void set_title_bar(char *);
void set_status(char *);

void highlight_button(int, int);

void get_key(void);
typedef unsigned short int keybits;

int register_keyhandler( void(*f)(keybits) );

void set_key_text(int, char *);
