/**
 *  NetNuke - Erases all storage media detected by the system
 *  Copyright (C) 2009  Joseph Hunkeler <jhunkeler@gmail.com, jhunk@stsci.edu>
 *
 *  This file is part of NetNuke.
 *
 *  NetNuke is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NetNuke is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NetNuke.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#ifndef CLK_TCK
	#define CLK_TCK CLOCKS_PER_SEC
#endif
#include <ctype.h>

FILE* loutfile;
static clock_t ltime_start;
static clock_t ltime_current;
static int logline;

int logopen(const char* logfile)
{
    logline = 0;
    ltime_start = clock() * CLK_TCK;
    loutfile = fopen(logfile, "wa+");

    if(loutfile == NULL)
    {
        printf("Cannot not write to log file: %s\n", logfile);
        return 1;
    }

    fseek(loutfile, 0, SEEK_SET);
    return 0;
}

void logclose()
{
    fclose(loutfile);
}

int lwrite(char *format, ...)
{
    ltime_current = clock() * CLK_TCK;

    char *str = (char*)malloc(sizeof(char) * 256);
    char tmpstr[256];
    int n;

    va_list args;
    va_start (args, format);
    n = vsprintf (str, format, args);
    va_end (args);

    float seconds = (ltime_current - ltime_start) / 1000;

    snprintf(tmpstr, 255, "[%d.%0.0f]  %s", logline, seconds, str);
    fprintf(loutfile, tmpstr);
    logline++;

    /* I am aware of the implications of using fflush constantly. */
    fflush(loutfile);
    free(str);
    return 0;
}
