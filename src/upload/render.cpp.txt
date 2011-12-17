


#include "recalc.h"

extern Camera camera ;
extern Engine engine ;
extern Area area ;
extern vector<Font*> fonts ;



// Resets matrices to a blank, usable 2D coordinate space measured 
// in pixels. 
void render_ortho_begin()
{
    // we assume the default matrix mode is modelview, since vastly more operations 
    // are done in glMatrixMode( GL_MODELVIEW ) ; 
    glMatrixMode( GL_MODELVIEW ) ; 
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
void render_world( int sec_progress )
{
    // render_fucking_world_shader() ;

    // shade this fucker muther 

    glMatrixMode( GL_MODELVIEW ) ; 
    glLoadIdentity() ; 

    // set camera perspective :
    //          - normalize to make the XY plane horizontal, and the Z axis be up-down. 
    glRotatef( -camera.pitch/10 , 1, 0, 0 ) ; 
    glRotatef( -camera.yaw/10 , 0, 1, 0 ) ; 
    glRotatef( -90 , 1, 0, 0 ) ; 

    //glTranslatef( -camera.pos.x, -camera.pos.y, -camera.pos.z ) ;
    glTranslatef( -camera.pos.x, -camera.pos.y, -(1.8+camera.pos.z) ) ;


    glColor3f( 0, 1, 0) ; 
    glBegin( GL_LINE_STRIP ) ; 
        glVertex3f( 0,  0, -50 ) ;
        glVertex3f( 0,  1, -50 ) ;
        glVertex3f( 1,  1, -50 ) ;
        glVertex3f( 1,  0, -50 ) ;
        glVertex3f( 0,  0, -50 ) ;
    glEnd() ; 

}

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
        glVertex3fv( v1 ); 
        v1[0] += 20 ; 
        glVertex3fv( v1 ); 

        v1[0] -= 10 ; 
        v1[1] -= 10 ; 
        glVertex3fv( v1 ); 
        v1[1] += 20 ; 
        glVertex3fv( v1 ); 

    glEnd() ;

    glPopMatrix() ;
    glMatrixMode( GL_PROJECTION ) ; 
    glPopMatrix() ;
    glMatrixMode( GL_MODELVIEW ) ; 
}

extern void draw_world_box() ;
extern bool hit_world ;

void render_editor()
{
    float v1[3] = { 0, 0, 0 } ; 
   /* 
    glMatrixMode( GL_MODELVIEW ) ; 
    glLoadIdentity() ; 

    glRotatef( -camera.pitch/10 , 1, 0, 0 ) ; 
    glRotatef( -camera.yaw/10 , 0, 1, 0 ) ; 
    glRotatef( -90 , 1, 0, 0 ) ; 
    */

    extern void update_editor() ;
    update_editor() ;
// Before we draw a frame we must find out what we are looking at
   
    if ( hit_world )
    {
        glColor3f( 1, 1, 0) ; 
    }
    {
        glColor3f( 0.5, 0.5, 0) ; 
    }


    render_ortho_begin() ;
    
    //prstr( 0, 0.f, fonts[0]->_height, "Engage Delta Niner." ) ;
    
    prstr( 0, 200.f, 300.f, "HOSSEN") ;
    
extern char message[256] ;
extern char message2[256] ;
extern char message3[256] ;
    prstr( 0, 200.f, fonts[0]->_height, message ) ;
    prstr( 0, 800.f, 2*fonts[0]->_height, message2 ) ;
    prstr( 0, 800.f, fonts[0]->_height, message3 ) ;
    render_ortho_end() ;
    //prstr
/*
    glBegin( GL_LINE_STRIP ) ; 
        glVertex3fv( v1 ); v1[0] = area.size ;
        glVertex3fv( v1 ); v1[1] = area.size ;
        glVertex3fv( v1 ); v1[0] = 0 ;
        glVertex3fv( v1 ); v1[1] = 0 ; 
    glEnd() ; 
*/

    // If we're aiming at the world, then we draw a helpful box 
    // around it. 
    draw_world_box() ;

    // draw a little dot
    draw_cursor() ;
}

void render_tester()
{


    float v1[3] = { 0, 50, 20 } ; 

    glBegin( GL_LINES ) ; 

        glColor3f( 1, 0, 0) ; 
        glVertex3fv( v1 ) ; 
        v1[0] = -10 ; 
        glVertex3fv( v1 ) ; 

        glColor3f( 0, 1, 0) ; 
        v1[0] = -10 ; 
        glVertex3fv( v1 ) ; 
        v1[0] = -20 ; 
        glVertex3fv( v1 ) ; 

    glEnd() ; 


}


extern vector<Font*> fonts ;

void render_menu()
{
    glMatrixMode( GL_MODELVIEW ) ; 
    glPushMatrix() ; 
    glLoadIdentity() ; 

    glMatrixMode( GL_PROJECTION ) ;
    glPushMatrix() ; 


    glLoadIdentity() ; 
    glOrtho( 0, engine.current_w, 0, engine.current_h, -1, 1000 ) ; 
        prstr( 0, 0.f, fonts[0]->_height, "Engage Delta Niner." ) ;


    glMatrixMode( GL_PROJECTION ) ;
    glPopMatrix() ; 

    glMatrixMode( GL_MODELVIEW ) ; 
    glPopMatrix() ; 
/*
    */
}


extern Console console ;
#define c console
/*
    Simple algorithm: 

    For the number of console lines that are visible on the screen, 
    starting at the line which is the 'cursor' position, show lines. 

    Start at the top of the screen, defined by Y = engine.current_h

*/
void render_console()
{

    glMatrixMode( GL_MODELVIEW ) ; 
    glPushMatrix() ; 
    glLoadIdentity() ; 

    glMatrixMode( GL_PROJECTION ) ;
    glPushMatrix() ; 

    glLoadIdentity() ; 
    glOrtho( 0, engine.current_w, 0, engine.current_h, -1, 1000 ) ; 
    glMatrixMode( GL_MODELVIEW ) ; 


// FIXME: print out from the current line in console buffer and the CONSOLE_V_SIZE-1 
// next lines. For now, only printing line buffer. 
    //int next_line = min( console.current_line, console.top_scr_line + CONSOLE_V_SIZE-1 ) ;

    // show all currently visible content of the console 
    glTranslatef(fonts[0]->width[SDLK_SPACE], engine.current_h-fonts[0]->_height, 0 ) ;
    for (int i=c.top_scr_line;i< CONSOLE_V_SIZE ;i++)
    {
        // visbile command history above 
        prstr( 0, 0, - i*fonts[0]->_height, c.lines[i] ) ;
        
        // command line buffer here 
    }
    glLoadIdentity() ; 


    prstr( 0, 0, 2*fonts[0]->_height, c.line_buffer ) ;

    // Cursor 
    float cursor_pos_x = c.current_line_pix_len ;
    float v[3] = { 
                    cursor_pos_x, 
                    1.5*fonts[0]->_height, 
                    0 
                 } ;
    glBegin( GL_LINES ) ; 
        glVertex3fv( v ) ;
        v[0] = cursor_pos_x + fonts[0]->_width;
        glVertex3fv( v ) ;
    glEnd() ; 



    glMatrixMode( GL_PROJECTION ) ;
    glPopMatrix() ; 

    glMatrixMode( GL_MODELVIEW ) ; 
    glPopMatrix() ; 
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
*/
void render_info()
{
}




