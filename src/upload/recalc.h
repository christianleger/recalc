
/* This file contains all definitions that are considered global to the 
   entire engine, or which have not been categorized yet. 
   
 */ 



#ifndef __recalc_h_
#define __recalc_h_

#include <stdio.h>
//#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <math.h>
#include <SDL.h>
#include <GL/gl.h>

#ifdef WIN32
	#include "windows.h"
#endif

#include "tools.h"
#include "math.h"
#include "geometry.h"
#include "input.h"
#include "text.h"
#include "render.h"
#include "console.h"

/*   
 *     INPUTS: the definitions of all our primitive types. 
 *
 */
//#include "entities.h"
//#include "elements.h"
//#include "graphics.h"

/* screen width, height, and bit depth */
/*#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600*/
#define SCREEN_BPP     16

#define TRUE  1
#define FALSE 0

//#define PHYSICS_FRAME_TIME 10
#define PHYSICS_FRAME_TIME 5 

void Quit( int returnCode );

/*
    Data declarations to be visible to the right modules. 
*/
struct Engine
{
    int scr_w ; // - Records size when windowed. 
    int scr_h ; // 

    int desktop_w ; // These remember the size of the fullscreen, in case
    int desktop_h ; // we switch to windowed mode for some time.         

    int current_w ; // these are used so that when we need the current display 
                    // window, we don't have to check if we're in fullscreen or not. 
    int current_h ;

    int fov ;

    bool fullscreen ;
    bool window_active;  // should be false when the window doesn't have input control 
    unsigned int maxfps;  /* if true then rendering happens at max rate */
    char use_vsync;  /* if true then max rate limited to screen refresh */


    bool paused ;
    bool playing ;
    bool rendering ;
    bool physics ;
    bool editing ;
    bool menu ;
    bool testing ;
    bool testing_physics ;
    bool console ;

    void initialize() ; 

    void pause_physics() ; 
    
    void pause_playing() ;
    void pause_rendering() ;

    void pause_editing() ;
    
    void pause_sound() ;


    void toggle_fullscreen() ;
   
//  GRAPHICS and WINDOWING 
    SDL_Surface* surface ;

    const SDL_VideoInfo* videoInfo;
    int videoFlags;
    int videoFlagsFS; // videoFlags with fullscreen 


} ;


/*
    This function belongs to the one Engine we'll be using. 

    Notice it's not a member function of the class in the C++ sense. 

    Nevertheless it belongs to the Engine class. 

    I did it this way because I don't know how to do any better by better 
    understanding the acrobatic syntax of class member function pointers. 

    I'm sure you understand. 

*/
void toggle_console(void *) ;

#define FORWARD  0x01 
#define BACKWARD 0x02
#define LEFT     0x04
#define RIGHT    0x08
struct Camera
{
    vec pos ; 
    vec dir ;
    vec up ;

    float vel ;

    bool move ; 
    bool forward ;
    bool backward ;

    bool strafe ;
    bool left ;
    bool right ;

    float yaw ;
    float pitch ;
    float roll ;

    void initialize() ;
    void mouse_move( float xrel, float yrel ) ;
    void set_forward() ;
    void set_backward() ;
    void set_strafe_left() ;
    void set_strafe_right() ;
    void stop_forward() ;
    void stop_backward() ;
    void stop_strafe_left() ;
    void stop_strafe_right() ;
} ; 

struct Area
{
    int size ;
    void initialize() ;
} ;

void initialize() ; 

typedef struct _dir_navigator
{

    int a;
    int fileIndex;                   /* -1 means not valid */
    char current_path[4096];         /* the so-called 'base' name of a full-pathed file name */
    char current_name[128];           /* the name of the current file, stripped of parent directories */
    struct dirent ** dir_entries;
    int numEntries;
    int textWidth; 

} DirNavigator; 

typedef struct _menu
{
    short pos; /* position of focs in a list of options */

} Menu; 



#define XY 0
#define XZ 1
#define YZ 2



/*
*/
typedef void (* console_command)(char * ); 
/* land of black sheep: components which are integrated here first, before they are 
   modularized, if ever */ 
typedef struct _command 
{
    char name[64]; 
    console_command function; 
} Command ;


typedef struct _commandNode
{
    Command com; 

    struct _commandNode * nextCommandNode; 

} CommandNode; 


/* this is the generic linked list node to be used by 
   any and all data types */
typedef struct _list_node
{
    void * data; 
    struct _list_node * next; 
} ListNode; 

typedef ListNode CElementNode; 
typedef ListNode MB_node; 

/*
    This defines a rectangular button. 

    It is used to determine which 

*/
typedef struct _rect_button
{
    int xmin; 
    int xmax; 
    int ymin; 
    int ymax; 

} RectButton; 


/*
    This is used to reference buttons which are grouped together 
    in a particular area of the screen, at a particular time. 
*/
typedef struct _screen_area
{
    int xmin; 
    int xmax; 
    int ymin; 
    int ymax; 

    int numButtons; 
    RectButton buttons[100]; 

} ScreenArea; 


// inputs 
void initialize_input() ;


// physics 
void pause_physics() ; 
void physics_frame( unsigned int ) ; 

// render


// text 
void initialize_text() ; 
void prstr( int font_idx, float x, float y, const char * fmt ) ; 

// input 
void initialize_input() ; 
void handle_key() ;
void handle_mouse_motion() ;
void handle_mouse_click() ;

#endif

// from main: function which resizes the window 
void resize_window( int w, int h, int fov) ;


