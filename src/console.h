
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

//#include <SDL.h>
#include "recalc.h"
#define CONSOLE_V_SIZE 40

// wrap lines? Hopefully not. Hopefully, no lines need to be more than 120 chars in length. 
#define CONSOLE_H_SIZE 120 


// surely 10,000 is enough .... eerr .. right? 
#define CONSOLE_BUFFER_LINES 10000

// Now if you complain about this here, well too bad !
// Or you can talk to me about setting up variable-length lines. 
#define CONSOLE_LINE_LENGTH 256

// Basic, default size of the console: 256 X 10,000 = 2,560,000 bytes. 2.5mb. 
// A perfectly acceptable price for an easily-managed console. 

// Note: Console internal behaviors. 
//      - when the buffer grows to its max size, then new lines wrap to the 
//        start of the line buffer. I don't think I'll always care if I 
//        lose the first few lines after more than 10,000 lines of output. 

struct Console
{
    // data containers 
    char lines[CONSOLE_BUFFER_LINES][CONSOLE_LINE_LENGTH] ;
    char line_buffer[CONSOLE_LINE_LENGTH];


    // cursor management 
    int current_line ;
    int current_line_pix_len ; // pixel length of the line, for cursor placement

    int top_scr_line ;
    int current_char ; // index of next character to use. Also used as length. 

    void initialize() ;

} ;

/*
    Yep. Another orphan child victim of my pointers to (* void)(void *) 
    for all functions campaign. Life is good sometimes. 
*/
void key_in( void * key ) ;




#endif //__CONSOLE_H__


