

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
//static GLubyte image [iDepth][iHeight][iWidth][4];
//static GLubyte mainfontpix[256][16][16][2] ;
//static GLubyte mainfontpix[256*16*16*2] ;
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

    int width ; //=  slot->bitmap.width ;
    int height ; //= slot->bitmap.rows ;
    int top =    slot->bitmap_top ;

    FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

    // This reference will make accessing the bitmap easier
    FT_Bitmap bitmap=bitmap_glyph->bitmap;

    if (check_phase)
    {
        _font->width[(int)ch]  = bitmap_glyph->left + (face->glyph->advance.x >> 6) ; 
        //_font->width[(int)ch]  = bitmap.width ;

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
    GLubyte* expanded_data = (GLubyte *)malloc ( sizeof (GLubyte) * 2 * width * height);
   


   //////////////////////////////////////////////////////////
   // GROK AND THEN MITOSE
   //////////////////////////////////////////////////////////
    //for( j=0; j <height;j++) {
     //   for( i=0; i < width; i++){
    for( j=0; j <16;j++) {
        for( i=0; i <16; i++){
            expanded_data[2*(i+j*width)]= 
            expanded_data[2*(i+j*width)+1] =
                (i>=bitmap.width || j>=bitmap.rows) ?
                0 : bitmap.buffer[i + bitmap.width*j] ;

            //mainfontpix[ch][j][i][0]  +2*(i+j*width)] = 
            mainfontpix[ch*(2*(16*16))+2*(i+j*width)] = 
            mainfontpix[ch*(2*(16*16))+2*(i+j*width)+1] = 
            //mainfontpix[ch][j][i][0] = 
            //mainfontpix[ch][j][i][1] = 
//mainfontpix[ch*(16*16*2)  +2*(i+j*width)+1] = 128 ; // (i%2==0)?(128):(0) ;
//mainfontpix[ch*(16*16*2)  +2*(i+j*width)+1] = 
            // 128 ;
                (i>=bitmap.width || j>=bitmap.rows) ?
                0 : bitmap.buffer[i + bitmap.width*j];
        }
    }
    CheckGlError() ;


    free (expanded_data);

     float   x=(float)bitmap.width / (float)width,
             y=(float)bitmap.rows / (float)height;

/*
printf("\n character: %c", ch) ;
printf("\n \t width: %d", _font->width[(int)ch]) ;
printf("\n \t height: %d", _font->height[(int)ch]) ;
printf("\n \ttop left: %d  %d", 0, -_font->_height + top ) ;
printf("\n \tbot left: %d  %d", 0, -_font->_height + bbox.yMin);
printf("\n \tbot right: %d  %d", bitmap.width, -_font->_height + bbox.yMin);
printf("\n \ttop right: %d  %d", bitmap.width, -_font->_height + top );
*/

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
    initializeFonts((fonts[0]), "../data/fonts/unifont.ttf", 16);
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
    the current OpenGL context. 

    parameters: 

        ft_font - global fonts array id 
        x, y - where from the current openGL draw position to render
        str - the message to display 

*/
static float _1_128th = 1.f/128.f ;
void prstr(float _x, float _y, const char * str)
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

        // advance positions
        x += w+2 ;
    }
glEnd() ;
    return ; 
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
    prstr: print a string using currently enabled font. 
void prstr(float x, float y, const char * str)
{
    printstr(x, y, str);
}
*/

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
        len += fonts[0]->width[str[i]] ;
    }

    return len ;
}

void init_text()
{
//    glEnable( GL_TEXTURE_3D ) ;
//    glDisable( GL_TEXTURE_3D ) ;

    printf("\nMAKE ME WORK::: TEXT AND FONTS !!!\n") ; 
    
//    glEnable( GL_TEXTURE_2D ) ;
    initfonts() ; 
//    glDisable( GL_TEXTURE_2D ) ;
    
//    glEnable( GL_TEXTURE_3D ) ;



    //#define iDepth 128
    #define iDepth 128
    #define iWidth 16
    #define iHeight 16

    static GLubyte image [iDepth][iHeight][iWidth][2];

//glClearColor(0.0, 0.0, 0.0, 0.0);
//glShadeModel(GL_FLAT);
//glEnable(GL_DEPTH_TEST);
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &mainfontID);
    glBindTexture(GL_TEXTURE_3D, mainfontID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, iWidth, iHeight, iDepth, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, mainfontpix);


//extern void CheckGlError() ;
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
}

void clean_up_text()
{
    clearFonts() ;
}


