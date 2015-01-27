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
struct Mixer *mix;

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

    gcm = GCGraphicsExposures;
    gcv.graphics_exposures = false;
    gc_gc = XCreateGC(d_display, w_root, gcm, &gcv);

    color[0] = mixColor(ledcolor, 0, backcolor, 100);
    color[1] = mixColor(ledcolor, 100, backcolor, 0);
    color[2] = mixColor(ledcolor, 60, backcolor, 40);
    color[3] = mixColor(ledcolor, 25, backcolor, 75);

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

    if (wmaker || ushape || astep) {
        XShapeCombineMask(d_display, w_activewin, ShapeBounding, winsize/2-32,
                winsize/2-32, pm_mask, ShapeSet);
    } else {
        XCopyArea(d_display, pm_tile, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);
    }

    XSetClipMask(d_display, gc_gc, pm_mask);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);
    XSetClipMask(d_display, gc_gc, None);

    mix = Mixer_create(card);

    readFile();

    if (mix->devices_no == 0)
        fprintf(stderr,"%s: Sorry, no supported channels found.\n", NAME);
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
                        curchannel = mix->devices_no - 1;
                    if (curchannel >= mix->devices_no)
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
                //  printf("%c", text_counter);
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
    Mixer_destroy(mix);
    return 0;
}

/* Mixer struct support functions */
struct Mixer *Mixer_create(char *devicename) {
    struct Mixer *mixer = malloc(sizeof(struct Mixer));
    int err;
    const char *name;
    slideCaptureMono capabilities;

    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_alloca(&sid);

    assert(mixer != NULL);
    mixer->devices_no = 0;

    if ((err = snd_mixer_open(&mixer->handle, 0)) < 0) {
        fprintf(stderr, "Mixer %s open error: %s", card, snd_strerror(err));
        exit(err);
    }

    if (smixer_level == 0 &&
            (err = snd_mixer_attach(mixer->handle, card)) < 0) {
        fprintf(stderr, "Mixer attach %s error: %s", card, snd_strerror(err));
        snd_mixer_close(mixer->handle);
        exit(err);
    }

    if ((err = snd_mixer_selem_register(mixer->handle,
                    smixer_level > 0 ? &smixer_options : NULL, NULL)) < 0) {
        fprintf(stderr, "Mixer register error: %s", snd_strerror(err));
        snd_mixer_close(mixer->handle);
        exit(err);
    }

    err = snd_mixer_load(mixer->handle);

    if (err < 0) {
        fprintf(stderr, "Mixer %s load error: %s", card, snd_strerror(err));
        snd_mixer_close(mixer->handle);
        exit(err);
    }
    for (elem = snd_mixer_first_elem(mixer->handle); elem;
            elem = snd_mixer_elem_next(elem)) {
        snd_mixer_selem_get_id(elem, sid);

        if (!snd_mixer_selem_is_active(elem))
            continue;

        capabilities = Mixer_getcapabilities(elem);

        if (!capabilities.isvolume)
            continue;
        name = snd_mixer_selem_id_get_name(sid);
        printf("DEBUG: capabilities cap: %d\n", capabilities.capture);
        selems[mixer->devices_no] = malloc(sizeof(struct Selem));
        selems[mixer->devices_no]->elem = elem;
        selems[mixer->devices_no]->capture = capabilities.capture;
        Mixer_set_limits(elem, selems[mixer->devices_no]);
        Mixer_set_selem_props(selems[mixer->devices_no], name);
        mixer->devices_no++;
        printf("DEBUG: Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
        if (mixer->devices_no == 32) {
            // stop here. This is ridiculous anyway
            break;
        }
    }
    return mixer;
}

/* Guess and return short name for the mixer simple element */
void Mixer_set_selem_props(struct Selem *selem, const char *name) {
    const char *pcm = "PCM",
          *mic = "Mic",
          *cap = "Capture",
          *boost = "Boost",
          *line = "Line";

    snd_mixer_selem_channel_id_t chn;
    char variable[4];
    int idx;

    // Get the name. Simple match.
    if (strcmp("Master", name) == 0) {
        selem->name = "mstr";
    } else if (strstr(pcm, name)) {
        selem->iconIndex = 1;
        if (strcmp(pcm, name) == 0) {
            selem->name = "pcm ";
        } else {
            sprintf(variable, "pcm%d", namesCount.pcm);
            selem->name = variable;
            namesCount.pcm++;
        }
    } else if (strstr("Headphone", name)) {
        selem->name = "hdph";
        selem->iconIndex = 8;
    } else if (strstr("Beep", name)) {
        selem->name = "beep";
        selem->iconIndex = 7;
    } else if (strstr("Digital", name)) {
        selem->iconIndex = selem->capture ? 3 : 6;
        selem->name = "digt";
    } else if (strstr(mic, name)) {
        if (strcmp(mic, name) == 0) {
            selem->name = "mic ";
        } else {
            if (strstr(boost, name)) {
                sprintf(variable, "mib%d", namesCount.micb);
                namesCount.micb++;
            } else {
                sprintf(variable, "mib%d", namesCount.mic);
                namesCount.mic++;
            }
            selem->name = variable;
        }
        selem->iconIndex = 4;
    } else if (strstr(line, name)) {
        if (strcmp(line, name) == 0) {
            selem->name = "line";
        } else {
            if (strstr(boost, name)) {
                sprintf(variable, "lnb%d", namesCount.lineb);
                namesCount.lineb++;
            } else {
                sprintf(variable, "lin%d", namesCount.line);
                namesCount.line++;
            }
            selem->name = variable;
        }
        selem->iconIndex = 5;
    } else if (strstr(cap, name)) {
        if (strcmp(cap, name) == 0) {
            selem->name = "cap ";
        } else {
            sprintf(variable, "cap%d", namesCount.capt);
            selem->name = variable;
            namesCount.capt++;
        }
        selem->iconIndex = 3;
    } else {
        /* defaults */
        sprintf(variable, "vol%d", namesCount.vol);
        selem->name = variable;
        namesCount.pcm++;
        selem->iconIndex = 6;
    }


    if (snd_mixer_selem_has_playback_volume(selem->elem)) {
        if (snd_mixer_selem_is_playback_mono(selem->elem)) {
            printf("DEBUG: playback mono\n");
            selem->stereo = false;
            selem->channels[0] = SND_MIXER_SCHN_MONO;
            selem->channels[1] = SND_MIXER_SCHN_MONO;
        } else {
            printf("DEBUG: playback stereo\n");
            idx = 0;
            for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++){
                if (!snd_mixer_selem_has_playback_channel(selem->elem, chn))
                    continue;
                selem->channels[idx] = chn;
                idx++;
                if (idx == 2)
                    break;
            }
        }
    }

    if (snd_mixer_selem_has_capture_volume(selem->elem)) {
        printf("Capture channels: ");
        if (snd_mixer_selem_is_capture_mono(selem->elem)) {
            printf("DEBUG: playback mono\n");
            selem->stereo = false;
            selem->channels[0] = SND_MIXER_SCHN_MONO;
            selem->channels[1] = SND_MIXER_SCHN_MONO;
        } else {
            idx = 0;
            for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++){
                if (!snd_mixer_selem_has_capture_channel(selem->elem, chn))
                    continue;
                selem->channels[idx] = chn;
                idx++;
                if (idx == 2)
                    break;
            }
        }
    }
}

slideCaptureMono Mixer_getcapabilities(snd_mixer_elem_t *elem) {
    slideCaptureMono retval;
    retval.mono = false;
    retval.capture = false;
    retval.isvolume = false;

    if (snd_mixer_selem_has_common_volume(elem)) {
        retval.isvolume = true;

        if (snd_mixer_selem_has_playback_volume_joined(elem) ||
                snd_mixer_selem_is_playback_mono(elem) )
            retval.mono = true;
    } else {

        if (snd_mixer_selem_has_playback_volume(elem)) {
            retval.isvolume = true;
            if (snd_mixer_selem_has_playback_volume_joined(elem) ||
                    snd_mixer_selem_is_playback_mono(elem))
                retval.mono = true;
        }

        if (snd_mixer_selem_has_capture_volume(elem)) {
            retval.isvolume = true;
            retval.capture = true;
            if (snd_mixer_selem_has_capture_volume_joined(elem) ||
                    snd_mixer_selem_is_playback_mono(elem))
                retval.mono = true;
        }
    }
    return retval;
}

void Mixer_set_limits(snd_mixer_elem_t *elem, struct Selem *selem) {
    long min = 0, max = 0;

    if (snd_mixer_selem_has_common_volume(elem)) {
        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    } else {
        if (snd_mixer_selem_has_capture_volume(elem))
            snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
        if (snd_mixer_selem_has_playback_volume(elem))
            snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    }

    selem->min = min;
    selem->max = max;
}

static int convert_prange(long val, long min, long max) {
	long range = max - min;
	int tmp;

	if (range == 0)
		return 0;

	val -= min;
	tmp = rint((double)val/(double)range * 100);
	return tmp;
}

/*
vol_ops = [
    {"has_volume": snd_mixer_selem_has_playback_volume,
    "v": [
        [snd_mixer_selem_get_playback_volume_range,
         snd_mixer_selem_get_playback_volume,
         set_playback_raw_volume],
        [snd_mixer_selem_get_playback_dB_range,
         snd_mixer_selem_get_playback_dB,
         set_playback_dB],
        [get_mapped_volume_range,
         get_playback_mapped_volume,
         set_playback_mapped_volume]]
        },
    // capt
    {...}
]
*/

long get_volume(snd_mixer_elem_t *elem,
        snd_mixer_selem_channel_id_t chn,
        long min, long max, bool capt) {
    long raw;
    int volume;

    if (!capt) {
        snd_mixer_selem_get_playback_volume(elem, chn, &raw);
    } else {
        snd_mixer_selem_get_capture_volume(elem, chn, &raw);
    }
    volume = convert_prange(raw, min, max);
    return volume;
}

void Mixer_set_left(int current, int value) {
}

void Mixer_set_right(int current, int value) {
}

void Mixer_write_volume(int current) {
}

int Mixer_read_left(int current) {
    struct Selem *selem = selems[current];
    return get_volume(selem->elem, selem->channels[0],
            selem->min, selem->max, selem->capture);
}

int Mixer_read_right(int current) {
    struct Selem *selem = selems[current];
    return get_volume(selem->elem, selem->channels[1],
            selem->min, selem->max, selem->capture);
}

int Mixer_read_volume(int current, bool read) {
    return 50;
}

void Mixer_destroy(struct Mixer *mixer) {
    assert(mixer != NULL);
    snd_mixer_close(mixer->handle);
    free(mixer);
}

void initXWin(int argc, char **argv)
{
  winsize=astep ? ASTEPSIZE : NORMSIZE;

  if ((d_display=XOpenDisplay(display))==NULL ) {
    fprintf(stderr,"%s: Unable to open X display '%s'.\n", NAME, XDisplayName(display));
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

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            fprintf(stderr, "%s version %s\n", NAME, VERSION);
            exit(0);
        }

        if (strcmp(argv[i], "-w") == 0)
            wmaker = !wmaker;

        if (strcmp(argv[i], "-s") == 0)
            ushape = !ushape;

        if (strcmp(argv[i], "-a") == 0)
            astep = !astep;

        if (strcmp(argv[i], "-novol") == 0)
            no_volume_display = 1;

        if (strcmp(argv[i], "-d") == 0) {
            if (i < argc - 1) {
                i++;
                if (strlen(argv[i]) > 255) {
                    fprintf(stderr, "%s: Illegal device name.\n", NAME);
                    exit(1);
                }
                sprintf(card, "%s", argv[i]);
                smixer_options.device = card;
            }
            continue;
        }

        if (strcmp(argv[i], "-l") == 0) {
            if ( i < argc - 1) {
                i++;
                if (strlen(argv[i]) > 255) {
                    fprintf(stderr, "%s: Illegal color name.\n", NAME);
                    exit(1);
                }
                sprintf(ledcolor, "%s", argv[i]);
            }
            continue;
        }

        if (strcmp(argv[i], "-b") == 0) {
            if (i < argc - 1) {
                i++;
                if (strlen(argv[i]) > 255) {
                    fprintf(stderr, "%s: Illegal color name.\n", NAME);
                    exit(1);
                }
                sprintf(backcolor, "%s", argv[i]);
            }
            continue;
        }

        if (strcmp(argv[i], "-position") == 0) {
            if (i < argc - 1) {
                i++;
                if (strlen(argv[i]) > 255) {
                    fprintf(stderr, "%s: Illegal position.\n", NAME);
                    exit(1);
                }
                sprintf(position, "%s", argv[i]);
            }
            continue;
        }

        if (strcmp(argv[i], "-display") == 0) {
            if (i < argc - 1) {
                i++;
                if (strlen(argv[i]) > 255) {
                    fprintf(stderr, "%s: Illegal display name.\n", NAME);
                    exit(1);
                }
                sprintf(display, "%s", argv[i]);
            }
            continue;
        }
    }
}

void readFile() {
    FILE *rcfile;
    int done, current = -1;
    char *confpath, buf[256];

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
        fprintf(stderr, "%s: No config file found.\n", NAME);
        free(confpath);
        return;
    }

    if ((rcfile = fopen(confpath, "r")) == NULL) {
        fprintf(stderr, "%s: Cannot open config file. Using defaults.\n",
                NAME);
        free(confpath);
        return;
    }

    do {
        fgets(buf, 250, rcfile);
        if ((done=feof(rcfile)) == 0) {
            buf[strlen(buf) - 1] = 0;

            if (strncmp(buf, "setchannel ", strlen("setchannel ")) == 0) {
                sscanf(buf, "setchannel %i", &current);
                if (current >= mix->devices_no ) {
                    fprintf(stderr,"%s: Sorry, this channel (%i) is "
                            "not supported.\n", NAME, current);
                    current = -1;
                }
            }

            if (strncmp(buf, "setmono ", strlen("setmono ")) == 0) {
                if (current == -1) {
                    fprintf(stderr,"%s: Sorry, no current channel.\n",
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
                    fprintf(stderr, "%s: Sorry, no current channel.\n",
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
                    fprintf(stderr, "%s: Sorry, no current channel.\n",
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
  Mixer_read_volume(curchannel, true);
  int nl = Mixer_read_left(curchannel);
  int nr = Mixer_read_right(curchannel);
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
    if (selems[curchannel]->stereo)
      drawLeft();
    else
      drawMono();
      }
      if (nr!=curright ) {
    curright=nr;
    if (selems[curchannel]->stereo)
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

void pressEvent(XButtonEvent *xev) {
    int inc, x, y, v;

    if (xev->button == Button4 || xev->button == Button5) {
        if (xev->button == Button4) {
            printf("inc\n");
            inc = 4;
        } else {
            printf("dec\n");
            inc = -4;
        }

        Mixer_read_volume(curchannel, false);
        Mixer_set_left(curchannel,
                CLAMP(Mixer_read_left(curchannel) + inc, 0, 100));
        Mixer_set_right(curchannel,
                CLAMP(Mixer_read_right(curchannel) + inc, 0, 100));
        Mixer_write_volume(curchannel);
        checkVol(false);
        return;
    }

    x = xev->x - (winsize / 2 - 32);
    y = xev->y - (winsize / 2 - 32);
    if (x >= 5 && y >= 47 && x <= 17 && y <= 57) {
        curchannel--;
        if (curchannel < 0)
            curchannel = mix->devices_no -1;
        btnstate |= BTNPREV;
        rpttimer = 0;
        drawBtns(BTNPREV);
        checkVol(true);
        return;
    }
    if (x >= 18 && y >= 47 && x <= 30 && y <= 57) {
        curchannel++;
        if (curchannel >= mix->devices_no)
            curchannel = 0;
        btnstate|=BTNNEXT;
        rpttimer = 0;
        drawBtns(BTNNEXT);
        checkVol(true);
        return;
    }
    if (x >= 37 && x <= 56 && y >= 8 && y <= 56) {
        v = ((60 - y) * 100) / (2 * 25);
        dragging = true;
        if (x <= 50)
            Mixer_set_left(curchannel, v);
        if (x >= 45)
            Mixer_set_right(curchannel, v);
        Mixer_write_volume(curchannel);
        checkVol(false);
        return;
    }
    if (x >= 5 && y >= 21 && x <= 30 && y <= 42) {
        drawText(selems[curchannel]->name);
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
      Mixer_set_left(curchannel, v);
    if (x>=45)
      Mixer_set_right(curchannel, v);
    Mixer_write_volume(curchannel);
    checkVol(false);
  }
}

void repaint()
{
  XCopyArea(d_display, pm_disp, w_activewin, gc_gc, 0, 0, 64, 64, winsize/2-32, winsize/2-32);
  XEvent xev;
  while(XCheckTypedEvent(d_display, Expose, &xev));
}

void update() {
    drawText(selems[curchannel]->name);

    XCopyArea(d_display, pm_icon, pm_disp, gc_gc,
            selems[curchannel]->iconIndex * 26, 0, 26, 24, 5, 19);
    if (selems[curchannel]->stereo) {
        drawLeft();
        drawRight();
    } else {
        drawMono();
    }
}

void drawText(char *text) {
    char *p = text;
    char p2;
    int i;

    for (i = 0; i < 4; i++, p++) {
        p2 = toupper(*p);
        if (p2 >= 'A' && p2 <= 'Z') {
            XCopyArea(d_display, pm_chars, pm_disp, gc_gc,
                    6 * ((int)p2 - 65), 0, 6, 9, 5 + (i * 6), 5);
        } else if (p2 >= '0' && p2 <= '9') {
            XCopyArea(d_display, pm_digits, pm_disp, gc_gc,
                    6 * ((int)p2 - 48), 0, 6, 9, 5 + (i * 6), 5);
        } else {
            if (p2 == '\0')
                p--;
            XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 60, 0, 6, 9,
                    5 + (i * 6), 5);
        }
    }
    if (!no_volume_display)
        text_counter = 10;
}

void drawVolLevel() {
    int i;
    int digits[4];

    int vol = (Mixer_read_left(curchannel) + Mixer_read_right(curchannel)) / 2;

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
   -d alsa_device      use specified device  (rather than 'default')\n\
   -position position  set window position   (see X manual pages)\n\
   -display display    select target display (see X manual pages)\n\n",
    stdout);
}
