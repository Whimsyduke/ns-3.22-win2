#include <time.h>
#include <ctime>
#include <windows.h>
///*
//* Structure used in select() call, taken from the BSD file sys/time.h.
//*/
//struct timeval {
// long    tv_sec;         /* seconds */
// long    tv_usec;        /* and microseconds */
//};
#ifdef WIN32
//如果你的工程中的数据结构没有和windows.h中包含的相关数据结构重复，可以使用这段代码，不用使用time.cpp
void gettimeofday(struct timeval *tp, void *tzp);
/*{
    time_t clock;
   
 struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year     = wtm.wYear - 1900;
    tm.tm_mon     = wtm.wMonth - 1;
    tm.tm_mday     = wtm.wDay;
    tm.tm_hour     = wtm.wHour;
    tm.tm_min     = wtm.wMinute;
    tm.tm_sec     = wtm.wSecond;
    tm. tm_isdst    = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
}*/
void timersub(struct timeval *endPre, struct timeval *beginPre, struct timeval *result);
void timeradd(struct timeval *endPre, struct timeval *beginPre, struct timeval *result);
#endif /* WIN32 */