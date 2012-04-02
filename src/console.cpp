

#include "recalc.h"


Console console ;
extern vector<Font*> fonts ;

extern Engine engine ;
/*
struct Console
{
    // data containers 
    char lines[CONSOLE_BUFFER_LINES][CONSOLE_LINE_LENGTH] ;
    char line_buffer[CONSOLE_LINE_LENGTH];


    // cursor management 
    int current_line ;
    int current_char ;

    void handle_key() ;
} ;
*/




/*
    This function pointers array will point to any 
    and all functions we want to use. 

    When the program starts up, the whole thing is packed with 
    a stub function, to prevent segfaults or whatever, 
    and afterwards all functions needed for specific keys are 
    assigned to those keys. 

*/
void (* console_commands[320])(void *) ;

// The purpose of this array is to provide the right value to the text system when a shift 
// key is in use. 
// Unlike the regular alphabet, for which capitalization is as simple as subtracting 32, 
// other characters, like '?', are not obtained simply by subtracting 32 from '/'. Hence, 
// this stupid array. 
// This doesn't support anything more than regular ascii for the moment. Shit. 
SDLKey shift_values[320] ;

void stub(void *)
{
    printf("\nI do nothing. Hit a useful key or define useful functions. \n") ;
}


extern void (* current_commands[320])(void *) ;
/*
    This function assigns to the commands array the functions which 
    we want to be in use. 

*/



/*

    Ok. The console does at least the following with incoming key commands: 

                - record letters and symbols being typed 
                - execute any available commands found in the command line buffer
                - 

*/

/*
    Yo (my) dumbass can't handle something to type into without a 
    backspace. So check this. 
*/
#define c console
void backspace( void * unused )
{
    if (c.current_char>0)
    {
        c.current_line_pix_len -= 
            fonts[0]->width[c.line_buffer[c.current_char-1]] ;
        // padding: this should already be like that 
        c.line_buffer[c.current_char] = '\0' ; 
        c.current_char-- ;
        c.line_buffer[c.current_char] = '\0' ;
    }
    //printf("line_buffer now holds: \n\t%s\n", console.line_buffer ) ; 
}

/*
    ascii coming in, baby. 
    How shall we deal with unicode? psych!
*/
bool shifted = false ; 
void key_in( void * in_key ) 
{
    char upper_case = 0 ;

    if (!in_key)
    {
        printf("\nReceiving no keys to a function call to read keys? \n") ; 
        return ;
    }

    SDLKey key = *((SDLKey *) in_key) ;
    if ( shifted )
    {
        if ( key>96 && key<123 ) // from the alphabet? 
        {
            upper_case = 32 ;
        }
    }
    else 
    {
        upper_case = 0 ;
    }

    if (c.current_char<CONSOLE_LINE_LENGTH+1)
    {
    if (key=='b')
    {
        c.line_buffer[ c.current_char ] = 129 ;
    }
    else
    {
        c.line_buffer[ c.current_char ] = key - upper_case ;
    }


        c.line_buffer[ c.current_char + 1 ] = '\0';
        c.current_char++ ;
        c.current_line_pix_len += fonts[0]->width[ key - upper_case ] ; // FIXME: this length should just be computed when console is drawn 
    }

}

/*
    Read the current line buffer and do what must be done with it. 

    c.current_line increments every time we put in a new line. 
*/
void line_in(void *)
{
    printf("\n before reading the input line, line_buffer holds: %s\n", 
        c.line_buffer
        )  ;
    // at this point, the 'current_line' marker always points to the 
    // next location in which we will write to the console buffer. 

    // dump the buffer's content into the console lines buffer. 
    strncpy(c.lines[c.current_line], c.line_buffer, strlen(c.line_buffer) ) ;

    c.line_buffer[0] = '\0' ;
    c.current_char = 0 ;
    c.current_line_pix_len = 0 ;
    // FIXME TODO: actually execute whatever commands are found in the 
    // input line buffer :) 

    c.current_line++ ;
    if (c.current_line>=CONSOLE_BUFFER_LINES)
    {
        c.current_line = 0 ; 
    }

    printf("\n after reading the input line, line_buffer now holds: %s\n", 
        c.line_buffer
        )  ;

}

void shift(void *)
{
    shifted = !shifted ;
}

void shift_in(void * in_key)
{
    SDLKey key = *((SDLKey *) in_key) ;
    if (shifted)
    {
        *((SDLKey*)in_key) = shift_values[*(SDLKey*)in_key];//  *((SDLKey*)in_key)
        printf(
            "\n processing shifted key: %c\n", 
            *((SDLKey*)in_key)
            ) ; 
        key_in(in_key) ;
    }
    else
    {
        key_in(in_key) ;
    }
}


void Console::initialize()
{
    for (int i=0;i<320;i++)
    {
        console_commands[i] = NULL ;
    }

    console_commands[SDLK_BACKQUOTE] = &toggle_console ;

    // This is how you do it. Whatever key you hit, is sent to the command 
    // you want. 

    // All this can be overridden, and restored. FIXME: group all the default 
    // commands into a struct and define its command defaults restore function. 

    // Your monkey ass better have a backspace, so you can fix your mistakes of 
    // 90% of the time. 
    console_commands[SDLK_BACKSPACE] = &backspace ;



    // Your puny brain is so infinitesimal that you need to separate into 
    // separate tokens simple identifier codes. 
    console_commands[SDLK_RETURN] = &line_in;
    console_commands[SDLK_SPACE] = &key_in ;
    console_commands[SDLK_QUOTE] = &key_in ;

    console_commands[SDLK_TAB] = &key_in ;
    console_commands[SDLK_HASH] = &key_in ;
    console_commands[SDLK_COLON] = &key_in ;
    console_commands[SDLK_SEMICOLON] = &key_in ;


    console_commands[SDLK_EQUALS] = &key_in ;


    console_commands[SDLK_COMMA] = &shift_in ;
        shift_values[SDLK_COMMA] = SDLK_LESS ;
    console_commands[SDLK_PERIOD] = &shift_in ;
        shift_values[SDLK_PERIOD] = SDLK_GREATER ;
    console_commands[SDLK_SLASH] = &shift_in ;
        shift_values[SDLK_SLASH] = SDLK_QUESTION ;
    console_commands[SDLK_0] = &key_in ;
    console_commands[SDLK_1] = &key_in ;
    console_commands[SDLK_2] = &key_in ;
    console_commands[SDLK_3] = &key_in ;
    console_commands[SDLK_4] = &key_in ;
    console_commands[SDLK_5] = &key_in ;
    console_commands[SDLK_6] = &key_in ;
    console_commands[SDLK_7] = &key_in ;
    console_commands[SDLK_8] = &key_in ;
    console_commands[SDLK_9] = &key_in ;
    console_commands[SDLK_a] = &key_in ;
    console_commands[SDLK_b] = &key_in ;
    console_commands[SDLK_c] = &key_in ;
    console_commands[SDLK_a] = &key_in ;
    console_commands[SDLK_b] = &key_in ;
    console_commands[SDLK_c] = &key_in ;
    console_commands[SDLK_a] = &key_in ;
    console_commands[SDLK_b] = &key_in ;
    console_commands[SDLK_c] = &key_in ;
    console_commands[SDLK_a] = &key_in ;
    console_commands[SDLK_b] = &key_in ;
    console_commands[SDLK_c] = &key_in ;
    console_commands[SDLK_a] = &key_in ;
    console_commands[SDLK_b] = &key_in ;
    console_commands[SDLK_c] = &key_in ;
    console_commands[SDLK_d] = &key_in ;
    console_commands[SDLK_e] = &key_in ;
    console_commands[SDLK_f] = &key_in ;
    console_commands[SDLK_g] = &key_in ;
    console_commands[SDLK_h] = &key_in ;
    console_commands[SDLK_i] = &key_in ;
    console_commands[SDLK_j] = &key_in ;
    console_commands[SDLK_k] = &key_in ;
    console_commands[SDLK_l] = &key_in ;
    console_commands[SDLK_m] = &key_in ;
    console_commands[SDLK_n] = &key_in ;
    console_commands[SDLK_o] = &key_in ;
    console_commands[SDLK_p] = &key_in ;
    console_commands[SDLK_q] = &key_in ;
    console_commands[SDLK_r] = &key_in ;
    console_commands[SDLK_s] = &key_in ;
    console_commands[SDLK_t] = &key_in ;
    console_commands[SDLK_u] = &key_in ;
    console_commands[SDLK_v] = &key_in ;
    console_commands[SDLK_w] = &key_in ;
    console_commands[SDLK_x] = &key_in ;
    console_commands[SDLK_y] = &key_in ;
    console_commands[SDLK_z] = &key_in ;


    console_commands[SDLK_LSHIFT] = &shift ;
    console_commands[SDLK_RSHIFT] = &shift ;
    
    //console_commands[SDLK_SPACE] = 

}



















