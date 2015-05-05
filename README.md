Toolchain
=========
  * gcc-arm-embedded toolchain >= 4.9-2015-q1
    * https://launchpad.net/gcc-arm-embedded

  * Command line tools (use cygwin for windows)
    * make
    * python 2
    * git

  * Flash programming
    * STM32 ST-LINK utility (windows)
    * https://github.com/texane/stlink (linux)

  * Eclipse IDE for C/C++ Developers >= Luna SR2 (4.4.2)

Eclipse settings
================
  * Help -> Install new Software
    * Add...
      * Name: `http://pydev.org/updates`
      * Location: `http://pydev.org/updates`
    * Work with: "All Available Sites", select:
       * Programming Languages
        * C/C++ Development Tools
      * Mobile and Device Development
        * C/C++ GCC Cross Compiler Support
        * C/C++ GDB Hardware Debugging
      * PyDev
        * PyDev for Eclipse
      * Collaboration
        * Eclipse Git Team Provider

  * Window -> Preferences
    * C/C++
      * Code Style -> Formatter -> Import: `Docs/Codestyle.xml`
      * Code Analysis -> Uncheck all options
      * Build -> Environment -> Add... -> Name: `LANG`, Value: `en_US.ISO-8859-1`
      * Build -> Console -> Limit console output: `5000`
      * Editor -> Scalability -> .. number of lines: `50000`
      * Editor -> Folding -> Enable folding of preprocessor branches
      * Indexer -> Build configuration for the indexer -> Use active build configuration

    * General -> Editors -> Text Editors -> Spelling -> Disable spell checking

Project Settings
----------------
  * File -> New -> Makefile Project with Existing Code
    * Toolchain for Indexer Settings: Cross GCC

  * Project -> Properties -> C/C++ Build -> Environment
    -> Double-Click on PATH,   prepend path to C-Compiler:
       `C:\code\gcc-arm-none-eabi-4_9-2015q1\bin;  (or similar)`

  * Project -> Make Target -> Create... -> "all -j8", "clean", "flash"

  * Project -> Properties -> C/C++ General
    * Preprocessor Include Paths, Macros, etc. -> Providers
      * CDT GCC Build Output Parser: `arm-none-eabi-gcc`
      * CDT Cross GCC Built-in Compiler Settings: `arm-none-eabi-gcc -E -P -v -dD "${INPUTS}"`

  * Project -> Uncheck "Build Automatically"

  * Project Explorer -> Select:
      * Attic
      * QuadControl

      * Right click -> Resource Configurations -> Exclude from Build... -> Select All, OK

Flashing
========

Windows
-------
Copy the ST-LINK utility to the Tools/st-link directory.

    make boot_flash
    make flash

Linux
-----
Make sure to install the udev rules, so you can flash without root
privileges.

    st-flash --reset write obj_boot/f00b00t.bin 0x08000000
    st-flash --reset write obj_app/drquad.bin 0x08010000

Debugging
=========
GDB may complain that auto-loading has been declined. If this is
the case, just create a .gdbinit in your home directory:

    $ nano ~/.gdbinit
    set auto-load safe-path /

Afterwards,

    $ arm-none-eabi-gdb obj_app/drquad32.elf

should do the trick.
