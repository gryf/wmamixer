wmamixer
========

Wmamixer is a fork of `wmsmixer`_ and is an `ALSA`_ mixer dockapp for Window
Maker.

The code for the `ALSA`_ part was taken and adapted from amixer and alsamixer
programs from alsa-utils package.

.. image:: /images/wmamixer.gif?raw=true
   :alt: wmamixer overview

Build
-----

To build the dockapp, just perform following command in the project topmost
directory:

.. code:: shell-session

   $ make

Next, copy binary ``wmamixer`` in convenient place.

Usage
-----

To see usage information for the dockapp, pass ``--help`` option from terminal
emulator, like:

.. code:: shell-session

   user@linux ~ $ wmamixer --help

which will output several lines of information and options:

* ``-h`` or ``--help`` will display options and exit,
* ``-v`` or ``--version`` will display version and exit,
* ``-w`` will use *withdrawn state*, which will make wmamixer behave like a
  Window Maker dockapp,
* ``-s`` will make some part of the window transparent,
* ``-a`` will make window a bit smaller. Instead of standard 64x64 pixels, it
  will be 56x56 pixels, additionally with transparent background. This mode is
  useful for placing it in AfterStep Wharf,
* ``-l`` with a color as an argument will change led color (default bright
  greenish),
* ``-b`` with a color as an argument will change background for the led color
  (default dark greenish). Colors can be specified using `X11 color names`_, or
  by hexadecimal number in #RGB format, like:

  .. code:: shell-session

     user@linux ~ $ wmamixer -l red -b '#ffff00'  # red led on yellow background
     user@linux ~ $ wmamixer -b darkslateblue  # Dark Slate Blue background

* ``-d`` selects desired ALSA device. By default, first device, which is called
  ``default`` is selected. That device is taken by pulseaudio nowadays, so
  you'll see only two controls: one for master volume and the other for capture
  volume. To be able to change all supported by soundcard controls, you need to
  pass right PCM device to this option. First, you need to know which device
  need to be passed as an argument to ``-d`` option. To list ALSA devices you
  might use ``aplay`` program from ``alsa-utils`` package:

  .. code:: shell-session

     user@linux ~ $ aplay -l
     **** List of PLAYBACK Hardware Devices ****
     card 0: HDMI [HDA Intel HDMI], device 3: HDMI 0 [HDMI 0]
       Subdevices: 1/1
       Subdevice #0: subdevice #0
     card 0: HDMI [HDA Intel HDMI], device 7: HDMI 1 [HDMI 1]
       Subdevices: 1/1
       Subdevice #0: subdevice #0
     card 0: HDMI [HDA Intel HDMI], device 8: HDMI 2 [HDMI 2]
       Subdevices: 1/1
       Subdevice #0: subdevice #0
     card 1: PCH [HDA Intel PCH], device 0: 92HD91BXX Analog [92HD91BXX Analog]
       Subdevices: 0/1
       Subdevice #0: subdevice #0

  So in above example there are two sound devices, which one is HDMI and the
  other PCH. To select PCH device, it is enough to invoke ``wmamixer`` like:

  .. code:: shell-session

     user@linux ~ $ wmamixer -d hw:1

  where ``hw:1`` means the second card (you can see it within the line ``card
  1: PCH [HDA Intel PCH], device 0: 92HD91BXX Analog [92HD91BXX Analog]`` on
  ``aplay -l`` output in the example above),
* ``-position`` with appropriate argument (usually ``+0+0`` for top-left
  corner) will try to convince window manager to place application in certain
  position,
* ``-display`` with correct *X11 display* will try to use specified display
  instead of current one (usually ``:0``).

User interface is straightforward.

.. image:: /images/wmamixer_gui.png
   :alt: wmamixer gui elements

1. Volume level indicator of currently selected mixer.
2. Icon for currently selected mixer. If clicked, will show abbreviated mixer
   name for a short time on volume level indicator.
3. Cycles through available mixers.
4. Volume bar. Clicking on the volume bar between the left and right channels
   will set same volume level for both of them. Mouse scroll will adjust
   volume for both channels at the same time.
5. If clicked on left or right bar, volume will be adjusted for selected mixer
   left or right channel accordingly. Note, that not all mixers have ability to
   adjust volume for each channels separately. Mouse scroll will adjust volume
   for both channels as in point 4.

Bugs
----

If spotted any bug, please report it using bug tracker on bitbucket or github.

Changes
-------

Changes with comparison with `wmsmixer`_:

* `ALSA`_ instead of OSS. This is the real thing, using alsa-lib, not just
  emulation of OSS.
* Removed config file support, since it doesn't apply anymore

Limitations
-----------

1. Currently, wmamixer does not support switches and enum type of controls.
Only volume is supported. Enum and switch based ALSA controls are simply
ignored.

2. There are controls with really small limit range, for example here is
control Beep (pc speaker in other words) which is represented by amixer like
this::

    Simple mixer control 'Beep',0
    Capabilities: pvolume pvolume-joined pswitch pswitch-joined
    Playback channels: Mono
    Limits: Playback 0 - 3
    Mono: Playback 1 [33%] [-12.00dB] [on]

Under "Limits" section, there is a Playback capability with range 0 - 3. Using
scrollwheel on such low ranges is somehow awkward. For that controls it's
better to use clicking instead of scrolling.

.. _wmsmixer: http://web.archive.org/web/20081024034859/http://www.hibernaculum.net/wmsmixer/index.php
.. _ALSA: http://www.alsa-project.org
.. _X11 color names: https://en.wikipedia.org/wiki/X11_color_names
