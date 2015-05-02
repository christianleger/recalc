/*
    File: utils.h


    Description: this file provides useful functions to help our program. 



    Modules defined here: 
        
            - time measurement 

            - debugging tools

            - logging tools

*/

#ifndef __UTILS__H_
#define __UTILS__H_



// FIXME: set this to be defined in a build config
#define DEV


#ifdef DEV
    #define DEBUGTRACE(msg) printf msg 
#else
    #define DEBUGTRACE(msg) 
#endif

////////////////////////////////////////////////////////////////////////////////
////
//// TIMING
////
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdint.h>

inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ __volatile__ 
    (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      : "=a" (lo), "=d" (hi)
      :
      : "%ebx", "%ecx"
    );

    return (uint64_t)hi << 32 | lo;
}

inline void testtiming()
{
    unsigned long long x;
    unsigned long long y;
    x = rdtsc();
    printf("%lld\n",x);
    y = rdtsc();
    printf("%lld\n",y);
    unsigned long long w = y - x ;
    unsigned long long n = 1600000000 ; // CPU Hz
    printf("it took this long to call printf: %lld\n",y-x);
    printf("This many could be done in a second: %lld\n", n/w);

}

inline void get_cycle(
    unsigned long long& init_time
)
{
    init_time = rdtsc() ;
}
inline void cycle_delta(unsigned long long& prev, unsigned long long& delta)
{
    unsigned long long x;
    x = rdtsc();
    delta = x - prev ;
}


#endif // ifndef __UTILS__H_
