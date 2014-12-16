Toolchain
=========
  * gcc-arm-embedded toolchain >= 4.9-2014-q4
    * https://launchpad.net/gcc-arm-embedded

  * Command line tools (use cygwin for windows)
    * make

  * Flash programming
    * STLINK Utility (windows only)
    * https://github.com/texane/stlink  (for linux)

  * Python 2 for the flashtool


Eclipse settings
================
  * Download Eclipse Luna and install plug-ins:
    * Help -> Install new Software -> Add
        * Name: http://pydev.org/updates
        * Location: http://pydev.org/updates
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
    -> C/C++ -> Build -> Console -> Limit console output: 5000
    -> C/C++ -> Editor -> Scalability -> .. number of lines: 50000
    -> C/C++ -> Editor -> Folding -> Enable folding of preprocessor branches
    -> C/C++ -> Code Style -> Formatter -> Import: "Docs/Codestyle.xml"
    -> C/C++ -> Code Analysis -> Uncheck all options
    -> General -> Editors -> Text Editors -> Spelling -> Disable spell checking

  * File -> New -> Makefile Project with Existing Code
    -> Toolchain for Indexer Settigns: Cross GCC

  * Project -> Uncheck "Build Automatically" 
  
  * Project -> Properties -> C/C++ General
      -> Preprocessor Include Paths, Macros, etc. -> Providers
        -> CDT GCC Build Output Parser:
           arm-none-eabi-gcc
        -> CDT Cross GCC Built-in Compiler Settings:
           arm-none-eabi-gcc -E -P -v -dD "${INPUTS}"


Flashing
--------
https://github.com/texane/stlink

Make sure to install the udev rules, so you can flash without root
privileges.

st-flash --reset write obj_boot/f00b00t.bin 0x08000000
st-flash --reset write obj_app/drquad.bin 0x08010000


Debugging
---------
GDB may complain that auto-loading has been declined. If this is
the case, just create a .gdbinit in your home directory:

$ nano ~/.gdbinit
set auto-load safe-path /

Afterwards, 

$ arm-none-eabi-gdb obj_app/drquad32.elf

should do the trick.
