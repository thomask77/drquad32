"""
Utility functions

Copyright (c)2000-2012 Thomas Kindler <mail@t-kindler.de>

2012-09-29: tk, ported to python
2009-05-14: tk, added some shell commands
2009-04-22: tk, added crc32 and statistics function
2005-11-30: tk, improved hexdump with ascii output
2002-01-10: tk, added strnbar function for progress bars
2000-04-24: tk, initial implementation

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

"""


def progressbar(value, minimum=0, maximum=100, width=20):
    n = round(float(value - minimum) * width / (maximum - minimum))
    s = bytearray(" " * width)

    for i in range(width):
        if i < n:
            s[i] = "="

    if n < 0:
        s[0] = "<"

    if n > width:
        s[width-1] = ">"

    return str(s)


def isprint(c):
    return 32 <= ord(c) < 128


def hexdump(data):
    s = "       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    0123456789ABCDEF\n"

    for i in xrange(0, len(data), 16):
        s += "%04x:  " % i

        for j in xrange(i, i+16):
            s += "%02X" % ord(data[j]) if j < len(data) else "  "
            s += " " if j & 1 else "-"

        s += "  "
        for j in xrange(i, i+16):
            if j < len(data):
                s += data[j] if isprint(data[j]) else "."
            else:
                s += " "

        s += "\n"

    return s
