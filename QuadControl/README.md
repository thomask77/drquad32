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


Known Bugs
==========

Ubuntu 14.10 / Qt 5.3.0
-----------------------
  * Mouse wheel does not work when running in a VMware virtual machine

    https://bugs.launchpad.net/ubuntu/+source/qtbase-opensource-src/+bug/1330977

    (should be fixed in Ubuntu 15.05 / Qt 5.3.2)

  * DockWidgets leave artifacts on screen and/or freeze when dragging

    https://bugreports.qt.io/browse/QTBUG-14083    
    https://bugreports.qt.io/browse/QTBUG-36967
    http://www.riverbankcomputing.com/pipermail/pyqt/2014-August/034678.html

  * Undocked DockWidgets stay on top

    https://bugreports.qt.io/browse/QTBUG-38964    

