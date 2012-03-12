

#include "recalc.h"


void Quit( int returnCode )
{
    /* clean up SDL */

    printf("\n QUIT -- CLEANING UP TEXT ... ") ;
    clean_up_text() ;
    printf("\n QUIT -- CLEANING UP TEXT FINISHED. ") ;
    printf("\n") ;

    printf("\n QUIT -- CLEANING UP SOUND ... ") ;
extern void clear_sound() ;
    clear_sound() ;
//extern void stopsounds() ;
 //   stopsounds() ;
//extern void stopchannels() ;
 //   stopchannels() ;
    printf("\n QUIT -- CLEANING UP SOUND FINISHED. ") ;

    SDL_Quit( );
    while (SDL_PollEvent(NULL)) ; 

    /* Make console a factor of 0.00001 less retarded */
    printf("\n\r");

    /* and exit appropriately */
    exit( returnCode );
}



Engine engine ; 
void Engine::initialize()
{
    engine.fullscreen = true ;
    engine.window_active = true ;
    engine.testing = true ; 
    engine.menu = true ; 
    engine.playing = true ; 
    engine.rendering = true ;
    engine.physics = true ;
    engine.paused = false ; 
    engine.fov = 75 ;
//    engine.gridscale = 10 ;
//    engine.gridsize = 2<<10 ;

    // utility (non-game) components 
    engine.info = true ;
    engine.console = false ;  // when this is set to true, then the pointer to 
                              // current_commands points to console_commands 
    engine.console_scale = 1.0f ;
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


void Engine::toggle_fullscreen()
{
    fullscreen = !fullscreen ;

    if (fullscreen)
    {
        current_w = desktop_w ;
        current_h = desktop_h ;
    }
    else
    {
        current_w = scr_w ;
        current_h = scr_h ;
    }

    // Now get our window to reflect this status

    SDL_SetVideoMode( 
        current_w, 
        current_h, 
        SCREEN_BPP,
        ((fullscreen)   ?  videoFlagsFS : videoFlags)
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
void Camera::initialize(World cur_world)
{
    left = false ;
    right = false ;
    pitch = 0 ; 
    yaw = 0 ; 
    roll = 0 ;
    pos = vec( cur_world.size/2, cur_world.size/2,  cur_world.size/2 ) ;
}

void Camera::mouse_move( float xrel, float yrel )
{
    yaw -= xrel ; 

    while ( yaw > 3600 ) yaw -= 3600 ;
    while ( yaw < 0 ) yaw += 3600 ;

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
void Camera::set_strafe_left( )
{
    left = true ; 
    strafe = true ; 
}
void Camera::set_strafe_right( )
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
void Camera::stop_strafe_left()
{
    left = false ; 
    strafe = false ; 
}
void Camera::stop_strafe_right()
{
    right = false ; 
    strafe = false ; 
}

bool Camera::inworld(World current_world)
{
    return  
       ( (pos.x >= 0) && (pos.x <= current_world.size) &&
         (pos.y >= 0) && (pos.y <= current_world.size) &&
         (pos.z >= 0) && (pos.z <= current_world.size) ) ;
}


Area area ;
void Area::initialize()
{
    size = 1 << 15 ;
}


extern World world ;

extern Console console ;
// First invoke general utility commands 
// Then invoke every module's initialization routine. 
void initialize_subsystems()
{
    setbuf(stdout, NULL ) ; 

	// modules 
    engine.initialize() ;
    console.initialize() ;
    camera.initialize(world) ;
    area.initialize() ;
    world.initialize() ;

    // non-class components 
    initialize_text() ;
    initialize_input() ;

    initialize_tests() ;

    return ; 
}

