
/*
 * 	The philosophy of menus: 
 *
 * 	A menu is any number of items which have a display and an 
 * 	associated set of key bindings. 
 *
 * 	Typically, the display will be a label and/or possibly an image, 
 * 	while the bound commands will be focus movement for the arrows, 
 * 	and a single executable command bound to enter. 
 *
 * 	Menus can be used to browse things: 
 * 		- files
 * 		- resources (maps, models, sounds, music, textures, etc.)
 * 	and menus can also be used to choose current mode (play, edit, 
 * 	demo, etc.)
 */

#include "recalc.h"


#define MAIN_MENU 0
#define MAP_MENU 1

struct MenuItem
{
    char name[64] ;
    void (*action)(void) ;
} ;


/*
    Basic menu structure.

    Main menu: 
        edit map
        load game
        save game
        load map
        exit
*/
struct Menu
{
    int numMenuItems ;      // Number of available items in this menu. 
    int CurrentMenuItem ;   // Currently selected menu item. 
    int LowestVisible ;
    int HighestVisible ;
    void (*activate)() ;    // What function is called when this menu is activated. 
    Menu* PreviousMenu ;    // To go back when menus are nested. 

    Menu()
    {
        numMenuItems = 0 ;        // Number of available items in this menu. 
        CurrentMenuItem = 0 ;     // Currently selected menu item. 
        LowestVisible = 0 ;
        HighestVisible = 0 ;
        activate = NULL ;  // What function is called when this menu is activated. 
    }

    vector<MenuItem> items ;
    //void activate()
    //{   
        //activate() ;
    //}

    void select()
    {
        if ( items[CurrentMenuItem].action )
        {
            items[CurrentMenuItem].action() ;
        }
        else
        {
            printf("menu item %s selected, but no action available. ", 
                items[CurrentMenuItem].name
                ) ;
        }
    }

    void menu_up()
    {
        CurrentMenuItem-- ;   // Decrement means up the list. 
        if ( CurrentMenuItem < 0 )
        {
            CurrentMenuItem = numMenuItems-1 ;
        }
    }

    void menu_down()
    {
        CurrentMenuItem++ ;   // Increment means down the list
        if ( CurrentMenuItem >= numMenuItems )
        {
            CurrentMenuItem = 0 ;
        }
    }

    void add(MenuItem item)
    {
        items.add(item) ;
        numMenuItems++ ;        // Number of available items in this menu. 
    }

    void render()
    {
    }

} ;


void menu_select(void*) ;
void menu_down(void*) ;
void menu_up(void*) ;

Menu* GetCurrentMenu() ;

void initialize_menu() ;

void ListMapFiles(bool toConsole) ;

void ActivateMainMenu() ;

void ActivateMapMenu() ;

void init_menus() ;


