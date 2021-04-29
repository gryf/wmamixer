// wmamixer - An ALSA mixer designed for WindowMaker with scrollwheel support
// Copyright (C) 2015  Roman Dobosz <gryf@vimja.com>
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net>
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
//
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the README file for a more complete notice.

#ifndef WMAMIXER_H_
#define WMAMIXER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

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


#define VERSION "1.0"

// User defines - custom
#define BACKCOLOR "#202020"
#define LEDCOLOR "#00c9c1"

#undef CLAMP
#define CLAMP(v, l, h) (((v) > (h)) ? (h) : (((v) < (l)) ? (l) : (v)))

/* Function to convert from percentage to volume. val = percentage */
#define convert_prange1(val, min, max) \
    ceil((val) * ((max) - (min)) * 0.01 + (min))

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
#include "XPM/wmamixer64.xpm"
#include "XPM/wmamixer128.xpm"
#include "XPM/wmamixer192.xpm"
#include "XPM/wmamixer256.xpm"
#include "XPM/tile64.xpm"
#include "XPM/tile128.xpm"
#include "XPM/tile192.xpm"
#include "XPM/tile256.xpm"

// Xpm images - custom
#include "XPM/icons64.xpm"
#include "XPM/icons128.xpm"
#include "XPM/icons192.xpm"
#include "XPM/icons256.xpm"
#include "XPM/digits64.xpm"
#include "XPM/digits128.xpm"
#include "XPM/digits192.xpm"
#include "XPM/digits256.xpm"
#include "XPM/chars64.xpm"
#include "XPM/chars128.xpm"
#include "XPM/chars192.xpm"
#include "XPM/chars256.xpm"

// Variables for command-line arguments - standard
bool wmaker = WINDOWMAKER;
bool ushape = USESHAPE;
bool astep = AFTERSTEP;
char display[256] = "";
char position[256] = "";
int winsize;
int scale = 1;
bool no_volume_display = 0;

// Variables for command-line arguments - custom
char card[256] = "default";
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

int channel[32];

struct Selem {
    char *name;
    bool stereo;
    int currentVolRight;
    int currentVolLeft;
    short int iconIndex;
    snd_mixer_elem_t *elem;
    long min;
    long max;
    bool capture;
    snd_mixer_selem_channel_id_t channels[2];
};

typedef struct {
    bool isvolume;
    bool capture;
    bool mono;
} slideCaptureMono;

struct NamesCount {
    short int pcm, line, lineb, mic, micb, capt, vol, aux;
} namesCount = {1, 1, 1, 1, 1, 1, 1, 1};

struct Selem *selems[32] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

struct Mixer {
    int devices_no;
    snd_mixer_t * handle;
};

struct Mixer *mix;

static int smixer_level = 0;
static struct snd_mixer_selem_regopt smixer_options;

// Procedures and functions - standard
void createWin(Window *win, int x, int y);
void freeXWin();
void initXWin(int argc, char **argv);
unsigned long mixColor(char *color1, int prop1, char *color2, int prop2);

// Procedures and functions - custom
void checkVol(bool forced);
void drawBtn(int x, int y, int w, int h, bool down);
void drawBtns(int btns);
void drawStereo(bool left);
void drawMono();
void drawText(char *text);
void drawVolLevel();
void motionEvent(XMotionEvent *xev);
void pressEvent(XButtonEvent *xev);
void releaseEvent();
void repaint();
void scanArgs(int argc, char **argv);
void update();
void usage();

struct Mixer *Mixer_create();
void Mixer_destroy(struct Mixer *mix);
slideCaptureMono Mixer_get_capabilities(snd_mixer_elem_t *elem);
int Mixer_get_volume(int current, int channelIndex);
int Mixer_read_left(int current);
int Mixer_read_right(int current);
void Mixer_set_selem_props(struct Selem *selem, const char *name);
void Mixer_set_channels(struct Selem *selem);
void Mixer_set_left(int current, int value);
void Mixer_set_right(int current, int value);
void Mixer_set_limits(snd_mixer_elem_t *elem, struct Selem *selem);
void Mixer_set_volume(int current, int channelIndex, int value);

void Selem_set_name(struct Selem *selem, const char *name, short int *count);
void Selem_destroy();

#endif  // WMAMIXER_H_
