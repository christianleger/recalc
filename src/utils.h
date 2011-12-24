/*
    File: utils.h


    Description: this file provides useful functions to help our program. 



    Modules defined here: 
        
            - time measurement 

            - debugging tools

            - logging tools

*/





// FIXME: set this to be defined in a build config
#define DEV


#ifdef DEV
    #define DEBUGTRACE(msg) printf msg 
#else
    #define DEBUGTRACE(msg) 
#endif







