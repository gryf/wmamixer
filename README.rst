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