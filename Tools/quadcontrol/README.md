Toolchain
=========

Windows
-------
  * Download the Qt Online Installer for Windows and select the `mingw` build.

  * Download the PuTTY installater from 
      http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html

    and install to the default location.

Linux
-----

Tested with Ubuntu >= 14.04:

    $ sudo apt-get install qtcreator libqt5serialport5-dev
    $ sudo apt-get install putty

To use the serial ports, you must add yourself to the `dialout` group:

    $ sudo adduser $USER dialout

Please log out to activate that change.


Compiling
=========

Open `quadcontrol.pro` with Qt Creator and start with `F5`

