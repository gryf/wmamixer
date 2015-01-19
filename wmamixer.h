// wmamixer - An alsa mixer designed for WindowMaker with scrollwheel support
// Copyright (C) 2015  Roman Dobosz <gryf@vimja.com>
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net>
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
//
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the README file for a more complete notice.

#ifndef WMAMIXER_H
#define WMAMIXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#include <alsa/asoundlib.h>

#define WINDOWMAKER false
#define USESHAPE false
#define AFTERSTEP false
#define NORMSIZE 64
#define ASTEPSIZE 56
#define NAME "wmamixer"
#define CLASS "Wmamixer"


#define VERSION "0.1"

// User defines - custom
#define BACKCOLOR "#202020"
#define LEDCOLOR "#00c9c1"

#undef CLAMP
#define CLAMP(v, l, h) (((v) > (h)) ? (h) : (((v) < (l)) ? (l) : (v)))


// Pixmaps - standard
Pixmap pm_main;
Pixmap pm_tile;
Pixmap pm_disp;
Pixmap pm_mask;

// Pixmaps - custom
Pixmap pm_icon;
Pixmap pm_digits;
Pixmap pm_chars;

// Xpm images - standard
#include "XPM/wmsmixer.xpm"
#include "XPM/tile.xpm"

// Xpm images - custom
#include "XPM/icons.xpm"
#include "XPM/digits.xpm"
#include "XPM/chars.xpm"

// Variables for command-line arguments - standard
bool wmaker = WINDOWMAKER;
bool ushape = USESHAPE;
bool astep = AFTERSTEP;
char display[256] = "";
char position[256] = "";
int winsize;
bool no_volume_display = 0;

// Variables for command-line arguments - custom
char mixer_device[256] = "default";
char backcolor[256] = BACKCOLOR;
char ledcolor[256] = LEDCOLOR;

// X-Windows basics - standard
Atom _XA_GNUSTEP_WM_FUNC;
Atom deleteWin;
Display *d_display;
Window w_icon;
Window w_main;
Window w_root;
Window w_activewin;

// X-Windows basics - custom
GC gc_gc;
unsigned long color[4];

int text_counter = 0;

// Misc custom global variables 
// ----------------------------

// Current state information
int curchannel = 0;
int curleft;
int curright;

// For buttons
int btnstate = 0;
#define BTNNEXT 1
#define BTNPREV 2

// For repeating next and prev buttons
#define RPTINTERVAL 5
int rpttimer = 0;

// For draggable volume control
bool dragging = false;

int channels = 2;
int channel[25];
int icon[25] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
    18, 19, 20, 21, 22, 23, 24};
char *small_labels[25] = {"vol ", "bass", "trbl", "synt", "pcm ",
    "spkr", "line", "mic ", "cd  ", "mix ", "pcm2", "rec ", "igai", "ogai", 
    "lin1", "lin2", "lin3", "dig1", "dig2", "dig3", "phin", "phou", "vid ",
    "rad ", "mon "};

struct Mixer {
    int openOK;
    int devices_no;
};

// Procedures and functions - standard
void initXWin(int argc, char **argv);
void freeXWin();
void createWin(Window *win, int x, int y);
unsigned long getColor(char *colorname);
unsigned long mixColor(char *colorname1, int prop1, char *colorname2, 
        int prop2);

// Procedures and functions - custom
void scanArgs(int argc, char **argv);
void readFile();
void usage();
void checkVol(bool forced);
void pressEvent(XButtonEvent *xev);
void releaseEvent();
void motionEvent(XMotionEvent *xev);
void repaint();
void update();
void drawLeft();
void drawRight();
void drawMono();
void drawVolLevel();
void drawText(char *text);
void drawBtns(int btns);
void drawBtn(int x, int y, int w, int h, bool down);

struct Mixer *Mixer_create(char *device);
void Mixer_set_left(int current, int value);
void Mixer_set_right(int current, int value);
void Mixer_write_volume(int current);
int Mixer_read_left(int current);
int Mixer_read_right(int current);
int Mixer_read_volume(int channel, bool read);
bool Mixer_get_stereo(int channel);

void Mixer_destroy(struct Mixer *mix);

#endif /* WMAMIXER_H */
