/**
 *  NetNuke - Erases all storage media deteced by the system
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
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>
#include <time.h>
#ifdef __FreeBSD__
   #include <libutil.h>
   #include <sys/disk.h>
   #include <termcap.h>
#else
   #include "human_readable.h"
   #include <sys/ioctl.h>
   #include <linux/fs.h>
#endif

#include "netnuke.h"

/* Global variables */
nukeLevel_t udef_nukelevel = NUKE_PATTERN; /* Static patterns is default */
bool udef_verbose = false;
bool udef_verbose_high = false;
int8_t udef_wmode = 0; /* O_SYNC is default */
int32_t udef_passes = 1;
bool udef_testmode = true; /* Test mode should always be enabled by default. */
int32_t udef_blocksize = 512; /* 1 block = 512 bytes*/

media_t *devices;
mediastat_t device_stats;

/* Static pattern array */
const char sPattern[] = {
	0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0,
	0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1,
	0xA2, 0xB2, 0xC2, 0xD2, 0xE2, 0xF2,
	0xA3, 0xB3, 0xC3, 0xD3, 0xE3, 0xF3
};

/* List of media types that we can nuke */
#ifdef __FreeBSD__
   const char* mediaList[18] = {
      "ad",  //ATAPI
      "da",  //SCSI
      "sa",  //SCSI Tape device
      "adv", //AdvanSys Narrow
      "adw", //AdvanSys Wide
      "amd", //AMD 53C974 (Tekram DC390(T))
      "ncr", //NCR PCI
      "bt",  //BusLogic
      "aha", //Adaptec 154x/1535
      "ahb", //Adaptec 174x
      "ahc", //Adaptec 274x/284x/294x
      "aic", //Adaptec 152x/AIC-6360/AIC-6260
      "isp", //QLogic
      "dpt", //DPT RAID
      "amr", //AMI MegaRAID
      "mlx", //Mylex DAC960 RAID
      "wt",  //Wangtek and Archive QIC-02/QIC-36
   };
#else
   const char* mediaList[5] = {
      "hd",  //ATAPI
      "sd",  //SCSI
      "sg",  //SCSI Generic
      "st",  //SCSI Tape
   };
#endif


void fillRandom(char buffer[], uint64_t length)
{
   uint32_t random = 0, random_count = 0;
   int32_t linebreak = 0;

   /* Initialize random seed */
   srand(time(NULL) * time(NULL) / 3 + 6201985 * 3.14159); 

   /* Fills the write buffer with random garbage */
   for(random_count = 0; random_count < length; random_count++)
   {
      /* Define a single static pattern */
      if(udef_nukelevel == NUKE_PATTERN)
      {
              random = rand() % strlen(sPattern) + 1;
              //printf("size = %d  STATIC = ### %x ###\n", strlen(sPattern), (int)random);
              buffer[random_count] = sPattern[random];
      }      

      if(udef_nukelevel == NUKE_RANDOM_SLOW || udef_nukelevel == NUKE_RANDOM_FAST)
      {
         random = rand() % RAND_MAX;
         //printf("RANDOM = ### %x ###\n", random);
         buffer[random_count] = random;
      }
	  
      /* This is a debug feature to prove the random generator is functioning */
      if(udef_verbose_high)
      {
         printf("0x%08X  ", (char)random);
              
         if(linebreak == 5)
         {
            putchar('\n');
            linebreak = -1;
         }

         linebreak++;
      }
   }
   if(udef_verbose_high)
      putchar('\n'); 

}

int nuke(media_t device)
{
   uint64_t size = device.size;
   char media[BUFSIZ]; 
   char mediashort[BUFSIZ];
   strncpy(media, device.name, strlen(device.name));
   strncpy(mediashort, device.nameshort, strlen(device.nameshort));

   /* test with 100MBs worth of data */
   if(udef_testmode == true)
      size = (1024 * 1024) * 100;
   
   errno = 0;
   char mediaSize[BUFSIZ];
   char writeSize[BUFSIZ];
   char writePerSecond[BUFSIZ];
   int fd;
   int32_t pass;
   uint64_t byteSize = udef_blocksize;
   uint64_t bytesWritten = 0L;
   uint64_t times, block;
   char wTable[byteSize];
   uint32_t startTime, currentTime, endTime; 

   if(wTable == NULL)
   {
      fprintf(stderr, "Could not allocate write table buffer at size %jd\n", byteSize);  
      return 1;
   }

   /* Set the IO mode */
   int O_UFLAG = udef_wmode ? O_ASYNC : O_SYNC;
   if(udef_testmode == true)
          sprintf(media, "%s", "/tmp/testmode.img");

   /* Generate a size string based on the media size. example: 256M */
   humanize_number(mediaSize, 5, (uint64_t)size, "", 
      HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
   printf("Wiping %s: %ju bytes (%s)\n", media, (intmax_t)size, mediaSize);
   
   /* Dump random garbage to the write table */
   if(udef_nukelevel == NUKE_RANDOM_SLOW || udef_nukelevel == NUKE_RANDOM_FAST)
	   fillRandom(wTable, byteSize);
   else if(udef_nukelevel == NUKE_ZERO)
	   memset(wTable, 0, byteSize);
   else
	   staticPattern(wTable, byteSize);

   /* Begin write passes */
   for( pass = 1; pass <= udef_passes ; pass++ )
   {
#ifdef __FreeBSD__
      fd = open(media, O_RDWR | O_TRUNC | O_UFLAG | O_DIRECT, 0700 );
#else
      /* Linux no longer supports O_DIRECT */
      fd = open(media, O_RDWR | O_TRUNC | O_UFLAG, 0700 );
#endif
      
      if(errno != 0)
      {
         perror("nuke");
         fprintf(stderr, "Skipping %s\n", media);
         /* The reason we stopped exiting the program here is because other 
          * devices still may  have to be wiped.  Just skip it */
         break;
      }      

      if((lseek(fd, 0L, SEEK_SET)) != 0)
      {
         fprintf(stderr, "\nCould not seek to the beginning of the device.\n");
         break;
      }

      /* Determine how many writes to perform, and at what byte size */
      times = size / byteSize;
      
      startTime = time(NULL);
      for( block = 0 ; block <= times; block++)
      {
         currentTime = time(NULL);
         long double bytes = (float)(size / times * block);

	 /* Generate a size string based on bytes written. example: 256M */
	 humanize_number(writeSize, 5,
	    bytes, "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
         
         /* Generate a size string based on writes per second. example: 256M */
	 humanize_number(writePerSecond, 5,
            (intmax_t)((long double)bytes / ((long double)currentTime - (long double)startTime)), "", 
            HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

         printf("%s: ", (char*)&device.nameshort);

         if(udef_passes > 1)
            printf("pass %d ", pass);

         /* Output our progress */
         printf("\t%jd of %jd blocks    [ %s / %3.1Lf%% / %s/s ]%c", 
               block, 
               times,
               writeSize,
               (bytes / (long double)size) * 100L,
               writePerSecond,
               0x0D //ANSI carriage return
               );

         /* Recycle the write table with random garbage */
         if(udef_nukelevel == NUKE_RANDOM_SLOW)
            fillRandom(wTable, byteSize); 

         /* Break out if we have written all of the data */
         if(block >= times)
         {
             break;
         }

         /* Dump data to the device */
         bytesWritten = write(fd, wTable, byteSize); 

         if(bytesWritten != byteSize)
            fprintf(stderr, "\nwrite() function is not returning the correct size.\n");

      } /* BLOCK WRITE */
      
      endTime = time(NULL);
      close(fd);
   } /* PASSES */

   putchar('\n');
   return 0;
}

void buildMediaList(media_t devices[])
{
   device_stats.total = 0;
   device_stats.ide = 0;
   device_stats.scsi = 0;
   device_stats.unknown = 0;

   int i = 0;
   int mt = 0;

   if(udef_verbose)
      printf("\n--Device List--\n");

   do
   {
      char media[BUFSIZ];

      /* Generate a device string based on the current interation*/
#ifdef __FreeBSD__
      sprintf(media, "/dev/%s%d", mediaList[mt], i);
#else
      sprintf(media, "/dev/%s%c", mediaList[mt], 'a' + i);
#endif

      media_t device = getMediaInfo(media);

#ifdef __FreeBSD__
      /* Account for SATA devices.  mediaList will always have IDE/SATA as position 0 */
      if(device.usable != USABLE_MEDIA && mt == 0 && i > 5)
         mt++;
#endif

      /* Primative statistics collection, also in this case device.usable 
       * must be explicitely checked */
      if(device.usable == USABLE_MEDIA) 
      {
         /* This is not the only way to do this.  Especially because it is NOT
          * dynamic to the mediaList at all... */
         if(strstr(device.name, "ad") == 0 || 
               strstr(device.name, "hd") == 0)
         {
            device_stats.ide++;
         }

         else if(strstr(device.name, "da") == 0 || 
               strstr(device.name, "sd") == 0)
         {
            device_stats.scsi++;
         }
         
         device_stats.total = 
            device_stats.ide + device_stats.scsi;

         /* Assign device to the array of devices */
         if(device_stats.total > 0)
         {
            devices[device_stats.total-1] = device;
         }
         else
         {
            device_stats.total = 0;
            devices[device_stats.total] = device;
         }

         if(udef_verbose)
         {
#ifdef __FreeBSD__
            printf("%s:\t%s:\t%jd bytes\n", device.nameshort, device.ident, device.size);
#else
            printf("%s:\t%jd bytes\n", device.nameshort, device.size);
#endif    
         }
      }
      else
      {
         /* To prevent overrunning the list, causing a segfault */
         if(mediaList[mt] == NULL || mediaList[mt] == '\0')
            break; 

         /* Increment the mediaList iterator */
         mt++;

         /* Reset iterator so that the next increment is zero */
         i = -1;
      }

      i++;

   } while( 1 );
   
   if(udef_verbose)
      putchar('\n');
}


media_t getMediaInfo(const char* media)
{
   int fd;
   media_t mi;

   /* Set defaults */
   mi.usable = !USABLE_MEDIA;
   mi.size = 0;
   mi.name[0] = '\0';
   mi.nameshort[0] = '\0';
   mi.ident[0] = '\0';

   /* Open media read-only and extract information using ioctl */
   fd = open(media, O_RDONLY);
   if(fd < 0)
   {
      /* Returns in an unusable state */
      return mi;
   }

#ifdef __FreeBSD__
   if((ioctl(fd, DIOCGMEDIASIZE, &mi.size)) != 0)
#else
   if((ioctl(fd, BLKGETSIZE, &mi.size)) != 0)
#endif
   {
      /* Returns in an unusable state */
      return mi;
   }

#ifdef __FreeBSD__
   if((ioctl(fd, DIOCGIDENT, mi.ident)) != 0)
      mi.ident[0] = '\0';
#endif

   memcpy(mi.name, media, strlen(media)+1);
   memcpy(mi.nameshort, &media[5], strlen(media));

   /* Mark the media as usuable or unusable */
   if(mi.size > 0)
      mi.usable = USABLE_MEDIA;
   else
      mi.usable = !USABLE_MEDIA;

   close(fd);

   /* Should be in a usuable state if it made it this far */
   return mi;
}

void usage(const char* cmd)
{
   printf("usage: %s [options] ...\n", cmd);
   printf("--help            -h       This message\n");
   printf("--write-mode s    -w  s    Valid values:\n\
                              0: Synchronous (default)\n\
                              1: Asynchronous\n");
   printf("--nuke-level n    -nl n    Varying levels of destruction:\n\
                              0: Zero out (quick wipe)\n\
                              1: Static patterns (0xA, 0xB, ...)\n\
                              2: Fast random (single random buffer across media)\n\
                              3: Slow random (regenerate random buffer)\n\
                              4: Ultra-slow re-writing method\n");
   printf("--passes n        -p  n    Number of wipes to perform on a single device\n");
   printf("--disable-test             Disables test-mode, and allows write operations\n");
   printf("--block-size n    -b  n    Blocks at once\n");
   printf("--verbose         -v\n");
   printf("--verbose-high    -v       Debug level verbosity\n");
   printf("--version         -V\n");
   putchar('\n');
}

void version_short()
{
    printf("NetNuke v%d.%d-%s\nCopyright (C) 2009  %s <%s>\n\
This software is licensed under %sv%d\n",
	NETNUKE_VERSION_MAJOR, NETNUKE_VERSION_MINOR,
	NETNUKE_VERSION_REVISION, NETNUKE_AUTHOR, 
	NETNUKE_AUTHOR_EMAIL, NETNUKE_LICENSE_TYPE,
	NETNUKE_LICENSE_VERSION);
}

void version(const char* cmd)
{
    printf("NetNuke v%d.%d-%s\n \
           Copyright (C) 2009  %s <%s>\n \
           This software is licensed under %sv%d\n\n%s\n\n",
                NETNUKE_VERSION_MAJOR, NETNUKE_VERSION_MINOR,
                NETNUKE_VERSION_REVISION, NETNUKE_AUTHOR,
                NETNUKE_AUTHOR_EMAIL, NETNUKE_LICENSE_TYPE,
                NETNUKE_LICENSE_VERSION, NETNUKE_LICENSE_PREAMBLE
                );
   printf("NOTE: humanize_number was ported from NetBSD to function on Linux.\n");
   printf("That source code (from libutil) is licensed under the BSD software license.\n\n");
}

int filterArg(const char* key, char* value, short flags)
{
   int test;
   int length = strlen(value);
 
   if((flags & NONEGATIVE) > 0)
   {
      if((atoi(value) < 0))
      {
         printf("argument %s received a negative number\n", key);
         exit(1);
      }
   }

   if((flags & NOZERO) > 0)
   {
      if((atoi(value) == 0))
      {
         printf("argument %s received zero.  not allowed.\n", key);
         exit(1);
      }
   }


   if((flags & NEEDNUM) > 0)
   {
      for(test = 0; test < length; test++)
      {
         if(!isdigit(value[test]))
         {
            printf("argument %s did not receive an integer\n", key);
            exit(1);
         }
      }
   }

   return 0;
}

int main(int argc, char* argv[])
{
   /* ANSI clear-screen sequence */
   //printf("\033[2J");
   int tok = 0;

   /* Static arguments that must happen first */
   for(tok = 0; tok < argc; tok++)
   {
      if(ARGMATCH("--help") || ARGMATCH("-h"))
      {
         usage(argv[0]);
         exit(0);
      }
      if(ARGMATCH("--version") || ARGMATCH("-V"))
      {
         version(argv[0]);
         exit(0);
      }
      if(ARGMATCH("--verbose") || ARGMATCH("-v"))
      {
         udef_verbose = true;
      }
      if(ARGMATCH("--verbose-high") || ARGMATCH("-vv"))
      {
         udef_verbose_high = true;
         udef_verbose = true;
      }
   }

   /* Dynamic arguments come second */
   for(tok = 0; tok < argc; tok++)
   {

      if(ARGMATCH("--write-mode") || ARGMATCH("-w"))
      {
         ARGNULL(+1);
         if(filterArg(argv[tok], argv[tok+1], NONEGATIVE|NEEDNUM) == 0)
         {
            ARGVALINT(udef_wmode);
         }
      }
      if(ARGMATCH("--nuke-level") || ARGMATCH("-n"))
      {
         ARGNULL(+1);
         if(filterArg(argv[tok-1], argv[tok+1], NONEGATIVE|NEEDNUM) == 0)
         {;
            ARGVALINT(udef_nukelevel);
            if(udef_nukelevel > NUKE_REWRITE)
	       udef_nukelevel = NUKE_PATTERN;
	 }
	 /* TODO: Remove this when it is implemented! */
	 if(udef_nukelevel == NUKE_REWRITE)
	 {
	    printf("*** Rewrite mode is not implemented, using default.\n");
	    udef_nukelevel = NUKE_PATTERN;
	 }
      }
      if(ARGMATCH("--passes") || ARGMATCH("-p"))
      {
         ARGNULL(+1);
         if(filterArg(argv[tok-1], argv[tok+1], NEEDNUM) == 0)
         {
            ARGVALINT(udef_passes);
            if(udef_passes < 1)
               udef_passes = 1;
         }
      }
      if(ARGMATCH("--disable-test"))
      {
         udef_testmode = false;
      }
      if(ARGMATCH("--block-size") || ARGMATCH("-b"))
      {
         ARGNULL(+1);
         if(filterArg(argv[tok-1], argv[tok+1], NONEGATIVE|NOZERO|NEEDNUM) == 0)
         {
            ARGVALINT(udef_blocksize);
         }
      }

      if(argv[tok+1] == NULL)
      {
         //usage(argv[0]);
         //exit(1);
         //break;
      }
   }

   /* Check for root privs before going any further */
   if((getuid()) != 0)
   {
      fprintf(stderr, "You must be root first, sorry.\n");
      exit(3);
   }

   int i = 0; 

   signal(SIGINT, cleanup);
   signal(SIGTERM, cleanup);
   signal(SIGABRT, cleanup);
   signal(SIGILL, cleanup);

   version_short();
   putchar('\n');

   if(udef_verbose)
   {
      char* nlstr = {0};
      switch(udef_nukelevel)
      {
         case NUKE_ZERO:
            nlstr = "Zeroing";
            break;
         case NUKE_PATTERN:
            nlstr = "Pattern";
            break;
         case NUKE_RANDOM_SLOW:
            nlstr = "Slow Random";
            break;
         case NUKE_RANDOM_FAST:
            nlstr = "Fast Random";
            break;
         default:
            nlstr = "Unknown";
            break;
       }

       printf("Test mode:\t%s\n", udef_testmode ? "ENABLED" : "DISABLED");
       printf("Block size:\t%d\n", udef_blocksize);
       printf("Wipe method:\t%s\n", nlstr);
       printf("Num. of passes:\t%u\n", udef_passes);
       printf("Write mode:\t%cSYNC\n", udef_wmode ? 'A' : 0);
   }

   /* Allocate base memory for the device array */
   devices = (media_t*)calloc(BUFSIZ, sizeof(media_t));
   if(devices == NULL)
   {
     fprintf(stderr, "Could not allocate %d bytes of memory for devices array.\n", BUFSIZ * sizeof(media_t));
      exit(1);
   }

   /* Fill the devices array */
   buildMediaList(devices);

   /* Allocate the correct amount of memory based on the total number of devices */
   devices = (media_t*)realloc(devices, device_stats.total * sizeof(media_t));
   if(devices == NULL)
   {
      printf("Could not reallocate %d bytes of memory for devices array.\n", device_stats.total * sizeof(media_t));
      exit(1);
   }

   printf("IDE Devices:\t%d\nSCSI Devices:\t%d\nTotal Devices:\t%d\n", 
         device_stats.ide, device_stats.scsi, device_stats.total);
   putchar('\n');

   do
   {
      if(devices[i].usable == USABLE_MEDIA && devices[i].name[0] != '\0' && i <= device_stats.total)
      {
         /* Pass control off to the nuker */
         nuke(devices[i]);
      }
      else
         break;

      i++;

   } while( 1 );

   /* Free allocated memory */
   free(devices);

   /* End */
   return 0;
}

void cleanup()
{
   fprintf(stderr, "\nSignal caught, cleaning up...\n");

#ifdef __FreeBSD__
   char terminalBuffer[2048];
   char* terminalType = getenv("TERM");

   if(tgetent(terminalBuffer, terminalType) < 0)
   {
      fprintf(stderr, "Could not access termcap database.\n");
   }

   int columns = tgetnum("co");
   int col = 0;
#else
   /* It appears the termcap library under Linux does not function correctly */
   int columns = 80;
   int col = 0;
#endif

   for(col = 0; col < columns; col++)
   {
      printf("%c", 0x20);
   }
   putchar('\n');

   free(devices);

   exit(2);
}
