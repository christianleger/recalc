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

/*
    Message presets
     
    How module messages work: 
    A number of pre-defined messages are placed at the start of this section's 
    messages array. Reserving a portion of the array for these regular messages 
    allows us to use the remaining space for other messages. 

    The result is two types of messages: 
        - constantly available messages, like states
        - time-dependent messages, like whatever the last frame had to say for 
          itself. 
*/

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


//void (* l_alt_commands[320])(void *) ;
//void (* r_alt_commands[320])(void *) ;
//void (* l_ctrl_commands[320])(void *) ;
//void (* r_ctrl_commands[320])(void *) ;
//void (* l_shift_commands[320])(void *) ;
//void (* r_shift_commands[320])(void *) ;


// TODO: figure out what this is for. Anything? 
void (* mouse_binding)(void *) ;

void (* mouse_scroll_command)(void *) ;

void toggle_modify_scroll()
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
    else    // scroll down 
    {
        world.gridscale -= 1 ;
        if (world.gridscale<2)
        {
            world.gridscale = 2 ;
        }
    }
    world.gridsize = 1<<(world.gridscale) ;
    printf("\n world.gridscale = %d (%dm)\n", world.gridscale, 2<<world.gridscale ) ;
    /*
    sprintf( input_msgs[input_msgs_num], "world.gridscale=%d", 
        world.gridscale
        ); input_msgs_num++ ;
    */
}


void toggle_gridsize_scroll( bool enable = true ) 
{
    //if (enable) { mouse_scroll_command = &gridsize_scroll ; }
    if (enable) { mouse_scroll_command = gridsize_scroll ; }
    else {
        mouse_scroll_command = NULL ;
        mouse_scroll_command = &modify ;
    }
}


// texture scrolling! 
int yeshello = 0 ;

/*
    TODO: when scrolling (selecting) textures, if we have any faces selected,
    the texture assigned to these faces should change along with the texture 
    selected. 
*/
void texture_scroll(void *args) 
{
printf("\n\ntex scroll: current texture is %d", engine.activetex) ;
    if ((*(bool*)args)) // scroll up 
    {
        engine.activetex -- ;
        yeshello -- ;
        if (engine.activetex<0)
        {
            engine.activetex = engine.numtex-1 ;
        }
    }
    else                // scroll down
    {
        yeshello ++ ;
        engine.activetex ++ ;
        if (engine.activetex>=engine.numtex)
        {
            engine.activetex = 0 ;
        }
    }
printf("\ntex scroll: current texture is now %d", engine.activetex) ;

    printf("\n active texture is now %d", engine.activetex) ;
 //   printf("\n now engine.tex/3.0f=%f", engine.tex/3.0f) ;
}

void toggle_texture_scroll( bool enable = true ) 
{
    printf("YES WE ARE SCROLLING TEXTURES") ;
    if (enable) { mouse_scroll_command = &texture_scroll ; }
    else {
        mouse_scroll_command = NULL ;
        mouse_scroll_command = &modify ;
    }
}


/*
    FUNCTION:
        deform_scroll

    DESCRIPTION:
        This function is used to turn cubes into different shapes, by pushing 
        and pulling on their corners. If the scroll is up, this results in 
        pushing a corner into the face currently selected. If the scroll is 
        down, this pulls on the corner.
    
*/
void deform_scroll(void * args)
{
    // FIXME: I have no code!
    if ((*(bool*)args)) // scroll up: push inwards
    {
        PushCorner() ;
    }
    // scroll down 
    else  { PullCorner() ; }
}


/*
    By default, activated by holding down q. 

    When active during edit mode, scrolling up or 
    down deforms cubes instead of modifying or 
    deleting them. 
*/
void toggle_deform_scroll( bool enable = false ) 
{
    if (enable)
    {
        mouse_scroll_command = &deform_scroll ;
        printf("\n cube deform enabled. \n") ;
    }
    else
    {
        //mouse_scroll_command = NULL ;
        mouse_scroll_command = &modify ;
        printf("\n cube deform disabled. \n") ;
    }
}


bool commands_initialized = false ;


// from menu.cpp 
//extern void (* menu_commands[320])(void *) ;
// from console.cpp 
//extern void (* console_commands[320])(void *) ;


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
bool grabinput = true ;
void tab_window(void*)
{
    //engine.window_active = false ;
    grabinput = !grabinput ;

    if (grabinput)
    {
        SDL_WM_GrabInput( SDL_GRAB_ON ) ;
        SDL_ShowCursor( SDL_DISABLE ) ;
    }
    else
    {
        SDL_WM_GrabInput( SDL_GRAB_OFF ) ;
        SDL_ShowCursor( SDL_ENABLE ) ;
    }
    //while ( SDL_PollEvent ( &ev )) ;
}

////////////////////////////////////////////////////////////////////////////////
//  Play Commands
////////////////////////////////////////////////////////////////////////////////
//  These commands are used by the play mode. 
////////////////////////////////////////////////////////////////////////////////

/*
    FUNCTION:
        set_editing

    DESCRIPTION: 
        This enables the edit mode. 
*/
void set_editing()
{
    // have visuals be set to edit mode (world frame, world, cursors)
}

////////////////////////////////////////////////////////////////////////////////
//  Editing Commands
////////////////////////////////////////////////////////////////////////////////
//  These commands are used by the edit mode. 
////////////////////////////////////////////////////////////////////////////////


/*
    FUNCTION:
        set_play

    DESCRIPTION: 
        This enables the play mode. 

    USAGE:
        The play mode can be enabled from the main menu, or from edit mode. 
*/
void set_play()
{
    // Set play mode visuals
//    rendersys.setplay() ;

    // Set play mode input handlers
//    intputsys.setplay() ;
}


/*
    Function: init_commands

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
void init_commands()
{
    for (int i=0;i<320;i++)
    {
        commands[i] = &hello ; 
    }
    commands_initialized = true ;

    // when this is set to null, it means defaults are in effect 
    current_commands = NULL ;

    mouse_scroll_command = &modify ;

//    l_alt_commands[SDLK_TAB] = &tab_window ;
}



int mousex = 0 ; 
int mousey = 0 ; 
float mouse_deltax = 0 ; 
float mouse_deltay = 0 ; 
float msensitivity = 3 ; 


// Let's count the mouse events to be able to 
// estimate how often they happen and when. 
unsigned int mouse_event_count = 0 ;

// here is the global mouse motion command. 

//
// It either invokes the default mouse-motion 
// commands, or any from the currently assigned 
// command set. 
//

/*  
    NOTES
        We use the convention that scrolling up is caused by passing true to 
        the scroll command. 
*/
void handle_mouse_scroll( bool up )
{
    if (mouse_scroll_command) 
    {
        mouse_scroll_command( (void*)&up ) ;
    }
}

bool mbutton1down = false ;
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
        msensitivity*mouse_deltax, 
        msensitivity*mouse_deltay 
    ) ;

    vec dir = camera.dir ;

    float pitch_cos =   cos(  camera.pitch      * deg_to_radians ) ;
    dir.x = pitch_cos * cos( (camera.yaw+900)   * deg_to_radians ) ;
    dir.y = pitch_cos * sin( (camera.yaw+900)   * deg_to_radians ) ;
    dir.z = sin(  camera.pitch                  * deg_to_radians ) ;

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

extern bool havesel ;
    if (    havesel     &&
            mbutton1down
       ) 
    {
        printf("havesel with mouse button down. ") ;
        set_sel_end() ;
    }

    return ;
}

void (*mousescrollup)(void) ;
void (*mousescrolldown)(void) ;

void (*leftmousedown)(void) ;
void (*middlemousedown)(void) ;
void (*rightmousedown)(void) ;

void handle_mouse_button_down( SDL_Event * event )
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
        if ( engine.editing ) // do edit click
        {
            if ( event->button.button == SDL_BUTTON_LEFT )
            {
                mbutton1down = !mbutton1down ;
                set_sel_start() ; 
            }
            else if ( event->button.button == SDL_BUTTON_RIGHT )
            {
                //mbutton1down = !mbutton1down ;
                set_sel_end() ; 
            }
        }
        else // do game/sim click 
        {
        }
    }
}

void handle_mouse_button_up(SDL_Event* ev)
{
    if ( ev->button.button == SDL_BUTTON_LEFT )
    {
        mbutton1down = !mbutton1down ;
        //set_sel_start() ; 
    }
}

// OPTIONS FOR GLOBAL BEHAVIOR WHICH AREN'T NECESSARY TO OPERATE APLENTY

//bool _3D_main_menu = false ;




/*
        Function: all_pause

        Purpose: tells all major modules to pause immediately

*/
//void toggle_pause() 
void all_pause() 
{

    // physics and main engine render: main 3D render pauses
    //engine.playing = !engine.playing ;
    engine.playing = false ; 
    
    //engine.testing_physics  = !engine.testing_physics ;
    engine.testing_physics  = false ;
//    engine.paused = true ;

//    if ( !_3D_main_menu ) engine.rendering = 

    // testing 
    //engine.playing = false ; 
//    engine.playing = !engine.playing ;

    // console 
    //engine.playing = false ; 
//    engine.playing = engine.playing ;

    return ;
}

void reset_commands()
{
    current_commands = last_commands ;
    last_commands = NULL ;
    while ( SDL_PollEvent( &ev ) ) ;
}


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

    // If we have an active command set (console, editing, etc.), then we try to use it. 
    // Otherwise, we use the base command set. 
    // Any commands requested that are not part of the current command set are 
    // obtained from the main module, and this case is detected when the 
    // current_commands[k] pointer is set to NULL. 
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
            else
            {
         //       printf("\nNo command assigned to requested key. \n") ;
            }
        }
    }
    else
    {
//        printf("\ncommand requested and current_commands is NULL\n")  ;
    }

    // default set of commands 
    if (event->type==SDL_KEYDOWN)
    {
        switch( k )
        {
            /*
            SDLK_UNKNOWN = 0,
            SDLK_FIRST    = 0,
            */
            case SDLK_BACKSPACE:
            {
                extern int geom_msgs_num ;
                geom_msgs_num = 0 ;
                break ;
            }
            case SDLK_TAB:
            {
                tab_window(NULL) ;
                //current_commands[ SDLK_TAB ] ;
                break ;
            }
            /*
            SDLK_CLEAR  = 12,
            SDLK_RETURN = 13,
            */
            case SDLK_PAUSE:
            {
                // this usage of event won't even send the 
                // information where it can be used ! 
                engine.paused = !engine.paused ;
                while ( SDL_PollEvent( event ) ) ; 
                break ; 
            }

            /*
            SDLK_ESCAPE = 27,
            */
            case SDLK_ESCAPE:
            {
//printf("\nENGINE MENU STATUS: %s\n", engine.menu ? "ACTIVE":"INACTIVE") ;
                playsound(0) ;
                //Quit( 0 ) ; 
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
                //toggle_pause() ; 
                //if (engine.paused) 
                if (engine.menu) 
                {
printf("\nRESUMING. ENGINE PAUSE STATUS: %s\n", engine.paused?"TRUE":"FALSE") ;
                    EditNewMap() ;
                    engine.paused = !engine.paused ;
                    engine.menu = !engine.menu ;
                }
                else
                {
printf("\nPAUSING. ENGINE PAUSE STATUS: %s\n", engine.paused?"TRUE":"FALSE") ;
                    ActivateMainMenu() ;
                //    engine.paused = !engine.paused ;
                 //   engine.menu = !engine.menu ;
                }
                printf("\nENGINE MENU STATUS: %s\n", engine.menu ? "ACTIVE":"INACTIVE") ;
                break ; 
            }
            case SDLK_SPACE: // = 32,
            {
extern bool onfloor ;
extern bool jumping ;
                if (!engine.editing)
                {
                    if (onfloor) { jumping = true ; }
                }
                else
                {
                //    extern void clear_selection() ;
                    clear_selection() ;
                }
                break ;
            }
/*
            SDLK_EXCLAIM   = 33,
            SDLK_QUOTEDBL= 34,
            SDLK_HASH= 35,
            SDLK_DOLLAR= 36,
            SDLK_AMPERSAND      = 38,
            SDLK_QUOTE      = 39,
            SDLK_LEFTPAREN      = 40,
            SDLK_RIGHTPAREN      = 41,
            SDLK_ASTERISK      = 42,
            SDLK_PLUS      = 43,
            SDLK_COMMA      = 44,
            SDLK_MINUS      = 45,
            SDLK_PERIOD      = 46,
            SDLK_SLASH      = 47,
            SDLK_0         = 48,
            SDLK_1         = 49,
            SDLK_2         = 50,
            SDLK_3         = 51,
            SDLK_4         = 52,
            SDLK_5         = 53,
            SDLK_6         = 54,
            SDLK_7         = 55,
            SDLK_8         = 56,
            SDLK_9         = 57,
            SDLK_COLON      = 58,
            SDLK_SEMICOLON      = 59,
            SDLK_LESS      = 60,
            SDLK_EQUALS      = 61,
            SDLK_GREATER      = 62,
            SDLK_QUESTION      = 63,
            SDLK_AT         = 64,
            // 
            //   Skip uppercase letters
            //
            SDLK_LEFTBRACKET   = 91,
            SDLK_BACKSLASH      = 92,
            SDLK_RIGHTBRACKET   = 93,
            SDLK_CARET      = 94,
            SDLK_UNDERSCORE      = 95,
            */
            case SDLK_BACKQUOTE: // value 96 
            {
                //playsound() ;
                playsound(0) ;
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
            SDLK_DELETE      = 127,
            // End of ASCII mapped keysyms 
            //@}

            // @name International keyboard syms 
            //@{
            SDLK_WORLD_0= 160,      // 0xA0
            SDLK_WORLD_1   = 161,
            SDLK_WORLD_2   = 162,
            SDLK_WORLD_3   = 163,
            SDLK_WORLD_4   = 164,
            SDLK_WORLD_5   = 165,
            SDLK_WORLD_6   = 166,
            SDLK_WORLD_7   = 167,
            SDLK_WORLD_8   = 168,
            SDLK_WORLD_9   = 169,
            SDLK_WORLD_10   = 170,
            SDLK_WORLD_11   = 171,
            SDLK_WORLD_12   = 172,
            SDLK_WORLD_13   = 173,
            SDLK_WORLD_14   = 174,
            SDLK_WORLD_15   = 175,
            SDLK_WORLD_16   = 176,
            SDLK_WORLD_17   = 177,
            SDLK_WORLD_18   = 178,
            SDLK_WORLD_19   = 179,
            SDLK_WORLD_20   = 180,
            SDLK_WORLD_21   = 181,
            SDLK_WORLD_22   = 182,
            SDLK_WORLD_23   = 183,
            SDLK_WORLD_24   = 184,
            SDLK_WORLD_25   = 185,
            SDLK_WORLD_26   = 186,
            SDLK_WORLD_27   = 187,
            SDLK_WORLD_28   = 188,
            SDLK_WORLD_29   = 189,
            SDLK_WORLD_30   = 190,
            SDLK_WORLD_31   = 191,
            SDLK_WORLD_32   = 192,
            SDLK_WORLD_33   = 193,
            SDLK_WORLD_34   = 194,
            SDLK_WORLD_35   = 195,
            SDLK_WORLD_36   = 196,
            SDLK_WORLD_37   = 197,
            SDLK_WORLD_38      = 198,
            SDLK_WORLD_39      = 199,
            SDLK_WORLD_40      = 200,
            SDLK_WORLD_41      = 201,
            SDLK_WORLD_42      = 202,
            SDLK_WORLD_43      = 203,
            SDLK_WORLD_44      = 204,
            SDLK_WORLD_45      = 205,
            SDLK_WORLD_46      = 206,
            SDLK_WORLD_47      = 207,
            SDLK_WORLD_48      = 208,
            SDLK_WORLD_49      = 209,
            SDLK_WORLD_50      = 210,
            SDLK_WORLD_51      = 211,
            SDLK_WORLD_52      = 212,
            SDLK_WORLD_53      = 213,
            SDLK_WORLD_54      = 214,
            SDLK_WORLD_55      = 215,
            SDLK_WORLD_56      = 216,
            SDLK_WORLD_57      = 217,
            SDLK_WORLD_58      = 218,
            SDLK_WORLD_59      = 219,
            SDLK_WORLD_60      = 220,
            SDLK_WORLD_61      = 221,
            SDLK_WORLD_62      = 222,
            SDLK_WORLD_63      = 223,
            SDLK_WORLD_64      = 224,
            SDLK_WORLD_65      = 225,
            SDLK_WORLD_66      = 226,
            SDLK_WORLD_67      = 227,
            SDLK_WORLD_68      = 228,
            SDLK_WORLD_69      = 229,
            SDLK_WORLD_70      = 230,
            SDLK_WORLD_71      = 231,
            SDLK_WORLD_72      = 232,
            SDLK_WORLD_73      = 233,
            SDLK_WORLD_74      = 234,
            SDLK_WORLD_75      = 235,
            SDLK_WORLD_76      = 236,
            SDLK_WORLD_77      = 237,
            SDLK_WORLD_78      = 238,
            SDLK_WORLD_79      = 239,
            SDLK_WORLD_80      = 240,
            SDLK_WORLD_81      = 241,
            SDLK_WORLD_82      = 242,
            SDLK_WORLD_83      = 243,
            SDLK_WORLD_84      = 244,
            SDLK_WORLD_85      = 245,
            SDLK_WORLD_86      = 246,
            SDLK_WORLD_87      = 247,
            SDLK_WORLD_88      = 248,
            SDLK_WORLD_89      = 249,
            SDLK_WORLD_90      = 250,
            SDLK_WORLD_91      = 251,
            SDLK_WORLD_92      = 252,
            SDLK_WORLD_93      = 253,
            SDLK_WORLD_94      = 254,
            SDLK_WORLD_95      = 255,      // 0xFF
            //@}

            // @name Numeric keypad 
            //@{
            SDLK_KP0      = 256,
            SDLK_KP1      = 257,
            SDLK_KP2      = 258,
            SDLK_KP3      = 259,
            SDLK_KP4      = 260,
            SDLK_KP5      = 261,
            SDLK_KP6      = 262,
            SDLK_KP7      = 263,
            SDLK_KP8      = 264,
            SDLK_KP9      = 265,
            SDLK_KP_PERIOD      = 266,
            SDLK_KP_DIVIDE      = 267,
            SDLK_KP_MULTIPLY   = 268,
            SDLK_KP_MINUS      = 269,
            SDLK_KP_PLUS      = 270,
            SDLK_KP_ENTER      = 271,
            SDLK_KP_EQUALS      = 272,
            //@}

           // @name Arrows + Home/End pad
            //@{
            */
            case SDLK_UP:     //    = 273,
            {
                printf("\n SDLK_UP hit. scroll command. ") ;
                bool truth = true ;
                mouse_scroll_command(&truth) ;
                // this is a keyboard duplicate of the mouse-scroll command. 
                break ;
            }
            case SDLK_DOWN:           //= 274
            {
                printf("\n SDLK_DOWN hit. scroll command. ") ;
                bool truth = false ;
                mouse_scroll_command(&truth) ;
                break ;
            }
            /*
            SDLK_RIGHT      = 275,
            SDLK_LEFT      = 276,
            SDLK_INSERT      = 277,
            SDLK_HOME      = 278,
            SDLK_END      = 279,
            SDLK_PAGEUP      = 280,
           SDLK_PAGEDOWN      = 281,
            //@}

           // @name Function keys 
            //@{
            */
            case SDLK_F1:   // = 282,
            {
                printf("\nF1 hit. \n") ;
                LoadMapNames(true) ;
                break ;
            }
            // TODO: define a command (such as F1, F12, Home, etc.) which resets controls to all default states 
            // (nothing pressed, correct command set for current interface). 
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
                // TODO: add to timestamped logging system, if logging level is appropriate.  
                engine.toggle_fullscreen() ;
                printf("\nFullscreen toggle requested. Engine reports screen status is now %s.\n", 
                    engine.fullscreen ? ("fullscreen") : ("windowed")
                    ) ;
                break ;
            }
           
            // toggle the showing of triangles 
            case SDLK_F4:
            {
                // TODO: put this in a menu that contains this and other dev and debug-related options. 
                extern bool showpolys ;
                showpolys = !showpolys ;
                break ;
            }

            case SDLK_F5: //         = 286,
            {
                // TODO: also put this in a dev menu. 
                //extern bool updatephysics ;
                //updatephysics = !updatephysics ;
                extern bool updateeditor ;
                updateeditor = !updateeditor ;
                break ;
            }
            case SDLK_F6: //         = 287,
            {
                extern void reset_physics() ;
                reset_physics() ;
                break; 
            }
            case SDLK_F7: //          = 288,
            {
                // TODO: when a screenshot is taken, a global variable should 
                // already know what the highest-numbered screenshot is, and save the 
                // new one to a higher number. 
                char duh[] = "hello.bmp" ;
                Screenshot(duh) ;
                break ;
            }
            case SDLK_F8:        // = 289,
            {
                // TODO: when a file being worked on is saved, a new one of that name 
                // gets saved here, after the latest one is renamed to a backup version.
                // If this is a new unnamed map, then a query pops up to ask the user what 
                // name is desired here. 
                world.SaveToFile("map.txt") ;
                break ;
            }
            case SDLK_F9:       //          = 290,
            {
                // TODO: when load is invoked, first a popup should ask which map to load. 
                // This popup will show a navigable list of maps, as well as some quick keys
                // to load a recent map. 
                //world.LoadFromFile("map.txt") ;
                LoadWorld("hyueuk heyuek") ;
                break ;
            }
               
            case SDLK_F10:      //       = 291,
            {
// TODO: this is just for testing during prototype stage. Remove when 
// shaders are the normal way to render. 
// extern void loadShaderFiles() ;
// loadShaderFiles() ;
engine.current_w -= 100 ;
engine.current_h -= 100 ;
        SDL_SetVideoMode( engine.current_w, engine.current_h, SCREEN_BPP, ((engine.fullscreen)   ?  engine.videoFlagsFS : (engine.videoFlags|SDL_RESIZABLE)) ) ; 

                resize_window( engine.current_w, engine.current_h, engine.fov ) ; 
                break ;
            }
/*
            SDLK_F11      = 292,
            SDLK_F12      = 293,
            SDLK_F13      = 294,
            SDLK_F14      = 295,
            SDLK_F15      = 296,
            //@}

           // @name Key state modifier keys
            //@{
            SDLK_NUMLOCK      = 300,
            SDLK_CAPSLOCK      = 301,
            SDLK_SCROLLOCK      = 302,
            SDLK_RSHIFT      = 303,
*/
            case SDLK_LSHIFT:
            {
                // TODO: place basic_velocity somewhere else. Like in Engine.physics. 
            extern float basic_velocity ;
                if ( basic_velocity > 25000 )
                {
                    basic_velocity = 100 ;
                }
                else
                {
                    basic_velocity *= 5 ;
                }
            }
            /*
            SDLK_RCTRL      = 305,
            SDLK_LCTRL      = 306,
            SDLK_RALT      = 307,
            */
            case SDLK_LALT:
            {
//                last_commands = current_commands ;
//                current_commands = l_alt_commands ; 

                // enable tabbing out of application 
                break ; 
            }
/*
            SDLK_RMETA      = 309,
            SDLK_LMETA      = 310,
            SDLK_LSUPER      = 311,      //< Left "Windows" key 
            SDLK_RSUPER      = 312,      //< Right "Windows" key 
            SDLK_MODE      = 313,      //< "Alt Gr" key 
            SDLK_COMPOSE      = 314,  //< Multi-key compose key 
            //@}

            // @name Miscellaneous function keys 
            //@{
            SDLK_HELP      = 315,
            SDLK_PRINT      = 316,
            SDLK_SYSREQ      = 317,
            SDLK_BREAK      = 318,
            SDLK_MENU      = 319,
            SDLK_POWER      = 320,      //< Power Macintosh power key 
            SDLK_EURO      = 321,      //< Some european keyboards 
            SDLK_UNDO      = 322,      //< Atari keyboard has Undo 
            //@}
*/
            case SDLK_a:
            {
                camera.set_move_left() ; 
                break ; 
            }
            //SDLK_b         = 98,
            //SDLK_c         = 99,
            case SDLK_d:
            {
                camera.set_move_right() ; 
                break ; 
            }
            case SDLK_e:
            {
                engine.editing = !engine.editing ;

                if (engine.editing) 
                    printf("\n now editing \n") ; 

                break ; 
            }
            //SDLK_f         = 102,
            
            case SDLK_g:         // = 103,
            {
                
                toggle_gridsize_scroll() ;
                break ;
            }
            //SDLK_h         = 104,
            //SDLK_i         = 105,
            //SDLK_j         = 106,
            //SDLK_k         = 107,
            //SDLK_l         = 108,
            //SDLK_m         = 109,
            //SDLK_n         = 110,
            //SDLK_o         = 111,
            //SDLK_p         = 112,
            //SDLK_q         = 113,
            case SDLK_q:
            {
                toggle_deform_scroll(true) ; // releasing q disables cube deformation
                break ;
            }
            //SDLK_r         = 114,
            case SDLK_s:
            {
                camera.set_backward() ; 
                break ; 
            }
            //SDLK_t         = 116,
            //SDLK_u         = 117,
            //SDLK_v         = 118,
            case SDLK_w:
            {
                camera.set_forward() ; 
                break ; 
            }
            
            //SDLK_x         = 120,
            case SDLK_y://         = 121,
            {
                toggle_texture_scroll( true ) ;
                // enable texture scrolling
                //zzz
                break ;
            }
            //SDLK_z         = 122,
            case SDLK_z:
            {
                static int lastfov = engine.fov ;
                if ( engine.fov > 50 )
                {
                    engine.fov = 30 ; 
                    msensitivity = 1.0f ;
                }
                else // zzz   mouse sensitivity needed here 
                     // (sensitive for normal operation, slow for in-fovus operation)
                {
                    engine.fov = lastfov ; 
                    msensitivity = 3.0f ;
                }
                resize_window( engine.current_w, engine.current_h, engine.fov ) ; 
                break ;
            }
            default: 
            {
//                if (!commands_initialized){init_commands() ;}
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
//                current_commands = last_commands ;
//                last_commands = NULL ;
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
                camera.stop_move_left() ;
                break ;
            }
            case SDLK_d:
            {   
                camera.stop_move_right() ;
                break ;
            }
            case SDLK_g:
            {
                // releasing g disables gridsize scrolling
                toggle_gridsize_scroll( false ) ; 
                break; 
            }
            case SDLK_q:
            {
                // releasing q disables cube deformation
                toggle_deform_scroll( false ) ; 
                break ;
            }
            case SDLK_y:
            {
                toggle_texture_scroll( false ) ;
                break ;
                // releasing y disables texture scrolling. 
            }
            default:
            {
                //if (annoying)printf(" unhandled and discarded command key-up ") ;
                return ;
                break ;
            }
        } 
    }// end input processing 

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
void handle_console_key_in()
{
    // possible actions: 
    //      - add a character to the input line buffer, if space remains
    //      - remove a character from the input line buffer
    //      - nothing
    //      - move the cursor along the input line buffer
    //      - trigger the execution of a command. 
}


void init_input()
{

    init_commands() ; 

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

    #define GRIDSCALE_MSG 0
    sprintf( input_msgs[GRIDSCALE_MSG], "world.gridscale=%d (%.2fm)", 
        world.gridscale,
        //1<<(world.gridscale-6)
        (1<<world.gridscale) / 64.0 
        ); input_msgs_num++ ;

    #define DIRVEC_MSG 1
    sprintf(
        input_msgs[DIRVEC_MSG], 
        "direction vector : %.2f %.2f %.2f %.2f", 
        camera.dir.x,
        camera.dir.y,
        camera.dir.z, 
        camera.pitch
        ) ; input_msgs_num++ ;

    #define ANGLES_MSG 2
    sprintf( input_msgs[ANGLES_MSG], "pitch = %.2f and sin (theta) = %.2f", 
        camera.pitch/10.f, sin(  camera.pitch      * deg_to_radians )
        ); input_msgs_num++ ;

extern float basic_velocity ;
    #define VEL_MSG 3
    sprintf(
        input_msgs[VEL_MSG], 
        " basic_velocity set to %.2f ",
        basic_velocity 
        ); input_msgs_num++ ;
    sprintf( input_msgs[input_msgs_num], "engine.activetex=%d   coord=%f", engine.activetex, (((float)engine.activetex)+0.5f)/3.0f); input_msgs_num++ ;
}


/*
    Actions which modify the key mappings
*/
void setcommands(void (*newcommands[320])(void*)) 
{
    current_commands = newcommands ;
}

//typedef void (**commandptr)(void*) ;

commandptr getcommands() 
{
    return commands ;
}

/*
    Return to default set of commands. 
*/
void resetcommands() 
{
}


