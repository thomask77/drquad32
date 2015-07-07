#!/bin/bash

# fix file permissions on cygwin
#
find . -type f -exec chmod 644 {} +

# set executable bit on some files only
#
chmod 755 Tools/*.sh
chmod 755 Tools/add_version_info/add_version_info.py
chmod 755 Tools/{gamma_tab.py,ws2812_tab.py}
chmod 755 Tools/st-link/*.exe
chmod 755 Tools/st-link/*.dll
