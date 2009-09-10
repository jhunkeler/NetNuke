#ifndef NETNUKE_H
#define NETNUKE_H

#define NETNUKE_VERSION_MAJOR 1
#define NETNUKE_VERSION_MINOR 0
#ifdef _SVN_SUPPORT
    #define NETNUKE_VERSION_REVISION SVN_REV
#else
    #define NETNUKE_VERSION_REVISION "0"
#endif
#define NETNUKE_AUTHOR "Joseph Hunkeler"
#define NETNUKE_AUTHOR_EMAIL "jhunk@stsci.edu"
#define NETNUKE_LICENSE_TYPE "GNU/GPL"
#define NETNUKE_LICENSE_VERSION 3
#define NETNUKE_LICENSE_PREAMBLE "\
    This program is free software: you can redistribute it and/or modify\n\
    it under the terms of the GNU General Public License as published by\n\
    the Free Software Foundation, either version 3 of the License, or\n\
    (at your option) any later version.\n\n\
    This program is distributed in the hope that it will be useful,\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
    GNU General Public License for more details.\n\n\
    You should have received a copy of the GNU General Public License\n\
    along with this program.  If not, see <http://www.gnu.org/licenses/>."


#endif /* NETNUKE_H */
