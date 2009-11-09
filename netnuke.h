#ifndef NETNUKE_H
#define NETNUKE_H

/* Prototypes */
void fillRandom(char buffer[], uint64_t length);
void staticPattern() __attribute__((alias("fillRandom")));
uint64_t getSize(const char* media);
int open_device(const char *media);
int close_device(int fd);
//int recycle_device(const char* media, int fd);
void echoList(void);
void usage(const char* cmd);
void version_short(void);
void version(const char* cmd);
void clearline(void);
void cleanup();
void skip();
#ifndef __FreeBSD__
int humanize_number(char *buf, size_t len, int64_t bytes,
          const char *suffix, int scale, int flags);
#endif

/* log.c */
int logopen(const char* logfile);
void logclose(void);
int lwrite(char* format, ...);

/* Defines */
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

#define NETNUKE_VERSION_STRING "NetNuke v%d.%d-%s\nCopyright (C) 2009  %s <%s>\n\
This software is licensed under %sv%d\n",\
        NETNUKE_VERSION_MAJOR, NETNUKE_VERSION_MINOR,\
        NETNUKE_VERSION_REVISION, NETNUKE_AUTHOR,\
        NETNUKE_AUTHOR_EMAIL, NETNUKE_LICENSE_TYPE,\
        NETNUKE_LICENSE_VERSION 

/* Per iteration of devices, how many should we test for */
#define MAX_SCAN 255
/* Output update speed based on writes */
#define RETAINER 0 

/* Used to assist argument parsing */
#define ARGMATCH(arg) strncmp(argv[tok], arg, strlen(arg)) == 0 
#define ARGNULL(arg) if(argv[tok arg] == NULL) exit(1);
#define ARGVALINT(ref) tok++; ref = atoi(argv[tok])
#define ARGVALSTR(ref) tok++; ref = argv[tok];

/* Flags for filterArgs */
#define NOZERO 0
#define NONEGATIVE 2
#define NEEDNUM 4
#define NEEDSTR 8

#ifndef __FreeBSD__
#define DISK_IDENT_SIZE BUFSIZ
#endif

#define USABLE_MEDIA 0

/* Enumerated lists */
typedef enum nlevel
{
   NUKE_ZERO=0,
   NUKE_PATTERN,
   NUKE_RANDOM_FAST,
   NUKE_RANDOM_SLOW,
   NUKE_REWRITE
} nukeLevel_t;

typedef struct MEDIASTAT_T
{
   int32_t total;
   int32_t ide;
   int32_t scsi;
   int32_t unknown;
} mediastat_t;

typedef struct MEDIA_T
{
   int usable;
   uint64_t size;
   char name[30];
   char nameshort[10];
   char ident[DISK_IDENT_SIZE];
} media_t;
int32_t nuke(media_t device);
void buildMediaList(media_t devices[]);
media_t getMediaInfo(const char* media);


#endif /* NETNUKE_H */
