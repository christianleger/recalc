

#include "recalc.h"



// the menu commands array 
void (* menu_commands[320])(void *) ; 

int current_menu_item = 0 ;
int num_menu_items = 0 ;

struct menu_item
{
    int bite_me ;
} ;
/*
    Default basic menu structure: 

    resume (if there is something to resume)
    load game
    save game
    load map
    exit
*/

/*
    Function: menu_select. 

    Execute a currently select command

    or 

    Go into a sub-menu. 
*/
void menu_select(void*)
{
}

void menu_up(void*)
{
    current_menu_item-- ;
    if ( current_menu_item > num_menu_items )
    {
        current_menu_item = 0 ;
    }
}

void menu_down(void*)
{
    current_menu_item++ ;
    if ( current_menu_item < 0 )
    {
        current_menu_item = num_menu_items ;
    }
}

extern void Quit(int) ;
void Quit(void *) { Quit(0) ;} 
void initialize_menu()
{

    // section: menu commands. the main menu is active when the 
    // program starts. 
    menu_commands[SDLK_RETURN] = &menu_select ;
    menu_commands[SDLK_DOWN] = &menu_down ;
    menu_commands[SDLK_UP] = &menu_up ;
    menu_commands[SDLK_BACKQUOTE] = &menu_up ;
}

