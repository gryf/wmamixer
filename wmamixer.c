// wmamixer - An ALSA mixer designed for WindowMaker with scrollwheel support
// Copyright (C) 2015  Roman Dobosz <gryf@vimja.com>
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net>
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
//
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the README file for a more complete notice.

#include "wmamixer.h"

int main(int argc, char **argv) {
    XGCValues gcv;
    unsigned long gcm;

    XpmAttributes xpmattr;

    scanArgs(argc, argv);
    initXWin(argc, argv);

    gcm = GCGraphicsExposures;
    gcv.graphics_exposures = false;
    gc_gc = XCreateGC(d_display, w_root, gcm, &gcv);

    color[0] = mixColor(ledcolor, 0, backcolor, 100);
    color[1] = mixColor(ledcolor, 100, backcolor, 0);
    color[2] = mixColor(ledcolor, 60, backcolor, 40);
    color[3] = mixColor(ledcolor, 25, backcolor, 75);

    XpmColorSymbol xpmcsym[4] = {
        {"back_color", NULL, color[0]},
        {"led_color_high", NULL, color[1]},
        {"led_color_med", NULL, color[2]},
        {"led_color_low", NULL, color[3]}
    };

    xpmattr.numsymbols = 4;
    xpmattr.colorsymbols = xpmcsym;
    xpmattr.exactColors = false;
    xpmattr.closeness = 40000;
    xpmattr.valuemask = XpmColorSymbols | XpmExactColors | XpmCloseness;
    XpmCreatePixmapFromData(d_display, w_root, wmamixer_xpm, &pm_main,
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

    mix = Mixer_create();

    if (mix->devices_no == 0) {
        fprintf(stderr, "%s: Sorry, no supported channels found.\n", NAME);
    } else {
        checkVol(true);

        XEvent xev;
        XSelectInput(d_display, w_activewin, ExposureMask | ButtonPressMask |
                ButtonReleaseMask | ButtonMotionMask);
        XMapWindow(d_display, w_main);

        bool done = false;
        while (!done) {
            while (XPending(d_display)) {
                XNextEvent(d_display, &xev);
                switch (xev.type) {
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
                        if (xev.xclient.data.l[0] == (int) deleteWin)
                            done = true;
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
            }

            XFlush(d_display);
            snd_mixer_handle_events(mix->handle);
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
    Selem_destroy();
    Mixer_destroy(mix);
    return 0;
}

/* Mixer struct support functions */
struct Mixer *Mixer_create() {
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

        capabilities = Mixer_get_capabilities(elem);

        if (!capabilities.isvolume)
            continue;
        name = snd_mixer_selem_id_get_name(sid);

        selems[mixer->devices_no] = malloc(sizeof(struct Selem));
        selems[mixer->devices_no]->elem = elem;
        selems[mixer->devices_no]->capture = capabilities.capture;

        Mixer_set_limits(elem, selems[mixer->devices_no]);
        Mixer_set_selem_props(selems[mixer->devices_no], name);
        Mixer_set_channels(selems[mixer->devices_no]);

        mixer->devices_no++;
        if (mixer->devices_no == 32) {
            // stop here. This is ridiculous anyway
            break;
        }
    }
    return mixer;
}

void Mixer_set_selem_props(struct Selem *selem, const char *name) {
    const char *pcm = "PCM",
          *mic = "Mic",
          *cap = "Capture",
          *boost = "Boost",
          *line = "Line",
          *aux = "Aux";

    if (strcmp("Master", name) == 0) {
        selem->name = "mstr";
        selem->iconIndex = 0;
    } else if (strstr(pcm, name)) {
        selem->iconIndex = 1;
        if (strcmp(pcm, name) == 0)
            selem->name = "pcm ";
        else
            Selem_set_name(selem, "pcm", &namesCount.pcm);

    } else if (strstr("Headphone", name)) {
        selem->name = "hdph";
        selem->iconIndex = 8;
    } else if (strstr("Beep", name) || strstr("PC Speaker", name)) {
        selem->name = "beep";
        selem->iconIndex = 7;
    } else if (strstr("Digital", name)) {
        selem->iconIndex = selem->capture ? 3 : 15;
        selem->name = "digt";
    } else if (strstr("Bass", name)) {
        selem->name = "bass";
        selem->iconIndex = 9;
    } else if (strstr("Treble", name)) {
        selem->name = "trbl";
        selem->iconIndex = 10;
    } else if (strstr("Synth", name)) {
        selem->name = "synt";
        selem->iconIndex = 11;
    } else if (strstr("CD", name)) {
        selem->name = " cd ";
        selem->iconIndex = 12;
    } else if (strstr("Phone", name)) {
        selem->name = "phne";
        selem->iconIndex = 13;
    } else if (strstr("Video", name)) {
        selem->name = "vdeo";
        selem->iconIndex = 14;
    } else if (strstr(mic, name)) {
        if (strcmp(mic, name) == 0) {
            selem->name = "mic ";
        } else {
            if (strstr(boost, name))
                Selem_set_name(selem, "mib", &namesCount.micb);
            else
                Selem_set_name(selem, "mic", &namesCount.mic);
        }
        selem->iconIndex = 4;
    } else if (strstr(aux, name)) {
        if (strcmp(aux, name) == 0) {
            selem->name = "aux ";
        } else {
            Selem_set_name(selem, "aux", &namesCount.aux);
        }
        selem->iconIndex = 5;
    } else if (strstr(line, name)) {
        if (strcmp(line, name) == 0) {
            selem->name = "line";
        } else {
            if (strstr(boost, name))
                Selem_set_name(selem, "lnb", &namesCount.lineb);
            else
                Selem_set_name(selem, "lin", &namesCount.line);
        }
        selem->iconIndex = 5;
    } else if (strstr(cap, name)) {
        if (strcmp(cap, name) == 0) {
            selem->name = "cap ";
        } else {
            Selem_set_name(selem, "cap", &namesCount.capt);
        }
        selem->iconIndex = 3;
    } else {
        namesCount.vol++;
        selem->iconIndex = 6;
        Selem_set_name(selem, name, &namesCount.vol);
    }
}

void Selem_set_name(struct Selem *selem, const char *name, short int *count) {
    char new_name[5], buf[5];

    if (*count > 10) {
        snprintf(new_name, sizeof(new_name), "%s", name);
    } else {
        snprintf(buf, sizeof(buf) - 1, "%s", name);
        snprintf(new_name, sizeof(new_name), "%s%d", buf, *count);
    }

    selem->name = new_name;
    *count = *count + 1;
}

void Mixer_set_channels(struct Selem *selem) {
    int idx;
    snd_mixer_selem_channel_id_t chn;

    selem->stereo = true;

    if (snd_mixer_selem_has_playback_volume(selem->elem)) {
        if (snd_mixer_selem_is_playback_mono(selem->elem)) {
            selem->stereo = false;
            selem->channels[0] = SND_MIXER_SCHN_MONO;
            selem->channels[1] = SND_MIXER_SCHN_MONO;
        } else {
            idx = 0;
            for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) {
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
        if (snd_mixer_selem_is_capture_mono(selem->elem)) {
            selem->stereo = false;
            selem->channels[0] = SND_MIXER_SCHN_MONO;
            selem->channels[1] = SND_MIXER_SCHN_MONO;
        } else {
            idx = 0;
            for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) {
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

slideCaptureMono Mixer_get_capabilities(snd_mixer_elem_t *elem) {
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

int Mixer_get_volume(int current, int channelIndex) {
    struct Selem *selem = selems[current];
    long raw;
    int volume;

    if (!selem->capture) {
        snd_mixer_selem_get_playback_volume(selem->elem,
                selem->channels[channelIndex], &raw);
    } else {
        snd_mixer_selem_get_capture_volume(selem->elem,
                selem->channels[channelIndex], &raw);
    }

    volume = convert_prange(raw, selem->min, selem->max);
    return volume;
}

void Mixer_set_volume(int current, int channelIndex, int value) {
    struct Selem *selem = selems[current];
    long raw;

    raw = (long)convert_prange1(value, selem->min, selem->max);

    if (!selem->capture) {
        snd_mixer_selem_set_playback_volume(selem->elem,
                selem->channels[channelIndex], raw);
    } else {
        snd_mixer_selem_set_capture_volume(selem->elem,
                selem->channels[channelIndex], raw);
    }
}

void Mixer_set_left(int current, int value) {
    Mixer_set_volume(current, 0, value);
}

void Mixer_set_right(int current, int value) {
    Mixer_set_volume(current, 1, value);
}

int Mixer_read_left(int current) {
    return Mixer_get_volume(current, 0);
}

int Mixer_read_right(int current) {
    return Mixer_get_volume(current, 1);
}

void Mixer_destroy(struct Mixer *mixer) {
    assert(mixer != NULL);
    snd_mixer_close(mixer->handle);
    free(mixer);
}

void Selem_destroy() {
    int i;

    for (i = 0; i < mix->devices_no; i++) {
        free(selems[i]);
    }
}


void initXWin(int argc, char **argv) {
    bool pos;
    winsize = astep ? ASTEPSIZE : NORMSIZE;
    XWMHints wmhints;
    XSizeHints shints;

    if ((d_display = XOpenDisplay(display)) == NULL) {
        fprintf(stderr, "%s: Unable to open X display '%s'.\n", NAME,
                XDisplayName(display));
        exit(1);
    }
    _XA_GNUSTEP_WM_FUNC = XInternAtom(d_display, "_GNUSTEP_WM_FUNCTION",
            false);
    deleteWin = XInternAtom(d_display, "WM_DELETE_WINDOW", false);

    w_root = DefaultRootWindow(d_display);

    shints.x = 0;
    shints.y = 0;
    shints.flags = 0;
    pos = (XWMGeometry(d_display, DefaultScreen(d_display), position, NULL, 0,
                &shints, &shints.x, &shints.y, &shints.width, &shints.height,
                &shints.win_gravity) & (XValue | YValue)) == 1;
    shints.min_width = winsize;
    shints.min_height = winsize;
    shints.max_width = winsize;
    shints.max_height = winsize;
    shints.base_width = winsize;
    shints.base_height = winsize;
    shints.flags = PMinSize | PMaxSize | PBaseSize;

    createWin(&w_main, shints.x, shints.y);

    if (wmaker || astep || pos)
        shints.flags |= USPosition;
    if (wmaker) {
        wmhints.initial_state = WithdrawnState;
        wmhints.flags = WindowGroupHint | StateHint | IconWindowHint;
        createWin(&w_icon, shints.x, shints.y);
        w_activewin = w_icon;
        wmhints.icon_window = w_icon;
    } else {
        wmhints.initial_state = NormalState;
        wmhints.flags = WindowGroupHint | StateHint;
        w_activewin = w_main;
    }
    wmhints.window_group = w_main;
    XSetWMHints(d_display, w_main, &wmhints);
    XSetWMNormalHints(d_display, w_main, &shints);
    XSetCommand(d_display, w_main, argv, argc);
    XStoreName(d_display, w_main, NAME);
    XSetIconName(d_display, w_main, NAME);
    XSetWMProtocols(d_display, w_activewin, &deleteWin, 1);
}

void freeXWin() {
    XDestroyWindow(d_display, w_main);
    if (wmaker)
        XDestroyWindow(d_display, w_icon);
    XCloseDisplay(d_display);
}

void createWin(Window *win, int x, int y) {
    XClassHint classHint;
    *win = XCreateSimpleWindow(d_display, w_root, x, y, winsize, winsize, 0,
            0, 0);
    classHint.res_name = NAME;
    classHint.res_class = CLASS;
    XSetClassHint(d_display, *win, &classHint);
}

unsigned long mixColor(char *colorname1, int prop1, char *colorname2,
        int prop2) {
    XColor color, color1, color2;
    XWindowAttributes winattr;
    XGetWindowAttributes(d_display, w_root, &winattr);
    XParseColor(d_display, winattr.colormap, colorname1, &color1);
    XParseColor(d_display, winattr.colormap, colorname2, &color2);
    color.pixel = 0;
    color.red = (color1.red * prop1 + color2.red * prop2) / (prop1 + prop2);
    color.green = (color1.green * prop1 + color2.green * prop2) /
        (prop1 + prop2);
    color.blue = (color1.blue * prop1 + color2.blue * prop2) / (prop1 + prop2);
    color.flags = DoRed | DoGreen | DoBlue;
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
                snprintf(card, sizeof(card), "%s", argv[i]);
                smixer_options.device = card;
            }
            continue;
        }

        if (strcmp(argv[i], "-l") == 0) {
            if (i < argc - 1) {
                i++;
                if (strlen(argv[i]) > 255) {
                    fprintf(stderr, "%s: Illegal color name.\n", NAME);
                    exit(1);
                }
                snprintf(ledcolor, sizeof(ledcolor), "%s", argv[i]);
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
                snprintf(backcolor, sizeof(backcolor), "%s", argv[i]);
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
                snprintf(position, sizeof(position), "%s", argv[i]);
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
                snprintf(display, sizeof(display), "%s", argv[i]);
            }
            continue;
        }
    }
}

void checkVol(bool forced) {
    int nl = Mixer_read_left(curchannel);
    int nr = Mixer_read_right(curchannel);
    if (forced) {
        curleft = nl;
        curright = nr;
        update();
        repaint();
    } else {
        if (nl != curleft || nr != curright) {
            if (nl != curleft) {
                curleft = nl;
                if (selems[curchannel]->stereo)
                    drawStereo(true);
                else
                    drawMono();
            }
            if (nr != curright) {
                curright = nr;
                if (selems[curchannel]->stereo)
                    drawStereo(false);
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
        if (xev->button == Button4)
            inc = 4;
        else
            inc = -4;

        Mixer_set_left(curchannel,
                CLAMP(Mixer_read_left(curchannel) + inc, 0, 100));
        Mixer_set_right(curchannel,
                CLAMP(Mixer_read_right(curchannel) + inc, 0, 100));
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

void motionEvent(XMotionEvent *xev) {
    int x = xev->x - (winsize / 2 - 32);
    int y = xev->y - (winsize / 2 - 32);
    if (x >= 37 && x <= 56 && y >= 8 && dragging) {
        int v = ((60 - y) * 100) / (2 * 25);
        if (v < 0)
            v = 0;
        if (x <= 50)
            Mixer_set_left(curchannel, v);
        if (x >= 45)
            Mixer_set_right(curchannel, v);
        checkVol(false);
    }
}

void repaint() {
    XCopyArea(d_display, pm_disp, w_activewin, gc_gc, 0, 0, 64, 64,
            winsize / 2 - 32, winsize / 2 - 32);
    XEvent xev;
    while (XCheckTypedEvent(d_display, Expose, &xev)) {
        continue;
    }
}

void update() {
    drawText(selems[curchannel]->name);

    XCopyArea(d_display, pm_icon, pm_disp, gc_gc,
            selems[curchannel]->iconIndex * 26, 0, 26, 24, 5, 19);
    if (selems[curchannel]->stereo) {
        drawStereo(true);
        drawStereo(false);
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

void drawStereo(bool left) {
    int i;
    short pos = left? 37 : 48;

    XSetForeground(d_display, gc_gc, color[0]);
    XFillRectangle(d_display, pm_disp, gc_gc, 46, 7, 2, 49);

    XSetForeground(d_display, gc_gc, color[1]);
    for (i = 0; i < 25; i++) {
        if (i == ((left ? curleft : curright) * 25) / 100)
            XSetForeground(d_display, gc_gc, color[3]);
        XFillRectangle(d_display, pm_disp, gc_gc, pos, 55 - 2 * i, 9, 1);
    }
}

void drawMono() {
    int i;
    XSetForeground(d_display, gc_gc, color[1]);
    for (i = 0; i < 25; i++) {
        if (i == (curright * 25) / 100)
            XSetForeground(d_display, gc_gc, color[3]);
        XFillRectangle(d_display, pm_disp, gc_gc, 37, 55 - 2 * i, 20, 1);
    }
}

void drawBtns(int btns) {
    if (btns & BTNPREV)
        drawBtn(5, 47, 13, 11, (btnstate & BTNPREV));
    if (btns & BTNNEXT)
        drawBtn(18, 47, 13, 11, (btnstate & BTNNEXT));
}

void drawBtn(int x, int y, int w, int h, bool down) {
    if (!down) {
        XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, w, h, x, y);
    } else {
        XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, 1, h - 1,
                x + w - 1, y + 1);
        XCopyArea(d_display, pm_main, pm_disp, gc_gc, x + w - 1, y + 1, 1,
                h - 1, x, y);
        XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, w - 1, 1, x + 1,
                y + h - 1);
        XCopyArea(d_display, pm_main, pm_disp, gc_gc, x + 1, y + h - 1, w - 1,
                1, x, y);
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
