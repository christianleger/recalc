

#include "recalc.h"


// from recalc.cpp
extern Camera camera ;
//FIXME: camera DIR should replace camdir? 

//vec camdir ;

// from input.cpp 
extern int mouse_deltax ;
extern int mouse_deltay ;

float distance_travelled = 0.f ;
int physics_count = 0 ; // physics frame count 


float basic_velocity = 10 ;

// FIXME: fix me!!!!!!!!!
void physics_frame( unsigned int time_delta )
{
    float vel ; 
    float dist_delta ;
    float time_mul = 0 ;

    time_mul = (float)time_delta / (float)PHYSICS_FRAME_TIME ;
    //time_mul = 1.0f ;

    physics_count ++ ; 
    if ( physics_count == 900 )
    {
        physics_count = 0 ; 
        //printf( "\n time delta = = %.2f \n", distance_travelled ) ; 
    }

    if (camera.forward || camera.backward )
    {
        vel = time_mul * basic_velocity * ( camera.forward ? 1 : -1 ) ; /* FIXME: change this to a vector and velocity-based movement mechanism */

        dist_delta = - vel * sin ( 2*M_PI*camera.yaw/3600 ) ; 
        camera.pos.x += dist_delta ; distance_travelled += dist_delta ;

        dist_delta = vel * cos ( 2*M_PI*camera.yaw/3600 ) ; 
        camera.pos.y += dist_delta ; distance_travelled += dist_delta ;
    
        dist_delta = vel * sin ( 2*M_PI*camera.pitch/3600 ) ; 
        camera.pos.z += dist_delta ; distance_travelled += dist_delta ;

    }

    if ( camera.left || camera.right )
    {
        vel = time_mul * basic_velocity * ( camera.left ? 1 : -1 ) ; /* FIXME: change this to a vector and velocity-based movement mechanism */
        camera.pos.x -= vel * sin ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
        camera.pos.y += vel * cos ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
    }

}


void move_entity(/* Entity * e*/)
{

}

void pause_physics()
{
}


