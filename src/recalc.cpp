

#include "recalc.h"

//extern void showfinaldata() ;
void Quit( int returnCode )
{
    /* clean up SDL */
    printf("\n QUIT -- CLEANING UP TEXT ... ") ;
    clean_up_text() ;
    printf("\n QUIT -- CLEANING UP TEXT FINISHED. ") ;

    printf("\n QUIT -- CLEANING UP SOUND ... ") ;

    if (0)
    {
        extern void stopsounds() ;
        stopsounds() ;
        extern void stopchannels() ;
        stopchannels() ;
        extern void clear_sound() ;
        clear_sound() ;
    }

    printf("\n QUIT -- CLEANING UP SOUND FINISHED. ") ;

//    while (SDL_PollEvent(NULL)) ; 
    SDL_Quit( );    

    /* Make console a factor of 0.00001 less retarded */
    printf("\n\r");

    /* and exit appropriately */
    exit( returnCode );
}


Engine engine ; 
#define e engine

Engine& GetEngine() 
{
    return engine ;
}

/*
    This function takes a message from one place 
    and places it for viewing in the desired 
    location. The desired location for a message 
    to go is the console. 
*/
void Engine::message(char const* msg, int where)
{
    if (where==0) // to console
    {
        GetConsole().message(msg) ;
    }
}

void Engine::initialize()
{
//    engine.fullscreen = true ;
    window_active   = true ;
    menu            = true ; 
    rendering       = true ;   // TODO: is this used for anything? 
    info            = true ;    // Useful for debug or learning about internals. 

    testing         = true ; 
    playing         = false ; 
    physics         = false ;
    paused          = false ; 
    console         = false ;  
    texatlas        = false ;
    fov = 90 ;


//    engine.gridscale = 10 ;
//    engine.gridsize = 2<<10 ;

    // utility (non-game) components 
    console_scale   = 1.0f ;

    texarray        = false ;   // Will be set to true at runtime if texarray feature detected. 
    activetex       = 0 ;       // currently active texture
    numtex          = 0 ;       // TODO: do we need this anywhere? 

    win_w           = 0 ;
    win_h           = 0 ;

    printf("\n\n ENGINE INITIALIZATION: this=%d\n\n", (int)(this)) ;
}



// this function serves to go back to default key bindings when we switch out 
// of a certain control mode, like console, 
extern void clear_main_commands() ;
extern void (** current_commands)(void *) ;
extern void (* console_commands[320])(void *) ;

void set_commands( void (* new_commands[320])(void *) )
{
    current_commands = new_commands ;
}


/*
    Sets console to active when inactive, and vice-versa. 
*/
void toggle_console(void *)
{
    engine.console = !engine.console ;

    if (engine.console)
    {
        set_commands( console_commands ) ; 
    }
    else
    {
        clear_main_commands() ;
    }
}


/*

*/
void Engine::toggle_fullscreen()
{
    printf("\n [ENGINE]: TOGGLE FULLSCREEN CALLED.") ; //
    fullscreen = !fullscreen ;
    printf("\n [ENGINE]: WINDOW SIZE = %dX%d", win_w, win_h) ;
    printf("\n [ENGINE]: DESKTOP SIZE = %dX%d", desktop_w, desktop_h) ;
    printf("\n [ENGINE]: CURRENT SIZE = %dX%d  ", current_w, current_h) ;
    
    
    printf("\n\t fullscreen status = %d", fullscreen) ;
    if (fullscreen)
    {
        current_w = desktop_w ;
        current_h = desktop_h ;
    }
    else
    {
        current_w = win_w ; ///scr_w ;
        current_h = win_h ; ///scr_h ;
    }
    printf("\n [ENGINE]: TOGGLE FULLSCREEN DONE. ") ;
    printf("\n [ENGINE]: WINDOW SIZE = %dX%d", win_w, win_h) ;
    printf("\n [ENGINE]: DESKTOP SIZE = %dX%d", desktop_w, desktop_h) ;
    printf("\n [ENGINE]: CURRENT SIZE = %dX%d  \n\n", current_w, current_h) ;

    // Now get our window to reflect this status

    SDL_SetVideoMode(
        current_w,
        current_h,
        SCREEN_BPP,
        ((fullscreen)   ?  videoFlagsFS : (videoFlags|SDL_RESIZABLE))
        ) ;

    resize_window(
        current_w,
        current_h,
        fov
        ) ;

    // since that last move just created some bogus mouse events, open the airlock and they're sucked out
    SDL_Event event ;
    while ( SDL_PollEvent( &event ) ) { ; }
    printf("\nTrying to toggle fullscreen with resolution = %d x %d \n", current_w, current_h ) ;
}

Camera camera ; 

#define numinsamples 10 
int xreli = 0 ;
float xrels[numinsamples] ;
float xrelavg = 0.0f ;
int yreli = 0 ;
float yrels[numinsamples] ;
float yrelavg = 0.0f ;
float avgdiv = 0.0f ;

void Camera::initialize(World cur_world)
{
    left = false ;
    right = false ;
    //pitch = 0 ; 
    pitch = -M_PI/4 ; 
    //yaw = 0 ; 
    yaw = -M_PI/4 ; 
    roll = 0 ;
    pos = vec( cur_world.size/2, cur_world.size/2,  cur_world.size/2 ) ;
    dir = vec( -1, -1, -1) ;
    //pos = vec( 0, 0, 0 ) ;

    // TODO: Please. Package up this functionality. 
    loopi(numinsamples)
    {
        xrels[i] = 0.0 ;
        yrels[i] = 0.0 ;
    }
    avgdiv = 1.0f / numinsamples ;
    printf("\n*******************************\n") ;
    printf("\navgdiv = %f \n", avgdiv) ;
    printf("\n*******************************\n") ;
}

float sensitivity = 0.9 ;

/*
    Description: 
        sign flips: unless we check for changes in direction of the movement 
        of the mouse, then we might get slight jerks in the wrong direction 
        at the start of motion in a certain direction. 
*/
void Camera::mouse_move( float xrel, float yrel )
{
    //yaw -= xrelavg ; 
    yaw -= xrel ; 
    while ( yaw > 3600 ) yaw -= 3600 ;
    while ( yaw < 0 ) yaw += 3600 ;

    //pitch -= yrelavg ;
    pitch -= yrel ;
    if ( pitch > 1000 ) pitch = 1000 ;
    if ( pitch < -1000 ) pitch = -1000 ;

    return ;
}

void Camera::set_forward( )
{
    forward = true ; 
    move = true ;
}
void Camera::set_backward( )
{
    backward = true ; 
    move = true ;
}
void Camera::set_move_left( )
{
    left = true ; 
    strafe = true ; 
}
void Camera::set_move_right( )
{
    right = true ;
    strafe = true ; 
}
void Camera::stop_forward()
{
    forward = false ; 
}
void Camera::stop_backward()
{
    backward = false ; 
}
void Camera::stop_move_left()
{
    left = false ; 
    strafe = false ; 
}
void Camera::stop_move_right()
{
    right = false ; 
    strafe = false ; 
}

bool Camera::inworld(World current_world)
{
    return  ( 
       (pos.x >= 0) && (pos.x <= current_world.size) &&
       (pos.y >= 0) && (pos.y <= current_world.size) &&
       (pos.z >= 0) && (pos.z <= current_world.size) 
       ) ;
}


Area area ;
void Area::initialize()
{
    size = 1 << 15 ;
}


extern World world ;

extern Console console ;


/*
    Function: Screenshot. 

    The goal is to record into an image file the contents or maybe a portion 
    of the contents visible on the screen. 


*/
int Screenshot(char *filename)
{
    SDL_Surface *screen = engine.surface ;
    SDL_Surface *savesurf;
    unsigned char *pixels;
    int i;

    int w = engine.current_w ;
    int h = engine.current_h ;

/*
    Blowing this away whenever I feel: I never use SDL for my rendering, 
    only OpenGL. 
    if (!(screen->flags & SDL_OPENGL))
    {
        SDL_SaveBMP(savesurf, filename);
        return 0;
    }
*/

    savesurf = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
    0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
    );
    
    if (savesurf == NULL)
        return -1;
//
//    pixels = (uchar *)malloc(3 * w * h);
//    if (pixels == NULL)
//    {
//        SDL_FreeSurface(savesurf);
//        return -1;
//    }

    /*  
        For all you noobs and non-eidetic robots like me, this is where we order 
        the CPU to order the GPU that the contents of the frame buffer interest 
        us and would you be so kind as to send my way a chunk of data to which 
        you have access and here are the boundaries of data I would like. 
    */
    //glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, savesurf->pixels);


// Apparently this reverses the image vertically. Confirm? 
//    for (i=0; i<h; i++)
//        memcpy(((char *) savesurf->pixels) + savesurf->pitch * i, pixels + 3*w * (h-i-1), w*3);
//    free(pixels);

    SDL_SaveBMP(savesurf, filename);
    SDL_FreeSurface(savesurf);
    return 0 ;
}


// First invoke general utility commands 
// Then invoke every module's initialization routine. 
void initialize_subsystems()
{
    setbuf(stdout, NULL ) ; 

	// modules 
    engine.initialize() ;
    printf("\n\n ENGINE INITIALIZED. ") ;
    console.initialize() ;
    printf("\n\n CONSOLE INITIALIZED. ") ;
    camera.initialize(world) ;
    printf("\n\n CAMERA INITIALIZED. ") ;
    area.initialize() ;
    printf("\n\n AREA INITIALIZED. ") ;
    world.initialize() ;
    printf("\n\n WORLD INITIALIZED. ") ;

    // non-class components 
    init_scripting() ;
    printf("\n\n SCRIPTING INITIALIZED. ") ;
    init_text() ;
    printf("\n\n TEXT INITIALIZED. ") ;
    init_input() ;
    printf("\n\n INPUT INITIALIZED. ") ;
    init_physics() ;
    printf("\n\n PHYSICS INITIALIZED. ") ;
    init_shaders() ;
    printf("\n\n SHADERS INITIALIZED. ") ;
    init_rendering() ;
    printf("\n\n RENDERING INITIALIZED. ") ;
    init_menus() ;
    printf("\n\n MENUS INITIALIZED. ") ;
    init_tests() ;
    printf("\n\n TESTS INITIALIZED. ") ;
    init_sound() ;
    printf("\n\n SOUND INITIALIZED. ") ;

    return ; 
}

