/*
    File: console.cpp

    Copyright 2012 Christian Leger. 
*/
#include "recalc.h"

void complete_command (void*) ;
Console console ;
#define c console
extern vector<Font*> fonts ;
extern Engine engine ;

/*
    Points to commands that can be mapped to key hits when in console mode. 
*/
void (* console_commands[320])(void *) ;

// The purpose of this array is to provide the right value to the text system when a shift 
// key is in use. 
// Unlike the regular alphabet, for which capitalization is as simple as subtracting 32, 
// other characters, like '?', are not obtained simply by subtracting 32 from '/'. Hence, 
// this stupid array. 
// This doesn't support anything more than regular ascii for the moment. Shit. 
SDLKey shift_values[320] ;  // TODO: needed? 

void stub(void *)
{
    printf("\nI do nothing. Hit a useful key or define useful functions. \n") ;
}


extern void (* current_commands[320])(void *) ;
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
void backspace( void * unused )
{
    if (c.current_char>0)
    {
        c.current_line_pix_len -= 
            fonts[0]->width[c.line_buffer[c.current_char-1]]+2 ;
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
    else { upper_case = 0 ; }

    if (c.current_char<CONSOLE_LINE_LENGTH+1)
    {
        c.line_buffer[ c.current_char ] = key - upper_case ;
        c.line_buffer[ c.current_char + 1 ] = '\0';
        c.current_char++ ;
        c.current_line_pix_len += fonts[0]->width[ key - upper_case ] + 2 ; // FIXME: this length should just be computed when console is drawn 
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

    console_commands[SDLK_TAB] = &complete_command ;
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


/*
    Receive a message from caller, and display in console. 
*/
void Console::message(char const* msg)
{
    //strncpy(lines[c.current_line], msg, strlen(msg) ) ;
    strcpy(lines[c.current_line], msg) ;//, strlen(msg) ) ;
    current_line++ ;
    if (current_line>=CONSOLE_BUFFER_LINES)
    {
        current_line = 0 ; 
    }
}

/*
*/
Console& GetConsole()
{
    return console ;
}


/*
*/
struct letternode
{
    char letter; 
    bool iscommand; 
    letternode* right; 
    letternode* down; 
    letternode* left; 
    letternode* up; 

    letternode() 
    {
        letter = 0; 
        iscommand = false; 
        right = NULL ; 
        down = NULL ; 
        left = NULL ; 
        up = NULL ; 
    }
}; 


vector<letternode*> letternodes; 
int letternodecount = 0; 

void initletternodes()
{
    if (letternodes.length() > 0) return ; 

    printf("\ninitializing the letternodes array. \n"); 

    letternode* first = new letternode() ; 
        
    letternodes.add(first); 

    return ;
}


/*
*/
void addlookupcommand(const char * name)
{
    initletternodes();

    letternode* letter = new letternode(); 
    letter = letternodes[0];  // this is the empty letternode. 

    int n = strlen(name); 
    bool letterdone = false; 

    // each iteration of the loop handles one letter of the command's name 
    for (int i=0 ; i<n; i++ ) 
    {
        // when we enter this loop, we have either started at the begining or incrementded i. 
        letterdone = false; 

        if (letter->right==NULL)
        {
            letter->right = new letternode() ;
            letter->right->left = letter; 
            letternodes.add( letter->right ) ;

            letter = letter->right; 
            letter->letter = name[i];

            letterdone = true ; 
        }
        else
        {
            letter = letter->right; 
        }

        // Now we're in the right column - find the position we want to be in this 
        // column with the current letter name[i]. 
        while (!letterdone)
        {
            if (name[i] == letter->letter)
            {
                letterdone = true; 
                continue; 
            }
            if ( name[i] > letter->letter ) // go down the column to where this letter is found or needs to be inserted. 
            {
                if (letter->down)
                {
                    letter = letter->down; 
                }
                else // we're at the end of this column and the letter we want to follow is lower than what's in the column. add. 
                {
                    letter->down = new letternode() ;
                    letternodes.add( letter->down ) ;

                    letter->down->up = letter ; 
                    letter->down->letter = name[i] ;
                    letter = letter->down; 
                    letterdone = true; 
                }
                continue; 
            }
            if ( name[i] < letter->letter ) // if we get to this case it means we're inserting between two letters. 
            {
                letternode* newletter = new letternode() ; 
                letternodes.add( newletter );

                newletter->letter = name[i] ; 

                // this case happens when letter is at the top of its column 
                if (letter->left) 
                { 
                    letter->left->right = newletter; 
                    newletter->left = letter->left; 
                    letter->left = NULL; 
                }

                if (letter->up)
                {
                    letter->up->down = newletter; 
                }

                newletter->down = letter ; 
                newletter->up = letter->up; 
                letter->up = newletter; 

                letter = newletter; 

                continue; 
            } // end if name[i] < letter->letter 
        } // end while ( !done ) 
    }
    // Add the end of this loop, our current letter node points to the end of the command's name. 
    // We check to see if this command doesn't already exists. If it doesn't, we set the command flag. 
    if ( !letter->iscommand )
    {
        letter->iscommand = true; 
    }

    printf("\n added a command to the lookup table. letternodes.length() = %d\n", letternodes.length()); 
}

// Here, LN stands for letter-node. 
#define LN_RIGHT 0 
#define LN_DOWN 1
#define LN_LEFT 2
#define LN_UP 3

/*
    iterate through all commands which begin with _prefix 
*/
void showallcommands(char * _prefix)
{
    initletternodes() ;
    int commandcount = 0 ; 
    char cname[256] ;  
    char prefix[256] ; 
    char lastmove = 0 ; 
    char done = false; 

    lastmove = LN_RIGHT ; 

    if (strlen(_prefix)==0) { return ; }
    if (_prefix[0]!='/') // if there's isn't a leading slash, it's not a command, so don't complete. 
    {
        printf("\nnot completing non-command. ") ;
        return ; 
    }

    // skip initial slash of the _prefix argument. Could be just \0 after the /. 
    strcpy(prefix, _prefix+1) ;  
    
    // letternodes[0] is just a handle. ->right is the first real node. 
    letternode* letter = letternodes[0]->right ;  
    if (letter==NULL) { return ; } /* unlikely but possible */ 
    letternode* lastletter = letter ; 
    
    int i = 0; 
    int len  = strlen(prefix); 

    //
    // first phase : follow through links as long as we have matching letters. 
    //
    while (!done && i<len)
    {
        if ((letter->down) && (prefix[i] > letter->letter))
        {
            letter = letter->down ; 
        }
        else { done = true ; }

        if (prefix[i]==letter->letter)
        {
            done = false ; 
            if (letter->right)
            {
                i++ ; 
                lastletter = letter ; 
                letter = letter->right ; 
            }
        }
    }
    // if we've found nodes for every letter of prefix (i==len-1), then we're showing suffixes that can 
    // be used from this point

    // otherwise, we get out because there are no commands with this prefix 
    strcpy(cname, prefix) ; 
    cname[i] = '\0' ; 
    letternode* startletter  = NULL ; 

    /* 2 possible conditions at this point : 

            1 - our prefix is a command prefix (there exist commands with this prefix)
            2 - our prefix is not a command prefix. 

            In case 2 we just leave. 
            In case 1 i equals len, so that i holds the next position to play with. 
    */

    if (i==len && i>0)
    {
        done = false ; 
        startletter = lastletter ; 
    }
    else { return ; }   // looks good up to here! 

    // Now that we've reached this point, we're at the point in the lookup-list that 
    // matches the prefix. Next is to find all names that complete from this one. 
    letter = startletter ; 
    while ( !done )
    {
        // draconian precaution...
        //guard++; if (guard>500) { printf("\nguard max. bug you me but me not :) \n") ; break; }

        if ( lastmove == LN_RIGHT || lastmove == LN_DOWN )
        {
            if (letter->iscommand)
            {
                commandcount ++ ; 
                // complete the commandbuf with the first available command 
                if (commandcount==1)
                {
                    // skip first character which has '/'
                    strcpy(_prefix+1, cname) ; 
                    //printf("\nfound first command: %s\n", cname) ; 
                    //printf("\ncommandbuf is now: %s\n", _prefix) ; 
                }
                //conoutf("command: %s", cname); 
                //printf("command: %s", cname); 
                //printf("\n********** found a command and commandcount == %d**********\n", commandcount ) ; 
            }
        }

        // condition for having no more commands 
        //if (letter==letternodes[0])
        if (letter==startletter && (lastmove==LN_UP || lastmove==LN_LEFT))
        {
            done = true; 
        }

        if ( letter->right  && (lastmove == LN_RIGHT || lastmove == LN_DOWN )) 
        { 
            //printf("\n (RIGHT) Comparing letter (%c) to prefix (%c) for position %d", letter->letter, prefix[i], i); 
            //if ( letter->letter == prefix[i-1] )
            {
                letter = letter->right ; 
                cname[i] = letter->letter ; 
                i++ ; 
                cname[i] = '\0' ; 
                // printf("\n (RIGHT) Pos %d Just assigned letter %c to command %s\n", i-1, letter->letter, cname); 
                lastmove = LN_RIGHT ; 
                continue ; 
            }
        }

        if ( letter->down && lastmove != LN_UP )  
        { 
            letter = letter->down  ; 
            cname[i-1] = letter->letter ;
            lastmove = LN_DOWN ; 
            continue ; 
        }

        if ( letter->left ) 
        { 
            letter = letter->left ; 
            i--; 
            cname[i] = '\0'; 
            lastmove = LN_LEFT ; 
            continue ; 
        }

        if ( letter->up ) 
        { 
            letter = letter->up ; 
            cname[i-1] = letter->letter ;
            lastmove = LN_UP ; 
            continue ; 
        }

    }
    return ; 
}


/*
    This function modifies the command buffer, extending until it completes a 
    name or until several commands are available for the name build thus far. 
*/
void complete_command (void*)
//complete(char *s)
{
    char* s = &console.line_buffer[0] ;
    static int commander = 0; 
    if ( commander == 0 )
    {
        commander ++; 
        addlookupcommand("engine.quit"); 
        addlookupcommand("quit"); 
        addlookupcommand("quitter"); 
        addlookupcommand("iamaschoolteacher"); 
        addlookupcommand("iaman ass"); 
        addlookupcommand("iama"); 
        addlookupcommand("iamanassdealer"); 
        addlookupcommand("iamaschoolteacher"); 
        addlookupcommand("onetwoonetontoomany"); 
    }
   
    /* evil debugging
    for (int i=0;i<letternodes.length();i++){
    conoutf("%c -> left -> %c", letternodes[i]->letter, 
                                (letternodes[i]->left)?(letternodes[i]->left->letter):('-') );}
    for (int i=0;i<letternodes.length();i++)
    {conoutf("%c -> up -> %c", letternodes[i]->letter, 
                               (letternodes[i]->up)?(letternodes[i]->up->letter):('-') );}
    for (int i=0;i<letternodes.length();i++){
    conoutf("%c -> right -> %c", letternodes[i]->letter, 
                                 (letternodes[i]->right)?(letternodes[i]->right->letter):('-') );}
    for (int i=0;i<letternodes.length();i++){
    conoutf("%c -> down -> %c", letternodes[i]->letter, 
                                (letternodes[i]->down)?(letternodes[i]->down->letter):('-') );}
    */
    showallcommands(s); 

    if(*s!='/')
    {
        string t;
        copystring(t, s);
        copystring(s, "/");
        concatstring(s, t);
    }

    return;
}


/*
*/
void cleanup_console() 
{
    // Reclaim memory for letter nodes
    // etc. 
}


