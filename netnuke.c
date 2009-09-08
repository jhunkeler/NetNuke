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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>
#include <time.h>
#ifdef __FreeBSD__
   #include <libutil.h>
   #include <sys/disk.h>
#else
   #include "human_readable.h"
   #include <sys/ioctl.h>
   #include <linux/fs.h>
#endif

/* Global variables */
int32_t error;
/* List of media types that we can nuke */
#ifdef __FreeBSD__
   const char* mediaList[17] = {
      "ad",  //ATAPI
      "da",  //SCSI
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
   const char* mediaList[3] = {
      "hd",  //ATAPI
      "sd",  //SCSI
   };
#endif

/* Prototypes */
void fillRandom(int32_t buffer[], uint64_t length);
int32_t nuke(const char* media, uint64_t size);
uint64_t getSize(const char* media);
void echoList(void);
int humanize_number(char *buf, size_t len, int64_t bytes,
          const char *suffix, int scale, int flags);


void fillRandom(int32_t buffer[], uint64_t length)
{
   uint32_t random, random_count;

   /* Initialize random seed */
   srand(time(NULL) * time(NULL) / 3 + 6201985 * 3.14159); 
   /* Fills the write buffer with random garbage */
   int32_t linebreak = 0;

   for(random_count = 0; random_count < length; random_count++)
   {
      random = rand() % RAND_MAX;
      buffer[random_count] = random;
      /*
      printf("0x%0.8X  ", (char)random);

      if(linebreak == 5)
      {
         printf("\n");
         linebreak = -1;
      }
      */
      linebreak++;
   }
   //printf("\n");
}

int nuke(const char* media, uint64_t size)
{
   /* test with 1G worth of data */
   size = (1024 * 1024) * 1000;

   char mediaSize[BUFSIZ];
   char writeSize[BUFSIZ];
   char writePerSecond[BUFSIZ];
   uint64_t byteSize = 1024;
   uint64_t times, block;
   int32_t  wTable[byteSize];
   uint32_t startTime, currentTime, endTime; 
   int32_t retainer = 0;

   int fd = open("/dev/null", O_WRONLY | O_ASYNC);
   if(!fd)
   {
      perror("nuke"); 
      exit(1);
   }

   /* Generate a size string based on the media size. example: 256M */
   humanize_number(mediaSize, 5, (uint64_t)size, "", 
      HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
   
   printf("Wiping %s: %ju (%s)\n", media, (intmax_t)size, mediaSize);
   /* Determine how many writes and at what byte size */
   times = size / byteSize;
   /* Dump random garbage to the write table */
   fillRandom(&wTable, byteSize);

   startTime = time(NULL);
   for( block = 0 ; block <= times ; block++)
   {
      currentTime = time(NULL);
      long double bytes = (float)(size / times * block);

      error = write(fd, (char*)wTable, sizeof(wTable));  
      
      switch(error)
      {
         case EIO:
         {
            int64_t blockPosition = (int64_t)lseek(fd, -1, SEEK_CUR);
            printf("I/O Error: Unable to write to block \"%jd\"\n", blockPosition);
         }
         break;

         default:
            break;
      };
      
      /* Generate a size string based on bytes written. example: 256M */
      humanize_number(writeSize, 5,
            bytes, "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

      /* Generate a size string based on writes per second. example: 256M */
      humanize_number(writePerSecond, 5,
            (intmax_t)((long double)bytes / ((long double)currentTime - (long double)startTime)), "", 
             HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

      /* Save I/O by printing our progress every X iterations with a retainer*/
      if(retainer > 128 || block == times)
      {
         /* Output our progress */
         printf("\t%jd of %jd blocks    [ %s / %3.1Lf%% / %s/s ]\r", 
               block, 
               times,
               writeSize,
               (bytes / (long double)size) * 100L,
               writePerSecond
            );
         retainer = -1;

         /* Recycle the write table with random garbage */
         fillRandom(&wTable, byteSize);
      }
      ++retainer;
   }
   endTime = time(NULL);

   putchar('\n');
   close(fd);
   return 0;
}

void echoList()
{
   int i = 0;
   int mt = 0;
   int mediaFound = 0;

   do
   {
      /* Media size */
      uint64_t size;
      /* Holds the media type, and number. example: ad0 */
      char media[BUFSIZ];
      char mediaShort[BUFSIZ];

      /* Generate a device string based on the current interation*/
#ifdef __FreeBSD__
      sprintf(media, "/dev/%s%d", mediaList[mt], i);
#else
      sprintf(media, "/dev/%s%c", mediaList[mt], 'a' + i);
#endif

      /* Set media size */
      size = getSize(media);

      /* We MUST use the int64_t cast.  Unsigned integers cannot have a negative value.
       * Otherwise this loop would never end. */
      if((int64_t)size > 0L)
      {
         strncpy(mediaShort, &media[5], strlen(&media[5]));
         printf("%s: %jd\n", mediaShort , (intmax_t)size);
         mediaFound++;
      }
      else
      {
         /* To prevent overrunning */
         if(mediaList[mt] == NULL || mediaList[mt] == '\0')
            break; 

         /* mediaList iteration */
         mt++;
         /* Reset iteratior so that the next increment is zero */
         i = -1;
      }

      i++;

   } while( 1 );

   printf("%d device%c detected\n", mediaFound, mediaFound < 1 || mediaFound > 1 ? 's' : 0 );
   putchar('\n');
}


/* Function: getSize
 * Argument 1: Fully qualified path to device 
 * Return: Media size, or -1 on failure
 */
uint64_t getSize(const char* media)
{  
   /* Media size */
   uint64_t size;
   /* Attempt to open the media read-only */
   int fd = open(media, O_RDONLY);

   if(fd)
   {
      /* Set media size */
#ifdef __FreeBSD__
      error = ioctl(fd, DIOCGMEDIASIZE, &size);
#else
      error = ioctl(fd, BLKGETSIZE, &size);
      /* For Linux we must adjust the size returned, because we want bytes, not blocks */
      size *= 512; 
#endif
   }
   else
      exit(1);

   if(error != 0)
   {
      /* Errors will be completely ambiguous due to the nature of this
       * program. */
      return -1;
   }

   /* Close the media file descriptor */
   close(fd);

   /* Return the size of the media */
   return size;
}

int main(int argc, char* argv[])
{

   int i = 0; 
   int mt = 0;

   echoList();

   do
   {
      /* Media size */
      uint64_t size;
      /* Holds the media type, and number. example: ad0 */
      char media[BUFSIZ];
         
      /* Generate a device string based on the current interation*/
#ifdef __FreeBSD__
      sprintf(media, "/dev/%s%d", mediaList[mt], i);
#else
      sprintf(media, "/dev/%s%c", mediaList[mt], 'a' + i);
#endif
      /* Set media size */
      size = getSize(media);

      /* We MUST use the int64_t cast.  Unsigned integers cannot have a negative value.
       * Otherwise this loop would never end. */
      if((int64_t)size > 0L)
      {
         //printf("%s: %jd (%s)\n", media, (intmax_t)size, buf); 
         nuke(media, size);
      }
      else
      {
         /* To prevent overrunning */
         if(mediaList[mt] == NULL || mediaList[mt] == '\0')
            break; 

         /* mediaList iteration */
         mt++;
         /* Reset iteratior so that the next increment is zero */
         i = -1;
      }

      i++;

   } while( 1 );

   /* End */
   return 0;
}

