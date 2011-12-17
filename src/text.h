
#ifndef __TEXT_H__
#define __TEXT_H__

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
/*
*/

/*
*/
struct FontCharacter
{
} ;


struct Font
{
    // have you had opportunity to learn about this 
    char name[64] ;

    Font(const char * in_name)
    {
        strcpy(name, in_name) ; 
        printf("\nINIT::FONTS: created a new font named %s", name) ; 
    }

    unsigned int id ;           // internal identifier 

    GLuint * gl_char_IDs ;
    GLuint gl_list_base ;

    int _height ;   // the height that a general line of this font would have
    int _width ;    // the max amount of space a character of this type would need 

    char height[128] ;
    char width[128] ;
    // this function loads as a bitmap a font image. 
    // the bitmap is kept in memory, while glID's
    // are used to refer to and render characters. 

    void LoadFontFile() ; 

    /*
        Draws a string on the screen, sending the string to whatever the XY plane 
        currently defines in the OpenGL context. 
    */
    void DrawString() ; 
} ; 

void initialize_text() ; 

#endif // __TEXT_H__


