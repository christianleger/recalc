


#include "recalc.h"

extern Camera camera ;
extern Area area ;

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


/*
    


    glBegin( GL_LINE_STRIP ) ; 
        glVertex3f( 0,  -50 + (50*(float)sec_progress/5000) , 0) ; 
        glVertex3f( 0,  -50 + (50*(float)sec_progress/5000) , 10) ; 
        glVertex3f( 1,  -50 + (50*(float)sec_progress/5000) , 10) ; 
        glVertex3f( 1,  -50 + (50*(float)sec_progress/5000) , 0) ; 
        glVertex3f( 0,  -50 + (50*(float)sec_progress/5000) , 0) ; 
    glEnd() ; 

    float v1[3] = { -10, -10, 1 } ; 
    float v2[3] = {  10, -10, 1 } ; 

    glBegin( GL_LINES ) ; 
        glVertex3fv( v1 ); glVertex3fv( v2 );

        v1[2] = -1 ; v2[2] = -1 ; glVertex3fv( v1 ); glVertex3fv( v2 );

        v1[1] = -15 ; v2[1] = -15 ; glVertex3fv( v1 ); glVertex3fv( v2 );

        v1[1] = -20 ; v2[1] = -20 ; glVertex3fv( v1 ); glVertex3fv( v2 );
    glEnd() ; 
*/
}

void draw_square() 
{
}
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
    
    glColor3f( 1, 1, 0) ; 

    glBegin( GL_LINE_STRIP ) ; 
        glVertex3fv( v1 ); v1[0] = area.size ;
        glVertex3fv( v1 ); v1[1] = area.size ;
        glVertex3fv( v1 ); v1[0] = 0 ;
        glVertex3fv( v1 ); v1[1] = 0 ; 
    glEnd() ; 

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
/*
    glMatrixMode( GL_MODELVIEW ) ; 
    glPushMatrix() ; 
    glLoadIdentity() ; 

    glMatrixMode( GL_PROJECTION ) ;
    glPushMatrix() ; 


    glLoadIdentity() ; 
    glOrtho( 0, 1600, 0, 800, -1, 1000 ) ; 
        prstr( 0, 50.f, 50.f, "Engage Delta Niner." ) ;


    glMatrixMode( GL_PROJECTION ) ;
    glPopMatrix() ; 

    glMatrixMode( GL_MODELVIEW ) ; 
    glPopMatrix() ; 
    */
}

void render_console()
{}


/*
    Function: render_info

    Purpose: to show everything we want to be watching at a particular time. 

    Framerates, polygons rendered, entity counts and types, inputs received, 
    vertex array counts, frame times, operations counts (operations like 
    physics frame, world render, menu render, console execute) , event counts, 
    operation durations (GL context changes, frames, render frames, physics frames, etc.).

*/
void render_info()
{
}




