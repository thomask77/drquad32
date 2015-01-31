/**
 * User defined extension for newlib's strerror() function
 *
 * Copyright (C)2015 Thomas Kindler <mail_drquad@t-kindler.de>
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
#include "small_printf.h"

const char *_user_strerror(int errnum)
{
    static char buf[80];

    switch (errnum) {

    #undef ERRORS_H
    #define MAKE_ERROR_TABLE
    #define ENUM(name, desc)    case name: return desc;
    #include "errors.h"

    default:
        sprintf(buf, "Unknown error %d", errnum);
        return buf;
    }

    return NULL;
}
