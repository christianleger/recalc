/*
    File: geometry.cpp

    Contains the elements of world geometry, the operations to manipulate 
    world geometry, and the operations to collect information about this 
    structure. 

    See geometry.h for the contents of the various geometrical structures. 

*/


#include "recalc.h"

extern Camera camera ;


// This is equivalent to 2^15 units, or 32,768
#define WORLD_SCALE 15
World world = { 15, 2<<15 } ;

bool aiming_at_world ;

#define X(orientation) ((orientation&0x03)   ) // index 0-2 in first three bits 
#define Y(orientation) ((orientation&0x18)>>3) // index 0-2 in next three bits 
#define Z(orientation) ((orientation&0xC0)>>6) // index 0-2 in last three bits 

int orientations[6] = {
                        1 | 2 << 3 | 0 << 6,  // X axis +ve
                        0 | 2 << 3 | 1 << 6,  // Y axis +ve
                        1 | 2 << 3 | 0 << 6,  // X axis -ve
                        0 | 2 << 3 | 1 << 6,  // Y axis -ve
                        0 | 1 << 3 | 2 << 6,  // Z axis -ve 
                        0 | 1 << 3 | 2 << 6,  // Z axis +ve 
                      } ;

#define o orientations


/*
    o - orientations. 
    c - center 

    They work as follows: 
    0 -  x-ve
    1 -  x+ve
    2 -  y-ve
    3 -  y+ve
    4 -  z-ve
    5 -  z+ve

*/
void draw_square(
                vec & center    /* center */,
                int size,       /* size of world box */
                int o           /* orientation */
            )
{
    vec c(center.v) ;

    glColor3f( 1, 1, 0 ) ;

    c[X(o)] -= size>>1 ;
    c[Y(o)] -= size>>1 ; // now positioned at 'bottom-left' corner. 

    // glLoadIdentity() ;  everything disappears if I do this! 

    glBegin( GL_LINE_LOOP ) ;
        glVertex3fv( c.v ) ; c[X(o)] += size ;  // bottom-right
        glVertex3fv( c.v ) ; c[Y(o)] += size ;  // top-right
        glVertex3fv( c.v ) ; c[X(o)] -= size ;  // top-left
        glVertex3fv( c.v ) ;
    glEnd() ;
}


void draw_world_box()
{
    int half = world.size>>1 ;

    vec center( 0, half, half ) ;

    glDisable( GL_DEPTH_TEST ) ; 

    draw_square(center, world.size, o[0]) ; center[0] += half ; center[1] += half ;
    draw_square(center, world.size, o[1]) ; center[0] += half ; center[1] -= half ;
    draw_square(center, world.size, o[2]) ; center[0] -= half ; center[1] -= half ;
    draw_square(center, world.size, o[3]) ;
    /*  */
}



// normal vectors to each axix-aligned plane 
vec world_normals[6] =
{
    vec( -1,  0,  0 ),
    vec(  1,  0,  0 ),
    vec(  0, -1,  0 ),
    vec(  0,  1,  0 ),
    vec(  0,  0, -1 ),
    vec(  0,  0,  1 )
} ;

int bounds[3][2] =
{
    {1, 2},  // on X plane, check Y and Z boundaries 
    {0, 2},  // on Y plane, check X and Z boundaries  
    {0, 1}   // on Z plane, check X and Y boundaries 
} ;


vec  camdir ;
/*
    Find where a ray hits a plane by setting the parameter t. 

    The parameter V, the direction of the ray, is taken from the camdir (camera direction). 
    Values of t have the following meanings: 
        - positive: the ray hits the plane, for the point P' = P + tV where
          P is the origin point for the 


*/
void rayHitPlane( vec* pos, vec* normal, float d, float* t )
{
    double numerator = 0, denominator = 0 ;

    numerator   =   d - ( pos->dot( *normal ) ) ;
    denominator =        camdir.dot( *normal )  ;
    if ( denominator != 0 ) { *t = ( numerator ) / ( denominator ) ;  }
    // if denominator in plane equation is 0, that means vector parallel to plane. 
    else  { *t = 0 ; }
}


bool hit_world = false ;


char message[256] ;
void update_editor()
{
    bool advancing = true ;
    bool hitworld = false ;
    int hitplanes = 0 ;

    float t = 0 ;             // this records the parameter in the equation P' = P + tV to find where a ray touches a plane. 
    float good_t = 0 ;         // Used to hold the smallest value of t encountered. 

    vec front = camera.pos ;  // the leading point of the advancing ray 
    vec pos = camera.pos ;

    ivec corner( 0, 0, 0 ) ;

    int scale = world.scale ;

    camdir = camera.dir ;

    // The variable hitplanes records which three axis-aligned planes are 
    // always possible hits for a ray going through an octree. 
    //
    // Later, hitplanes can be used to quickly express plane indexes into the 
    // world_normals array. 
    hitplanes |= ( camdir.x>0 ? 0x00 : 0x01 ) ;
    hitplanes |= ( camdir.y>0 ? 0x02 : 0x03 ) << 3 ;
    hitplanes |= ( camdir.z>0 ? 0x04 : 0x05 ) << 6 ;

    sprintf(
        message, 
        "The plane indexes from hitplanes is  X%d, Y%d, Z%d, ", 
        hitplanes   &0x01, 
        hitplanes>>3&0x01, 
        hitplanes>>6&0x01
    ) ; 
/*
*/

    // are we aiming at the world? 
/*
    loopi(3)
    {
        //if rayHitPlane(camera.pos, world_normals[     ], hitplanes??, &t   )
        //{
       // }
    }
*/
}



