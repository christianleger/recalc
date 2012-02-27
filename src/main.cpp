/*
 * Copyright Christian Leger
 * 
 * Inspiration and code bits from the countless selfless contributors from the cyberverse, including folks from 
 *
 * Sauerbraten - NeHe - Valve - ID Software - 
 *
 * This code is being created with the intention of enhancing humanity's ability to entertain itself 
 * and communicate with itself. The various ways in which this is achieved include: 
 *
 *  - easier creation of 3D worlds and object representations
 *     - easier understanding of the tools which create and animate virtual worlds
 *     - exposure to new ideas for having fun (in educational and self-actualizing    ways)
 *     - exposure to new ideas for thinking and representing technical and symbolic information 
 *     - exposure to new ideas for collaborating with other beings. 
 *     - meshing tools which, before this, were always used separately and toward separate problem domains
 *
 *     All of these are simply coins on a pile; I don't know that anything here will change anything completely, 
 *     but I think it can change everything a little bit, and so contribute momentum to our cultural evolution. 
 * 
 */
 
//#include <time.h>       // used for time measurement function clock_gettime
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <SDL.h>
#include <SDL_image.h>

#include "recalc.h"


/* all other globals are found in recalc.c */

SDL_Surface *surface;

#define SCREEN_WIDTH 2600
#define SCREEN_HEIGHT 900

int default_screen_width = 1200 ; 
int default_screen_height = 900 ; 

/*-----------------------------------------------------------------*/
//                  function prototypes 
/*-----------------------------------------------------------------*/


void read_args( int, char** ) ;
bool testonly = false ;
bool cancelsound = false ;

void read_configs() ;


void initSDL( Engine * engine ) ;


void initGL( ); 


void resize_window( int width, int height ) ;


void initialize_subsystems() ; 


uint texid = 0 ;
void load_texture() 
{
    SDL_Surface* data_image ;

    data_image = IMG_Load("../data/textures/1.jpg") ;
    if (data_image == NULL)
    {
        printf("\nPROBREM LOADING FILE\n");
        printf("\nThe error is: %s\n", IMG_GetError());
        Quit(1);
    }

    char* data ;
    data = (char *)(data_image->pixels);

//        /* this call is made so we can get some anti-aliasing */
//        RGBtoRGBA(data_image->w, data_image->h, data, &aux_data);

//    int k = (sizeof(char) * data_image->w * data_image->h * 3);
    
    glGenTextures(1,&(texid));

    printf("\nJust created a opengl texture, ID %d", texid);

    glBindTexture(GL_TEXTURE_2D, texid);
/*printf("\nHAVING LOADING SHIP TEXTURES. value of texture ID: %d", shipTextures[0]);*/
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data_image->w, data_image->h, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data_image->w, data_image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    SDL_FreeSurface(data_image);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    // use SDL_image to get a handle on the pixels

    // tell OpenGL to give us an id to refer to this texture. 

    // tell OpenGL that these are the pixels to this texture. 

    // Now we can use this texture everywhere we want! 
}


/*-----------------------------------------------------------------*/
//                  function definitions 
/*-----------------------------------------------------------------*/


void read_args( int argc, char** argv )
{
    // Args

    int i = 1 ; // first argument is the name of this program (unless this is a lib)
    int args = argc ;
    while (args>0 && i < argc)
    {
        if (argv[i][0]=='-')
        {
            switch (argv[i][1])
            {
                case 'q':
                {
                    // extern bool nosound ;
                    // soundoff() ; FIXME: one day maybe soundoff will work. For now it's boring. 
                    printf("\nQUIET MODE SELECTED. SOUND SHOULDN'T PLAY. \n") ;
                    break ; 
                }
                case 't':
                {
                    printf("\nTEST ONLY MODE SELECTED. NOW PROBABLY EXITING. \n") ;
                    args = 0 ;
                    testonly = true ;
                    break ;
                }
                default:
                {
                    printf("\nUnhandled argument: %s\n", argv[i]) ;
                    break ;
                }
            }
        }
        else
        {
            printf("\nNot Unhandled argument: '%s' from args)\n", argv[i]) ;
        }
        args-- ;
        i++ ;
    }
}


void read_configs()
{
}


void initSDL( Engine * engine )
{
    /* prepare SDL */
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
    {
        fprintf( stderr, "Video initialize failed: %s\n", SDL_GetError( ) );
        Quit( 1 );
    }
    
    engine->videoInfo = SDL_GetVideoInfo( );

    if ( !engine->videoInfo )
    {
        fprintf( stderr, "Video query failed: %s\n", SDL_GetError( ) );
        Quit( 1 );
    }

    /* record desktop resolution for later use in fullscreen */
    engine->desktop_w = engine->videoInfo->current_w; 
    engine->desktop_h = engine->videoInfo->current_h; 

    printf("\nDesktop dimensions provided by videoInfo: %d x %d: \n", 
           engine->videoInfo->current_w , 
           engine->videoInfo->current_h 
           ) ; 

    /* the flags to pass to SDL_SetVideoMode */
    engine->videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    engine->videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    engine->videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    engine->videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */

    /* This checks to see if surfaces can be stored in memory */
    if ( engine->videoInfo->hw_available )
        engine->videoFlags |= SDL_HWSURFACE;
    else
        engine->videoFlags |= SDL_SWSURFACE; // are you kidding me. 

    /* This checks if hardware blits can be done */
    if ( engine->videoInfo->blit_hw ) 
        engine->videoFlags |= SDL_HWACCEL;

    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    
    /* get a SDL surface */
    engine->videoFlagsFS  = engine->videoFlags; 
    engine->videoFlagsFS |= SDL_FULLSCREEN; 


    // FIXME: this is convoluted bullshit
    // WINDOW
    if ( engine->fullscreen )
    {
        engine->scr_w = engine->desktop_w; 
        engine->scr_h = engine->desktop_h; 
    }
    else
    {
        engine->scr_w = default_screen_width; 
        engine->scr_h = default_screen_height; 
    }
    engine->current_w = engine->scr_w ;
    engine->current_h = engine->scr_h ;


    // VISUAL
    printf("\nSetting video mode: \n") ; 
    printf("\n %d X %d \n", engine->scr_w, engine->scr_h ) ; 
    engine->surface = SDL_SetVideoMode( 
        engine->current_w, 
        engine->current_h, 
        SCREEN_BPP, 
        ((engine->fullscreen)   ?  engine->videoFlagsFS : engine->videoFlags)
    );

    /* Verify there is a surface */
    if ( !engine->surface )
    {
        fprintf( stderr,  "Video mode set failed: %s\n", SDL_GetError( ) );
        Quit( 1 );
    }
    else
    {
        fprintf( stderr,  "GREAT SUCCESS FOR THE COMPUTER GENERATING A SURFACE: %s\n", SDL_GetError( ) );
        //"Messages given by SDL_GetError: %s\n", 
    }

    printf("\n current screen width: %d", engine->scr_w ) ; 
    printf("\n current screen height: %d", engine->scr_h ) ; 
}


void initGL( )
{
    /* Enable smooth shading */
    glShadeModel( GL_SMOOTH );
    /* Set the background black */
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    /* Depth buffer setup */
    glClearDepth( 1.0f );
    /* Enables Depth Testing */
    
    
    /* Really Nice Perspective Calculations */
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_SMOOTH );
    // glHint( GL_TEXTURE_COMPRESSION_HINT, GL_NICEST );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    // glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_BLEND );
    // glDepthFunc( GL_GREATER ) ;
    // glEnable(GL_CULL_FACE) ;
    glCullFace(GL_CW) ;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* The Type Of Depth Test To Do */
    // glDepthFunc( GL_EQUAL | GL_NOTEQUAL ) ;
    glDepthFunc( GL_LESS ) ;

    //glPolygonMode(GL_FRONT_AND_BACK,GL_LINE) ;

    return ; 
}

/* reset our viewport after a window resize */
void resize_window( int width, int height, int fov )
{
    /* Field of view factor; 1 gives proper proportions in a realistic scene. 
       For instance, a square looks square. 
    */
    GLfloat ratio;
    GLfloat angle ; 
    /* avoid cancelling the universe */
    if ( height == 0 ){   height = 1;  }

    ratio = ( GLfloat )width / ( GLfloat )height;
  

    printf( "\n attemping to resize window with fov %d \n", fov ) ; 
    if (fov==-1)
    {
        angle = 60 ; // default somewhat fish-eye perspective 
    }
    else 
    {
        angle = fov ;
    }

    /* viewport. */
    glViewport( 0, 0, ( GLsizei )width, ( GLsizei )height );

    /* By default, large viewing volume. Culling in software. */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    ////gluPerspective( (GLfloat)angle, ratio, 0.01f, 10000000.0f );
    gluPerspective( (GLfloat)angle, ratio, 10.0f, 300000.0f );
    //gluPerspective( 60.0f, ratio, 0.01f, 50000.0f );

    /* default matrix to manipulate is modelview */
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );

    return ;
}


// Main resources to use 
extern Engine engine ;
extern Camera camera ; 
extern int playsound( vec*v, int a, int b, int c, int d, int e) ;

int main_msgs_num = 0 ;
char main_msgs[100][256] ;
uint millis = 0 ;
int main( int argc, char **argv )
{
    bool done = false ;


    /* whether or not the window is active (in focus) */
    int windowActive = true ;

    // time and frame calculations 
    bool time_delta_big_enough ;

    unsigned long long first_cycle = 0 ;
    unsigned long long delta_cycle = 0 ;
    get_cycle(first_cycle) ;
    printf("\nENGINE STARTING ON CLOCK CYCLE %lld\n", first_cycle) ;

  
//  extern int addsound(const char *name, int vol, int maxuses, vector<soundslot> &sounds) ;
int vol = 128 ;
char hello[100] = "../data/cranberry-radio_edit.mp3" ;      // USELESS SHIT registersound("../data/cranberry-radio_edit.mp3", vol) ;
int thesound = 0 ;
    // registersound(hello, &vol) ;
    // registersound(hello, &vol) ;
    // registersound(hello, &vol) ;

    vec loc(0,0,0) ;
    vec* p_loc = &loc ;

extern int registersound(char *name, int *vol) ;
thesound = registersound(hello, &vol) ;
printf("\nREGISTERED SOUND = %d\n", thesound) ;


// extern void startsound() ;
// startsound() ;
// soundoff() ;
justplay(thesound) ;

read_args(argc, argv); 

    // Testing code. This can run stuff just to evaluate the characteristics of some 
    // code without having to load and run everything. 
    // Move this block further down is some resources are needed to perform some tests. 
    if (testonly)
    {
        
        // Things to test: 
        // sub-milimeter timing code
        testtiming() ;
        // floating-point to integer conversion
        // Whoops haha we leave now because we only wanted to run some arbitrary code ! 
        // get_cycle(delta_cycle) ;
        cycle_delta(first_cycle, delta_cycle) ;
        printf("\nENGINE EXITING AFTER %lld CYCLES\n", delta_cycle) ;
        Quit(0) ;
    }

    read_configs() ; 

    // WANTED: an 'engine' module which knows what good defaults are and how 
    // to present other options. 
    // cheap, expendable initialization section 
    engine.fullscreen = false ; 
    engine.window_active = true ;

    initSDL( &engine ); // this by itself does a lot: it gives us our window and rendering context
    /* OpenGL SUBSYSTEM */
    initGL( );


    printf(" \n\n resolution defaults: \n %d  %d", 
        default_screen_width, 
        default_screen_height
    ) ; 


    engine.scr_w = default_screen_width ;
    engine.scr_h = default_screen_height ;
        printf("\n INITIALIZATION: resizing window after SDL init. dimensions are     %d x %d\n", engine.scr_w, engine.scr_h ) ; 

    initialize_subsystems(); 

    // Any reason this can't be done in the previous subsystem initialization? 
    resize_window( engine.scr_w , engine.scr_h, engine.fov ) ;

    printf("\n engine.scr_w = %d   engine.src_h = %d\n", engine.scr_w, engine.scr_h) ;


    // Bookkeeping and timing variables. 
    bool frame_drawn = true ;
    uint framecount = 0; 
    uint last_millis = 0;
    uint sec_progress = 0 ; 
    int delta = 0 ; 
    int delta_millis = 0 ; 
    int physics_millis = 0 ; 
    int delay_count = 0 ;

    millis = SDL_GetTicks();
    last_millis = millis ; 


    bool annoying = false ; 

    // pump the event queue empty before starting 
    SDL_Event event;
    while ( SDL_PollEvent( &event ) ) ;

// If we grab input while debugging, we can't access the debugger when a breakpoint hits! 
#ifndef DEBUG
    SDL_WM_GrabInput(SDL_GRAB_ON);
#else
//#error WE HAVE DEBUG!
#endif

    SDL_ShowCursor(SDL_DISABLE);         

    //unsigned int last_frame = SDL_GetTicks() ;
    load_texture() ;
    
    /* main loop */
#define FRAME_TIME 5 // 5 gives about 200 fps. Not relevant when in vsync. 
    while ( !done )
    {
        millis = SDL_GetTicks();
        delta = millis - last_millis ;
        if ( delta >= 1 ) // approximately 200 fps, unless vsync is enabled. 
        {
            delta_millis += delta ;
            physics_millis += delta ;
            last_millis = millis ; 
        }


        // This never pauses because without it, we lose 
        // control of the application (unless you want to assume sole control 
        // of it through a network interface but while I would find that interesting
        // I won't for now).
        while ( SDL_PollEvent( &event ) )
        {

            switch( event.type )
            {

                case SDL_MOUSEMOTION: 
                {
                    if (!engine.paused) 
                    handle_mouse_motion( &event ) ;
                    break ; 
                }


                case SDL_MOUSEBUTTONDOWN:
                {
                    handle_mouse_button( &event ) ;
                    break ;
                }


                case SDL_KEYDOWN: 
                case SDL_KEYUP:         
                {
                    handle_key( &event ) ;
                    break ; 
                }


                // Application master control
                case SDL_QUIT: 
                {
                    Quit( 0 ) ;
                    break ; 
                }


                // Interaction with OS
                case SDL_ACTIVEEVENT:
                {
                    //SDL_WM_GrabInput( SDL_GRAB_ON ) ;
                    // returns current_commands to whatever last_commands holds
                    extern void reset_commands() ;
                    reset_commands() ; 
                    SDL_GL_SwapBuffers(); 
                    break ; 
                }
                default:
                    break ; 
            }
        } // end of event polling 
     

        // PHYSICS 
        if (!engine.paused)
        if ( physics_millis >= PHYSICS_FRAME_TIME )
        {
            // Reset counter if frame was run
            if ( engine.physics )
            { 
                physics_frame( physics_millis ) ; 
            }

            physics_millis = 0 ;
        }


        if (
             //  ( delta_millis >= FRAME_TIME) &&
               ( engine.window_active ) &&
               ( !engine.paused )
           )
        {
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

            if ( engine.rendering ) 
            {
                render_world() ; 
            }

            if ( engine.editing ) 
            {
                render_editor() ; 
            }

            if ( engine.testing ) 
            {
                render_tester() ;
            }

            if ( engine.menu ) 
            {
                render_menu() ; 
            }
            
            // Various stats. Mostly useful during debugging. Can be used to 
            // supplement editing as well. 
            if ( engine.info ) 
            {
                render_info() ; 
            }

            if ( engine.console ) 
            {
                render_console() ; 
            }

            // frame counting 
            sec_progress += delta_millis ;

            SDL_GL_SwapBuffers(); 
            framecount++; 

            delta_millis = 0 ;
        } // end if delta_millis > FRAME_TIME 
        else
        {
            if ( !engine.window_active )
            {
                delta_millis = 0 ;
                SDL_GL_SwapBuffers(); 
            }
            // SDL_Delay(1) ; 
            delay_count++ ;
            // glFlush() ;
                SDL_GL_SwapBuffers(); 
            // glFinish() ;
        }

        
        // FRAME COUNTING and other 1Hz actions. 
        if ( sec_progress >= 1000 )
        {
            main_msgs_num = 0 ;
            sprintf( main_msgs[0], 
                "  running time: %d. FPS=%d.", 
                SDL_GetTicks()/1000, framecount 
                ) ; main_msgs_num ++ ;

            delay_count = 0 ;
            framecount = 0 ; 
            sec_progress = 0 ; 
        }
    } // end of 'while not done'


    printf("\n Exiting Recalc normally. ");

    /* clean up, exit */
    Quit( 0 );

    /* this should not execute */
    return( 0 );
}


// BELOW are miscellaneous conveniences and curiosities.

/*
SDL_WarpMouse(0,0);         
SDL_WM_GrabInput(SDL_GRAB_ON);
SDL_WM_ToggleFullScreen( surface ) ;
*/
void testee(char * arg)
{
    printf("\nfunction called: %s ", arg);
}
void test_command()
{
    Command _c; 

    _c.function = testee; 

    _c.function((char*)"pleasure");

    return;
}

