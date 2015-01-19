// wmamixer - An alsa mixer designed for WindowMaker with scrollwheel support
// Copyright (C) 2015  Roman Dobosz <gryf@vimja.com>
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net>
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
//
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the README file for a more complete notice.

#include "wmamixer.h"

// global mixer struct
struct Mixer *mixer_ctrl;

int main(int argc, char **argv) {
    int i;
    XGCValues gcv;
    unsigned long gcm;
    char back_color[] = "back_color";
    char led_color_high[] = "led_color_high";
    char led_color_med[] = "led_color_med";
    char led_color_low[] = "led_color_low";
    XpmAttributes xpmattr;
    XpmColorSymbol xpmcsym[4] = {
        {back_color, NULL, color[0]},
        {led_color_high, NULL, color[1]},
        {led_color_med, NULL, color[2]},
        {led_color_low, NULL, color[3]}
    };

    scanArgs(argc, argv);
    initXWin(argc, argv);

    xpmattr.numsymbols = 4;
    xpmattr.colorsymbols = xpmcsym;
    xpmattr.exactColors = false;
    xpmattr.closeness = 40000;
    xpmattr.valuemask = XpmColorSymbols | XpmExactColors | XpmCloseness;
    XpmCreatePixmapFromData(d_display, w_root, wmsmixer_xpm, &pm_main,
            &pm_mask, &xpmattr);
    XpmCreatePixmapFromData(d_display, w_root, tile_xpm, &pm_tile, NULL, 
            &xpmattr);
    XpmCreatePixmapFromData(d_display, w_root, icons_xpm, &pm_icon, NULL, 
            &xpmattr);
    XpmCreatePixmapFromData(d_display, w_root, digits_xpm, &pm_digits, NULL,
            &xpmattr);
    XpmCreatePixmapFromData(d_display, w_root, chars_xpm, &pm_chars, NULL,
            &xpmattr);
    pm_disp = XCreatePixmap(d_display, w_root, 64, 64, DefaultDepth(d_display, 
                DefaultScreen(d_display)));

    gcm = GCGraphicsExposures;
    gcv.graphics_exposures = false;
    gc_gc = XCreateGC(d_display, w_root, gcm, &gcv);

    color[0] = mixColor(ledcolor, 0, backcolor, 100);
    color[1] = mixColor(ledcolor, 100, backcolor, 0);
    color[2] = mixColor(ledcolor, 60, backcolor, 40);
    color[3] = mixColor(ledcolor, 25, backcolor, 75);

    if (wmaker || ushape || astep) {
        XShapeCombineMask(d_display, w_activewin, ShapeBounding, winsize/2-32,
                winsize/2-32, pm_mask, ShapeSet);
    } else {
        XCopyArea(d_display, pm_tile, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);
    }

    XSetClipMask(d_display, gc_gc, pm_mask);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);
    XSetClipMask(d_display, gc_gc, None);

    mixer_ctrl = Mixer_create(mixer_device);

    if (mixer_ctrl->openOK != 0) {
        fprintf(stderr, "%s : Unable to open mixer device '%s'.\n", NAME,
                mixer_device);
    } else {
        for (i=0; i <mixer_ctrl->devices_no; i++) {
            if (i > 24) {
                fprintf(stderr,"%s : Sorry, can only use channels 0-24\n", NAME);
                break;
            }
            channel[channels] = i;
            channels++;
        }
    }

    readFile();

    if (channels==0)
        fprintf(stderr,"%s : Sorry, no supported channels found.\n", NAME);
    else {
        checkVol(true);

        XEvent xev;
        XSelectInput(d_display, w_activewin, ExposureMask | ButtonPressMask |
                ButtonReleaseMask | ButtonMotionMask);
        XMapWindow(d_display, w_main);

        bool done = false;
        while(!done ) {
            while(XPending(d_display) ) {
                XNextEvent(d_display, &xev);
                switch(xev.type ) {
                    case Expose:
                        repaint();
                        break;
                    case ButtonPress:
                        pressEvent(&xev.xbutton);
                        break;
                    case ButtonRelease:
                        releaseEvent();
                        break;
                    case MotionNotify:
                        motionEvent(&xev.xmotion);
                        break;
                    case ClientMessage:
                        if (xev.xclient.data.l[0] == deleteWin) done=true;
                        break;
                }
            }

            if (btnstate & (BTNPREV | BTNNEXT)) {
                rpttimer++;
                if (rpttimer >= RPTINTERVAL) {
                    if (btnstate & BTNNEXT)
                        curchannel++;
                    else
                        curchannel--;
                    if (curchannel < 0)
                        curchannel = channels - 1;
                    if (curchannel >= channels)
                        curchannel = 0;
                    checkVol(true);
                    rpttimer = 0;
                }
            } else {
                checkVol(false);
            }

            if (text_counter) {
                text_counter--;
                if (!text_counter) {
                    drawVolLevel();
                    repaint();
                }
                //	printf("%c", text_counter);
            }

            XFlush(d_display);

            usleep(50000);
        }
    }

    XFreeGC(d_display, gc_gc);
    XFreePixmap(d_display, pm_main);
    XFreePixmap(d_display, pm_tile);
    XFreePixmap(d_display, pm_disp);
    XFreePixmap(d_display, pm_mask);
    XFreePixmap(d_display, pm_icon);
    XFreePixmap(d_display, pm_digits);
    XFreePixmap(d_display, pm_chars);
    freeXWin();
    Mixer_destroy(mixer_ctrl);
    return 0;
}


/* Mixer struct support functions */
struct Mixer *Mixer_create(char *device) {
    struct Mixer *mixer_ctrl = malloc(sizeof(struct Mixer));
    assert(mixer_ctrl != NULL);
    mixer_ctrl->openOK = 0;
    mixer_ctrl->devices_no = 0;
    mixer_ctrl->devices_no = 0;
    return mixer_ctrl;
}

void Mixer_set_left(int current, int value) {
}

void Mixer_set_right(int current, int value) {
}

void Mixer_write_volume(int current) {
}

int Mixer_read_left(int current) {
    return 50;
}

int Mixer_read_right(int current) {
    return 50;
}

int Mixer_read_volume(int channel, bool read) {
    return 50;
}

bool Mixer_get_stereo(int channel) {
    return true;
}

void Mixer_destroy(struct Mixer *mix) {
    assert(mix != NULL);
    //free(mix->name);
    free(mix);
}

void initXWin(int argc, char **argv)
{
  winsize=astep ? ASTEPSIZE : NORMSIZE;

  if ((d_display=XOpenDisplay(display))==NULL ) {
    fprintf(stderr,"%s : Unable to open X display '%s'.\n", NAME, XDisplayName(display));
    exit(1);
  }
  _XA_GNUSTEP_WM_FUNC=XInternAtom(d_display, "_GNUSTEP_WM_FUNCTION", false);
  deleteWin=XInternAtom(d_display, "WM_DELETE_WINDOW", false);

  w_root=DefaultRootWindow(d_display);

  XWMHints wmhints;
  XSizeHints shints;
  shints.x=0;
  shints.y=0;
  shints.flags=0;
  bool pos=(XWMGeometry(d_display, DefaultScreen(d_display), position, NULL, 0, &shints, &shints.x, &shints.y,
			&shints.width, &shints.height, &shints.win_gravity) & (XValue | YValue));
  shints.min_width=winsize;
  shints.min_height=winsize;
  shints.max_width=winsize;
  shints.max_height=winsize;
  shints.base_width=winsize;
  shints.base_height=winsize;
  shints.flags=PMinSize | PMaxSize | PBaseSize;

  createWin(&w_main, shints.x, shints.y);

  if (wmaker || astep || pos)
    shints.flags |= USPosition;
  if (wmaker ) {
    wmhints.initial_state=WithdrawnState;
    wmhints.flags=WindowGroupHint | StateHint | IconWindowHint;
    createWin(&w_icon, shints.x, shints.y);
    w_activewin=w_icon;
    wmhints.icon_window=w_icon;
  }
  else{
    wmhints.initial_state=NormalState;
    wmhints.flags=WindowGroupHint | StateHint;
    w_activewin=w_main;
  }
  wmhints.window_group=w_main;
  XSetWMHints(d_display, w_main, &wmhints);
  XSetWMNormalHints(d_display, w_main, &shints);
  XSetCommand(d_display, w_main, argv, argc);
  XStoreName(d_display, w_main, NAME);
  XSetIconName(d_display, w_main, NAME);
  XSetWMProtocols(d_display, w_activewin, &deleteWin, 1);
}

void freeXWin()
{
  XDestroyWindow(d_display, w_main);
  if (wmaker)
    XDestroyWindow(d_display, w_icon);
  XCloseDisplay(d_display);
}

void createWin(Window *win, int x, int y)
{
  XClassHint classHint;
  *win=XCreateSimpleWindow(d_display, w_root, x, y, winsize, winsize, 0, 0, 0);
  classHint.res_name=NAME;
  classHint.res_class=CLASS;
  XSetClassHint(d_display, *win, &classHint);
}

unsigned long getColor(char *colorname)
{
  XColor color;
  XWindowAttributes winattr;
  XGetWindowAttributes(d_display, w_root, &winattr);
  color.pixel=0;
  XParseColor(d_display, winattr.colormap, colorname, &color);
  color.flags=DoRed | DoGreen | DoBlue;
  XAllocColor(d_display, winattr.colormap, &color);
  return color.pixel;
}

unsigned long mixColor(char *colorname1, int prop1, char *colorname2, int prop2)
{
  XColor color, color1, color2;
  XWindowAttributes winattr;
  XGetWindowAttributes(d_display, w_root, &winattr);
  XParseColor(d_display, winattr.colormap, colorname1, &color1);
  XParseColor(d_display, winattr.colormap, colorname2, &color2);
  color.pixel=0;
  color.red=(color1.red*prop1+color2.red*prop2)/(prop1+prop2);
  color.green=(color1.green*prop1+color2.green*prop2)/(prop1+prop2);
  color.blue=(color1.blue*prop1+color2.blue*prop2)/(prop1+prop2);
  color.flags=DoRed | DoGreen | DoBlue;
  XAllocColor(d_display, winattr.colormap, &color);
  return color.pixel;
}

void scanArgs(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(); 
            exit(0);
        }
    if (strcmp(argv[i], "-v")==0 || strcmp(argv[i], "--version")==0) {
      fprintf(stderr, "wmsmixer version %s\n", VERSION);
      exit(0);
    }
    if (strcmp(argv[i], "-w")==0)
      wmaker=!wmaker;
    if (strcmp(argv[i], "-s")==0)
      ushape=!ushape;
    if (strcmp(argv[i], "-a")==0)
      astep=!astep;
    if (strcmp(argv[i], "-novol")==0)
      no_volume_display = 1;
    if (strcmp(argv[i], "-d")==0 ) {
      if (i<argc-1 ) {
	i++;
    sprintf(mixer_device, "%s", argv[i]);
      }
      continue;
    }
    if (strcmp(argv[i], "-l")==0 ) {
      if (i<argc-1 ) {
	i++;
	sprintf(ledcolor, "%s", argv[i]);
      }
      continue;
    }
    if (strcmp(argv[i], "-b")==0 ) {
      if (i<argc-1 ) {
	i++;
	sprintf(backcolor, "%s", argv[i]);
      }
      continue;
    }
    if (strcmp(argv[i], "-position")==0 ) {
      if (i<argc-1 ) {
	i++;
	sprintf(position, "%s", argv[i]);
      }
      continue;
    }
    if (strcmp(argv[i], "-display")==0 ) {
      if (i<argc-1 ) {
	i++;
	sprintf(display, "%s", argv[i]);
      }
      continue;
    }
  }
}

void readFile() {
    FILE *rcfile;
    char buf[256];
    int done;
    int current=-1;
    char *confpath;

    if (getenv("XDG_CONFIG_HOME")) {
        confpath = malloc(snprintf(NULL, 0, "%s%s", getenv("XDG_CONFIG_HOME"),
                    "/wmamixer.conf") + 1);
        sprintf(confpath, "%s%s", getenv("XDG_CONFIG_HOME"), "/wmamixer.conf");
    } else {
        confpath = malloc(snprintf(NULL, 0, "%s%s", getenv("HOME"),
                "/.config/wmamixer.conf") + 1);
        sprintf(confpath, "%s%s", getenv("HOME"), "/.config/wmamixer.conf");
    }

    if(access(confpath, 0) != 0) {
        printf("no conf file\n");
        free(confpath);
        return;
    }

    if ((rcfile = fopen(confpath, "r")) == NULL) {
        printf("cannot open conf file\n");
        free(confpath);
        return;
    }

    channels=0;
    do {
        fgets(buf, 250, rcfile);
        if ((done=feof(rcfile)) == 0) {
            buf[strlen(buf) - 1] = 0;

            if (strncmp(buf, "addchannel ", strlen("addchannel ")) == 0) {
                sscanf(buf, "addchannel %i", &current);
                if (current >= mixer_ctrl->devices_no ) {
                    fprintf(stderr, "%s : Sorry, this channel (%i) is "
                            "not supported.\n", NAME, current);
                    current = -1;
                }
                else{
                    channel[channels]=current;
                    channels++;
                }
            }

            if (strncmp(buf, "setchannel ", strlen("setchannel ")) == 0) {
                sscanf(buf, "setchannel %i", &current);
                if (current >= mixer_ctrl->devices_no ) {
                    fprintf(stderr,"%s : Sorry, this channel (%i) is "
                            "not supported.\n", NAME, current);
                    current = -1;
                }
            }

            if (strncmp(buf, "setname ", strlen("setname ")) == 0) {
                if (current == -1) {
                    fprintf(stderr, "%s : Sorry, no current channel.\n",
                            NAME);
                } else {
                    small_labels[current] = (char *) malloc(sizeof(char) *
                            5);
                    sscanf(buf, "setname %4s", small_labels[current]);
                }
            }

            if (strncmp(buf, "setmono ", strlen("setmono ")) == 0) {
                if (current == -1) {
                    fprintf(stderr,"%s : Sorry, no current channel.\n", 
                            NAME);
                } else {
                    int value;
                    sscanf(buf, "setmono %i", &value);
                    Mixer_set_left(current, value);
                    Mixer_set_right(current, value);
                    Mixer_write_volume(current);
                }
            }

            if (strncmp(buf, "setleft ", strlen("setleft ")) == 0 ) {
                if (current == -1) {
                    fprintf(stderr, "%s : Sorry, no current channel.\n", 
                            NAME);
                } else {
                    int value;
                    sscanf(buf, "setleft %i", &value);
                    Mixer_set_left(current, value);
                    Mixer_write_volume(current);
                }
            }

            if (strncmp(buf, "setright ", strlen("setright ")) == 0 ) {
                if (current == -1) {
                    fprintf(stderr, "%s : Sorry, no current channel.\n", 
                            NAME);
                } else{
                    int value;
                    sscanf(buf, "setleft %i", &value);
                    Mixer_set_right(current, value);
                    Mixer_write_volume(current);
                }
            }
        }
    }  while (done == 0);
    fclose(rcfile);
    free(confpath);
}

void checkVol(bool forced) {
  Mixer_read_volume(channel[curchannel], true);
  int nl = Mixer_read_left(channel[curchannel]);
  int nr = Mixer_read_right(channel[curchannel]);
  if (forced ) {
    curleft=nl;
    curright=nr;
    update();
    repaint();
  }
  else{
    if (nl!=curleft || nr!=curright ) {
      if (nl!=curleft ) {
	curleft=nl;
	if (Mixer_get_stereo(channel[curchannel]))
	  drawLeft();
	else
	  drawMono();
      }
      if (nr!=curright ) {
	curright=nr;
	if (Mixer_get_stereo(channel[curchannel]))
	  drawRight();
	else
	  drawMono();
      }
      if (!no_volume_display)
	drawVolLevel();
      repaint();
    }
  }
}

void pressEvent(XButtonEvent *xev)
{
  if (xev->button == Button4 || xev->button == Button5) {
    int inc;
    if (xev->button == Button4) inc = 4;
    else inc = -4;

    Mixer_read_volume(channel[curchannel], false);
    Mixer_set_left(channel[curchannel],
		    CLAMP(Mixer_read_left(channel[curchannel]) + inc, 0, 100));
    Mixer_set_right(channel[curchannel], 
            CLAMP(Mixer_read_right(channel[curchannel]) + inc, 0, 100));
    Mixer_write_volume(channel[curchannel]);
    checkVol(false);
    return;
  }

  int x=xev->x-(winsize/2-32);
  int y=xev->y-(winsize/2-32);
  if (x>=5 && y>=47 && x<=17 && y<=57 ) {
    curchannel--;
    if (curchannel<0)
      curchannel=channels-1;
    btnstate |= BTNPREV;
    rpttimer=0;
    drawBtns(BTNPREV);
    checkVol(true);
    return;
  }
  if (x>=18 && y>=47 && x<=30 && y<=57 ) {
    curchannel++;
    if (curchannel>=channels)
      curchannel=0;
    btnstate|=BTNNEXT;
    rpttimer=0;
    drawBtns(BTNNEXT);
    checkVol(true);
    return;
  }
  if (x>=37 && x<=56 && y>=8 && y<=56 ) {
    int v=((60-y)*100)/(2*25);
    dragging=true;
    if (x<=50)
      Mixer_set_left(channel[curchannel], v);
    if (x>=45)
      Mixer_set_right(channel[curchannel], v);
    Mixer_write_volume(channel[curchannel]);
    checkVol(false);
    return;
  }
  if (x>=5 && y>=21 && x<=30 && y<=42) {
    drawText(small_labels[channel[curchannel]]);
    return;
  }

}

void releaseEvent() {
    dragging = false;
    btnstate &= ~(BTNPREV | BTNNEXT);
    drawBtns(BTNPREV | BTNNEXT);
    repaint();
}

void motionEvent(XMotionEvent *xev)
{
  int x=xev->x-(winsize/2-32);
  int y=xev->y-(winsize/2-32);
  if (x>=37 && x<=56 && y>=8 && dragging ) {
    int v=((60-y)*100)/(2*25);
    if (v<0)
      v=0;
    if (x<=50)
      Mixer_set_left(channel[curchannel], v);
    if (x>=45)
      Mixer_set_right(channel[curchannel], v);
    Mixer_write_volume(channel[curchannel]);
    checkVol(false);
  }
}

void repaint()
{
  XCopyArea(d_display, pm_disp, w_activewin, gc_gc, 0, 0, 64, 64, winsize/2-32, winsize/2-32);
  XEvent xev;
  while(XCheckTypedEvent(d_display, Expose, &xev));
}

void update()
{
  drawText(small_labels[channel[curchannel]]);

  XCopyArea(d_display, pm_icon, pm_disp, gc_gc, icon[channel[curchannel]]*26, 0, 26, 24, 5, 19);
  if (Mixer_get_stereo(channel[curchannel])) {
    drawLeft();
    drawRight();
  }
  else {
    drawMono();
  }
}

void drawText(char *text)
{
  char *p = text;
  char p2;
  int i;

  for (i=0; i<4; i++, p++) {
    p2 = toupper(*p);
    if (p2 >= 'A' && p2 <= 'Z') { 
      XCopyArea(d_display, pm_chars, pm_disp, gc_gc, 6*((int)p2-65), 0, 6, 9, 5+(i*6), 5);
    }
    else if (p2 >= '0' && p2 <= '9') { 
      XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 6*((int)p2-48), 0, 6, 9, 5+(i*6), 5);
    }
    else {
      if (p2 == '\0')
	p--;
      XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 60, 0, 6, 9, 5+(i*6), 5);
    }
  }
  if (!no_volume_display)
    text_counter = 10;
}

void drawVolLevel() {
    int i;
    int digits[4];

    int vol = (Mixer_read_left(channel[curchannel]) + 
            Mixer_read_right(channel[curchannel])) / 2;

    digits[0] = (vol/100) ? 1 : 10;
    digits[1] = (vol/10) == 10 ? 0 : (vol/10);
    digits[2] = vol%10;
    digits[3] = 10;

    for (i = 0; i < 4; i++) {
        XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 6*digits[i], 0, 6, 9,
                5+(i*6), 5);
    }
}

void drawLeft() {
    int i;
  XSetForeground(d_display, gc_gc, color[0]);
  XFillRectangle(d_display, pm_disp, gc_gc, 46, 7, 2, 49);

  XSetForeground(d_display, gc_gc, color[1]);
  for (i = 0; i < 25; i++) {
    if (i==(curleft*25)/100)
      XSetForeground(d_display, gc_gc, color[3]);
    XFillRectangle(d_display, pm_disp, gc_gc, 37, 55-2*i, 9, 1);
  }
}

void drawRight()
{
    int i;
  XSetForeground(d_display, gc_gc, color[0]);
  XFillRectangle(d_display, pm_disp, gc_gc, 46, 7, 2, 49);

  XSetForeground(d_display, gc_gc, color[1]);
  for (i=0;i<25;i++) {
    if (i==(curright*25)/100)
      XSetForeground(d_display, gc_gc, color[3]);
    XFillRectangle(d_display, pm_disp, gc_gc, 48, 55-2*i, 9, 1);
  }
}

void drawMono()
{
    int i;
  XSetForeground(d_display, gc_gc, color[1]);
  for (i=0;i<25;i++ ) {
    if (i==(curright*25)/100)
      XSetForeground(d_display, gc_gc, color[3]);
    XFillRectangle(d_display, pm_disp, gc_gc, 37, 55-2*i, 20, 1);
  }
}


void drawBtns(int btns)
{
  if (btns & BTNPREV)
    drawBtn(5, 47, 13, 11, (btnstate & BTNPREV));
  if (btns & BTNNEXT)
    drawBtn(18, 47, 13, 11, (btnstate & BTNNEXT));
}

void drawBtn(int x, int y, int w, int h, bool down)
{
  if (!down)
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, w, h, x, y);
  else {
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, 1, h-1, x+w-1, y+1);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x+w-1, y+1, 1, h-1, x, y);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, w-1, 1, x+1, y+h-1);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x+1, y+h-1, w-1, 1, x, y);
  }
}

void usage() {
    printf("%s - An ALSA mixer designed for WindowMaker with ", NAME);
    fputs("\
scrollwheel support\n\
Copyright (C) 2015  Roman Dobosz <gryf@vimja.com>\n\
Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net>\n\
Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>\n\
This software comes with ABSOLUTELY NO WARRANTY\n\
This software is free software, and you are welcome to redistribute it\n\
under certain conditions\n\
See the README file for a more complete notice.\n\n", stdout);
    printf("usage:\n\n   %s [options]\n\noptions:\n\n", NAME);
    fputs("\
   -h | --help         display this help screen\n\
   -v | --version      display the version\n\
   -w                  use WithdrawnState    (for WindowMaker)\n\
   -s                  shaped window\n\
   -a                  use smaller window    (for AfterStep Wharf)\n\
   -l led_color        use the specified color for led display\n\
   -b back_color       use the specified color for backgrounds\n\
   -d mix_device       use specified device  (rather than /dev/mixer)\n\
   -position position  set window position   (see X manual pages)\n\
   -display display    select target display (see X manual pages)\n\n",
    stdout);
}
