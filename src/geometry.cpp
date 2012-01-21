/*
    File: geometry.cpp

    Contains the elements of world geometry, the operations to manipulate 
    world geometry, and the operations to collect information about this 
    structure. 

    See geometry.h for the contents of the various geometrical structures. 

*/


#include "recalc.h"

extern Camera camera ;






//------------------------------------------------------------------------
//                  GLOBAL VARIABLES
//------------------------------------------------------------------------
vec  front ;
vec  pos ;
vec  camdir ;

// How about 100 lines for monitoring your geometry action? 
// How much will that cost? 25K my good man. 
// You can afford that how many times? At least 2000 times? Splendid! 
// You won't regret this sir!!

int geom_msgs_num = 0 ;
char geom_msgs[100][256] ;

int geom_msgs_num2 = 0 ;
char geom_msgs2[100][256] ;



// sprintf(geom_msgs[geom_msgs_num], "pointcount=%d.", pointcount ) ; geom_msgs_num++ ;
//------------------------------------------------------------------------
//                  GEOMETRY SUPPORT FUNCTIONS  (octree, memory, etc.)
//------------------------------------------------------------------------
Octant* findNode(ivec at, int* nscale, int* out_size=NULL) ;

int numchildren = 0 ;
void extrude( void * _in ) ;
void update_editor() ;
// This is equivalent to 2^15 units, or 32,768

#define WORLD_SCALE 15


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// The World and its Dimensions
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// here 'world scale' really means the largest piece out of the world, which 
// is one of the 8 cubes that divide the world
             // default 15   size: 2^15=65536    gridscale    max gridsize: 2^15
World world = { WORLD_SCALE, 2<<WORLD_SCALE , WORLD_SCALE, 2<<(WORLD_SCALE-1) } ;
#define w world


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool aiming_at_world ;

bool hit_world = false ;

int orientation = 0 ;

Octant::Octant()
{
    children = NULL ;
    geom = NULL ;
    set_all_edges(0) ;
}

bool Octant::has_geometry() 
{
    return (
            (edge_check[0]==MAX_INT) &&
            (edge_check[1]==MAX_INT) &&
            (edge_check[2]==MAX_INT)
           ) ;
}
bool Octant::has_children()
{
    return (children != NULL) ;
}

//printf("\n set_all_edges: edges of this octant located at %d", (int)&edges[0]) ;
//printf("\n set_all_edges: edges[%d] = %d", (i) , edges[i]) ;
    void Octant::set_all_edges(int in_value=255)
    {
        //DEBUGTRACE(("\n setting all edges with d=%d\n", in_value)) ;
        loopi(12)
        {
            edges[i] = in_value ;
        }
    }

void new_octant( Octant& oct )
{
    oct.children = new Octant[8] ;
    return ;
}   
//------------------------------------------------------------------------
//                  EDITING VARIABLES AND FUNCTIONS
//------------------------------------------------------------------------
#define o orientation 
#define octstep(x,y,z,scale) ((z>>(scale))&1)<<2 | ((y>>(scale))&1)<<1 | ((x>>(scale))&1)
// corner determines the selection: it is the minimum in X,Y,Z of a box 
// of size gridsize*(xCount,yCount,zCount). 
vec corner( 0, 0, 0 ) ;

bool havesel = false ;
bool havesel_end = false ;
bool havenewcube = false ;
bool newcubes_new = false ;

ivec newcube ;
//vec newcube ;
#define NC newcube 

int sel_size = 0 ;
int sel_o = 0 ;
ivec sel_start ;
ivec sel_end ;
ivec sel_counts ;
ivec sel_min ;
//vec sel_min ;
ivec sel_max ;
Octant* ray_start_node ;
bool have_ray_start_node = false ;
ivec ray_start_vec ;
int ray_start_orientation = 0 ;

void clear_selection()
{
}

void set_sel_start()
{
    havesel = true ;
    havesel_end = false ;
    havenewcube = false ; 
    sel_start = corner ;
    sel_end = corner ;
    sel_counts = vec(1,1,1) ;
    sel_o = orientation ;
    sel_size = world.gridsize ;
    printf("\nselection starting at %d  %d  %d\n", 
        sel_start.x, 
        sel_start.y, 
        sel_start.z 
        ) ; 
}
void set_sel_end()
{
    if (!havesel || orientation!=sel_o ) 
    {
        set_sel_start() ;
        return ;
    }
    havenewcube = false ; // reset potential set of new cubes
    havesel_end = true ;
    sel_end = corner ;
    sel_size = world.gridsize ;
    loopi(3)
    {
        sel_counts[i] = (( max(sel_start[i], sel_end[i]) - min(sel_start[i], sel_end[i]) ) >> w.gridscale ) + 1 ;
    }
    sprintf(geom_msgs[geom_msgs_num], "selection counts: %d %d ", sel_counts.x, sel_counts.y) ; geom_msgs_num++ ;
    /*
    printf("\nselection ending at %d  %d  %d\n", 
        sel_end.x, 
        sel_end.y, 
        sel_end.z 
        ) ; 
    printf("\nselection counts = %d  %d  %d\n", 
        sel_counts.x, 
        sel_counts.y, 
        sel_counts.z
        ); 
        */
}

int orientation_indexes[6][3] = 
{
    {1,2,0}, 
    {1,2,0}, 
    {0,2,1}, 
    {0,2,1}, 
    {0,1,2}, 
    {0,1,2} 
} ; 

float direction_multipliers[6][3] = 
{
    {-1, 1, -1}, 
    { 1, 1,  1}, 
    { 1, 1, -1}, 
    {-1, 1,  1}, 
    {-1, 1, -1}, 
    { 1, 1,  1}
} ; 

// This stuff makes us able to pretend that any axis-aligned plane is just 
// the plain old XY plane, with X increasing to the right and Y increasing upwards. 
/*
    Orientations are defined as follows: 

    0 -  x-ve
    1 -  x+ve
    2 -  y-ve
    3 -  y+ve
    4 -  z-ve
    5 -  z+ve

*/
#define X(_o) orientation_indexes[_o][0]
#define Y(_o) orientation_indexes[_o][1]
#define Z(_o) orientation_indexes[_o][2]
#define Dx(_o) direction_multipliers[_o][0]
#define Dy(_o) direction_multipliers[_o][1]
#define Dz(_o) direction_multipliers[_o][2]


// Oriented square centered on a point
void draw_square(
                vec & center    /* center */,
                int size,       /* size of box */
                int _o           /* orientation */
            )
{
    vec c(center.v) ;
    c[X(_o)] -= Dx(_o)*(size>>1) ;
    c[Y(_o)] -= Dy(_o)*(size>>1) ; // now positioned at 'bottom-left' corner. 

    glColor3f( 1, 1, 0 ) ;

    glBegin( GL_LINE_LOOP ) ;
        glVertex3fv( c.v ) ; c[X(_o)] += Dx(_o)*size ;  // bottom-right
        glVertex3fv( c.v ) ; c[Y(_o)] += Dy(_o)*size ;  // top-right
        glVertex3fv( c.v ) ; c[X(_o)] -= Dx(_o)*size ;  // top-left
        glVertex3fv( c.v ) ;
    glEnd() ;
}

/*
    Draw a square that takes 'corner' as its lower-left corner. 

    The direction of the other corners is determined by the 
    orientation parameter. 

*/
void draw_corner_square(
    vec& _corner,        // lowest values for the coords not in the orientation vector (see function description for info)
    int size, 
    int _o
    )
{
    ivec c(_corner) ;
    //ivec v(_corner) ;
// FIXME FIXME FIXME FIXME

    glBegin( GL_LINE_LOOP ) ;
        glVertex3iv( c.v ) ; c[X(_o)] += size ;  // bottom-right
        glVertex3iv( c.v ) ; c[Y(_o)] += size ;  // top-right
        glVertex3iv( c.v ) ; c[X(_o)] -= size ;  // top-left
        glVertex3iv( c.v ) ;
    glEnd() ;
/*
*/
}

void draw_corner_cube(
    vec & _corner,
    int size
    )
{
    // ivec icorner = _corner ;
    // first two squares cornered at origin 
    draw_corner_square( _corner, size, 0) ; 
    draw_corner_square( _corner, size, 2) ; 
    // third square cornered at origin.x + size 
    corner[0] += size ; 
    draw_corner_square( _corner, size, 1) ;
    corner[0] -= size ; corner[1] += size ; 
    draw_corner_square( _corner, size, 3) ;
/*
*/
}

void draw_corner_square()
{
    glColor3f( 1, 1, 0 ) ;
    draw_corner_square(
        corner, 
        //2<<(world.scale-1), 
        world.gridsize,
        orientation
        ) ;
}


void draw_sel_start()
{
    if ( !havesel ) return ;
    glColor3f( 1, 0, 0 ) ;
    vec v( sel_start.v ) ;
    draw_corner_square( 
        v, 
        sel_size,
        sel_o
        ) ;
}

void draw_sel_end()
{
    if ( !havesel_end ) return ;
    glColor3f( 0, 0, 1 ) ;
    vec v( sel_end.v ) ;
    draw_corner_square( 
        v,
        sel_size,
        sel_o
        ) ;
}

void draw_ray_start_node()
{
    glColor3f(0.f,1.f,0.f) ;
    if (have_ray_start_node)
    {
        vec v(ray_start_vec.v) ;
        draw_corner_square( 
            v,
            world.gridsize,
            ray_start_orientation
            ) ;

        /*
        draw_corner_cube(
            v,
            world.gridsize
            ) ;
        */
    }
}

void draw_world_box()
{
    int half = world.size>>1 ;

    vec center( 0, half, half ) ;

    // glDisable( GL_DEPTH_TEST ) ; 

    draw_square(center, world.size, 0) ; center[0] += half ; center[1] += half ;
    draw_square(center, world.size, 3) ; center[0] += half ; center[1] -= half ;
    draw_square(center, world.size, 1) ; center[0] -= half ; center[1] -= half ;
    draw_square(center, world.size, 2) ;
    
    // glEnable ( GL_DEPTH_TEST ) ; 
    /*  */
}


/*
    There is a relationship between orientations and the planes. 
    Orientation (of a cube face, for example) will be numbered 0-5. 
    normal vectors to each axix-aligned plane 
*/
plane world_planes[6] =
{
    plane( -1,  0,  0 , world.size ),
    plane(  1,  0,  0 , 0 ),
    plane(  0, -1,  0 , world.size ),
    plane(  0,  1,  0 , 0 ),
    plane(  0,  0, -1 , world.size ),
    plane(  0,  0,  1 , 0 )
} ;
#define wp world_planes 

char plane_names[3][2] = { "X", "Y", "Z" } ;

int bounds[3][2] =
{
    {1, 2},  // point is on X plane; check Y and Z boundaries 
    {0, 2},  // point is on Y plane; check X and Z boundaries  
    {0, 1}   // point is on Z plane; check X and Y boundaries 
} ;
#define b bounds


/*
    Find where a ray hits a plane by setting the parameter t. 

    The parameter V, the direction of the ray, is taken from the camdir (camera direction). 
    Values of t have the following meanings: 
        - positive: the ray hits the plane, for the point P' = P + tV where
          P is the origin point for the 
        - negative: the ray would hit the plane behind the camera, meaning 
          this plane is not ahead of us. Ignore. 
        - t==0: this means that our ray is parallel to the plane and perpendicular 
          to its normal. No intersection; ignore. 
*/
void RayHitPlane( vec& pos, vec ray, plane& pl, float* t )
{
    double numerator = 0, denominator = 0 ;

    numerator   = - pl.offset - ( pos.dot( pl.v) ) ;
    denominator =        camdir.dot( pl.v )  ;

    if ( denominator != 0 ) 
    { 
        *t = ( numerator ) / ( denominator ) ;  
    }
    // if denominator in plane equation is 0, that means vector parallel to plane. 
    else  
    { 
        *t = 0 ; 
    }
}
vec RayHitPlanePosition( vec& pos, vec ray, plane& pl, float* t )
{
    double numerator = 0, denominator = 0 ;

    numerator   = - pl.offset - ( pos.dot( pl.v) ) ;
    denominator =        camdir.dot( pl.v )  ;

    if ( denominator != 0 ) 
    { 
        *t = ( numerator ) / ( denominator ) ;  
    }
    // if denominator in plane equation is 0, that means vector parallel to plane. 
    else  
    { 
        *t = 0 ; 
    }
    vec hitpos = pos.add(ray.mul(*t)) ;
    return hitpos ;
}


/* 
    sel_path 

    This char array records where we are in the octree, when we have a selection. 
    
    Values of 0-7 represent the different possible octree children. 

    Value -1 means nothing is selected - it means we're pointing at a world boundary. 

    Value -2 means we're neither pointing at something inside the world nor a 
    a world boundary - it should mean we're outside the world and not pointing at 
    the world. 

*/
int sel_path[20] = {-2} ;
/*
    Outputs of this function: 

        - which, if any, world geometry is currently highlighted. 

        - which, if any, world planes are currently selected. 

*/

void update_editor()
{
    //bool advancing = true ;
    bool hitworld = false ;
    int hitplanes = 0 ;

    float t = 0 ;             // this records the parameter in the equation P' = P + tV to find where a ray touches a plane. 
    float min_t = 0 ;

    //pos = camera.pos ;
    camdir = camera.dir ;
    pos = camera.pos ;

    int gridscale = world.gridscale ; // maximum size of a selection square is half the world size. Else we wouldn't know! 
    //int gridscale = world.gridscale ; // maximum size of a selection square is half the world size. Else we wouldn't know! 
    geom_msgs_num2 = 0 ; 


    #define hittingplane (hitplanes>>(3*i))&0x07

    vec fcorner ;
    ivec icorner ;

    // TARGET FINDER PHASE 1: near world planes (if we're outside the world). 
    loopi(3)
    {
        hitplanes |= ( camdir[i]<0 ? 2*i : 2*i+1 ) << (3*i) ;
    }
    min_t = FLT_MAX ;

    have_ray_start_node = false ;


    vec dir ;

    if (camera.inworld(world))
    {
        front = pos ;
        // ray = camdir ;
    }
    else
    {
        if (!hitworld)
        {
            loopi(3)
            {
                RayHitPlane( pos, camdir, wp[ hittingplane ], &t ) ;

                if (t<min_t && t>0)
                {
                    front = pos ;
                    dir = camdir ;
                    front.add(dir.mul(t)) ; // this works because camdir has length 1. 
                    //front.add(t*camdir.x+t*camdir.y+t*camdir.z) ; // this works because camdir has length 1. 
                    // Here we check that the point // hitting the plane is inside // the world limits. 

                    int f1 = front[b[i][0]] ;
                    int f2 = front[b[i][1]] ;
                    int w = wp[2*i].offset ;

                    //if (front[b[i][0]] < wp[2*i].offset && front[b[i][0]] > 0  &&    
                    //    front[b[i][1]] < wp[2*i].offset && front[b[i][1]] > 0

                    if ( f1 > 0 && f1 < w && 
                         f2 > 0 && f2 < w 
                       )
                    {
                        orientation = (hittingplane) ;
                        min_t = t ;
                        fcorner = vec(front) ;
                        sel_path[0] = -1 ;
                        have_ray_start_node = true ;
                    }
                }
            } // end looping over all hitplane candidates 

            loopj(3)
            {


                // Truncate coordinates: keep them grid-aligned. 
                // We only truncate the variables which aren't the major 
                // axis for the current orienatation plane. If we did, it might 
                // give us a smaller value than we need. 
                if (!( (orientation==2*j) || (orientation==2*j+1) ))
                {
                    //printf("\nHOWDY\n") ;
                    icorner[j] = (((int)(fcorner[j])) >> gridscale ) << gridscale ;
                }
                else
                {   
                    // This nastiness is so that we are always flat against grid boundaries
                    icorner[j] = floor( fcorner[j] ) ;
                }
            }
        }
    }

 //    sprintf(geom_msgs[geom_msgs_num], "update_editor: point at %d %d %d", icorner[0], icorner[1], icorner[2]) ; geom_msgs_num++ ;



    // TARGET FINDER PHASE 2: Compute the first node our ray starts tracking 

    /*
    if (!have_ray_start_node )
    {
        loopi(3)
        {
            icorner[i] = ( ((int)(pos[i])) >> gridscale ) << gridscale ;
        }
    }
    */

/*
    // Some helpful debug info
    #define VERBOSE
    #ifdef VERBOSE
    geom_msgs_num2++ ;
    sprintf(
        geom_msgs2[geom_msgs_num2], 
        "ray start node: (has children = %c, has geom = %c, has extent = %c, size = %d)", 
        (oct->children) ? 'Y' :'N',
        (oct->geom) ? 'Y':'N',
        (oct->has_geometry()) ? 'Y':'N',
        size
    ) ; geom_msgs_num2++ ;
    #endif
*/

    //sprintf(geom_msgs[geom_msgs_num], "update_editor: point at %d %d %d", ) ; geom_msgs_num++ ;


    // TARGET FINDER PHASE 2: in-world content. 
    /*  When this block begins, we know that 'oct' is a non-null node which contains 
        either the camera or the ray front where it hits the world from the outside. 

        In other words, it is the place where the ray 'begins' inside the world. 

        So we need to check from this point forward whether the ray hits geometry or 
        entities. 

        Parameters: */

    front = pos ;
    vec ray = camdir ;
    front.add(ray.mul(min_t)) ; 
    dir = camdir ;
    ray = dir ;              // reset ray to length 1
    
    vec rayfront = front ;
    loopj(3) 
    {
        if (rayfront[j]<0) { rayfront[j]=0;}
    }

    // ray_start_node = oct ;
    ray_start_vec = icorner ;
    ray_start_orientation = orientation ;


    // pos is float vector of where we are
    float fx = rayfront.x, fy = rayfront.y, fz = rayfront.z ;
    float rx = ray.x, ry = ray.y, rz = ray.z ;
    float dx = 0.f, dy = 0.f, dz = 0.f ;
    float r = 0.f ;
    float d = 0.f ;
    t = 0.f ; // divisions are minimized 
    int i = 0 ;


    // If we're flat against a world boundary, then in the case that a ray component 
    // is negative we need to make the corresponding corner coordinate 1 unit smaller 
    // so that the findNode function will resolve something that is inside the world. 
    int limit = 0 ;
    int NS = 0 ; // target node size
    bool havetarget = false ;

    sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
    sprintf( geom_msgs2[geom_msgs_num2], "numchildren = %d", numchildren) ; geom_msgs_num2++ ;
    sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
    sprintf( geom_msgs2[geom_msgs_num2], "position : %.2f %.2f %.2f", pos.x, pos.y, pos.z) ; geom_msgs_num2++ ;
    sprintf( geom_msgs2[geom_msgs_num2], "ray = %.4f %.4f %.4f", rx, ry, rz ) ; geom_msgs_num2++ ;


    if (have_ray_start_node)
    {
        int Nscale = 0; 

        // set icorner 'equal' to the ray front

        // icorner[0] = (int)rayfront.x ; icorner[1] = (int)rayfront.y ; icorner[2] = (int)rayfront.z ;
       // loopj(3) { 
            // if (!(i==j))   { icorner[j] = (icorner[j] >> Nscale) << Nscale ; }
            // else        
        //    { icorner[j] = floor(rayfront[j]+.5f) ;} 
        //}

       // if (rx<0 && icorner.x>=world.size) {icorner.x = world.size ;} 
       // if (ry<0 && icorner.y>=world.size) {icorner.y = world.size ;} 
       // if (rz<0 && icorner.z>=world.size) {icorner.z = world.size ;} 
       // if (rx>=0 && icorner.x<0) {icorner.x = 0 ;} 
       // if (ry>=0 && icorner.y<0) {icorner.y = 0 ;} 
       // if (rz>=0 && icorner.z<0) {icorner.z = 0 ;} 

        //Octant* oct = findNode(icorner, &Nscale, &NS) ;
        // if we're exceeded the limits of the world, then we're 

        // loopj(3) { if ((orientation>>1)<=j) { i = j>>1 ; break ; } }
        // loopj(3) { if ((orientation>>1)<=j) { i = j>>1 ; break ; } }

        i = (orientation>>1) ;

//        loopj(3) { icorner[j] = (int)rayfront[j] ; }
        // In our direction of penetration, we want to make sure that our probe is guaranteed 
        // inside the node we should be finding. 
        if (ray[i]<0 && icorner[i]>=world.size) {icorner[i] -= 1 ;} else {icorner[i]+=1;}


/*
        loopj(3)
        { 
            if 
            icorner[j] = (icorner[j] >> Nscale) << Nscale ; 
        }
*/

        sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "********** RAY START **********") ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "  penetration plane: %s   (orientation=%d)", plane_names[i], orientation) ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "  rayfront: %.2f %2.f %.2f", rayfront.x, rayfront.y, rayfront.z); geom_msgs_num2++ ;


    int WS = world.size ;

    while (!havetarget)
    {


        limit++ ; if (limit>5) { break ; } // Kill runaway loops
       
        loopj(3) {icorner[j] = (int)rayfront[j] ;}
        sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "new ray front at :            %.2f    %.2f    %.2f    (crossing through %s plane) d = %.2f (size = %d)", 
            rayfront.x, rayfront.y, rayfront.z, plane_names[i], d, NS) ; geom_msgs_num2++ ;
        icorner[i] += (ray[i]>0?1:-1) ;     // Snap to inside of node we're looking at


        if ( (icorner[0]<=0 && rx<0)      ||
             (ray.x>=0 && icorner[0]>=WS) ||
             (icorner[1]<=0 && ry<0)      ||
             (ray.y>=0 && icorner[1]>=WS) ||
             (icorner[2]<=0 && rz<0)      ||  
             (ray.z>=0 && icorner[2]>=WS)
           )
        { 
            sprintf( geom_msgs2[geom_msgs_num2], "Ray exiting world bounds at %.2f %.2f %.2f ", 
                rayfront[0], rayfront[1], rayfront[2] ) ; geom_msgs_num2++ ;
            break ; 
        }



        Octant* oct = findNode(icorner, &Nscale, &NS) ; // Find out what tree node encloses this point


        loopj(3) { icorner[j] = (icorner[j] >> Nscale) << Nscale ; } // Now icorner is right on the node corner. 
        sprintf( geom_msgs2[geom_msgs_num2], "icorner at        %d %d %d    (i=%d) (i>>1=%d)", icorner.x, icorner.y, icorner.z, i, i>>1 ) ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "CURRENT NODE SIZE: %d (scale %d)", NS, Nscale) ; geom_msgs_num2++ ;
        
// if ( oct->has_geometry() || oct->has_children() )
        if ( oct->has_geometry() ) {
            sprintf( geom_msgs2[geom_msgs_num2], "HAVE TARGET NODE") ; geom_msgs_num2++ ;
            havetarget = true ; break ; }


        vec f = rayfront ;
        vec ds = vec(0) ;
        // fx = rayfront.x ; fy = rayfront.y ; fz = rayfront.z ;
        // rx = ray.x ; ry = ray.y ; rz = ray.z ;
        r = 0.f ; d = 0.f ; t = 0.f ; 

/*
*/

        // Distances 
        loopj(3) { ds[j] = (ray[j]>=0?(float(icorner[j]+NS)-f[j]):(f[j]-float(icorner[j]))) ; }

        if (fabs(ray.x*ds.y) > fabs(ray.y*ds.x))              // is rx/dx bigger than ry/dy? 
        { r = ray.x ; d = ds.x ; i = 0 ; }
        else
        { r = ray.y ; d = ds.y ; i = 1 ; }
        if ((fabs(ray.z*d) > fabs(r*ds.z)))              // is the last best smaller not bigger 
        { r = ray.z ; d = ds.z ; i = 2 ; }
        
        t = fabs(d/r) ;                       // divisions are minimized 

        sprintf( geom_msgs2[geom_msgs_num2], "distances = %.2f %.2f %.2f    rx=%.4f ry=%.4f rz=%.4f  r=%.4f d=%.4f t=%.4f",
            fabs(ds.x), fabs(ds.y), fabs(ds.z), ray.x, ray.y, ray.z, r, d, t  ) ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "r = %.4f, d = %.4f, t = %.4f", r, d, t) ; geom_msgs_num2++ ;

        rayfront.add(ray.mul(t)) ;  // move ray to new plane
        ray = dir ;                 // reset ray to length 1

    }
    } // end if (have_ray_start_node)






    // Draw a color-coded cross centered on the rayfront


    // glBegin(GL_LINES) ;

    vec fc ;
    fc.x = (float)icorner.x ;
    fc.y = (float)icorner.y ;
    fc.z = (float)icorner.z ;
    // draw_corner_cube( fc, NS) ;
    // glEnd() ;

    sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;

/*
    if (have_ray_start_node)
    {
        sprintf( geom_msgs2[geom_msgs_num2], "node search steps: %d", limit
            ) ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "distances from front to walls are: %.2f %.2f %.2f",
            dx, dy, dz
            ) ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "ray front at: : %7.2f %7.2f %7.2f", 
            rayfront.x, rayfront.y, rayfront.z
            ) ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "new corner located at: : %d %d %d", 
            icorner.x, icorner.y, icorner.z 
            ) ; geom_msgs_num2++ ;
        
        sprintf( geom_msgs2[geom_msgs_num2], "r = %.2f  d = %.2f  t = %.2f i = %d  d/r=%.2f", r, d, t, i, fabs(d/r)
            ) ; geom_msgs_num2++ ;
        
    // sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
    // sprintf( geom_msgs2[geom_msgs_num2], "current target size: %d ", NS ) ; geom_msgs_num2++ ;
        if (havetarget)
        {
            sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
            sprintf( geom_msgs2[geom_msgs_num2], 
                "                              Ray hitting a node of size %d", NS
                ) ; geom_msgs_num2++ ;
        }
    }
        */
/*
*/

    // TARGET FINDER PHASE 3: far world planes. 
    // The variable hitplanes encodes which three axis-aligned planes are 
    // always possible hits for a ray going through an octree. 
    //
    // Later, hitplanes can be used to quickly express plane indexes into the 
    // world_normals array. 
    
    // This is the calculation to determine which plane, 
    // positive or negative (see world_planes array), is 
    // to be looked up. This approach is taken because 
    // we chose to record the set of potentially hit planes
    // using a single int, 'hitplanes'. 
    // are we aiming at the world? 

    // If we hit nothing in the world, we find out which part of the grid 
    // frontier we're hitting. 

    // Are we hitting a far plane?

    /*
    FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
    FIXME: use only integer vectors for corner. 
    FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
    */
    hitplanes = 0 ;
    loopi(3)
    {
        hitplanes |= ( camdir[i]<0 ? 2*i+1 : 2*i ) << (3*i) ;
    }
    min_t = FLT_MAX ;
    t = 0 ;
    
    if (!hitworld)
    {
        loopi(3)
        {
            RayHitPlane( pos, camdir, wp[ hittingplane ], &t ) ;

            if (t<min_t && t>0)
            {
                front = pos ;
                front.add(camdir.mul(t)) ; // this works because camdir has length 1. 
                // Here we check that the point // hitting the plane is inside // the world limits. 
                if ( front[b[i][0]] < wp[2*i].offset && front[b[i][0]] > 0  &&    
                     front[b[i][1]] < wp[2*i].offset && front[b[i][1]] > 0
                   )
                {
                    orientation = (hittingplane) ;
                    min_t = t ;
                    corner = vec(front) ;
                    sel_path[0] = -1 ;
                }
            }
        } // end looping over all hitplane candidates 
        loopj(3)
        {
            // Truncate coordinates: keep them grid-aligned. 

            // We only truncate the variables which aren't the major 
            // axis for the current orienatation plane. If we did, it might 
            // give us a smaller value than we need. 
            if (!( (orientation==2*j) || (orientation==2*j+1) ))
            {
                corner[j] = (((int)(corner[j])) >> gridscale ) << gridscale ;
            }
            else
            {   
                // This nastiness is so that we are always flat against grid boundaries
                corner[j] = floor( corner[j] + .5f ) ;
            }
        }
    }

/*
    Octant* oct = findNode(icorner) ;
    sprintf(
        geom_msgs[geom_msgs_num], 
        "current selection: (%.0f %.0f %.0f) -> (children = %c, geom = %c, extent = %c)", 
        corner.x, 
        corner.y, 
        corner.z, 
        (oct->children) ? 'Y' :'N',
        (oct->geom) ? 'Y':'N',
        (oct->has_geometry()) ? 'Y':'N'
    ) ; geom_msgs_num++ ;
*/

} // end update_editor 



/*
    Specialized struct to hold vectors in the vectormap. 

    Adds the vector itself, so we can compare incoming entries (collisions 
    are possible). 

    Adds a bool, to tell us when elements in the map are 'real', 
    and when they are just unused slots. 

*/
struct hashvector
{
    ivec v ;
    bool set ; // true when a value is there
    int childcount ;
    // uchar rank ;  // what was rank for again? 
    // Aligned to 12+2 bytes. Probably rounded to 16 bytes. 
    // Meaning free to use 2 more bytes with little cost. 
} ;

// should really be called vectormap1024 but that's ugly. 
/*
    This is a very simple structure that serves a single purpose, hopefully well: 

        To give a quick look-up mechanism for vectors that we are gathering to 
        build a vertex array from. This will always be a small set of vectors: 
            - We never try to make a vertex array out of more than 255 vectors, 
              so we can address them with unsigned bytes. 
            - When we iterate through the geometry, if more than 255 vectors are 
              potentially needed, then we create vertex arrays for the 8 children 
              separately. 

        This structure is empty at first, and elements are added until no more 
        vectors are needed for the current vertex array. 

        Then, the vectormap is iterated through and all vertices are dumped 
        into the vertex array. 

        Then, the vectormap is completely emptied, and ready to use for the 
        next contruction operation. 

*/
struct vectormap
{   
    hashvector hashvectors[1024] ; // 1024 * 4 = 4096 bytes
    uchar count ;

    /* 
        The stupidest most simple hash function ever! Not literally stupidest, but 
        possibly stupidest to ever use seriously! Anyway, maybe it works...

        Which is acceptable because we use an array that is grossly bigger than 
        any set of vectors we need to use to construct our vertex arrays and VBOs. 
    */
    int HashVec( ivec& v )
    {
        int k = (((v.x/29)<<2 + (v.y/23)<<1 + v.z%37))%1024 ;
        //DEBUGTRACE(("key hash = %d", k)) ;
        return k ;
    }


    /*
        HashFindVector: return -1 if not found, and a location 
        index otherwise

    */
    int HashFindVec(ivec& v)
    {
        int hash = HashVec(v) ; 
        while ( hashvectors[hash].set && hashvectors[hash].v!=pos && hash<1024 ) 
        {
            hash++ ;
        }
        if ( !hashvectors[hash].set && hash < 1024 )
        {
            DEBUGTRACE(("key hash = %d", hash)) ;
            return hash ;
        }
        else
        {
            return -1 ;
        }
    }

    /*
        AddHashVector: 
            check if element is already in there
            if so, do nothing
            if not, add it
            FIXME 
            FIXME 
            FIXME 
            FIXME 
            FIXME 
            LOL FIXME 
    */
    bool AddHashVector( ivec& v )
    {
        int hash = HashFindVec(v) ;
        if (hash > -1)
        {
            while (hashvectors[hash].set)
            {
                hash++ ;
                if (hash==1024)
                {
                    hash = 0 ;
                }
            }
            if (hash < 1024)
            {
                hashvectors[hash].v = v ;
                hashvectors[hash].set = true ;
                return true ;
            }
        }
        else
        {
            printf("\nERROR: unable to find a valid hash location for a vertex !") ; 
        }
        return false ;
    }

    vectormap()
    {
        for (int i=0;i<1024;i++)
        {
            hashvectors[i].v = ivec(0,0,0) ;
            hashvectors[i].set = false ;
            //hashvectors[i].rank = 0 ;
        }
        count = 0 ;
    }

    void reset()
    {
        for (int i=0;i<1024;i++)
        {
            hashvectors[i].v = ivec(0,0,0) ;
            hashvectors[i].set = false ;
            hashvectors[i].childcount = 0 ;
            //hashvectors[i].rank = 0 ;
        }
        count = 0 ;
    }
} ;

/*
    va_triangle is used in combination with the hashvectors array to 
    create vertex arrays. 

    We're dealing with VA's constructed from at most 256 distinct 
    vertices. 
*/
struct va_triangle
{
    uchar v1 ;
    uchar v2 ;
    uchar v3 ;
} ;

struct idx_triangle
{
} ;


ivec vbo_array[1024] = {ivec(0)} ;
uchar vbo_elements[1024] = {0} ;

/*
    Function: delete_subtree. 


    Purpose: to remove all children a node may have, and their 
    properties. 

    Typical usage: when content needs to be removed from a portion 
    of the tree, it can be removed by using the nodes whose combined 
    volume exactly encompasses this content. 

*/
void delete_subtree(Octant* in_oct)
{
    int32_t d = 0 ;             // depth
    Octant* CN = in_oct ;  // current node 
    Octant* CC = NULL ;         // current child 
    Octant* path[20] = {NULL} ;
    path[0] = CN ; // This is and always will be the root. 
    int32_t idxs[20] ;   // idxs[d] tracks which child of node path[d] is being used, if any. 
    loopi(20) { idxs[i] = 0 ; }

    d = 0 ;     // We start at the root. 

    ivec pos( 0, 0, 0 );

    while (d>=0)
    {
        if ( CN->children )
        {
            if (idxs[d]<8)
            {
                CN = &CN->children[idxs[d]] ; 
                d++ ;
                path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 

                continue ;
            }
        }
        
        path[d] = NULL ;
        d-- ;
        // Every time we reach this part of the loop, it means we are 
        // finished looking at CN's children. We can delete CN's children 
        // now. 
        if (d>=0) // Of course, don't bother if we've backed up 'past' the start of our node path. 
        {
            CN = path[d] ;
            // Delete children. 
            if (CN)
            if (CN->children)
            {
                delete [] CN->children ;
                CN->children = NULL ;
                //printf("\ndeleting 8 children. \n") ; 
            }
            idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
        }
    }   // end while d>=0
}


// Finds the smallest node enclosing the supplied integer vector. 
Octant* findNode(ivec at, int* Nscale, int* out_size) 
{
    int WGSc = world.gridscale ;
    int CGS = world.scale ;
    int i = 0 ;
    Octant* oct = &world.root ;

    *Nscale = CGS ;
    while (CGS>2)
    {
        if ( oct->children ) 
        {
            i = octastep(at.x,at.y,at.z,CGS) ;
            oct = &oct->children[i] ;
            *Nscale = CGS ;
        }
        else
        {
            break ;
            /*
            if (oct!=NULL)
            {
                if ( oct->has_geometry() ) 
                {
                    break ;
                }
            }
            else
            {
                break ;
            }
            */
        }
        CGS-- ;
    }

    if (out_size!=NULL)
    {
        *out_size = 2<<CGS ;
    }

    return oct ;
}

static Geom* NEED_UPDATE = NULL ; // dummy used to signal that a node needs its geom rebuilt. 
/*
    Function: extrude. 

    Description: this function calculates and then creates a set of new cubes 
    based on the location, dimensions and orientation of the current selection.

    Orientations are defined in the world_planes array: XYZ, each from +ve to 
    -ve, gives orientations 0-5. Each orientation refers to a direction vector 
    which is also the plane normal to one of the major axis-aligned planes. 
    This normal vector is used as the direction in which extrusion occurs. 

    Selections are defined by four parameters: 
   
        gridsize        -> the size of cubes to select
        sel_start       -> the cube with min XYZ that bounds the selection
        sel_end         -> the cube with max XYZ that bounds the selection
        orientation     -> the side of the selection in which extrusion will 
                           occur. 

        These variables are all globals, so the only needed parameter is the 
        in/out boolean, where 'in' means we're not really intruding but 
        collapsing and deleting geometry. 


    Parameters: 
        
        out: Pointer to boolean. If true, it means extruding outwards. If 
             false, then 'intruding'; extruding inwards, which deletes 
             geometry. 



        Branches: 

            1) extruding from a particular selection 

            2) extruding from world boundary

            (these two situations create slightly different calculations 
            for the creation of new geometry.)
*/
//            printf("\n orientation = %d     and Zi = %d and Dz = %f", orien, Z(orien), Dz(orien)) ;
//            printf("\n New cube: %d  %d  %d", newcube.x, newcube.y, newcube.z ) ;
//                    sprintf(geom_msgs[geom_msgs_num], "world.gridscale=%d.", world.gridscale) ; geom_msgs_num++ ;
//                    sprintf(geom_msgs[geom_msgs_num], "world.gridsize=%d.", world.gridsize) ; geom_msgs_num++ ;


void extrude( void * _in )
{
    if (!havesel) return ;

    bool in = *(bool *)_in ;

    int O = (sel_o) ;               // orientation
    int WS = world.scale ;
    int GS = world.gridsize ;
    int WGSc = world.gridscale ;
    int CGS = WS ;                  // current grid scale
    int i = 0 ;                     // child index
    ivec pos(0) ;                   // used to compute vertices that go into our vbo_array
    // vectormap vmap ;                // used to record new nodes and then to construct and update vector arrays

    // define a range of cubes that will be created, bounded by 
    // sel_min and sel_max. These will bound the same rectangle 
    // as sel_start and sel_end, but with easier to compute properties. 
    loopi(3)
    {
        sel_min[i] = min( sel_end[i], sel_start[i] ) ;
        sel_max[i] = max( sel_end[i], sel_start[i] ) ;
    }


    // --------------------------------------------
    // PHASE 1: Create tree nodes 
    // --------------------------------------------
    DEBUGTRACE(("\n ********** EXTRUDE - PHASE 1 (node creation) ********** \n")) ;

    // Where the extrusion originates is affected by what the 
    // selection cursor is currently pointed at. 
    // Are we pointed at something within the tree? 

    /* 
       step: 
        populate the geometry tree with the newly created cubes ( deleting
        stuff completely enclosed, and subdividing anything that is of bigger
        scale than our current extrusion size that was partially enclosed ). 

       populating the tree: 
            - For every cube of size==world.gridsize in our selection, create a 
              child.
    */
    // CASE: our cursor is on something within the octree. 
    if ( sel_path[0] >= 0 )
    {
    }
    // CASE: the cursor is pointing at a world boundary 
    else if ( sel_path[0] == -1 )
    {
        // extruding inwards (not extruding)
        if ( in )
        {
            havenewcube = false ;
        }
        else
        {
            // The first cube is *based* on the one at sel_min, 
            // though doesn't necessarily equal it. They will be 
            // equal if sel_min is at a world boundary with its
            // coordinate on the extrusion axis equal to zero. 
            // In other words, extruding from the planes X=0 or Y=0 or Z=0
            // results in a new cube whose corner is given by the 
            // world boundary. 
            // 
            // Every cube's position can be uniquely defined 
            // by its min corner.
            newcube = sel_min ; 
            //printf("\n\n\t\t\t\t\t    selection: %.2f   %.2f   %.2f  \n\n", NC.x, NC.y, NC.z ) ;
            newcubes_new = true ;

            // check if our orientation is even. This allows 'popping out' 
            // geometry from coordinates where to do so means to decrement our coords. 
            if ( !(O%2) )
            {
                NC[Z(O)] += wp[O][Z(O)] * (sel_size) * sel_counts[Z(O)] ;
            }
            // If the new cube coordinates do not exceed world 
            // limits, then we have a new cube. 
            havenewcube = true ;

            Octant* oct = &world.root ;
            int depth = 0 ; // first child of root


            int x_count = 0 ;
            int y_count = 0 ;

            x_count = 0 ;
            y_count = 0 ;
            
            ivec NCstart = NC ;
            // Do this for as many 'rows' and 'columns' as the selection is wide. 
            // while ( y_count < sel_counts.y )

            while ( y_count < sel_counts[Y(O)] )
            {
                //while ( x_count < sel_counts.x )
                while ( x_count < sel_counts[X(O)] )
                {
                    while (CGS>=WGSc)
                    {
                        if ( !oct->children ) 
                        { 
                            oct->children = new Octant[8] ; 
                            if (!oct->children)
                            {
                                printf("ERROR -- ASSIGNING CHILDREN TO A NODE! ") ;
                            }
                        }
                        i = octastep(NC.x,NC.y,NC.z,CGS) ;
                        sel_path[depth] = i ;
                        oct = &oct->children[i] ;

                        depth++ ;
                        CGS-- ;
                    }

                    if (oct)
                    {
                        if (oct->children)
                        {
                            delete_subtree( oct ) ;
                        }
                        oct->set_all_edges() ;
                        numchildren += 1 ;
                    }
                    // Reset variables for any subsequent new nodes. 
                    CGS = WS ; 
                    depth = 0 ;
                    oct = &world.root ;

                    // Move to next position for a new node 
                    NC[X(O)] += GS ;
                    x_count ++ ;
                } // end while ( x_count < sel_counts.x )
                x_count = 0 ;
                y_count ++ ;
                NC[X(O)] = NCstart[X(O)] ;
                NC[Y(O)] += GS ;
            }
            // Reset NC to initial 'first node' of the group. 
            NC = NCstart ;
        }
    }
    // CASE: the cursor is not set on anything; this can happpen if we are outside 
    // world boundaries and pointing the cursor at any part of the world. 
    else if ( sel_path[0] == -2 )
    {
        DEBUGTRACE(("\nCannot extrude when no selection is in view. \n")) ; 
    }
    ivec counts(1) ; 
    // min one cube 

    // --------------------------------------------
    // PHASE 2: Produce leaf vertex counts (lvc) so 
    // that in the final phase vertex array sets can be 
    // assigned to the right nodes. 
    // --------------------------------------------

    // Iteration is over all nodes which have their geom==NEED_UPDATE. 
    // zzz
    // local variables. 
    int32_t d = 0 ;             // depth
    Octant* CN = &world.root ;  // current node 
    Octant* CC = NULL ;         // current child 
    int32_t ccidx = 0 ;         // current child index

    // path
    Octant* path[20] = {NULL} ;
    int32_t idxs[20] ;   // Used to track path indexes. -1 means unused. 
    loopi(20) { idxs[i] = -1 ; }
    path[0] = CN ;
    idxs[0] = 0 ;
    d = 0 ;

    printf("\n root node is at %d\n", (int)CN) ;

    DEBUGTRACE(("\n ********** EXTRUDE - PHASE 2 (lvc processing) ********** \n")) ;

    // FIXME:  instead of changing lvc.c when something HAS geometry (which alters its geometry!!!), 
    // we need to instead check: 

    while (d>=0)
    {
        if ( CN->children )
        {
            if (idxs[d]<8) // if we have nodes left to cover at this level ...
            {
                path[d] = CN ;                // save the current node so we can go back up to it when done with this child
                CN = &CN->children[idxs[d]] ; // now set current node to this child
                idxs[d]++ ;                   // Next child access at this level goes up by one.
                d++ ;                         // Since we're down a child, depth is ++'d. 
                idxs[d] = 0 ;                 // New level means cycling through new group of children. Starts at 0. 
                continue ;
            }
            else
            {
                CC = &CN->children[0] ;
                int count = 0 ;
                loopi(8) 
                { 
                    if (CC->has_geometry()) // Should only happen if CC is a child ... with geometry! 
                    {
                        // ... calculate how many visible triangles will result from this node's geometry 
                        // and position. 
                        count += 8 ; // the 8 is a sorry fake until we have a real count. 
                    }
                    else
                    {
                        // ... or just collect its lvc which should be available if we got to here. 
                        count += CC->lvc.c ;
                    }
                    CC++ ; // next child 
                }
                CN->lvc.c = count ; // record total 
                //DEBUGTRACE(("\n At depth %d, adding child lvc's. Total = %d \n", d, CN->lvc.c)) ;
            }
        } // end if ( CN->children )
        // 'GO UP'
        path[d] = NULL ;
        idxs[d] = -1 ;
        d-- ;           // We're going up to our parent's depth.
        CN = path[d] ;  // Current node is now the last node we went down from. 
    }

    // step: determine which faces of our new cubes are visible 

    // --------------------------------------------
    // PHASE 3: Produce vertex array sets
    // --------------------------------------------
    DEBUGTRACE(("\n ********** EXTRUDE - PHASE 3 (vertex array set creation) ********* \n")) ;
    /*
        Procedure: 

        Iterate through tree. 
            - follow all nodes whose geom == NEED_UPDATE
            - nodes that have lvc>256 tell their children to form VBOs from their 
              child trees. 
    
    */

} // end extrude 


/*

zzz

*/
void draw_new_octs()
{
    int32_t d = 0 ;             // depth
    Octant* CN = &world.root ;  // current node 
    Octant* CC = NULL ;         // current child 

    int32_t SI = world.scale ;  // Size increment scale. Used to compute offsets from node corners. 
    int32_t incr = 0 ;
    int32_t yesorno = 0 ;

    Octant* path[20] = {NULL} ;
    int32_t idxs[20] ;   // idxs[d] tracks which child of node path[d] is being used, if any. 

    loopi(20) { idxs[i] = 0 ; }
    path[0] = CN ; // This is and always will be the root. 
    //idxs[0] = -1 ;
    d = 0 ;     // We start at the root. 

    ivec pos( 0, 0, 0 );

    //geom_msgs_num = 0 ;
    int pointcount = 0 ;

    glPointSize(4.0f) ;
    glColor3f(1.0f, 0.0f, 0.0f) ;

    glBegin( GL_POINTS ) ;
    while (d>=0)
    {
        if ( CN->children )
        {
            if (idxs[d]<8)
            {
                // We're going down to a child, marked relative to its parent by idxs[d]. 
                loopi(3) {
                    incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ;
                    pos.v[i] += yesorno * incr ;
                }

                CN = &CN->children[idxs[d]] ; 
                d++ ;
                path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 

                continue ;
            }
        }
        // If we don't have children, then maybe we have geometry.
        else
        {
            if ( CN->has_geometry() )
            {
                loopi(3) {
                    incr = (1<<(SI-d)) ;
                    pos.v[i] += (incr);
                }

                glVertex3iv(pos.v) ;
                pointcount++ ;

                loopi(3) {
                    incr = (1<<(SI-d)) ;
                    pos.v[i] -= (incr);
                }
            }
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        CN = path[d] ;

        loopi(3) {
            incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ;
            pos.v[i] -= yesorno * incr ;
        }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    }                   // end while d>=0
    glEnd() ;
    // sprintf(geom_msgs[geom_msgs_num], "pointcount=%d.", pointcount ) ; geom_msgs_num++ ;
}

void World::initialize()
{
    world.gridscale = 10 ;
    world.gridsize = 1<<10 ;

    printf("\n\n****************************WORLD SIZE = %d************************************\n\n", world.size ) ;

    // world.root = 0 ;
}






//__m128 a ;
//int b ;
//float af[4] = { 101.25, 200.75,300.5, 400.5 };
//a = _mm_loadu_ps(af);
//b = _mm_cvtt_ss2si(a);





