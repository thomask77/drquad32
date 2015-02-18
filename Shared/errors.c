/**
 * User defined extension for newlib's strerror() function
 *
 * Copyright (c)2015 Thomas Kindler <mail_drquad@t-kindler.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "errors.h"
#include <stdio.h>


const char *_user_strerror(int errnum)
{
    static char buf[80];

    switch (errnum) {

    #define _ERR_MAKE_STRING_TABLE
    #define _ERR_GROUP(id, value)
    #define _ERR_NAME(id, desc)  case id: return desc;

    #include "errors.h"

    default:
        sprintf(buf, "Unknown error %d", errnum);
        return buf;
    }

    return NULL;
}
