/*
    File: input.cpp


    Defines functions and facilities to handle mouse and keyboard inputs. 


    The input module should be aware of every other module, 
    since the possibility exists to want to be able to 
    bind any function to a key, and to act on any data from the 
    engine. 

*/

#include "recalc.h"

//FIXME: put me where this is useful to everybody
float deg_to_radians = M_PI / 1800.0f ;

// Data that will often get manipulated by user inputs 
extern Engine engine ;
extern World world ;
extern Camera camera ;

int input_msgs_num = 0 ;
char input_msgs[100][256] ;
///////////////////////////////////////////////////////////////////////////////
// Function pointers for binding management 
///////////////////////////////////////////////////////////////////////////////

// default global commands active by default
// replaced by other command sets when 
// modifier keys are hit or command_set_change events 
// are received through scripting. 
void (* commands[320])(void *) ;                


void (** current_commands)(void *) ;                // default global commands 
void (** last_commands)(void *) ;                // default global commands 


void (* l_alt_commands[320])(void *) ;
void (* r_alt_commands[320])(void *) ;
void (* l_ctrl_commands[320])(void *) ;
void (* r_ctrl_commands[320])(void *) ;
void (* l_shift_commands[320])(void *) ;
void (* r_shift_commands[320])(void *) ;


void (* mouse_binding)(void *) ;

void (* mouse_scroll_command)(void *) ;

void toggle_extrude_scroll()
{
}

//    printf("\n world.gridscale = %d \n", world.gridscale ) ;
void gridsize_scroll(void * args)
{

    if ((*(bool*)args)) // scroll up 
    {
        world.gridscale += 1 ;
        if (world.gridscale>world.scale)
        {
            world.gridscale = world.scale ;
        }
    }
    else                // scroll down 
    {
        world.gridscale -= 1 ;
        if (world.gridscale<2)
        {
            world.gridscale = 2 ;
        }
    }
    world.gridsize = 1<<(world.gridscale) ;
    printf("\n world.gridscale = %d \n", world.gridscale ) ;

    /*sprintf( input_msgs[input_msgs_num], "world.gridscale=%d", 
        world.gridscale
        ); input_msgs_num++ ;*/
}

void toggle_gridsize_scroll( bool enable = true ) 
{
    if (enable)
    {
        mouse_scroll_command = &gridsize_scroll ;
    }
    else
    {
        mouse_scroll_command = NULL ;
        mouse_scroll_command = &extrude ;
    }
}

void deform_scroll(void * args)
{
    // FIXME: I have no code!
}
/*
    By default, activated by holding down q. 

    When active during edit mode, scrolling up or 
    down deforms cubes instead of extruding or 
    deleting them. 
*/
void toggle_deform_scroll( bool enable = false ) 
{
    if (enable)
    {
        mouse_scroll_command = &deform_scroll ;
    }
    else
    {
        mouse_scroll_command = NULL ;
        mouse_scroll_command = &extrude ;
    }
}

bool commands_initialized = false ;

// from menu.cpp 
extern void (* menu_commands[320])(void *) ;
// from console.cpp 
extern void (* console_commands[320])(void *) ;


// purely utilitarian - temporary, intemporal, casual. Only for scratch. 
SDL_Event ev ; 
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void hello(void * nothing)
{
    printf("\n hello from command executed! ") ; 
}



/*
    What happens when we hit left alt? 

    It sets current_commands to the l_alt set.


*/
void tab_window(void*)
{
    engine.window_active = false ;
    SDL_WM_GrabInput( SDL_GRAB_OFF ) ;
    SDL_ShowCursor( SDL_ENABLE ) ;
    //while ( SDL_PollEvent ( &ev )) ;
}
/*
    Function: initialize_commands

    Purpose: To assign in groups the functions we want to be able to 
    invoke using the keyboard and mouse. 

    Types of groups: 
                        - editing key commands
                        - editing mouse commands 
                        - playing key commands
                        - playing mouse commands 
                        - console commands 
                        - global commands (such as ESCAPE, which always takes 
                          us to the main menu. 

    The 'commands' function pointers array will point to any and all 
    binds defined in config files. 

*/
void initialize_commands()
{
    for (int i=0;i<320;i++)
    {
        commands[i] = &hello ; 
    }
    commands_initialized = true ;

    // when this is set to null, it means defaults are in effect 
    current_commands = NULL ;

    mouse_scroll_command = &extrude ;

    l_alt_commands[SDLK_TAB] = &tab_window ;
}



int mousex = 0 ; 
int mousey = 0 ; 
float mouse_deltax = 0 ; 
float mouse_deltay = 0 ; 
float mouse_sensitivity = 3 ; 




// Let's count the mouse events to be able to 
// estimate how often they happen and when. 
unsigned int mouse_event_count = 0 ;

// here is the global mouse motion command. 

//
// It either invokes the default mouse-motion 
// commands, or any from the currently assigned 
// command set. 
//

void handle_mouse_scroll( bool up )
{
    if (mouse_scroll_command) //scroll is up
    {
        mouse_scroll_command( (void*)&up ) ;
    }
}

void handle_mouse_motion( SDL_Event* event )
{
    // Record the current position of the mouse 
    mousex = event->motion.x ;
    mousey = event->motion.y ;

    // The deltas accumulate mouse motion so that the next physics frame to run 
    // will incorporate all change so far, and then the deltas will be reset. 
    mouse_deltax = (float)event->motion.xrel ;
    mouse_deltay = (float)event->motion.yrel ;


    camera.mouse_move( 
        mouse_sensitivity*mouse_deltax, 
        mouse_sensitivity*mouse_deltay 
    ) ;

    vec dir = camera.dir ;

    float pitch_cos =   cos(  camera.pitch      * deg_to_radians ) ;
    dir.x = pitch_cos * cos( (camera.yaw+900)   * deg_to_radians ) ;
    dir.y = pitch_cos * sin( (camera.yaw+900)   * deg_to_radians ) ;
    dir.z = sin(  camera.pitch      * deg_to_radians ) ;

    float len = dir.x*dir.x + dir.y*dir.y + dir.z*dir.z ;
    len = sqrt( len ) ;

    camera.dir.x = dir.x / len ;
    camera.dir.y = dir.y / len ;
    camera.dir.z = dir.z / len ;

    input_msgs_num = 0 ;

    //camera.dir = dir ;

    len = sqrt( 
        camera.dir.x * camera.dir.x  + 
        camera.dir.y * camera.dir.y  + 
        camera.dir.z * camera.dir.z 
        ) ;
    // printf("\nLength of camdir is : %0.2f\n", len ) ;
    //SDL_WarpMouse(
    //    engine.scr_w/2,
    //    engine.scr_h/2
    //); 
    //while (SDL_PollEvent( &ev )) ;

    //(*mouse_binding)() ;
    return ;
}


void handle_mouse_button( SDL_Event * event )
{
    // scroll down 
    if ( event->button.button == SDL_BUTTON_WHEELUP )
    {
        handle_mouse_scroll( true ) ;
        //printf("\n ScrollUP event. \n") ;
        return ;
    }
    if ( event->button.button == SDL_BUTTON_WHEELDOWN )
    {
        handle_mouse_scroll( false ) ;
        //printf("\n ScrollDOWN event. \n") ;
        return ;
    }


    if (!engine.window_active)
    {
        printf("\n Grabbing input \n") ;
        engine.window_active = true ;
        SDL_WM_GrabInput( SDL_GRAB_ON ) ;
        SDL_ShowCursor( SDL_DISABLE ) ;
        while (SDL_PollEvent( &ev )) ; 
    }
    else
    {
        printf("\nGetting mouse click \n") ;

        if ( engine.editing ) // do edit click
        {
            // if left button 
            if ( event->button.button == SDL_BUTTON_LEFT )
            {
                set_sel_start() ; 
            }
            // if right button 
            else if ( event->button.button == SDL_BUTTON_RIGHT )
            {
                set_sel_end() ; 
            }
        }
        else // do game/sim click 
        {
        }
    }
}

// OPTIONS FOR GLOBAL BEHAVIOR WHICH AREN'T NECESSARY TO OPERATE APLENTY

bool _3D_main_menu = false ;




/*
        Function: all_pause

        Purpose: tells all major modules to pause immediately

*/
void all_pause() 
{

    // physics and main engine render: main 3D render pauses
    engine.playing = false ; 
    engine.testing_physics  = false ;

    if ( !_3D_main_menu ) engine.rendering = 

    // testing 
    engine.playing = false ; 

    // console 
    engine.playing = false ; 

    return ;
}

void reset_commands()
{
    current_commands = last_commands ;
    last_commands = NULL ;
    while ( SDL_PollEvent( &ev ) ) ;
}


extern void Quit( int ) ; 

// Main key handling routine: 
//     
//      either: 
//
//      - invoke commands from the currently used command set
//      - switch command set temporarily by hitting a modifier key
//      - switch to a different command set (by default using a script function; 
//        this can be mapped to a key) .
//
void handle_key( SDL_Event* event )
{
    SDLKey k = event->key.keysym.sym ;

    // If we have an active command set (console, editing, etc.), then we use it. 
    // Otherwise, we use the base command set. 
    // Any commands requested that are not part of the current command set are 
    // obtained from the main module, and this case is detected when the 
    // current_commands pointer is set to NULL. 
    if (current_commands)
    {
        if (event->type==SDL_KEYDOWN)
        {
            if (current_commands[k]) 
            {
                // call this function 
                // FIXME: the argument to the function called should be a
                // struct that takes arguments if needed 
                current_commands[k]((void*)(&k)) ;
                return ;
            }
        }
    }
    else
    {
        //printf("\ncommand requested and current_commands is NULL\n")  ;
    }

    // printf("\nNow executing the base command set \n")  ;
    // base set of commands 
    if (event->type==SDL_KEYDOWN)
    {
        switch( k )
        {
/*
            SDLK_UNKNOWN		= 0,
            SDLK_FIRST		= 0,
            */
            case SDLK_BACKSPACE:
            {
                extern int geom_msgs_num ;
                geom_msgs_num = 0 ;
                break ;
            }
            case SDLK_TAB:
            {
                current_commands[ SDLK_TAB ] ;
                break ;
            }
            /*
            SDLK_CLEAR		= 12,
            SDLK_RETURN		= 13,
*/
            case SDLK_PAUSE:
            {
                engine.paused = !engine.paused ;
                while ( SDL_PollEvent( event ) ) ; // this usage of event won't even send the information where it can be used ! 
                break ; 
            }
/*
            SDLK_ESCAPE		= 27,
*/
            case SDLK_ESCAPE:
            {
                // TODO FIXME:  assign me to the activation of the main menu and 
                // TODO FIXME:  pausing all other activity. 

                // sequence of events: 
                //                      All the control booleans 
                //                      that allow other loops than 
                //                      the top control loop to run are 
                //                      disabled. A global pause function 
                //                      is called which informs all subordinate 
                //                      polling modules to pause themselves at once. 

                all_pause() ; 
                if (engine.paused) 
                {
                    engine.paused = !engine.paused ; 
                }
                else
                {
                    Quit( 0 ) ; 
                }
                break ; 
            }
            case SDLK_SPACE: // 		= 32,
            {
                extern bool onfloor ;
                extern bool jumping ;
                if (!engine.editing)
                {
                if (onfloor)
                {
                    jumping = true ;
                }
                }
                else
                {
                    extern void clear_selection() ;
                    clear_selection() ;
                }
                break ;
            }
/*
            SDLK_EXCLAIM		= 33,
            SDLK_QUOTEDBL		= 34,
            SDLK_HASH		= 35,
            SDLK_DOLLAR		= 36,
            SDLK_AMPERSAND		= 38,
            SDLK_QUOTE		= 39,
            SDLK_LEFTPAREN		= 40,
            SDLK_RIGHTPAREN		= 41,
            SDLK_ASTERISK		= 42,
            SDLK_PLUS		= 43,
            SDLK_COMMA		= 44,
            SDLK_MINUS		= 45,
            SDLK_PERIOD		= 46,
            SDLK_SLASH		= 47,
            SDLK_0			= 48,
            SDLK_1			= 49,
            SDLK_2			= 50,
            SDLK_3			= 51,
            SDLK_4			= 52,
            SDLK_5			= 53,
            SDLK_6			= 54,
            SDLK_7			= 55,
            SDLK_8			= 56,
            SDLK_9			= 57,
            SDLK_COLON		= 58,
            SDLK_SEMICOLON		= 59,
            SDLK_LESS		= 60,
            SDLK_EQUALS		= 61,
            SDLK_GREATER		= 62,
            SDLK_QUESTION		= 63,
            SDLK_AT			= 64,
            // 
            //   Skip uppercase letters
            //
            SDLK_LEFTBRACKET	= 91,
            SDLK_BACKSLASH		= 92,
            SDLK_RIGHTBRACKET	= 93,
            SDLK_CARET		= 94,
            SDLK_UNDERSCORE		= 95,
            */
            case SDLK_BACKQUOTE: // value 96 
            {
                toggle_console(NULL) ;
/*
                if (engine.console)
                {
                    current_commands = console_commands ; 
                }
                else
                {
                    current_commands = NULL ; 
                }
*/
                break ;
            }
/*
            SDLK_DELETE		= 127,
            // End of ASCII mapped keysyms 
            //@}

            // @name International keyboard syms 
            //@{
            SDLK_WORLD_0		= 160,		// 0xA0
            SDLK_WORLD_1		= 161,
            SDLK_WORLD_2		= 162,
            SDLK_WORLD_3		= 163,
            SDLK_WORLD_4		= 164,
            SDLK_WORLD_5		= 165,
            SDLK_WORLD_6		= 166,
            SDLK_WORLD_7		= 167,
            SDLK_WORLD_8		= 168,
            SDLK_WORLD_9		= 169,
            SDLK_WORLD_10		= 170,
            SDLK_WORLD_11		= 171,
            SDLK_WORLD_12		= 172,
            SDLK_WORLD_13		= 173,
            SDLK_WORLD_14		= 174,
            SDLK_WORLD_15		= 175,
            SDLK_WORLD_16		= 176,
            SDLK_WORLD_17		= 177,
            SDLK_WORLD_18		= 178,
            SDLK_WORLD_19		= 179,
            SDLK_WORLD_20		= 180,
            SDLK_WORLD_21		= 181,
            SDLK_WORLD_22		= 182,
            SDLK_WORLD_23		= 183,
            SDLK_WORLD_24		= 184,
            SDLK_WORLD_25		= 185,
            SDLK_WORLD_26		= 186,
            SDLK_WORLD_27		= 187,
            SDLK_WORLD_28		= 188,
            SDLK_WORLD_29		= 189,
            SDLK_WORLD_30		= 190,
            SDLK_WORLD_31		= 191,
            SDLK_WORLD_32		= 192,
            SDLK_WORLD_33		= 193,
            SDLK_WORLD_34		= 194,
            SDLK_WORLD_35		= 195,
            SDLK_WORLD_36		= 196,
            SDLK_WORLD_37		= 197,
            SDLK_WORLD_38		= 198,
            SDLK_WORLD_39		= 199,
            SDLK_WORLD_40		= 200,
            SDLK_WORLD_41		= 201,
            SDLK_WORLD_42		= 202,
            SDLK_WORLD_43		= 203,
            SDLK_WORLD_44		= 204,
            SDLK_WORLD_45		= 205,
            SDLK_WORLD_46		= 206,
            SDLK_WORLD_47		= 207,
            SDLK_WORLD_48		= 208,
            SDLK_WORLD_49		= 209,
            SDLK_WORLD_50		= 210,
            SDLK_WORLD_51		= 211,
            SDLK_WORLD_52		= 212,
            SDLK_WORLD_53		= 213,
            SDLK_WORLD_54		= 214,
            SDLK_WORLD_55		= 215,
            SDLK_WORLD_56		= 216,
            SDLK_WORLD_57		= 217,
            SDLK_WORLD_58		= 218,
            SDLK_WORLD_59		= 219,
            SDLK_WORLD_60		= 220,
            SDLK_WORLD_61		= 221,
            SDLK_WORLD_62		= 222,
            SDLK_WORLD_63		= 223,
            SDLK_WORLD_64		= 224,
            SDLK_WORLD_65		= 225,
            SDLK_WORLD_66		= 226,
            SDLK_WORLD_67		= 227,
            SDLK_WORLD_68		= 228,
            SDLK_WORLD_69		= 229,
            SDLK_WORLD_70		= 230,
            SDLK_WORLD_71		= 231,
            SDLK_WORLD_72		= 232,
            SDLK_WORLD_73		= 233,
            SDLK_WORLD_74		= 234,
            SDLK_WORLD_75		= 235,
            SDLK_WORLD_76		= 236,
            SDLK_WORLD_77		= 237,
            SDLK_WORLD_78		= 238,
            SDLK_WORLD_79		= 239,
            SDLK_WORLD_80		= 240,
            SDLK_WORLD_81		= 241,
            SDLK_WORLD_82		= 242,
            SDLK_WORLD_83		= 243,
            SDLK_WORLD_84		= 244,
            SDLK_WORLD_85		= 245,
            SDLK_WORLD_86		= 246,
            SDLK_WORLD_87		= 247,
            SDLK_WORLD_88		= 248,
            SDLK_WORLD_89		= 249,
            SDLK_WORLD_90		= 250,
            SDLK_WORLD_91		= 251,
            SDLK_WORLD_92		= 252,
            SDLK_WORLD_93		= 253,
            SDLK_WORLD_94		= 254,
            SDLK_WORLD_95		= 255,		// 0xFF
            //@}

            // @name Numeric keypad 
            //@{
            SDLK_KP0		= 256,
            SDLK_KP1		= 257,
            SDLK_KP2		= 258,
            SDLK_KP3		= 259,
            SDLK_KP4		= 260,
            SDLK_KP5		= 261,
            SDLK_KP6		= 262,
            SDLK_KP7		= 263,
            SDLK_KP8		= 264,
            SDLK_KP9		= 265,
            SDLK_KP_PERIOD		= 266,
            SDLK_KP_DIVIDE		= 267,
            SDLK_KP_MULTIPLY	= 268,
            SDLK_KP_MINUS		= 269,
            SDLK_KP_PLUS		= 270,
            SDLK_KP_ENTER		= 271,
            SDLK_KP_EQUALS		= 272,
            //@}

	        // @name Arrows + Home/End pad
            //@{
            SDLK_UP			= 273,
            SDLK_DOWN		= 274,
            SDLK_RIGHT		= 275,
            SDLK_LEFT		= 276,
            SDLK_INSERT		= 277,
            SDLK_HOME		= 278,
            SDLK_END		= 279,
            SDLK_PAGEUP		= 280,
	        SDLK_PAGEDOWN		= 281,
            //@}

	        // @name Function keys 
            //@{
            SDLK_F1			= 282,
            */
            case SDLK_F2:
            {
                if (engine.console_scale>=3.0f)
                {
                    engine.console_scale = 1.0f ;
                }
                else
                {
                    engine.console_scale *= 1.5f ;
                }
                break ;
            }
            case SDLK_F3:
            {
                engine.toggle_fullscreen() ;
                printf("\nengine ordered to toggle fullscreen. engine reports fullscreen status is %d\n", engine.fullscreen ) ;
                break ;
            }
            
            case SDLK_F4:
            {   
                extern bool hhello ;
                hhello = !hhello ;
                ///extern bool use_dl ;
                ///use_dl = !use_dl ;
                ///if ( use_dl ) printf("\n now using display list ") ; 
                break ;
            }
/*
            SDLK_F5			= 286,
            SDLK_F6			= 287,
            SDLK_F7			= 288,
            SDLK_F8			= 289,
            SDLK_F9			= 290,
            SDLK_F10		= 291,
            SDLK_F11		= 292,
            SDLK_F12		= 293,
            SDLK_F13		= 294,
            SDLK_F14		= 295,
            SDLK_F15		= 296,
            //@}

	        // @name Key state modifier keys
            //@{
            SDLK_NUMLOCK		= 300,
            SDLK_CAPSLOCK		= 301,
            SDLK_SCROLLOCK		= 302,
            SDLK_RSHIFT		= 303,
*/
            case SDLK_LSHIFT:
            {
            extern float basic_velocity ;
                if ( basic_velocity > 25000 )
                {
                    basic_velocity = 100 ;
                }
                else
                {
                    basic_velocity *= 5 ;
                }
                //printf("") ; 
                /*
                TRACE(("\n basic_velocity just set to %.2f \n",basic_velocity ));
                sprintf(input_msgs[input_msgs_num], " basic_velocity just set to %.2f ",basic_velocity );input_msgs_num++ ;
                break ;
                */
            }
            /*
            SDLK_RCTRL		= 305,
            SDLK_LCTRL		= 306,
            SDLK_RALT		= 307,
            */
            case SDLK_LALT:
            {
                last_commands = current_commands ;
                current_commands = l_alt_commands ; 

                // enable tabbing out of application 
                break ; 
            }
/*
            SDLK_RMETA		= 309,
            SDLK_LMETA		= 310,
            SDLK_LSUPER		= 311,		//< Left "Windows" key 
            SDLK_RSUPER		= 312,		//< Right "Windows" key 
            SDLK_MODE		= 313,		//< "Alt Gr" key 
            SDLK_COMPOSE		= 314,  //< Multi-key compose key 
            //@}

            // @name Miscellaneous function keys 
            //@{
            SDLK_HELP		= 315,
            SDLK_PRINT		= 316,
            SDLK_SYSREQ		= 317,
            SDLK_BREAK		= 318,
            SDLK_MENU		= 319,
            SDLK_POWER		= 320,		//< Power Macintosh power key 
            SDLK_EURO		= 321,		//< Some european keyboards 
            SDLK_UNDO		= 322,		//< Atari keyboard has Undo 
            //@}

*/
            case SDLK_a:
            {
                camera.set_strafe_left() ; 
                break ; 
            }
            //SDLK_b			= 98,
            //SDLK_c			= 99,
            case SDLK_d:
            {
                camera.set_strafe_right() ; 
                break ; 
            }
            case SDLK_e:
            {
                engine.editing = !engine.editing ;

                if (engine.editing) 
                    printf("\n now editing \n") ; 

                break ; 
            }
            //SDLK_f			= 102,
            
            case SDLK_g:			// = 103,
            {
                
                toggle_gridsize_scroll() ;
                break ;
            }
            //SDLK_h			= 104,
            //SDLK_i			= 105,
            //SDLK_j			= 106,
            //SDLK_k			= 107,
            //SDLK_l			= 108,
            //SDLK_m			= 109,
            //SDLK_n			= 110,
            //SDLK_o			= 111,
            //SDLK_p			= 112,
            //SDLK_q			= 113,
            case SDLK_q:
            {
                toggle_deform_scroll() ; // releasing q disables cube deformation
            }
            //SDLK_r			= 114,
            case SDLK_s:
            {
                camera.set_backward() ; 
                break ; 
            }
            //SDLK_t			= 116,
            //SDLK_u			= 117,
            //SDLK_v			= 118,
            case SDLK_w:
            {
                camera.set_forward() ; 
                break ; 
            }
            
            //SDLK_x			= 120,
            //SDLK_y			= 121,
            //SDLK_z			= 122,
            case SDLK_z:
            {
                static int lastfov = engine.fov ;
                if ( engine.fov > 50 )
                {
                    engine.fov = 30 ; 
                    mouse_sensitivity = 1.0f ;
                }
                else // zzz   mouse sensitivity needed here 
                     // (sensitive for normal operation, slow for in-fovus operation)
                {
                    engine.fov = lastfov ; 
                    mouse_sensitivity = 3.0f ;
                }
                resize_window( engine.current_w, engine.current_h, engine.fov ) ; 
                break ;
            }
            default: 
            {
                if (!commands_initialized){initialize_commands() ;}
                (*commands[k])(NULL) ;
                printf("\n You have hit a key with SDL code %d", k) ;
                break ; 
            }
        }
    }


    else if (event->type==SDL_KEYUP)
    {
        switch( k )
        {
            case SDLK_LALT:
            {
                current_commands = last_commands ;
                last_commands = NULL ;
                break ; 
            }
            case SDLK_TAB:
            {
                break ;
            }
            case SDLK_w:
            {
                camera.stop_forward() ;
                break ;
            }
            case SDLK_s:
            {
                camera.stop_backward() ;
                break ;
            }
            case SDLK_a:
            {   
                camera.stop_strafe_left() ;
                break ;
            }
            case SDLK_d:
            {   
                camera.stop_strafe_right() ;
                break ;
            }
            case SDLK_g:
            {
                toggle_gridsize_scroll( false ) ; // releasing g disables gridsize scrolling
            }
            case SDLK_q:
            {
                toggle_deform_scroll( false ) ; // releasing q disables cube deformation
            }
            default:
            {
                //if (annoying)printf(" unhandled and discarded command key-up ") ;
                return ;
                break ;
            }
        } 
    }// end input processing 
    /*
        */

    return ;

} // end handle_key( SDL_Event* event )


/*
    This function eventually will move to the file console.cpp. 
    
    Data required by the console input system: 

        - input line buffer
        - console view buffer 
            -> at least 10,000 lines of previous command history. 
            -> records all files loaded and all their contents, 
               as well as any output from the execution of loaded 
               files. 
*/
/*
*/
void handle_console_key_in()
{
    // possible actions: 
    //      - add a character to the input line buffer, if space remains
    //      - remove a character from the input line buffer
    //      - nothing
    //      - move the cursor along the input line buffer
    //      - trigger the execution of a command. 
}


void initialize_input()
{

    initialize_commands() ; 

    // section: menu commands. the main menu is active when the 
    // program starts. 
}


// This is the main input system off switch. 
// You hit this sucker when you want to resume full control of basic
// global commands. 
void clear_main_commands() 
{
    current_commands = NULL ;
}

/*
*/
void update_input_messages()
{
    input_msgs_num = 0 ;

    sprintf( input_msgs[input_msgs_num], "world.gridscale=%d", 
        world.gridscale
        ); input_msgs_num++ ;

    sprintf(
        input_msgs[input_msgs_num], 
        "direction vector : %.2f %.2f %.2f %.2f", 
        camera.dir.x,
        camera.dir.y,
        camera.dir.z, 
        camera.pitch
        ) ; input_msgs_num++ ;

    sprintf( input_msgs[input_msgs_num], "pitch = %.2f and sin (theta) = %.2f", 
        camera.pitch/10.f, sin(  camera.pitch      * deg_to_radians )
        ); input_msgs_num++ ;

            extern float basic_velocity ;
    sprintf(
        input_msgs[input_msgs_num], 
        " basic_velocity set to %.2f ",
        basic_velocity 
        ); input_msgs_num++ ;
}

