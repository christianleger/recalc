

#include "recalc.h"
#include "text.h"


/*
*/
FT_Library test_library;
FT_Face test_face;
FT_Glyph test_glyph;

int currentfont = 0 ; // default font is 0. Could be something else. 

/*  FUNCTION: next_p2 

    DESCRIPTION: utility function for font initialization. It is used in the
    creation of power-of-2 textures. This is likely unneeded with modern
    hardware, but it doesn't hurt with the small size of the textures
    involved. 

    Stolen from some tutorial on the net. 
*/
int next_p2( int a)
{
    int rval = 1;
    while (rval<a) { rval <<= 1; }
    return rval ;
}


void Font::LoadFontFile()
{
}


bool textenabled = true ;
vector<Font *> fonts ;
int numFonts = 0 ;
int curfont = 0 ;   // current font

int initialized = 0;
int yMax = 0; 
int yMin = 0; 
int desc = 0; 
int check_phase_done = 0; 

//Font mainfont ;
// luminance-alpha bitmaps of 16 by 16. 256 of them. 

// TODO FIXME: make mainfontpix a dynamic memory block - only used once for text 
// font read and once for submitting font pixels to GPU! 
static GLubyte mainfontpix[128*16*16*2] ;
GLuint mainfontID = 0 ;

void createFontDisplayList(
    FT_Face face,
    int ch, 
    Font * _font, bool check_phase=false
    )
{
    int i = 0; int j = 0;
    FT_Glyph glyph;

    FT_BBox bbox;

    FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT );

    if (ch>128) { ch = 129 ;return ; }

    FT_Get_Glyph( face->glyph, &glyph );
    FT_Glyph_Get_CBox( glyph, FT_GLYPH_BBOX_PIXELS, &bbox ); 

    FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
    FT_GlyphSlot slot = face->glyph; 

    // Now in all honesty the usage made here of the Freetype image dimension properties 
    // was established by trial and error and I have little understand of any but the most 
    // obvious of these properties. 
    int width ; //=  slot->bitmap.width ;
    int height ; //= slot->bitmap.rows ;
    int top =    slot->bitmap_top ;

    FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

    // This reference will make accessing the bitmap easier
    FT_Bitmap bitmap=bitmap_glyph->bitmap;

    // In order to do this process properly, we need a first run through to determine 
    // the dimensions we are dealing with. 
    if (check_phase)
    {
        _font->width[(int)ch]  = bitmap_glyph->left + (face->glyph->advance.x >> 6) ; 

        if ( ( ch > SDLK_a && ch < SDLK_z ) || ch==' ')
        {
            printf("\nwidth of letter %c = %d", ch, _font->width[(int)ch]) ; 
        }
        // character height
        _font->height[(int)ch] = bitmap.rows ; 

        // overall font dimensions 
        if (_font->_width < _font->width[(int)ch]) { _font->_width = _font->width[(int)ch]; }
        // seriously? rows not _font height?
        if (_font->_height < bitmap.rows) { _font->_height = bitmap.rows; }

        return; 
    }

    width = 16; // next_p2( bitmap.width );
    height = 16; // next_p2( bitmap.rows );

   //////////////////////////////////////////////////////////
   // GROK AND THEN MITOSE
   //////////////////////////////////////////////////////////
    //for( j=0; j <height;j++) {
     //   for( i=0; i < width; i++){
    for( j=0; j <16;j++) {
        for( i=0; i <16; i++) {
            // luminance
            char lum = (i>=bitmap.width || j>=bitmap.rows) ?  0 : bitmap.buffer[i + bitmap.width*j];

            mainfontpix[ch*(2*(16*16))+2*(i+j*width)+1] = lum ;
            // alpha
            if (lum>0)
                mainfontpix[ch*(2*(16*16))+2*(i+j*width)] = 0 ;
            else 
                mainfontpix[ch*(2*(16*16))+2*(i+j*width)] = lum ;

        }
    }
    CheckGlError() ;

     float   x=(float)bitmap.width / (float)width,
             y=(float)bitmap.rows / (float)height;

    _font->bot[(int)ch] = bbox.yMin ;
    _font->width[(int)ch] = bitmap.width ;

    _font->tcoords[(int)ch][0][0] = 0 ;
    _font->tcoords[(int)ch][0][1] = 0 ;
                  
    _font->tcoords[(int)ch][1][0] = 0 ;
    _font->tcoords[(int)ch][1][1] = y ;
                  
    _font->tcoords[(int)ch][2][0] = x ;
    _font->tcoords[(int)ch][2][1] = y ;

    _font->tcoords[(int)ch][3][0] = x ;
    _font->tcoords[(int)ch][3][1] = 0 ;

    return;
}


void initializeFonts(
    Font * _font, 
    const char * font_name, 
    unsigned int height,
    int char_map=0
    )
{

    int i = 0;
    FT_Face face;

    int error = 0 ;

    _font->gl_char_IDs = (GLuint *)malloc(sizeof(GLuint)*128);


    FT_Library library;
    if (FT_Init_FreeType( &library )!=0) { printf("\nProbrem roading fleetype liblaly\n"); }
    if (error = FT_New_Face( library , font_name, 0, &face )!=0)
    {
        printf("\nUnable to complete font loading request. ");

        if ( error == FT_Err_Unknown_File_Format ) 
        { 
            printf("\nthe font file could be opened \n") ;
            printf("\n and read, but it appears \n") ;
            //... that its font format is unsupported 
        }
        return; 
    }
    printf("\nFont init working . ");

    /*FT_Set_Char_Size( face, height << 6, height << 6, 18, 18);*/
    //FT_Set_Char_Size( face, 0, height*64, 300, 300);
    FT_Set_Char_Size( face, 0, height*64, 72, 72);

    /*FT_Set_Char_Size( face, 4096,4096, 18, 18);*/

    _font->gl_list_base = glGenLists(128);
    glGenTextures(129, _font->gl_char_IDs);

    _font->_width = 0 ;
    _font->_height = 0 ;

    /* due to the sfml principle, we first do a learning pass */ 
    for ( i = 0 ; i < 128 ; i++ )
    {
        createFontDisplayList(face, i, _font, true);
    }
    /* then we do a real pass */
    for ( i = 0 ; i < 128 ; i++ )
    {
        createFontDisplayList(face, i, _font, false);
    }
    printf("\n widest character width: %d\n", _font->_width);
    printf("\n height of font: %d\n", _font->_height);

    _font->width[' '] = 8 ;

//        createFontDisplayList(face, 0x2210, _font, false);

    if (face) 
    {
        if (FT_Done_Face(face))
        {
            printf("\n\nFREETYPE DONE FACE REPORTED SOMETHING \n\n") ;
        }
        else
        {
            printf("\n\nFREETYPE DONE FACE ALL GOOD. \n\n") ;
        }
    }

    if (library) FT_Done_FreeType( library );

    // FIXME: we need to check whether any more wrapping up/cleaning should be 
    // done besides these two last statements. 

    return;
}

/*
    Here we determine font name, font file, on-screen size.
*/
void initfonts()
{
    initialized = 1; 

    fonts.add( new Font("default font") ) ;
    numFonts++ ;
    initializeFonts((fonts[0]), "data/fonts/unifont.ttf", 16);
    //initializeFonts((fonts[0]), "../data/fonts/unifont.ttf", 32);
    //initializeFonts((fonts[0]), "../data/fonts/electrb.ttf", 16);

    // 
    //initializeFonts((fonts[0]), "../data/fonts/JuraBook.ttf", 16);  
    //initializeFonts((fonts[0]), "../data/fonts/unifont.ttf", 16);
    //initializeFonts((fonts[1]), "../data/fonts/unifont.ttf", 16, 0x0022);
    //initializeFonts((fonts[0]), "../data/fonts/unifont.ttf", 32);

    // this one is pretty good. no question mark! 
    //initializeFonts((fonts[0]), "../data/fonts/FreeMono.ttf", 16);
    //initializeFonts((fonts[0]), "../data/fonts/classic-robot.ttf", 16);


    //initializeFonts((fonts[0]), "../data/fonts/LiberationMono-Bold.ttf", 16);
    //initializeFonts((fonts[0]), "../data/fonts/SM.TTF", 16);


    //initializeFonts((fonts[0]), "../data/fonts/JuraBook.ttf", 16);  
    //initializeFonts((fonts[0]), "../data/fonts/Abduction.ttf", 16);

    //initializeFonts((fonts[0]), "../data/fonts/new_athena_unicode.ttf", 16);

    /*
    initializeFonts(&(fonts[1]), "./packages/fonts/new_athena_unicode.ttf", 32);
    initializeFonts(&(fonts[1]), "./packages/fonts/new_athena_unicode.ttf", 32);
    initializeFonts(&(fonts[0]), "./packages/fonts/JuraBook.ttf", 32);  
    initializeFonts(&(fonts[0]), "./data/fonts/JuraBook.ttf", 128); 
    */
    return; 
}


void clearFonts()
{
    loopi(numFonts)
    {
        glDeleteLists(fonts[i]->gl_list_base, 128);
        glDeleteTextures(128, fonts[i]->gl_char_IDs);
    }
    return; 
}


/*  FUNCTION: measureText

    DESCRIPTION: this function takes in a string and determines 
    its cartesian span when using the current font. 

*/
void textwidth(Font * in_font, char * text, int * width)
{  
    int i = 0;
    int j = 0;
    int l = 0; 

    i = j = strlen(text) ; 
    while (i>0) 
    {
        i--; 
        l += in_font->width[(int)text[i]]; 
    } 
    *width = l + j; 

    return; 
}


/*  FUNCTION: prstr

    DESCRIPTION: to draw text on the screen, positioned at 
    the current OpenGL geometrical context. 

    parameters: 

        ft_font - global fonts array id 
        x, y - where from the current openGL draw position to render
        str - the message to display 

*/
// TODO: replace this with texture_array with backup texture atlas option? Yes, but later. Much later. 
static float _1_128th = 1.f/128.f ; // Nasty way to precompute the index factor for the 3rd coordinate in 3D textures. 
int prstr(float _x, float _y, const char * str)
{
    int x = _x ;
    int y = _y ;
    int len = strlen(str) ;

    glBegin( GL_QUADS ) ;
    for (int i=0;i<len;i++)
    {
        int c = (int)str[i] ;
        int w = fonts[0]->width[c] ;
        int h = fonts[0]->height[c] ;
        y = _y + fonts[0]->bot[c] ;
        // draw char

            //0
        glTexCoord3d( 0, 0, (float)c*_1_128th);
        glVertex3f( x, y+h, 0) ;

            //1 
        glTexCoord3d( fonts[0]->tcoords[(int)c][1][0], fonts[0]->tcoords[(int)c][1][1], (float)c*_1_128th) ;
        glVertex3f( x, y, 0);

            //2 // bottom right of a letter 
        glTexCoord3d( fonts[0]->tcoords[(int)c][2][0], fonts[0]->tcoords[(int)c][2][1], (float)c*_1_128th) ;
        glVertex3f( x+w, y, 0);

            //3 
        glTexCoord3d( fonts[0]->tcoords[(int)c][3][0], fonts[0]->tcoords[(int)c][3][1], (float)c*_1_128th) ;
        glVertex3f( x+w, y+h, 0);

        // advance positions. Why +2 in addition to w? Experimentally determined, no other reason at this time. The root cause of this is 
        // the fact that Freetype's definition of character properties and dimensions are still incompletely understood by myself (CL). 
        x += w+2 ;
    }
    glEnd() ;

    return x ;  // This should be the horizontal on-screen length of this string
}

/*
    Function: prquad

    Description: 

        prints a colored quad, transluscent if desired, to serve as the background 
        for some text. Makes text more legible regardless of the background behind the 
        text. 
*/
void prquad(
    int minx, 
    int miny, 
    int width, 
    int height
//    ,     vec4 color
    )
{
    glDisable( GL_TEXTURE_3D ) ;
    glDisable( GL_DEPTH_TEST ) ;
    //glColor4fv(color.v) ;
    //glColor4f(1.f, 1.f, 0.f, 0.85f ) ;
//    glColor4f(0.f, 0.f, 0.f, 0.8f ) ;


    glBegin( GL_QUADS ) ;
        glVertex3f( minx,         miny,        -10) ;
        glVertex3f( minx,         miny-height, -10) ;
        glVertex3f( minx + width, miny-height, -10) ;
        glVertex3f( minx + width, miny,        -10) ;
    glEnd( ) ;
    glEnable(GL_TEXTURE_3D) ;
    glEnable( GL_DEPTH_TEST ) ;
    
    //glColor4f(0.5f, 0.5f, 1.f, 1.f) ;
}


/*  FUNCTION: printstr_cont

    DESCRIPTION: this function does the same as printstr, however it does 
    not push and pop the current matrix, allowing to write more than 
    one thing on a line. 

*/
void printstr_cont(float x, float y, char * fmt)
{

/*
    glListBase(ft_font->gl_list_base);
    glTranslatef(x, y, 0.0f);
    glCallLists(strlen(fmt), GL_UNSIGNED_BYTE, fmt);
*/
/*
    FIXME
    int len = strlen(fmt) ;
    for (int i=0;i<len;i++) 
    {
    }
*/

    return;
}

void tryttfstr()
{
    if (!initialized)
    {
        initfonts(); 
    }
}

void prstrstart()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
    
    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, mainfontID) ;
}

void prstrend()
{
    glDisable(GL_TEXTURE_3D);
}

/*
    scrstrlen

    Description: 
        This function provides the length of a string in the number 
        of pixels that the text will occupy on the screen, with the 
        currently enabled font. 

*/
int scrstrlen(const char * str)
{
    int len = 0 ;

    int i = 0 ;
    int _len = strlen(str) ; 

    for (i=0;i<_len;i++)
    {
        len += fonts[0]->width[str[i]]+2 ;
    }

    return len ;
}

void init_text()
{

    printf("\n[TEXT::init_text] called... ") ;
    
    initfonts() ; 

    // 3D font image dimensions
    #define iDepth 128   // number of characters
    #define iWidth 16
    #define iHeight 16

    glGenTextures(1, &mainfontID);
    glBindTexture(GL_TEXTURE_3D, mainfontID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage3D(
        GL_TEXTURE_3D, 
        0, 
        GL_RGBA, 
        iWidth, 
        iHeight, 
        iDepth, 
        0, 
        GL_LUMINANCE_ALPHA, 
        GL_UNSIGNED_BYTE, 
        mainfontpix);
    CheckGlError() ;
/*
    void glTexImage3D  (
    GLenum  target, 
        0,                  // mip level, 
        GL_LUMINANCE_ALPHA, // GLint  internalformat, 
        16,                 // width, 
        16,                 // height, 
        GLsizei  depth,
        GLint  border, 
        GLenum  format, 
        GLenum  type, 
        const GLvoid * pixels
    ) ;
*/
    printf("\n[TEXT::init_text] done.") ;
}

void clean_up_text()
{
    clearFonts() ;
}


