/*
    File: platform.h

    Contains functions that provide functions which are normally implemented differently 
    on the most popular platforms. 

    Examples: 
        linux: clock_gettime
        windows: queryperformancecounter 

        linux: vsync control
        windows: gl extension APIENTRY

    When functions have different names but identical inputs and outputs, they can 
    be used via define. 

    When functions do similar things but differ in all details including in/out 
    parameters, then they need to be wrapped in a way that they can be used the same 
    way every time in this program. 

*/

#ifdef __LINUX
    #include <GL/glx.h>
#endif
#ifdef WIN32
    #include "windows.h"
#endif


#ifdef WIN32

typedef void (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
typedef int (*PFNWGLEXTGETSWAPINTERVALPROC) (void);

void initWin32Vsync() ;

#endif


void toggle_vsync( bool use_vsync ) ;
