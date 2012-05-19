

#include "recalc.h"


////////////////////////////////////////////////
// Main switch on this system
////////////////////////////////////////////////
extern bool textenabled ;

extern Camera camera ;
extern Engine engine ;
extern Area area ;
extern vector<Font*> fonts ;


void CheckGlError()
{
    int err = glGetError() ;
    if (err)
    {
        // FIXME: logging system. 
        printf("\n\nGL ERROR IS %d\n\n", (err)) ; 
    }
    else
    {
        printf("\nNo problem. ", (err)) ; 
    }
}

// Resets matrices to a blank, usable 2D coordinate space measured 
// in pixels. 
void render_ortho_begin()
{
    // we assume the matrix mode is always GL_MODELVIEW
    glPushMatrix() ; 
    glLoadIdentity() ; 
    glMatrixMode( GL_PROJECTION ) ;

    glPushMatrix() ; 
    glLoadIdentity() ; 
    glOrtho( 0, engine.current_w, 0, engine.current_h, -1, 1000 ) ; 

    glMatrixMode( GL_MODELVIEW ) ; 
}


// Returns to state prior to engaging the 2D space. 
void render_ortho_end()
{
    glPopMatrix() ; 
    glMatrixMode( GL_PROJECTION ) ;
    glPopMatrix() ; 
    glMatrixMode( GL_MODELVIEW ) ; 
}


void render_world( )
{
    glMatrixMode( GL_MODELVIEW ) ; 
    glLoadIdentity() ; 

    // set camera perspective :
    //          - normalize to make the XY plane horizontal, and the Z axis be up-down. 
    glRotatef( -camera.pitch/10 , 1, 0, 0 ) ; 
    glRotatef( -camera.yaw/10 , 0, 1, 0 ) ; 
    glRotatef( -90 , 1, 0, 0 ) ; 

    //glTranslatef( -camera.pos.x, -camera.pos.y, -camera.pos.z ) ;
    //glTranslatef( -camera.pos.x, -camera.pos.y, -(1.8+camera.pos.z) ) ;
    glTranslatef( -camera.pos.x, -camera.pos.y, -(camera.pos.z) ) ;

    // render_fucking_world_shader() ;
    // shade this fucker muther 
extern void drawworld() ;
    drawworld() ;

extern void renderentities() ;
    renderentities() ;
}


// mouselook cursor - what's highlights what's directly ahead of the camera. 
void draw_cursor()
{

    // Yes this is way too much work for such a small amout of stuff. 
    // Should be folded into another function that justifies all this. 
    glMatrixMode( GL_PROJECTION ) ; 
    glPushMatrix() ;

    glLoadIdentity() ;
    glOrtho( 0, engine.current_w, 0, engine.current_h, -1, 1000 ) ; 
    glMatrixMode( GL_MODELVIEW ) ; 

    glPushMatrix() ;
    glLoadIdentity() ;

    float v1[3] = { engine.current_w/2, engine.current_h/2, 0 } ; 

    glPointSize( 10.0 ) ;

    glBegin( GL_LINES ) ;
                           v1[0] -= 10 ; 
        glVertex3fv( v1 ); v1[0] += 20 ; 
        glVertex3fv( v1 ); v1[0] -= 10 ; v1[1] -= 10 ; 
        glVertex3fv( v1 ); v1[1] += 20 ; 
        glVertex3fv( v1 ); 
    glEnd() ;

    glPopMatrix() ;
    glMatrixMode( GL_PROJECTION ) ; 
    glPopMatrix() ;
    glMatrixMode( GL_MODELVIEW ) ; 
}

extern void draw_world_box() ;
extern void draw_square() ;
extern bool hit_world ;

void render_editor()
{
    float v1[3] = { 0, 0, 0 } ; 

extern void update_editor() ;
    update_editor() ;
// Before we draw a frame we must find out what we are looking at
   
    if ( hit_world ) { glColor3f( 1, 1, 0) ; }
    {
        glColor3f( 0.5, 0.5, 0) ; 
        glColor3f( .75, .75, 0) ; 
    }

    // glDisable( GL_DEPTH_TEST ) ;
    glEnable ( GL_DEPTH_TEST ) ;
    // If we're aiming at the world, then we draw a helpful box 
    // around it. 
    draw_world_box() ;

    // draw a little dot or cross or crosshair. 
    draw_cursor() ;
    
extern void draw_highlight() ; // FIXME lol externs everywhere. 
    draw_highlight() ;

    draw_selection() ;
    
    // green square that shows where on world boundary ray is entering world, 
    // if camera is looking at world from outside. 
extern void draw_ray_start_node() ;
    draw_ray_start_node() ;

//    CheckGlError() ;
}

void render_tester()
{
    glEnable( GL_DEPTH_TEST ) ;

    float v1[3] = { 0, 50, 20 } ; 

/*
    glBegin( GL_LINES ) ; 

    glColor3f( 1, 0, 0) ; 
                        glVertex3fv( v1 ) ; 
        v1[0] = -10 ;   glVertex3fv( v1 ) ; 
    glColor3f( 0, 1, 0) ; 
        v1[0] = -10 ;   glVertex3fv( v1 ) ; 
        v1[0] = -20 ;   glVertex3fv( v1 ) ; 

    glEnd() ; 
*/
    //extern void render_test_001() ;
    //render_test_001() ;
}

extern GLuint mainfontID ;
extern vector<Font*> fonts ;
void drawchar(int x,int _y, char c)
{

    glBegin(GL_QUADS) ;

    int y = _y + 2* fonts[0]->bot[(int)c] ;

if (1)
{
        //0
    //glTexCoord3d( 0, 0, c);    
    glTexCoord3d( 0, 0, (float)c/128);    
    glVertex3f( x, y+2*fonts[0]->height[(int)c], 0) ;

        //1 
    //glTexCoord3d( 0, fonts[0]->tcoords[c][1][1], c) ;
    glTexCoord3d( 
        fonts[0]->tcoords[(int)c][1][0],
        fonts[0]->tcoords[(int)c][1][1],
        (float)c/128) ;
    glVertex3f( x, y, 0);

        //2 // bottom right of a letter 
    //glTexCoord3d( fonts[0]->tcoords[c][2][0], fonts[0]->tcoords[c][2][1], c);
    glTexCoord3d( 
        fonts[0]->tcoords[(int)c][2][0],
        fonts[0]->tcoords[(int)c][2][1],
        (float)c/128) ;
    glVertex3f( x+2*fonts[0]->width[(int)c], y, 0);

        //3 
    //glTexCoord3d( fonts[0]->tcoords[c][3][0], 0, c);
    glTexCoord3d(
        fonts[0]->tcoords[(int)c][3][0],
        fonts[0]->tcoords[(int)c][3][1],
        (float)c/128) ;
    glVertex3f( x+2*fonts[0]->width[(int)c], y+2*fonts[0]->height[(int)c], 0);
}


    glEnd() ;

/*
*/
    //CheckGlError() ;
}

void render_menu()
{
    render_ortho_begin() ;
    //CheckGlError() ;
//glTranslatef(400,0,0) ;
    glColor3f(1,1,1) ;

//glBindTexture(GL_TEXTURE_2D, engine.texids[engine.tex]) ;
extern GLuint surfacetex ;

    //glEnable( GL_TEXTURE_2D ) ;
    glEnable( GL_TEXTURE_3D ) ;

glBindTexture(GL_TEXTURE_3D, surfacetex) ;


extern int yeshello ; // where is this from ? 
    // texture scrolling! , actually. 
    float f = engine.tex ;
    f = (f+0.5f) / 3.0f ;   // fuck this shit this is what you have to do to specify the layer you want. 
    
    // This block demonstrates texture coordinate repetition
    if (engine.numtex>0)
    {
        glBegin(GL_QUADS) ;
            glTexCoord3f( 0.0f, 0.0f, f);
            glVertex3f(1000.0f, 200.0f, 0.0f);

            glTexCoord3f( 0.0f, 1.0f, f);
            glVertex3f(1000.0f, 000.0f, 0.0f);

            glTexCoord3f( 2.0f, 1.0f, f);         // when texture is repeating, this makes two get drawn. 
            glVertex3f(1400.0f, 000.0f, 0.0f);

            glTexCoord3f( 2.0f, 0.0f, f);
            glVertex3f(1400.0f, 200.0f, 0.0f);
        glEnd() ;
    }

    int next = 0 ;
    glColor3f(1,1,1) ;

    //glDisable( GL_TEXTURE_2D ) ;
    glDisable( GL_TEXTURE_3D ) ;


/*

    // This block demonstrates 3D texture text 
    glEnable( GL_TEXTURE_3D ) ;
    glBindTexture(GL_TEXTURE_3D, mainfontID) ;

        next = 100 ;
        drawchar(next,400, 'A') ;    next += 2*fonts[0]->width['A']+3 ;
        drawchar(next, 400, 'B') ;       next += 2*fonts[0]->width['B']+3 ;
        drawchar(next, 400, 'C') ;       next += 2*fonts[0]->width['C']+3 ;
        drawchar(next, 400, 'D') ;       next += 2*fonts[0]->width['D']+3 ;
        drawchar(next, 400, 'E') ;       next += 2*fonts[0]->width['E']+3 ;
        drawchar(next, 400, 'F') ;       next += 2*fonts[0]->width['F']+3 ;
        drawchar(next, 400, 'G') ;       next += 2*fonts[0]->width['G']+3 ;
        drawchar(next, 400, 'H') ;       next += 2*fonts[0]->width['H']+3 ;
        drawchar(next, 400, 'I') ;       next += 2*fonts[0]->width['I']+3 ;
        drawchar(next, 400, 'J') ;       next += 2*fonts[0]->width['J']+3 ;
        drawchar(next, 400, 'K') ;       next += 2*fonts[0]->width['K']+3 ;
        drawchar(next, 400, 'L') ;       next += 2*fonts[0]->width['L']+3 ;
        drawchar(next, 400, 'M') ;       next += 2*fonts[0]->width['M']+3 ;
        drawchar(next, 400, 'N') ;       next += 2*fonts[0]->width['N']+3 ;
        drawchar(next, 400, 'O') ;       next += 2*fonts[0]->width['O']+3 ;
        drawchar(next, 400, 'P') ;       next += 2*fonts[0]->width['P']+3 ;
        drawchar(next, 400, 'j') ;       next += 2*fonts[0]->width['j']+3 ;
  

    // This demonstrates that we can draw a 2D texture which is stored as a 
    // subset of a 3D texture which was set using glTexSubImage3D. 
    extern GLuint surfacetex ;
    glBindTexture(GL_TEXTURE_3D, surfacetex) ;
    glBegin(GL_QUADS) ;
            glTexCoord3f( 0.0f, 0.0f, 0.0f ); glVertex3f(400.0f, 200.0f, 0.0f);

            glTexCoord3f( 0.0f, 1.0f, 0.0f ); glVertex3f(400.0f, 0.0f, 0.0f);

            glTexCoord3f( 1.0f, 1.0f, 0.0f ); glVertex3f(600.0f, 0.0f, 0.0f);

            glTexCoord3f( 1.0f, 0.0f, 0.0f ); glVertex3f(600.0f, 200.0f, 0.0f);
    glEnd() ;
   
    glDisable( GL_TEXTURE_3D ) ;
*/   
    render_ortho_end() ;
//    CheckGlError() ;
}


extern Console console ;
#define c console
/*
    Simple algorithm: 

    For the number of console lines that are visible on the screen, 
    starting at the line which is the 'cursor' position, show lines. 

    Start at the top of the screen, defined by Y = engine.current_h

    FIXME: add a translucent rectangle behind console so that it's 
    visible against most backgrounds. 
*/
void render_console()
{
    render_ortho_begin() ;
    
    // Position console contents to the top of the screen  
extern void prstrstart() ;
    prstrstart() ;
    glTranslatef(0, engine.current_h-fonts[0]->_height, 0 ) ;
    
    for (int i=0 ; i<CONSOLE_V_SIZE ; i++)
    {
        prstr( 0, -(i+1)*fonts[0]->_height, c.lines[i] ) ;
    }

    // Console entry buffer
    glLoadIdentity() ; 
    prstr( 0, 2*fonts[0]->_height, c.line_buffer ) ;

extern void prstrend() ;
    prstrend() ;

    // Cursor 
    glColor3f(1,1,1) ;
    float cursor_pos_x = c.current_line_pix_len ;
    float v[3] = { cursor_pos_x, 1.5*fonts[0]->_height, 0 } ;
    glBegin( GL_LINES ) ; 
        glVertex3fv( v ) ; v[0] = cursor_pos_x + fonts[0]->_width;
        glVertex3fv( v ) ;
    glEnd() ; 

    render_ortho_end() ;
//    CheckGlError() ;
}


/*
    Function: render_info

    Purpose: to show everything we want to be watching at a particular time. 

    Framerates, polygons rendered, entity counts and types, inputs received, 
    vertex array counts, frame times, operations counts (operations like 
    physics frame, world render, menu render, console execute) , event counts, 
    operation durations (GL context changes, frames, render frames, physics frames, etc.).

    Possible other information: 

        - console contents 

        - geometry updates 

        - input updates 

        - graphics system stats 

    FIXME: add a translucent rectangle so that text is visible against 
    most backgrounds. 
*/
void render_info()
{
    if (!textenabled) { return ; }


    int i = 10 ; // Number of lines above bottom of screen. Leave room for command entry line. 
    int j = 0 ;

    #define next_line ((i++)+1)     // is that the stupidest thing to do or just effective? 
    int height = fonts[0]->_height ;

    render_ortho_begin() ;

    extern void prstrstart() ;
    prstrstart() ;
        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM INFO SYSTEM
        ////////////////////////////////////////////////////////////////////////////////
        char info_msg[256] ;
        sprintf(info_msg, "height=%d ", height) ;
//        prstr( 60.f, 0 + next_line*height, "info messages here") ; 

glTranslatef(0, 3*engine.current_h/4, 0) ;
        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM INPUT SYSTEM
        ////////////////////////////////////////////////////////////////////////////////
        prstr( 10.f, - next_line*height, "----input system: ----") ; // j++ ;
extern int input_msgs_num ;
extern char input_msgs[100][256] ;
extern void update_input_messages() ;
update_input_messages() ;
        while (j<input_msgs_num)
        {
            prstr( 10.f, - next_line*height, input_msgs[j]) ; j++ ;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM GEOMETRY
        ////////////////////////////////////////////////////////////////////////////////
        // PER ACTION messages
        prstr( 10.f, - next_line*height, "----geometry messages:----") ; // j++ ;
extern int geom_msgs_num ;
extern char geom_msgs[100][256] ;
        j = 0 ;
        while (j<geom_msgs_num)
        {
            prstr( 10.f, - next_line*height, geom_msgs[j]) ; j++ ;
        }


        // PER FRAME messages
extern int geom_msgs_num2 ;
extern char geom_msgs2[100][256] ;
        // messages from geometry 
        j = 0 ;
        while (j<geom_msgs_num2)
        {
            prstr( 10, - next_line*height, geom_msgs2[j]) ; j++ ;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM MAIN
        ////////////////////////////////////////////////////////////////////////////////
        prstr( 10.f, - next_line*height, "----main messages:----") ; // j++ ;
extern int main_msgs_num ;
extern char main_msgs[100][256] ;
        j = 0 ;
        while (j<main_msgs_num)
        {
            prstr( 10, - next_line*height, main_msgs[j]) ; j++ ;
        }
        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM PHYSICS
        ////////////////////////////////////////////////////////////////////////////////
        prstr( 10.f, - next_line*height, "----physics messages:----") ; //j++ ;
extern int phys_msgs_num ;
extern char phys_msgs[100][256] ;
        j = 0 ;
        while (j<phys_msgs_num)
        {
            prstr( 10, - next_line*height, phys_msgs[j]) ; j++ ;
        }

////////////////////////////////////////////////////////////////////////////////
    extern void prstrend() ;
    prstrend() ;

        int h = next_line*height ;
        glColor3f(0.5f,0.5f,0.9f) ;
        glBegin( GL_LINES ) ;
            glVertex3f(50, h, 0) ;
            glVertex3f(500, h, 0) ;
        glEnd() ;



    render_ortho_end() ;
//    CheckGlError() ;
}




