

#pragma once 







#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
  

#define FATAL_RETURN do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); return(-1); } while(0)

 
#if defined(DEBUG)
        #define DEBUG_PRINT(fmt, args...) printf("DEBUG %s %s:%d(): " fmt, \
                __FILE__,__func__, __LINE__, ##args)
#else
        #define DEBUG_PRINT(fmt, args...) /* do nothing */
#endif

//#define DUPPRINT(fd, fmt,args...) do {FILE *fp;DEBUG_PRINT(fmt,##args);fp = fdopen(fd, "a");fprintf(fp,fmt,##args);fclose (fp);} while(0)
#define DUPPRINT(fd, fmt,args...) do {char p[300];int aaa;DEBUG_PRINT(fmt,##args);aaa=snprintf(p,300,fmt,##args);write(fd,p,aaa);} while(0)
//#define DUPPRINT(fp, fmt,args...) do {DEBUG_PRINT(fmt,##args);} while(0)