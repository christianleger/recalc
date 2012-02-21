

#include "recalc.h"


char phys_msgs[100][256] ;
int phys_msgs_num = 0 ;

// from recalc.cpp
extern Camera camera ;
extern Engine engine ;
//FIXME: camera DIR should replace camdir? 

//vec camdir ;

// from input.cpp 
extern int mouse_deltax ;
extern int mouse_deltay ;

float distance_travelled = 0.f ;
int physics_count = 0 ; // physics frame count 
/*
    physics_count ++ ; 
    if ( physics_count == 900 )
    {
        physics_count = 0 ; 
        //printf( "\n time delta = = %.2f \n", distance_travelled ) ; 
    }
*/













float basic_velocity = 100 ;

// FIXME: fix me!!!!!!!!!
bool hhello = false ;
bool jumping = false ;
bool onfloor = false ;

void physics_frame( unsigned int time_delta )
{
    float dist_delta ;
    float time_mul = 0 ;
    phys_msgs_num = 0 ;

    // time_mul = (float)time_delta / (float)PHYSICS_FRAME_TIME ;
    time_mul = (float)time_delta / (float)1000 ;
    //time_mul = 1.0f ;


    /*
        For every dynamic entity, move it by the amount that the time slice 
        and its current velocity and position allow. 
    */

//zzzz
   sprintf(phys_msgs[phys_msgs_num], "HELLO FROM PHYSICS ") ;  phys_msgs_num++ ;



float advance = 0.f ;
float t = 0.f ;
static float fallspeed = 0 ;
while (advance<t)
{
/*
    if ray from feet to direction of velocity times time slice hits something, 
    then advance the amount that brings us to that collision, and then see if there's 
    any advance left. 

    vec loc ;
    RayHitWorld(pos, ray, &loc, t) ; // t is the multiplier on the ray that specifies how far to check the ray

    When we collide with a piece of the world, we reduce our velocity by the component that 
    would take us through an obstacle, because that isn't happening. 


    every step: 

        if the way collides with something, 

        subtract ray's projection on that something's normal

        resume advance until t is <=0. 
*/
}



vec loc ;
vec norm ; 
float speed = 0.0f ; // magnitude of velocity
extern float RayHitWorld(vec pos, vec ray, float max_t, vec* loc, vec* normal) ;
float the_t = -10.f ;
vec newpos = camera.pos ;
vec dir = camera.dir ;
vec vel(0) ; // = camera.vel ; 


// Determine amount of impulse on our velocity
// Speed is calculated here to be how much we will travel is there are no obstacles. 
speed = time_mul * basic_velocity * ( camera.forward ? 1 : -1 ) ; 


   //sprintf(phys_msgs[phys_msgs_num], "time_mul = %f", time_mul) ;  phys_msgs_num++ ;
   //sprintf(phys_msgs[phys_msgs_num], "FALLSPEED = %d", fallspeed) ;  phys_msgs_num++ ;
   //sprintf(phys_msgs[phys_msgs_num], "FALLSPEED = %f", fallspeed) ;  phys_msgs_num++ ;

    // Bound time*speed (amount to travel) by any obstacle in front of us
    if (camera.forward || camera.backward )
    {
        // t = RayHitWorld(camera.pos, dir, speed, &loc, &norm) ; 
        if (!engine.editing)
        {
            speed = RayHitWorld(camera.pos, dir, speed, &loc, &norm) ; 
        }

        // In fps mode, only update x and y velocity
        loopi(2)
        {
            vel[i] = speed * dir[i] ; 
        }
        if (engine.editing)// If we're editing, then we can fly
        {
            vel.z = speed * dir.z ;
        }

    }
    else { speed = 0 ; }


if (!engine.editing)
{
    // gravity? adjusted 'by ear'
    fallspeed += -90.8*time_mul ;
    // If we're on the floor - and we attempt to jump - jump! 
    if (jumping && onfloor)
    {
//        printf( "\n FALLSPEED BEING NEGATIVATED \n") ; 
        //if (jumptime<30)
        {
            fallspeed = 30 ; // falling up!
            jumping = false ;
         //   jumptime++ ;
        }
        vel.z = fallspeed ;

        //else
        //{   
         //   jumping = false ;
          //  jumptime = 0 ;
        //}
    }
    else
    // Unless we're in the air
    if (!onfloor)
    {
        //vel.z = fallspeed ;
    }
    vel.z = fallspeed ;
   //     printf( "\n FALLSPEED = %.2f \n", fallspeed) ; 
}
else
{
}
//vel.x = 0 ;
//vel.y = 0 ;
//vel.z = 0 ;
    // Side movement 
    if ( ( camera.left ) || ( camera.right ) )
    {
        float sidespeed = time_mul * basic_velocity * ( camera.right ? 1 : -1 ) ; 
        ///vel = time_mul * basic_velocity * -1 ; // * FIXME: change this to a vector and velocity-based movement mechanism * /
        vel.x += sidespeed * sin ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
        vel.y -= sidespeed * cos ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
        //newpos.x += sidespeed * sin ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
        //newpos.y -= sidespeed * cos ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
    }

    newpos.x += vel.x ; 
    newpos.y += vel.y ; 
    newpos.z += vel.z ; 

   //if ((the_t=RayHitWorld(camera.pos, dir, vel, &loc, &norm))>0)

//newpos.z += - 9.8 * time_mul ; 
//newpos.z += - 9.8 ; 
/*
    After movement, we can update camera position, followed by adjustments if 
    we're too close to world geometry. 
*/
{
/*
    Octant* newnode = findNode(newposi) ;
    if (newnode)
    if (!(newnode->has_geometry())) 
    {
         camera.pos = newpos ;
         Octant* currentNode = findNode(posi) ;
         float disp = 100.0 ;
         Octant* nNode = NULL ;
         loopi(3)
         {
            newposi[i]+=100 ; 
            nNode = findNode(newposi) ;
            if (nNode->has_geometry()) { camera.pos[i] -= disp ; }
            newposi[i]-=200 ; 
            nNode = findNode(newposi) ;
            if (nNode->has_geometry()) { camera.pos[i] += disp ; }
            else if (i==2)
            {
               //newpos.z -= 9.8 ; 
            }
            newposi[i]+=1 ;  // back to 'newposi' from before this loop
         }
        //camera.pos = newpos ;
    }
    else { sprintf(phys_msgs[phys_msgs_num], "WHOOPS FUCKER") ;  phys_msgs_num++ ; }
*/
}

if (!engine.editing)
{
t = RayHitWorld(newpos, vec(0,0,-1), 500, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
if (t<120)
{
    fallspeed = 0 ;
    onfloor = true ;
    if (t<100) 
    { //newpos.z += loc.z + 100.f ;
        newpos.z += 100.f-t ; 
    }
}
else
{
    onfloor = false ;
}

t = RayHitWorld(newpos, vec(1,0,0), 100, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
if (t<30) { newpos.x -= 30.f-t ; }
t = RayHitWorld(newpos, vec(-1,0,0), 100, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
if (t<30) { newpos.x += 30.f-t ; }

t = RayHitWorld(newpos, vec(0,1,0), 100, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
if (t<30) { newpos.y -= 30.f-t ; }
t = RayHitWorld(newpos, vec(0,-1,0), 100, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
if (t<30) { newpos.y += 30.f-t ; }
}

camera.pos = newpos ;
//camera.vel = vel ;
    sprintf(phys_msgs[phys_msgs_num], "position = %.2f %.2f %.2f", 
        newpos.x, 
        newpos.y, 
        newpos.z 
        ) ;  phys_msgs_num++ ;
/*
t = RayHitWorld(newpos, vec(0,0,-1), 100, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
if (t<100)
{
    //newpos.z += loc.z + 100.f ;
    newpos.z += 100.f-t ;
}
*/
    /*
        Here we adjust our position, in case we're too close to world geometry. 
        Even if we didn't enter geometry yet, getting too close can lead to 
        floating-point artifacts where moving too close to a plane, towards 
        the plane, leads to us being inside without us wanting to. 
    extern Octant* findNode(ivec at, int* out_size=NULL, int* nscale=NULL) ;
    ivec newposi(newpos) ;
    ivec posi(camera.pos) ;
    int ns ; // node size
    int disp = 0 ;
    Octant* nNode = NULL ;
    loopi(3)
    {
        newposi[i]+=10 ; 
        nNode = findNode(newposi, &ns) ;
        if (nNode->has_geometry()) 
        { 
            // So reduce newposi by the amount we figure will put us at a 
            // reasonable distance from the node in question. 
            disp = newposi[i] % ns ;
            //camera.pos[i] -= ( disp + 1 ) ; 
            newpos[i] -= ( disp + 1 ) ; 
        }

        newposi[i]-=20 ; 
        nNode = findNode(newposi, &ns) ;
        if (nNode->has_geometry()) 
        { 
            disp = ns - (newposi[i] % ns) ;
            //camera.pos[i] += ( disp + 1 ) ; 
            newpos[i] += ( disp + 1 ) ; 
        }
        else if (i==2)
        {
            //newpos.z -= 9.8 ; 
        }
        newposi[i]+=10 ;  // back to 'newposi' from before this loop
    }
    */



}

/*
    Basics: 
        Using the entity's position, velocity and bounding box, 
        determine the furthest it can move in a given time span. 
        The time span is given by the number of milliseconds since 
        the last step.

    Steps: 
        Compute the aabbox which completely contains the entity's 
        current position and the position it's trying to move to. 
        If any geometry 
*/
void move_entity(/* Entity * e*/)
{

}

void pause_physics()
{
}


