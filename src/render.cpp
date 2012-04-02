


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
        printf("\n\nGL EROR IS %d\n\n", (err)) ; 
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
    //glTranslatef( -camera.pos.x, -camera.pos.y, -(1.8+camera.pos.z) ) ;
    glTranslatef( -camera.pos.x, -camera.pos.y, -(camera.pos.z) ) ;

    glColor3f( 0, 1, 0) ; 
    glBegin( GL_LINE_STRIP ) ; 
        glVertex3f( 0,  0, -50 ) ;
        glVertex3f( 0,  1, -50 ) ;
        glVertex3f( 1,  1, -50 ) ;
        glVertex3f( 1,  0, -50 ) ;
        glVertex3f( 0,  0, -50 ) ;
    glEnd() ; 
    
    extern void drawworld() ;
    drawworld() ;
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
        v1[0] -= 10 ; glVertex3fv( v1 ); 
        v1[0] += 20 ; glVertex3fv( v1 ); 

        v1[0] -= 10 ; 
        v1[1] -= 10 ; glVertex3fv( v1 ); 
        v1[1] += 20 ; glVertex3fv( v1 ); 
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

    glBegin( GL_LINES ) ; 

        glColor3f( 1, 0, 0) ; 
                        glVertex3fv( v1 ) ; 
        v1[0] = -10 ;   glVertex3fv( v1 ) ; 
        glColor3f( 0, 1, 0) ; 
        v1[0] = -10 ;   glVertex3fv( v1 ) ; 
        v1[0] = -20 ;   glVertex3fv( v1 ) ; 

    glEnd() ; 

    //extern void render_test_001() ;
    //render_test_001() ;
}

extern vector<Font*> fonts ;
void drawchar(int x,int y, char c)
{
    glBindTexture(GL_TEXTURE_2D, fonts[0]->gl_char_IDs[(int)(c)]) ;

    glBegin(GL_QUADS) ;

    int bot = fonts[0]->bot[(int)c] ;

        //0
    glTexCoord2d(0,0);    // bottom left of a letter 
    //glVertex2f( 0, -_font->_height + top );
    glVertex2f( 
        x, 
        //y+2*fonts[0]->height[(int)c]
        y+fonts[0]->height[(int)c]+bot
        );


        //1 
    glTexCoord2d(
        0,
        fonts[0]->tcoords[c][1][1]
        );
    glVertex2f( 
        x, 
        y+bot
        );


        //2 
    glTexCoord2d(
        fonts[0]->tcoords[c][2][0],
        fonts[0]->tcoords[c][2][1]
        );    // bottom right of a letter 
    glVertex2f( 
        //x+2*fonts[0]->width[(int)c], 
        x+fonts[0]->width[(int)c], 
        y+bot
        );


        //3 
    glTexCoord2d(
        fonts[0]->tcoords[c][3][0],
        0
        );
    glVertex2f(
        //x+2*fonts[0]->width[(int)c], 
        x+fonts[0]->width[(int)c], 
        //y+2*fonts[0]->height[(int)c]
        y+fonts[0]->height[(int)c]+bot
        );
    
    glEnd() ;
/*
*/
    //CheckGlError() ;
}

void render_menu()
{
    render_ortho_begin() ;
    //CheckGlError() ;


    glColor3f(1,1,1) ;

    extern uint texid ;

    glEnable( GL_TEXTURE_2D ) ;
    glBindTexture(GL_TEXTURE_2D, texid) ;
    glBegin(GL_QUADS) ;

        glTexCoord2f( 0.0f, 0.0f );
        glVertex3f(1000.0f, 200.0f, 00.0f);

        glTexCoord2f( 0.0f, 1.0f );
        glVertex3f(1000.0f, 000.0f, 00.0f);

        glTexCoord2f( 1.0f, 1.0f );
        glVertex3f(1200.0f, 000.0f, 00.0f);

        glTexCoord2f( 1.0f, 0.0f );
        glVertex3f(1200.0f, 200.0f, 00.0f);
    glEnd() ;

int next = 0 ;
    glColor3f(0,.8,0) ;
        next = 400 ;
        drawchar(400,400, 'A') ;    next += fonts[0]->width['A']+3 ;
        drawchar(next, 400, 'J') ;       next += fonts[0]->width['J']+3 ;
        drawchar(next, 400, 'C') ;       next += fonts[0]->width['C']+3 ;
        drawchar(next, 400, 'D') ;       next += fonts[0]->width['D']+3 ;
        drawchar(next, 400, 'E') ;       next += fonts[0]->width['E']+3 ;
        drawchar(next, 400, 'a') ;       next += fonts[0]->width['a']+3 ;
        drawchar(next, 400, 'd') ;       next += fonts[0]->width['d']+3 ;
        drawchar(next, 400, 'j') ;       next += fonts[0]->width['j']+3 ;
        drawchar(next, 400, 'a') ;       next += fonts[0]->width['a']+3 ;
        drawchar(next, 400, 'c') ;       next += fonts[0]->width['c']+3 ;
        drawchar(next, 400, 'i') ;       next += fonts[0]->width['i']+3 ;
        drawchar(next, 400, 'n') ;       next += fonts[0]->width['n']+3 ;
        drawchar(next, 400, 't') ;       next += fonts[0]->width['t']+3 ;
        drawchar(next, 400, 't') ;       next += fonts[0]->width['t']+3 ;
        drawchar(next, 400, 'g') ;       next += fonts[0]->width['g']+3 ;
        drawchar(next, 400, 'q') ;       next += fonts[0]->width['q']+3 ;
        drawchar(next, 400, 't') ;       next += fonts[0]->width['t']+3 ;
/*        drawchar(480,400, 'C') ;
        drawchar(520,400, 'D') ;
        drawchar(560,400, 'E') ;
        drawchar(600,400, 'J') ; */

    glColor3f(.9,.9,.9) ;

    glDisable( GL_TEXTURE_2D ) ;

    glBegin( GL_LINES ) ;
        glVertex3f(400,400,0) ;
        glVertex3f(500,400,0) ;
    glEnd() ;






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
    
    
    extern void prstrstart() ;
    prstrstart() ;

// FIXME: print out from the current line in console buffer and the CONSOLE_V_SIZE-1 
// next lines. For now, only printing line buffer. 
    //int next_line = min( console.current_line, console.top_scr_line + CONSOLE_V_SIZE-1 ) ;

    // show all currently visible content of the console 
    glTranslatef(fonts[0]->width[SDLK_SPACE], engine.current_h-fonts[0]->_height, 0 ) ;
    for (int i=c.top_scr_line;i< CONSOLE_V_SIZE ;i++)
    {
        // visible command history above 
        prstr( 0, - (i+1)*fonts[0]->_height, c.lines[i] ) ;
    }
    glLoadIdentity() ; 


    prstr( 0, 2*fonts[0]->_height, c.line_buffer ) ;

    // Cursor 
    float cursor_pos_x = c.current_line_pix_len ;
    float v[3] = { cursor_pos_x, 1.5*fonts[0]->_height, 0 } ;
    glBegin( GL_LINES ) ; 
        glVertex3fv( v ) ; v[0] = cursor_pos_x + fonts[0]->_width;
        glVertex3fv( v ) ;
    glEnd() ; 

    extern void prstrend() ;
    prstrend() ;

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


    int i = 0 ;
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
        prstr( 600.f, 0 + next_line*height,
                   info_msg) ; 
        prstr( 600.f, 0 + next_line*height,
                   "howdy toons") ; 
        prstr( 600.f, 0 + next_line*height,
                   "howdy bloggers") ; 

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM INPUT SYSTEM
        ////////////////////////////////////////////////////////////////////////////////
        extern int input_msgs_num ;
        extern char input_msgs[100][256] ;
        extern void update_input_messages() ;
        update_input_messages() ;
        while (j<input_msgs_num)
        {
            //prstr( 0, 600.f, engine.current_h - next_line*height,
            prstr( 60.f, 0 + next_line*height,
                   input_msgs[j]) ; j++ ;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM GEOMETRY
        ////////////////////////////////////////////////////////////////////////////////
        // PER ACTION messages
        extern int geom_msgs_num ;
        extern char geom_msgs[100][256] ;
        j = 0 ;
        while (j<geom_msgs_num)
        {
            //prstr( 0, 600.f, engine.current_h - next_line*height, 
            prstr( 60.f, 0 + next_line*height, 
                   geom_msgs[j]) ; j++ ;
        }


        // PER FRAME messages
        extern int geom_msgs_num2 ;
        extern char geom_msgs2[100][256] ;
        // messages from geometry 
        j = 0 ;
        while (j<geom_msgs_num2)
        {
            //prstr( 0, 600.f, engine.current_h - next_line*height, 
            //prstr( engine.current_w-300, 60.f, 0 + next_line*height, 
            prstr( engine.current_w-600, 0 + (j+20)*height, 
                   geom_msgs2[j]) ; j++ ;
        }



        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM MAIN
        ////////////////////////////////////////////////////////////////////////////////
        extern int main_msgs_num ;
        extern char main_msgs[100][256] ;
        j = 0 ;
        while (j<main_msgs_num)
        {
            //prstr( 0, engine.current_w - scrstrlen(main_msgs[1]), engine.current_h - next_line*height, 
            //prstr( 0, 10, engine.current_h - next_line*height, 
            prstr( 10, 400 + next_line*height, 
                     main_msgs[j]) ; j++ ;
        }
        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM PHYSICS
        ////////////////////////////////////////////////////////////////////////////////
        extern int phys_msgs_num ;
        extern char phys_msgs[100][256] ;
        j = 0 ;
        while (j<phys_msgs_num)
        {
            prstr( 10, 400 + next_line*height, 
                     phys_msgs[j]) ; j++ ;
        }
////////////////////////////////////////////////////////////////////////////////
    extern void prstrend() ;
    prstrend() ;
    render_ortho_end() ;
//    CheckGlError() ;
}




