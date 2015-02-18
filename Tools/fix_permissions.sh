#!/bin/bash
# fix permissions on cygwin
#
find . -type f -exec chmod 644 {} +
chmod 755 Tools/fix_permissions.sh
chmod 755 Tools/add_version_info/add_version_info.py
chmod 755 Tools/tablegen/{gamma_tab.py,ws2812_tab.py}
chmod 755 Tools/st-link/*.exe
chmod 755 Tools/st-link/*.dll

