/*
    File: scripting.cpp

    Description: 
        
        Defines a simplistic language for Recalc's internal scripting needs. 


    Notes

        The basic functioning of a script function is that it looks for arguments 
        that it needs to do its job. Input arguments are always found in the same 
        static location, and the same goes for output arguments. 

        Thus, arbitrary arguments can be passed in, and arbitrary arguments can 
        also be returned. 
    
    NO RECURSION: 
        Every function call squashes the only stack frame we have. 

        Therefore, every function needs to collect its arguments at the begining, 
        and then it can call other functions no problem. 
*/
#include "recalc.h"

// I dub my scripting language: Calcscript. 
#define CS calcscript

// A reasonable number
#define SCRIPT_LINE_SIZE 128


struct CalcScriptVar
{
    char name[64] ; 
    void* values ;      // Arbitrary data. Functions know how to handle data destined for them. 
    UT_hash_handle hh ;
} ;
#define CSVar CalcScriptVar

// A hash node to hold all the command names and their corresponding functions. 
/*
    Script commands are restricted to 8 arguments - it's enough for me. 
*/
struct CalcScriptCommand
{
    char name[64] ;            // Hey: with simplicity, I ask for 64 chars being ok for your function names. 
    char sig[8] ;              // Signature: chars 0-8 give whether args to a function are int, float, str or CSVar. 
    void (*command)(void*) ;
    UT_hash_handle hh ;
} ;
#define CSC CalcScriptCommand

struct ScriptArg
{
    char stringflag ;   // These flags indicate which args are available. 
    char intflag ;
    char floatflag ;

    char sargs[8][64] ;
    int iargs[8] ;
    float fargs[8] ;

    void reset()    // When a function uses args, it should clear them. 
    {
        stringflag = 0 ;
        intflag = 0 ;
        floatflag = 0 ;
    }
} ;

ScriptArg CSArgs ;

void testfunc(void*)
{
    printf("\n testfunc called from script!") ;
}

void loadtexture(void*)
{
}

void loadmodel(void*)
{
}

void loadsound(void*)
{
}

void loadmusic(void*)
{
}

/*
    Here we define all our known language words and symbols. 
*/
CalcScriptCommand CSCommandDefs[] = 
{
      {"testfunc",      "", testfunc}
    , {"readconfig",    "s", readconfig}
    , {"runscript",    "s", runscript}
    , {"texture",       "s", loadtexture}
    , {"model",         "s", loadmodel}
    , {"sound",         "s", loadsound}
    , {"music",         "s", loadmusic}
    , {"", NULL}
} ;

// The hash structure to hold all the command names and their corresponding functions. 
struct CalcScriptCommand* CSCommands = NULL ;

// The hash structure to hold all the CalcScript variables 
struct CalcScriptVar* CSVars = NULL ;




/*
    inputs: 
        n - which (of 8) arg is being set.
        val - the value of the arg. 

*/
void SetStrScriptArg(int n, const char* val)
{
    strcpy(CSArgs.sargs[n], val) ;
    CSArgs.stringflag |= 1<<n ;
}


/*
*/
void SetIntScriptArg(int n, int val)
{
    CSArgs.iargs[n] = val ;
    CSArgs.intflag |= 1<<n ;
}


/*
*/
void SetFloatScriptArg(int n, float val)
{
    CSArgs.fargs[n] = val ;
    CSArgs.floatflag |= 1<<n ;
}


/*
    Get the next set of characters in the given buffer 
    which do not equal the delimiter character. 

    Default delimiter: space. 

    Permanent secondary delimiters: '\0' and '\n'
*/
bool gettoken(const char* buf, char* result, char delimiter=' ')
{
    int i = 0 ;
    char c = 0 ;
    for (i=0;i<128;i++)
    {
        c = buf[i] ;
        result[i] = c ;
        if (c == delimiter || c =='\0' || c == '\n') 
        { 
            result[i] = '\0' ;
//       result[i+1] = '\0' ;
            break ; 
        }
    }
    if (i==0)   { return false ; }  // If no token found, say false
    else        { return true ; }
//        printf("\ngettoken: str = %s (i=%d) (delimiter=%d)", result, i, delimiter) ;
}


/*
    Read a config file and execute its contents. 

    Config file commands are normally focused on setting up assets and resources, 
    but can also be arbitrary script commands if needed. 

    Inputs: 
        This is a script function, so the C++ function doesn't take parameters, 
        but it actually looks for parameters in the arguments struct. 

*/
void readconfig(void* nothing)
{
    /*
        EVERY SCRIPT FUNCTION SHOULD DO THIS: 
            - copy args to local vars
            - clear args so that other script functions called can get their 
              args from the same location. 

        This artificial rule allows us to treat functions with very simple 
        assumptions. 
    */
    if (!(CSArgs.stringflag&0x1)) { //printf("\n************      readconfig quitting cuz no args.************      ") ;
        return ;
    }
    char fname[64] ; 
    strcpy(fname, CSArgs.sargs[0]) ;
    CSArgs.reset() ;

// TODO: log instead   printf("\n READCONFIG called on %s", fname) ;

    FILE* fp = fopen(fname, "r") ;

    if (!fp)
    {
        printf("\n readconfig error: no file %s found. Aborting. ", fname) ;
        return ; //Quit(1) ;
    }
    char buf[SCRIPT_LINE_SIZE] ;

    while (!feof(fp))
    {
        fgets(buf, SCRIPT_LINE_SIZE, fp) ;

        // For each line: tokenize. 
        char str[64] ; 
        char* str_ptr = str ;
        int i = 0 ;

        // replace by call to gettoken
        gettoken(buf, str_ptr) ;

        CSC* s = NULL ;
        HASH_FIND_STR(CSCommands, str_ptr, s) ;

        // If we identify a known command, we fetch needed args and execute. 
        if (s) 
        {  
            str_ptr = &buf[strlen(str_ptr)+1] ;
            loopj(8)
            {
                // We use the command signature to decide which arguments to look for. 
                if (!s->sig[j]) 
                {
                    // If we get here, we're done fetching args for this function. 
                    if (gettoken(str_ptr, str)) 
                    {
                        // If too many args, we quit this particular function. 
                        printf("\n %s: too many arguments for command %s", s->name, s->name) ; 
                        return ;
                    }
                    else
                    {
                        printf("\n finishing line read: last character was '%c'", str_ptr[0]) ;
                    }
                    break ;
                }
                // If needed args not found, then we abort this command. 
                if (!gettoken(str_ptr, str)) 
                { 
                    printf("\n %s: arg %d (%c) not found!", s->name, j, s->sig[j]) ;
                    printf("\n Command %s unable to complete. Quitting. ", s->name) ;
                    return ;
                }
                if (s->sig[j]=='s')         // String type
                {   
                    SetStrScriptArg(i, str) ;
                    printf("\n %s: setting script arg: %s", s->name, str) ;
                }
                else if (s->sig[j]=='i')    // Int type
                {
                    SetIntScriptArg(i, atoi(str)) ;
                }
                else if (s->sig[j]=='f')    // Float type
                {
                    SetFloatScriptArg(i, atof(str)) ;
                }
                else if (s->sig[j]=='v')    // General Var type
                {
                }
                str_ptr+= strlen(str)+1 ;
            }

            // Execute. 
            s->command(NULL) ;
        }
        // If a token wasn't a command we check to see if it was a new or old variable
        else
        {
        }

        // Identify each token as either function or variable
        // HASH_FIND_STR(CSVars, str_ptr, s) ;

        // Execute command
    }

    fclose(fp) ;
}


/*
    This function is just a synonym for readconfig. Purely out of convenience 
    for a human reader of CalcScript. 
*/
void runscript(void* nothing)
{
    readconfig(NULL) ;
}

/*
*/
void init_scripting()
{
    //  Add all commands to commands table. 
    for (int i=0;CSCommandDefs[i].command!=NULL;i++)
    {
        // Because HASH_ADD_STR is macro and not a function, we need a pointer csc for it to work. 
        CSC* csc = &CSCommandDefs[i] ;          
        printf("\n adding to commands table: %s", csc->name) ;
        HASH_ADD_STR(CSCommands, name, csc) ;
    }

    // Read and execute main config file. 
    SetStrScriptArg(0, "data/config.cfg") ;
    readconfig(NULL) ;
}





