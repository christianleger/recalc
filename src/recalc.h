/* 
    File: recalc.h

    This file contains all definitions that are considered global to the 
    entire engine, or which have not been categorized yet. 
*/ 



#ifndef __recalc_h_
#define __recalc_h_

// RETARD MAGIC YOU GOTTA FIND OUT ABOUT WITH PAIN AND MISERY OR LUCK

#define GL_GLEXT_PROTOTYPES
//--------------------------------------------------------------------------------------------------
//                  SYSTEM INCLUDE FILES 
//--------------------------------------------------------------------------------------------------
//#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <math.h>
#include <float.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <zlib.h>

#ifdef WIN32
	#include "windows.h"
#endif

//--------------------------------------------------------------------------------------------------
//                  LOCAL INCLUDE FILES 
//--------------------------------------------------------------------------------------------------
#include "tools.h"      // data structures and primitives
#include "utils.h"      // timing, debugging, logging, timing and profiling
#include "math.h"       // vectors, certain number crunchers
#include "geometry.h"
#include "input.h"
#include "text.h"
#include "test.h"
#include "render.h"
#include "console.h"


#include "Newton.h"
/*   
 *     INPUTS: the definitions of all our primitive types. 
 *
 */
//#include "entities.h"
//#include "elements.h"
//#include "graphics.h"

/* screen width, height, and bit depth */
/*
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
*/
#define SCREEN_BPP     16

#define TRUE  1
#define FALSE 0

//#define PHYSICS_FRAME_TIME 10
// #define PHYSICS_FRAME_TIME 5 
#define PHYSICS_FRAME_TIME 5 

#define TRACE(args) printf args

void Quit( int returnCode );

/*
    Data declarations to be visible to the right modules. 
*/
struct Engine
{
    // WINDOWING AND RENDERING PARAMETERS 
    int scr_w ; // - Records size when windowed. 
    int scr_h ; // 

    int desktop_w ; // These remember the size of the fullscreen, in case
    int desktop_h ; // we switch to windowed mode for some time.         

    int current_w ; // these are used so that when we need the current display 
                    // window, we don't have to check if we're in fullscreen or not. 
    int current_h ;

    int fov ;

    // EDITING PARAMETERS 
    int gridscale ;
    int gridsize ;      // 2^gridscale 

    bool fullscreen ;
    bool window_active;  // should be false when the window doesn't have input control 
    unsigned int maxfps;  /* if true then rendering happens at max rate */
    char use_vsync;  /* if true then max rate limited to screen refresh */


    bool info ;                 // when true, the information subsystem provides text readouts of engine data
    bool paused ;
    bool playing ;
    bool rendering ;
    bool physics ;
    bool editing ;
    bool menu ;
    bool testing ;
    bool testing_physics ;
    bool console ;

    float console_scale ;

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

    vec vel ;

    bool move ; 
    bool forward ;
    bool backward ;

    bool strafe ;
    bool left ;
    bool right ;

    float yaw ;
    float pitch ;
    float roll ;

    void initialize(World current_world) ;
    void mouse_move( float xrel, float yrel ) ;
    void set_forward() ;
    void set_backward() ;
    void set_strafe_left() ;
    void set_strafe_right() ;
    void stop_forward() ;
    void stop_backward() ;
    void stop_strafe_left() ;
    void stop_strafe_right() ;
    bool inworld(World current_world) ;
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
int scrstrlen( const char * ) ; 
void clean_up_text() ;

// input 
void initialize_input() ; 
void handle_key() ;
void handle_mouse_motion() ;
void handle_mouse_button( SDL_Event * ev ) ;

#endif

// from main: function which resizes the window 
void resize_window( int w, int h, int fov) ;

// geometry

void draw_selection() ;
void draw_sel_start() ;
void draw_sel_end() ;
void set_sel_start() ;
void set_sel_end() ;
void clear_selection() ;
void extrude( void * ) ; 
void draw_new_octs() ;
void initialize() ;

// sound

    // sound status
    void soundoff() ;
    void soundon() ;

    // sound commands
    void startsound() ; // initialize sound subsystem
    
    int justplay(int thesound) ; // a dysfunctional little puppy that insists a piece of audio be played. 

    int playsound( vec* loc, int n, int loops, int fade, int chanid, int radius, int expire) ;


