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
    switch (errnum) {
        #define ERROR_ID_CASE_FN(ID, NAME, HELP) \
        case ID: return HELP;

        ERROR_ID_MAP(ERROR_ID_CASE_FN)
    }

    return NULL;
}
