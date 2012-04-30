

#include "recalc.h"

#include "physics.h" 

static NewtonWorld* g_world;

char phys_msgs[100][256] ;
int phys_msgs_num = 0 ;

// from recalc.cpp
extern Camera camera ;
extern Engine engine ;
extern World world ;
//FIXME: camera DIR should replace camdir? 

//vec camdir ;

// from input.cpp 
extern int mouse_deltax ;
extern int mouse_deltay ;

float distance_travelled = 0.f ;
int physics_count = 0 ; // physics frame count 



void renderentity(Entity* e) ;

/*
    physics_count ++ ; 
    if ( physics_count == 900 )
    {
        physics_count = 0 ; 
        //printf( "\n time delta = = %.2f \n", distance_travelled ) ; 
    }
*/





float basic_velocity = 2500 ;

// FIXME: fix me!!!!!!!!!
bool hhello = false ;
bool jumping = false ;
bool onfloor = false ;
bool updatephysics = false ;

void physics_frame( unsigned int time_delta )
{

    // Calculate the time slice for which this next physics frame is being run

    float dist_delta ;
    float time_mul = 0 ;
    phys_msgs_num = 0 ;

    time_mul = (float)time_delta / (float)1000 ;


    // Have newton compute one frame for 1/60th of a second. 

if (updatephysics)
{
//    printf("\n updating Newton world. ") ;
    NewtonUpdate (g_world, (1.0f / 60));
//    NewtonUpdate (g_world, ( 60));
}


    sprintf(phys_msgs[phys_msgs_num], "HELLO FROM PHYSICS ") ;  phys_msgs_num++ ;
    sprintf(phys_msgs[phys_msgs_num], "time_mul=%f", time_mul) ;  phys_msgs_num++ ;

    float t = 0.f ;
    static float fallspeed = 0 ;

    vec loc ;
    vec norm ; 
    float speed = 0.0f ; // magnitude of velocity

    vec newpos = camera.pos ;
    vec dir = camera.dir ;
    vec vel(0) ; // = camera.vel ; 

    // Determine amount of impulse on our velocity
    // Speed is calculated here to be how much we will travel is there are no obstacles. 
    speed = time_mul * basic_velocity * ( camera.forward ? 1 : -1 ) ; 

    if (camera.forward || camera.backward )
    {
        // In fps mode, only update x and y velocity
        vel.x = speed * dir.x ; 
        vel.y = speed * dir.y ; 
        if (engine.editing) { vel.z = speed * dir.z ; }
    }

    // Z velocity
    if (!engine.editing)
    {
        // gravity? adjusted 'by ear'
        fallspeed += -90.8*time_mul ;
        // If we're on the floor - and we attempt to jump - jump! 
        if (jumping && onfloor)
        {
            {
                fallspeed = 30 ; // falling up!
                jumping = false ;
             //   jumptime++ ;
            }
        }
        vel.z = fallspeed ;
    }

    // Side movement 
    if ( ( camera.left ) || ( camera.right ) )
    {
        float sidespeed = time_mul * basic_velocity * ( camera.right ? 1 : -1 ) ; 
        ///vel = time_mul * basic_velocity * -1 ; // * FIXME: change this to a vector and velocity-based movement mechanism * /
        vel.x += sidespeed * sin ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
        vel.y -= sidespeed * cos ( 2*M_PI*(camera.yaw+900)/3600 ) ; 
    }

    // FIXME: you go through walls when you update these without checking if there is an obstacle first! 
    // At least, you go through walls when you're going fast. And you don't check for obstacles. 
    newpos.x += vel.x ; 
    newpos.y += vel.y ; 
    newpos.z += vel.z ; 

    extern Octant* FindNode(ivec at, int* out_size=NULL, int* nscale=NULL) ;
    ivec posi(camera.pos) ;
    ivec newposi(newpos) ;
    Octant* newnode = FindNode(newposi) ;


//speed = 0 ;


if (!engine.editing)
{
    extern float RayHitWorld(vec pos, vec ray, float max_t, vec* loc, vec* normal) ;
    t = RayHitWorld(newpos, vec(1,0,0), speed, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
    if (t<speed) { newpos.x -= speed-t ; }

    t = RayHitWorld(newpos, vec(-1,0,0), speed, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
    if (t<speed) { newpos.x += speed-t ; }

    t = RayHitWorld(newpos, vec(0,1,0), speed, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
    if (t<speed) { newpos.y -= speed-t ; }

    t = RayHitWorld(newpos, vec(0,-1,0), speed, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
    if (t<speed) { newpos.y += speed-t ; }

    t = RayHitWorld(newpos, vec(0,0,-1), 120, &loc, &norm) ; // speed gets adjusted in case we're about to hit something
    if (t<120)
    {
        fallspeed = 0 ;
        onfloor = true ;
        if (t<100) { newpos.z += 100.f-t ; }
    }
    else { onfloor = false ; }


}   // end if not editing

camera.pos = newpos ;

sprintf(phys_msgs[phys_msgs_num], "position = %.2f %.2f %.2f", newpos.x, newpos.y, newpos.z ) ;  phys_msgs_num++ ;

}   // end physics_frame

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

vector<Entity*> entities ;
void move_ent(Entity * e)
{
}

void move_entities()
{
    loopv(entities)
    {
        move_ent(entities[i]) ;
    }
}

void pause_physics()
{
}


struct nVector
{
    float x ;
    float y ;
    float z ;
    float w ;

    nVector():x(0),y(0),z(0),w(0)
    {
    }

    nVector(float in_x, float in_y, float in_z): 
        x(in_x), y(in_y), z(in_z), w(0)
    {
    }
    nVector(float in_x, float in_y, float in_z, float in_w): 
        x(in_x), y(in_y), z(in_z), w(in_w)
    {
//        printf("\n vector init: %f %f %f %f", x, y, z, w) ;
    }

/*
    nVector(float in_x, float in_y, float in_z, float in_w)
    {
        nVector(in_x, in_y, in_z) ;
        w = in_w ;
    }
*/
    float& operator[](int i)
    {
        return (&x)[i] ;
    }
} ;

struct nMatrix
{
    nMatrix(): 
        a(1,0,0,0), 
        b(0,1,0,0), 
        c(0,0,1,0), 
      pos(0,0,0,1)
    {
    }

    nVector& operator[](int i)
    {
        return (&a)[i] ;
    }

    nVector a ;
    nVector b ;
    nVector c ;
    nVector pos ;
} ;


void NullForceCallback(
    const NewtonBody* body, 
    float timestep, 
    int threadIndex
    )
{
}

bool resetphysics = false ;
/*
    Newton will call this on every NewtonBody that we have registered. 
*/
void ApplyForceCallback(
    const NewtonBody* body, 
    float timestep, 
    int threadIndex
    )
{


    float Ixx;
    float Iyy;
    float Izz;
    float mass;
 
    // for this tutorial the only external force in the Gravity
    NewtonBodyGetMassMatrix (body, &mass, &Ixx, &Iyy, &Izz);
 
    nVector gravityForce(0.0f, - mass * 9.8*64, 0.0f) ; // *64 because world meters are 64 units

    NewtonBodySetForce(body, &gravityForce[0]);
}


/*
    Newton will call this on every NewtonBody that needs to be moved. 
*/
void SetTransformCallback(
    const NewtonBody* body, 
    const float* matrix, 
    int threadIndex
    )
{
    Entity* ent;
    ent = (Entity*) NewtonBodyGetUserData(body) ;

    loopv(entities) { if (ent==entities[i]) { printf("\n Entity %d: ", i ) ; } }


    // Get the position from the matrix
    nVector posit (matrix[12], matrix[13], matrix[14]);
    
    printf("pos=%f %f %f ",
        ent->pos[0], ent->pos[1], ent->pos[2]
        ) ;
    printf("\nrot=%f %f %f ",
        matrix[0], matrix[1], matrix[2]
        ) ;

if (ent==entities[1]) return ;

/*
    if (resetphysics)
    {
    float t[3] = {0.0f,0.0f,0.0f} ;
        printf("\n physics reset! ") ;
        nMatrix m ;
        m.pos = nVector(200,4000,0,1) ;
        nVector v(0,0,0,1) ;
        NewtonBodySetMatrix (body, &m[0][0]);
        NewtonBodySetVelocity(body, &v[0]);
        NewtonBodySetOmega(body, &v[0]);
        NewtonBodySetTorque(body,&t[0]) ;
        resetphysics = false ;
    }
*/


    ent->pos[0] = posit[0] ; 
    ent->pos[1] = posit[2] ; 
    ent->pos[2] = posit[1] ;

    //printf("pos=%f %f %f   nPos=%f %f %f", ent->pos[0], ent->pos[1], ent->pos[2], posit[0], posit[1], posit[2]) ;
    nVector min ;
    nVector max ;
    NewtonBodyGetAABB(body, &min[0], &max[0]) ;
//        min[0], min[2], min[1], 
 //       max[0], max[2], max[1]
}

void CollideCallback(NewtonUserMeshCollisionCollideDesc* collideDescData)
{
    //printf("\n helllllo collision! ") ;
}


int materialID ;

int AABBOverlapProcess(
    const NewtonMaterial* material, 
    const NewtonBody* body0, 
    const NewtonBody* body1, 
    int threadIndex
    ) 
{
//printf("\n YO COLLISION") ;
    return 1 ;
}

void ContactCallback(
    const NewtonJoint* contactJoint, dFloat timestep, int threadIndex
    )
{
    printf("\n Contact callback. "
        //NewtonMaterialGetContactNormalSpeed(materialID) 
        ) ;





    NewtonBody* const body = NewtonJointGetBody0(contactJoint);
    for (void* contact = NewtonContactJointGetFirstContact (contactJoint); contact; contact = NewtonContactJointGetNextContact (contactJoint, contact)) {
        float speed;
        nVector point;
        nVector normal;
        nVector dir0;
        nVector dir1;
        nVector force;
        NewtonMaterial* material;

        material = NewtonContactGetMaterial (contact);

        NewtonMaterialGetContactForce (material, body, &force.x);
        NewtonMaterialGetContactPositionAndNormal (material, body, &point.x, &normal.x);
        NewtonMaterialGetContactTangentDirections (material, body, &dir0.x, &dir1.x);
        speed = NewtonMaterialGetContactNormalSpeed(material);


        //speed = NewtonMaterialGetContactNormalSpeed(material);
        // play sound base of the contact speed.
        //
    }

}

void printmatrix(nMatrix& m)
{
    printf("\n Matrix contents: --------------------------------------") ;
    loopi(4) 
    {
        printf("\n ") ;
        loopj(4) printf(" %f ", m[i][j]) ;
    }
}


NewtonBody* body1 ;
NewtonBody* body2 ;


/*
    Box Body characteristics: 

        - box dimensions

        - offset of center of mass from box origin

        - mass

        - inertia matrix (identity) 

        - position



*/
void AddBody(float x,float y,float z, int entID)
{
float mass = 1000 ;
NewtonCollision* shape ;
NewtonBody* body ;

nMatrix boxOrigin ;
nVector minBox ;
nVector maxBox ;
nVector origin ;
nVector inertia ;
materialID = NewtonMaterialGetDefaultGroupID(g_world) ;

    Entity* e = entities[entID] ;

    boxOrigin.pos = nVector(0,0,0,1) ;

    shape = NewtonCreateBox (g_world, 600, 1200, 600, 0, &boxOrigin[0][0]);
    NewtonCollisionCalculateAABB( shape, &boxOrigin[0][0], &minBox[0], &maxBox[0]) ;
    printf("\n collision shape result: \n min=%f %f %f \n max=%f %f %f", minBox[0], minBox[2], minBox[1], maxBox[0], maxBox[2], maxBox[1]) ;
    body = NewtonCreateBody (g_world, shape, &boxOrigin[0][0]);
   
    NewtonBodySetUserData(body, e) ; 
    e->pos = vec(x,y,z) ;
    boxOrigin.pos = nVector(e->pos.x, e->pos.z, e->pos.y, 1) ;
    NewtonBodySetMatrix (body, &boxOrigin[0][0]);
    NewtonBodySetLinearDamping(body, 0) ;
    NewtonConvexCollisionCalculateInertialMatrix (shape, &inertia[0], &origin[0]);
    NewtonBodySetMassMatrix (body, mass, mass*inertia.x, mass*inertia.y, mass*inertia.z);
    origin[1] -= 600 ;
    NewtonBodySetCentreOfMass (body, &origin[0]);
   
    if (entID==0)
    {
    body1 = body ;
        NewtonBodySetForceAndTorqueCallback (body, ApplyForceCallback);
    NewtonBodySetMassMatrix (body, mass, mass*inertia.x, mass*inertia.y, mass*inertia.z);
    }
    else
    {
    mass = 0 ; // immovable static object
    body2 = body ;
        NewtonBodySetForceAndTorqueCallback (body, NullForceCallback);
    NewtonBodySetMassMatrix (body, mass, mass*inertia.x, mass*inertia.y, mass*inertia.z);
    }

    NewtonBodySetTransformCallback (body, SetTransformCallback);
    NewtonBodySetMaterialGroupID(body, materialID) ;


}

void init_physics()
{
    //g_world = NewtonCreate (AllocMemory, FreeMemory);
    nVector worldMin(-1000,-1000,-1000) ;
    nVector worldMax(world.size,world.size,world.size) ;
    
    if (g_world) NewtonDestroyAllBodies (g_world);
    g_world = NewtonCreate() ;
    NewtonSetWorldSize (g_world, &worldMin[0], &worldMax[0]);

    NewtonSetSolverModel(g_world, 2) ;

//int materialID = NewtonMaterialCreateGroupID(g_world) ;
int materialID = NewtonMaterialGetDefaultGroupID(g_world) ;

float mass = 10 ;
NewtonCollision* shape ;
NewtonBody* body ;

nMatrix boxOrigin ;
nVector minBox ;
nVector maxBox ;
nVector origin ;
nVector inertia ;

//-----------------------------------------------------------
// Body 1
//-----------------------------------------------------------
printf("\n\n--------------------Body 1--------------------") ;

entities.add(new Entity()) ;
AddBody(800,800,8000,0) ;


//--------------------------------------------------------
// Body 2
//--------------------------------------------------------
printf("\n\n--------------------Body 2--------------------") ;

//AddBody(0,0,3500) ;
entities.add(new Entity()) ;
AddBody(600,800,2000,1) ;


//--------------------------------------------------------
//FIXME: figure out if this should be called    NewtonReleaseCollision (g_world, shape);

    NewtonMaterialSetCollisionCallback (
        g_world, 
        materialID, 
        materialID, 
        NULL,               // void* userData,
        AABBOverlapProcess, // NewtonOnAABBOverlap aabbOverlap, 
        ContactCallback   
        );
   
    // Set up Newton callbacks
    // Give Newton information about bodies (will be made dynamic once 
    // prototypes a functional. )
}


/*


Empty space so I can read coder higher than the baseline of my monitor. Don't like it? 
Maybe you should have thought of that before you gave your money to the people who 
killed Star Trek. Explanation available. 


*/

/*
    Here is the sequence of handling moving, colliding objects using Newton. 

    Assumptions: for any given set of objects, there is one world active, one world 
    that contains them. 

    - the world is a static, immovable object relative to the other objects
    - all other objects can be moved
    - as a design choice for simplicity and performance, all small and medium 
      objects can be boxes 

    

------------------------------
    Information for Newton
------------------------------
    Setting world dimensions: 
        - world dimensions
        -> NewtonSetWorldSize

How do we set an object's matrix: 
        
        -> NewtonBodySetMatrix ( NewtonBody* b, Matrix* m)

            - NewtonBody is defined by Newton
            - Matrix has columns front, up, right, pos. 
              Thus, to get the position from the matrix, we can 
              get the last four elements. 

    Tell Newton a body's dimensions: 
        - the mass
        - the sizes
        -> NewtonCollisionCalculateAABB

    Setting a force-and-torque callback: 
        - a force-and-torque callback

    Setting a transform callback: 
        - a transform callback

    Tell Newton to advance the simulation by a frame: 

        - call NewtonUpdate. 
          This function will: 
            - apply forces to all objects (using our callback)
            - internally: will compute collisions and changes 
              in direction
            - apply transforms based on new velocities and size 
              of time step. 

------------------------------
    Information for us
------------------------------
    How do we get our data from Newton: 
        -> NewtonBodyGetMatrix
*/



/*
    Draw six faces that indicate the entity's AABB. 
*/
void renderentity(Entity* e)
{
//    e-> pos = vec(32000,32000,4000) ;
    vec p = e->pos ;

    p[0] += 300 ;
    glBegin( GL_QUADS ) ;

        glVertex3fv(p.v) ; p[0] -= 600 ;
        glVertex3fv(p.v) ; p[2] += 1200 ;
        glVertex3fv(p.v) ; p[0] += 600 ;
        glVertex3fv(p.v) ; 

        glVertex3fv(p.v) ; p[1] -= 600 ;
        glVertex3fv(p.v) ; p[2] -= 1200 ;
        glVertex3fv(p.v) ; p[1] += 600 ;
        glVertex3fv(p.v) ; 

        glVertex3fv(p.v) ; p[1] -= 600 ;
        glVertex3fv(p.v) ; p[0] -= 600 ;
        glVertex3fv(p.v) ; p[1] += 600 ;
        glVertex3fv(p.v) ; 
        
        glVertex3fv(p.v) ; p[1] -= 600 ;
        glVertex3fv(p.v) ; p[2] += 1200 ;
        glVertex3fv(p.v) ; p[1] += 600 ;
        glVertex3fv(p.v) ; 

    glEnd() ;
}

void renderentities() 
{
    loopv(entities) 
    {
        renderentity(entities[i]) ;
    }
}

void reset_physics() 
{
resetphysics = true ;
    nMatrix m ;
    nVector v(0,0,0,1) ;

    entities[0]->pos = vec(200,6000,4000) ;
    m.pos = nVector(200,4000,6000,1) ;
    NewtonBodySetMatrix (body1, &m[0][0]);
    NewtonBodySetVelocity(body1, &v[0]);
    NewtonBodySetOmega(body1, &v[0]);

    entities[1]->pos = vec(0,6000,2000) ;
    m.pos = nVector(0,2000,6000,1) ;
    NewtonBodySetMatrix (body2, &m[0][0]);
    NewtonBodySetVelocity(body2, &v[0]);
    NewtonBodySetOmega(body2, &v[0]);
}


