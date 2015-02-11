#include "time.h"
#define CLKS_PER_SEC ((clock_t)1000)
void gettimeofday(struct timeval *tp, void *tzp)
{
 //time_t clock;
 //tm ttime;
 struct tm *newtime;
 time_t aclock;

 time_t clockn;
 time( &aclock );   // Get time in seconds
 newtime = localtime( &aclock );   // Convert time to struct tm form
 clockn = mktime(newtime);
 tp->tv_sec = clockn;
 double uS = (clock() / CLKS_PER_SEC) * 1000000;
 //tp->tv_usec = wtm.wMilliseconds * 1000;
 //struct tm tm;
  //  SYSTEMTIME wtm;
 //   GetLocalTime(&wtm);
 //   tm.tm_year     = wtm.wYear - 1900;
 //   tm.tm_mon     = wtm.wMonth - 1;
 //   tm.tm_mday     = wtm.wDay;
 //   tm.tm_hour     = wtm.wHour;
 //   tm.tm_min     = wtm.wMinute;
 //   tm.tm_sec     = wtm.wSecond;
 //   tm. tm_isdst    = -1;
 //   clock = mktime(&tm);
 //   tp->tv_sec = clock;
 //   tp->tv_usec = wtm.wMilliseconds * 1000;
}

void timersub(struct timeval *endPre, struct timeval *beginPre, struct timeval *result)
{
 do
 {                                                                        
  (result)->tv_sec = (endPre)->tv_sec - (beginPre)->tv_sec;                             
  (result)->tv_usec = (endPre)->tv_usec - (beginPre)->tv_usec;                          
  if ((result)->tv_usec < 0)
  {                                              
   --(result)->tv_sec;                                                     
   (result)->tv_usec += 1000000;                                           
  }                                                                         
 }
 while (0);
}
void timeradd(struct timeval *endPre, struct timeval *beginPre, struct timeval *result)
{
 do
 {                                                                        
  (result)->tv_sec = (endPre)->tv_sec + (beginPre)->tv_sec;                             
  (result)->tv_usec = (endPre)->tv_usec + (beginPre)->tv_usec;                          
  if ((result)->tv_usec >= 1000000)
  {                                              
   ++(result)->tv_sec;                                                     
   (result)->tv_usec -= 1000000;                                           
  }                                                                         
 }
 while (0);
}