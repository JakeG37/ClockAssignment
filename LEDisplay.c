/*
 * Copyright (C) Darren Provine, 2009-2025, All Rights Reserved
 */

#include "LEDisplay.h"

// these are for the "access()" call to ensure we're on Elvis.
#include <fcntl.h>           /* Definition of AT_* constants */
#include <unistd.h>


LED_memory LEDbits;

char title_bar[81];

const int INDICATOR_24   = 0x08;
const int INDICATOR_AM   = 0x04;
const int INDICATOR_PM   = 0x02;
const int INDICATOR_DATE = 0x01;

const int COLON_UL = 0x10;
const int COLON_UR = 0x20;
const int COLON_LL = 0x40;
const int COLON_LR = 0x80;

// The faces of the keys. 
// First row of keys is fixed:
char KeyStr[5][7] = { "24 Hr", "AM/PM", " Date", " Test", " Off" };

// Second row of keys can be changed.
char RowTwoKeys[5][7] = {"", "", "", "", ""};


// Layout of LEDs - changes every so often to deter cheating
// This layout is Spring 2026
const int TOP_HORIZ = 0x01 ;  // top horizontal bar
const int UL_VERT   = 0x02 ;  // upper-left vertical bar
const int LL_VERT   = 0x04 ;  // lower-left vertical bar
const int BOT_HORIZ = 0x08 ;  // bottom horizontal bar

const int LR_VERT   = 0x10 ;  // lower-right vertical bar
const int UR_VERT   = 0x20 ;  // upper-right vertical bar
const int MID_HORIZ = 0x40 ;  // middle horizontal bar
const int DECIMAL   = 0x80 ;  // decimal point


LED_memory *get_display_location()
{
     return &LEDbits;
}


void start_display(void)
{
    int  i;
    void init_screen(void);
    void redraw(void);
    void setstatus(char *);    
    char *showbits = getenv("SHOWCLOCKLEDBITS");

    if ( ! showbits || strcmp(showbits, "YES") != 0 ) {
	init_screen();
    }

    for (i = 0; i < 5; i++)
        LEDbits.digit_data[i] = 0;
    LEDbits.extras = 0;
    LEDbits.repaint = &redraw;
    LEDbits.set_status = &setstatus;
    setstatus("");
    redraw();
}

int old_cursor_setting;

void end_display(void)
{
    char *showbits = getenv("SHOWCLOCKLEDBITS");

    // only show bits, don't do fullscreen mode
    if ( showbits && strcmp(showbits, "YES") == 0 ) {
        return;
    }
    
    nocbreak();
    echo();
    curs_set(old_cursor_setting);    
    endwin();
}

void init_screen(void)
{
    int      status;
    char    *term;
    int     winrows, wincols;

    char  *username = getlogin();

    // need a version that uses an environment variable,
    // otherwise they'll all run when I run them.
    // if CLOCKRUN == YES then it'll run for me no matter what.
    // otherwise, it will check /home/kilroy/.elvis/clockusers/YYYYMM
    // for the current user's username.
    // If the file doesn't exist, they get the message that it only
    // runs on Elvis.
    // If the file exists but their name isn't in it, they get a
    // message that the device driver is outdated and needs to be
    // upgraded.
    // Then update the semester being looked for each semester,
    // and create the file with all the students in LabTech.
    if ( strcmp(username,"kilroy") != 0 &&
	 access ("/home/kilroy/.elvis", F_OK)  == -1 )  {
        fprintf(stderr, "This clock was configured to run on Elvis.\n");
        exit(1);
    }

    int     thisversion = 202604; // April 2026
    int     version;
    int     okay = 0;   
    FILE   *version_file;
    if ( ( version_file = fopen("/home/kilroy/.elvis", "r") ) == NULL &&
	 ( version_file = fopen("/Users/kilroy/.elvis", "r") ) == NULL  ) {
        fprintf(stderr, "This clock was configured to run on Elvis.\n");
        exit(1);	
    }
    while ( ! feof(version_file) ) {
	int howmany = fscanf(version_file, "%d", &version);
	if (howmany == 0) {
	    break;
	}
	if (version == thisversion) {
	    okay = 1;
	    break;
	}
    }
    if ( ! okay ) {
        fprintf(stderr, "This clock is outdated.  Please upgrade.\n");
        exit(1);		
    }
    // BETTER IDEA:
    // For each version, put the usernames of students allowed to use it.
    // Update that every semester.
    if ( ( term = getenv("TERM") ) == NULL ) {
        printf("You have not set your TERM variable.");
        exit(1);
    }

    setupterm(term, 1, &status);
    if ( status != 1 ) {
        printf("I can't find information for your %s terminal.", term);
        exit(1);
    }

    initscr();

    old_cursor_setting = curs_set(0);

    // 2017-05-24 14:22:47 EDT (Wednesday)
    // https://www.linux.com/forums/command-line/ncurses-startcolor-screen-color
    if(has_colors() == FALSE) {    // Check the terminal has colour capability
        endwin();                // Close Window and leave ncurses
        fprintf(stderr,
                "Your terminal does not support colour.   ");
        exit (1);
    }
    
    start_color(); // DFP 2017-05-24

    //        #  FOREGROUND   BACKGROUND
    init_pair(1, COLOR_BLACK,   COLOR_RED); // black on red: print spaces
    init_pair(2, COLOR_RED,   COLOR_BLACK); // title bar and status LEDs
    init_pair(3, COLOR_YELLOW, COLOR_BLUE); // buttons
    init_pair(4, COLOR_WHITE,  COLOR_BLUE); // hightlight button
    init_pair(5, COLOR_WHITE, COLOR_BLACK); // status bar
    
    attron(COLOR_PAIR(1));
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    if (mousemask(ALL_MOUSE_EVENTS, 0) == 0) {
        fprintf(stderr, "No mouse\n");
        exit(1);
    }

    getmaxyx(stdscr, winrows, wincols);
    if (winrows < 24 || wincols < 80) {
        clear();
        refresh();
        endwin();
        fprintf(stderr,
                "Your window is %dx%d; must be at least 80x24.\n",
                wincols, winrows);
        exit (1);
    }
}

void set_title_bar(char *title_bar_text)
{
     memcpy(title_bar, title_bar_text, 81);
}

/* Draw a box with some text in it; used for keys.
 * Width is "total width", including both sides.
 * Height is "total height", including top and bottom lines.
 * Text is not truncated!  (So make sure it'll fit.)
 */
void dobox(int top, int left, int height, int width, // location
	   char *text,
	   int highlight) // make it white?
{
    if (highlight) {
	attron(COLOR_PAIR(4)); // white lines
    } else {
	attron(COLOR_PAIR(3)); // yellow lines
    }
    // corners
    mvaddch(top, left, ACS_ULCORNER);                  // top left
    mvaddch(top, left+width-1, ACS_URCORNER);          // top right
    mvaddch(top+height-1, left, ACS_LLCORNER);         // bottom left
    mvaddch(top+height-1, left+width-1, ACS_LRCORNER); // bottom right
    
    // top and bottom lines
    for (int i = 1; i <= width-2; i++) {
        mvaddch(top, left + i, ACS_HLINE);
        mvaddch(top + height - 1, left + i, ACS_HLINE);
    }

    // left and right sides
    for (int i = 1; i <= height-2; i++) {
        mvaddch(top+i, left, ACS_VLINE);
        mvaddch(top+i, left+width-1, ACS_VLINE);
    }

    // text
    move( top+height / 2, left + 2 );
    printw("%s", text);   
}


void set_key_text(int key, char *text)
{
    strncpy(RowTwoKeys[key], text, 6);
    RowTwoKeys[key][6] = '\0';
}

// highlight whichever button was clicked
int high_col;
int high_row;
void highlight_button(int col, int row) {
    high_col = col;
    high_row = row;
}

char statusbar[81] = "";
void setstatus(char *text)
{
    strncpy(statusbar, text, 80);
}

void display(void)
{
    int  digit;
    int  x, y;

    int  KeyOffsetX, KeyOffsetY;
    int  Key;

    char *showbits = getenv("SHOWCLOCKLEDBITS");

    // only show bits, don't do fullscreen mode
    if ( showbits && strcmp(showbits, "YES") == 0 ) {
        for (digit = 0; digit <=5 ; digit++) {
            printf ("%d : 0x%02x - ", digit, LEDbits.digit_data[digit]);
        }
        printf (" x : 0x%02x \n", LEDbits.extras);
        return;
    }    
    
    erase();

    // print 12 lines of 80 columns all in black (for LEDs)
    attron(COLOR_PAIR(2));
    for (int i = 0; i < 12; i++) {
        move(i,0);
        printw("                                        "          
               "                                        ");
    }

    // print 12 lines of 80 columns all white (for buttons)
    attron(COLOR_PAIR(3));
    for (int i = 12; i < 23; i++) {
        move(i,0);
        printw("                                        "          
               "                                        ");
    }
    // print frame on left/right
    for (int i = 0; i < 22; i++) {
        move(i,0); printw("   ");
        move(i,1); addch(ACS_VLINE);
        move(i,77); printw("   ");
        move(i,78); addch(ACS_VLINE);        
    }

    // print frame along top (overwritten by title bar)
    attron(COLOR_PAIR(3));
    move(0,0);
    printw("                                        "          
           "                                        ");
    move(0,1);
    for (int i=0; i < 78; i++) {
        if (title_bar[i] == '-') {
            move(0,1+i);
            addch(ACS_HLINE);
        } else if (title_bar[i] == ' ') {
            printw(" ");
        } else {
            printw("%c", title_bar[i]);
        }
    }

    // print frame along center (between areas)
    move(12,1);
    for (int i=0; i < 78; i++) {
        addch(ACS_HLINE);
    }

    // print frame along bottom (below buttons)
    move(22,1);
    for (int i=0; i < 78; i++) {
        move(22,1+i);
        addch(ACS_HLINE);
    }
    
    // add corners
    move(0,1);   addch(ACS_ULCORNER);
    move(0,78);  addch(ACS_URCORNER);    
    move(12,1);  addch(ACS_LTEE);
    move(12,78); addch(ACS_RTEE);
    move(22,1);  addch(ACS_LLCORNER);
    move(22,78); addch(ACS_LRCORNER);
    

    // set to black text on red background; makes spaces red
    attron(COLOR_PAIR(1));
    // draw the colons
    if ( LEDbits.extras & COLON_UL ) {
        move(5, 27);
        printw("  ");
    }
    if ( LEDbits.extras & COLON_LL ) {
        move(7, 27);
        printw("  ");
    }

    if ( LEDbits.extras & COLON_UR ) {
        move(5, 48);
        printw("  ");
    }
    if ( LEDbits.extras & COLON_LR ) {
        move(7, 48);
        printw("  ");
    }

    /* This draws the 6 digits.
     */
    for (digit = 0; digit <=5 ; digit++) {
        x = digit * 9 + 10;
        if (digit > 1) x += 3; // skip first set of colons
        if (digit > 3) x += 3; // skip second set of colons
        y = 3;

        // top segment
        if (LEDbits.digit_data[digit] & TOP_HORIZ) {
            move(y, x);
            printw("      ");
        }

        // upper segment on left
        if (LEDbits.digit_data[digit] & UL_VERT) {
            move(y, x);
            // draw line between two segments
            printw(" ");
            move(y+1, x);
            printw(" ");
            move(y+2, x);
            printw(" ");
            move(y+3, x);
            printw(" ");
        }

        // upper segment on right
        if (LEDbits.digit_data[digit] & UR_VERT) {
            move(y, x+5);
            // draw line between two segments
            printw(" ");
            move(y+1, x+5);
            printw(" ");
            move(y+2, x+5);
            printw(" ");
            move(y+3, x+5);
            printw(" ");
        }

        // center segment
        if (LEDbits.digit_data[digit] & MID_HORIZ) {
            move(y+3, x);
            printw("      ");
        }        
        
        // lower segment on left
        if (LEDbits.digit_data[digit] & LL_VERT) {
            move(y+3, x);
            printw(" ");
            move(y+4, x);
            printw(" ");
            move(y+5, x);
            printw(" ");
            move(y+6, x);
            printw(" ");            
        }

        // lower segment on right
        if (LEDbits.digit_data[digit] & LR_VERT) {
            move(y+3, x+5);
            printw(" ");
            move(y+4, x+5);
            printw(" ");
            move(y+5, x+5);
            printw(" ");
            move(y+6, x+5);
            printw(" ");            
        }


        // bottom segment
        if (LEDbits.digit_data[digit] & BOT_HORIZ) {
            move(y+6, x);
            // draw line between two segments
            printw(" ");
            printw("    ");
            // draw line between two segments
            printw(" ");
        }

        // decimal point
        if (LEDbits.digit_data[digit] & DECIMAL) {
            move(y+6, x+7);
            printw(" ");
        }
    }

    /* This draws the keys.
     */
    KeyOffsetX = 5;

    // top row of fixed keys
    KeyOffsetY = 13;
    for (Key = 0; Key < 5; Key++) {
	int highlight = 0;
	attrset(A_NORMAL);
	if (high_row == 0 && high_col == Key) {
	    highlight = 1;
	    attrset(A_BOLD);
	}
        dobox(KeyOffsetY + 1,            // top
              KeyOffsetX + Key * 14 + 2, // left
              3, // height
              10, // width
              KeyStr[Key], // text
	      highlight
            );        
    }

    // second row of user-defined keys
    KeyOffsetY = 17;
    for (Key = 0; Key < 5; Key++) {
	int highlight = 0;
	attrset(A_NORMAL);
	if (high_row == 1 && high_col == Key) {
	    highlight = 1;
	    attrset(A_BOLD);
	}
        // only draw key if any text is set
        if (strlen(RowTwoKeys[Key]) > 0) {
            dobox(KeyOffsetY + 1, // top
                  KeyOffsetX + Key * 14 + 2, // left
                  3, // height
                  10, // width
                  RowTwoKeys[Key], // text
		  highlight
                );        
        }
    }

    //  This draws the AM/PM/24H indicator
    attron(COLOR_PAIR(2)); // red text on black background
    if ( LEDbits.extras & INDICATOR_AM ) {
        move(5, 69);
        printw("AM");
    }
    if ( LEDbits.extras & INDICATOR_PM ) {
        move(6, 69);
        printw("PM");
    }
    if ( LEDbits.extras & INDICATOR_24 ) {
        move(7, 69);
        printw("24H");
    }
    if ( LEDbits.extras & INDICATOR_DATE ) {
        move(8, 69);
        printw("Date");
    }

    // status bar
    if ( strlen(statusbar) > 0 ) {
	move (23, 0);
	attron(COLOR_PAIR(5));
	printw(statusbar);
    }
    move (0,0);
    refresh();
}

/* if the returned u_short_int has the first byte clear, then it's
 * a bit pattern: the first 4 bits of the second byte are which
 * column, and the next 4 bits are which row.
 *
 * if the first eight bits are set, then it's an ASCII char corresponding
 * to a keystroke from the keyboard
 */
void (*keyhandler)(keybits);
int register_keyhandler( void(*f)(keybits) )
{
    keyhandler = f;

    return 1;
}

void get_key()
{
    int     c;
    int     mouse_return;
    MEVENT  mouse_data;

    int     KeyRow, KeyCol;
    int     ColCheck;

    keybits KeyboardBits;

    char *showbits = getenv("SHOWCLOCKLEDBITS");

    // only show bits, don't do fullscreen mode
    if ( showbits && strcmp(showbits, "YES") == 0 ) {
        pause();
        return;
    }    

    c = wgetch(stdscr); // blocks until a key is hit

    if ( c != KEY_MOUSE ) {
        if ( c < 128 )
            keyhandler((keybits) c << 8 );
        else
            keyhandler((keybits) (c - 128) << 8 );
        return;
    }

    mouse_return = getmouse(&mouse_data);
    if (mouse_return == ERR) {
        beep();
        fprintf(stderr, "Error reading mouse.");
    }

    KeyRow = -1;

    if (14 <= mouse_data.y && mouse_data.y <= 16) 
        KeyRow = 0;
    else if (18 <= mouse_data.y && mouse_data.y <= 20)
        KeyRow = 1;

    if (KeyRow == -1)
        return;

    KeyCol = (mouse_data.x - 8) / 14;
    ColCheck = mouse_data.x - 8 - (KeyCol * 14);
    if (ColCheck < 0 || ColCheck > 8)
        return;

    KeyboardBits =  (KeyRow << 4) + (KeyCol << 0);

    keyhandler(KeyboardBits);
}

void redraw()
{
    display();
    fflush(stdout);
}
