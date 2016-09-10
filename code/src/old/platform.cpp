/*
    File: platform.cpp

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
#include "platform.h"
#include "GL/gl.h"
//#include "GL/glxext.h"

#if 0
#ifdef WIN32

PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
PFNWGLEXTGETSWAPINTERVALPROC wglGetSwapIntervalEXT = NULL;

void initWin32Vsync()
{
    char* extensions = NULL;

    extensions = (char*)glGetString(GL_EXTENSIONS);

    if ( strstr( extensions, "WGL_EXT_swap_control" ) )
    {
        /* get address's of both functions and save them*/
        wglSwapIntervalEXT = ( PFNWGLEXTSWAPCONTROLPROC ) wglGetProcAddress( "wglSwapIntervalEXT" );
        wglGetSwapIntervalEXT = ( PFNWGLEXTGETSWAPINTERVALPROC ) wglGetProcAddress( "wglGetSwapIntervalEXT" );
    }

    return;

}
#endif

#ifdef __linux__

typedef GLvoid  
(*glXSwapIntervalSGI) (GLint);  

void toggle_vsync( bool use_vsync )
{
    #ifdef WIN32
    initWin32Vsync();
    wglSwapIntervalEXT( use_vsync );
    #endif

    #if defined(__LINUX__) || defined(__LINUX)|| defined(__linux__)
    glXSwapIntervalSGI( use_vsync );
    #endif
    return ; 
}

#endif
#endif // end if 0
