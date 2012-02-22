/*
    File: geometry.cpp
    
    (C) Christian Léger 2011-2012 

    Contains the elements of world geometry, the operations to manipulate 
    world geometry, and the operations to collect information about this 
    structure. 

    See geometry.h for the contents of the various geometrical structures. 

    Also see geometry.h for all the details we were able to document about this 
    here module. 

----------------------------------
    Overview of general operations
----------------------------------

----------------------
    Deleting geometry: 
----------------------

        Any rectangular polyhedron (shape that occupies R^3) whose boundaries 
        lie on power-of-two coordinates can be selected. 

        Of course, we should not select large areas with a tiny gridsize; this 
        will lead to some problems, the least of which would be slowdowns. 

        If a volume can be selected, it can be created or deleted. 

        When geometry gets deleted, it means three things must be done: 

            - The world tree has to eliminate some subtrees. During this 
              elimination process, any Geoms found must be deleted. This 
              cleans up VBOs. 

            - any Geom (see geometry.h) structures that are used by those 
              subtrees must be eliminated
            - any geometry left over which was previously drawn using the 
              now-deleted Geoms must now be reconstructed. 

    When Geoms are deleted, the nodes that point to them must be flagged as
    needing update. This is done by setting the nodes' Geom pointer equal to
    the dummy pointer called GEOM_NEED_UPDATE. 

    Once tree nodes have been created and destroyed as needed, then we 
    traverse the tree looking for nodes that need their Geoms updated. 
    This doesn't mean that a node which formerly held a Geom must now have 
    one. It simply means, for any such node, that it or its subtree must 
    be reanalyzed and new Geoms created where needed. A node needing update 
    tells all its ancestors they also require an update. This is what will 
    help us find the nodes needing Geom-updates. Even if the root will thus 
    get flagged as needing update, only the children whose subtrees really 
    have altered geometry will also be flagged as needing update. 

    All the information to recreate the world's Geoms is always implicit 
    withing the leaf nodes and their ancestry. Leaf nodes tell us the 
    presence and shape of geometry. They also tell us which textures are 
    applied to which of their faces. The ancestry of a node, on the other 
    hand, allows us to compute the coordinates of a leaf node's corner, as 
    well as its size. 


----------------------
    Creating geometry: 
----------------------


*/


#include "recalc.h"

extern Camera camera ;

//------------------------------------------------------------------------
//                  GLOBAL VARIABLES
//------------------------------------------------------------------------

// player/user control and status
vec  front ;
//vec  pos ;
vec  camdir ;

// main renderable geometry data
vector<Geom*> worldgeoms ;

vector<Geom*> worldgeometry ;

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
//Octant* findNode(ivec at, int* nscale=NULL, int* out_size=NULL) ;
Octant* findNode(ivec at, int* out_size=NULL, int* nscale=NULL) ; //, int* out_size=NULL) ;

int numchildren = 0 ;

// The main high-level functions of the geometry module
void update_editor() ;
void extrude( void * _in ) ;
void flagPathForUpdate( Octant** path, int depth) ;
void AnalyzeGeometry(Octant* tree, ivec corner, int scale) ;
void AssignNewVBOs(Octant* tree, ivec corner, int scale) ;
void makeSubtreeVBO(Octant* parent, ivec corner, int NS) ;
// This is equivalent to 2^15 units, or 32,768

#define WORLD_SCALE 15


////////////////////////////////////////////////////////////////////////////////
//      The World and its Dimensions
////////////////////////////////////////////////////////////////////////////////
// here 'world scale' really means the largest piece out of the world, which 
// is one of the 8 cubes that divide the world

/*
    About world dimensions: we treat 64 units (as seen when rendering graphics) 
    to roughly mean a meter. This means something of size 2^6 is a meter. This 
    also means that a typical 'place', or map, is 1024 'meters' on a side. A lot 
    can be done and created in that space. 
*/
/* 
    SOME OF THESE VALUES ARE OVERRIDDEN DURING INITIALIZATION ROUTINES
*/
World world = 
{ 
    WORLD_SCALE,        // scale: default 15   
    2<<WORLD_SCALE ,    // size: 2<<15=65536    (total size equals 2^16)
    WORLD_SCALE,        // gridscale (scale used to size things while editing)
    2<<(WORLD_SCALE-1)  // gridsize (size of new nodes when editing) max gridsize: 2^15
} ;
#define w world


////////////////////////////////////////////////////////////////////////////////
//      Octant member and auxiliary functions
////////////////////////////////////////////////////////////////////////////////
bool aiming_at_world ;

bool hit_world = false ;

int orientation = 0 ;

// create a node with nothing in it
Octant::Octant()
{
    children = NULL ;
    geom = NULL ;
    set_all_edges(0) ;
    vc = 0 ;
    flags = EMPTY ;
}

bool Octant::has_geometry() 
{
    return (
        (edge_check[0]!=0) &&
        (edge_check[1]!=0) &&
        (edge_check[2]!=0)
        ) ;

/*
    return (
        (edge_check[0]==MAX_INT) &&
        (edge_check[1]==MAX_INT) &&
        (edge_check[2]==MAX_INT)
        ) ;
*/
}

bool Octant::has_children()
{
    return (children != NULL) ;
}

/*
    No arguments means make this solid. 
    Having an argument means set edges to that value. 
*/
void Octant::set_all_edges(int in_value=255)
{
    //DEBUGTRACE(("\n setting all edges with d=%d\n", in_value)) ;

/*
    loopi(12)
    {
        edges[i] = in_value ;
    }
    */
    loopi(12)
    {
        edges[i] = in_value ;
    }
    flags = SOLID ;
}

void new_octants( Octant* oct )
{
    oct->children = new Octant[8] ;
    return ;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------
//                  EDITING VARIABLES AND FUNCTIONS
//------------------------------------------------------------------------

#define o orientation 
// lol misspelled and real one defined in the .h 
//#define octstep(x,y,z,scale) ((z>>(scale))&1)<<2 | ((y>>(scale))&1)<<1 | ((x>>(scale))&1)

// corner determines the selection: it is the minimum in X,Y,Z of a box 
// of size gridsize*(xCount,yCount,zCount). 
ivec ifront ;   // used to remember the location where a ray hits a node
ivec icorner ;
vec corner( 0, 0, 0 ) ;

bool havetarget = false ;
bool havesel = false ;
bool havesel_end = false ;
bool havenewcube = false ;
bool newcubes_new = false ;

ivec newcube ;
#define NC newcube 

int sel_size = 0 ;
int sel_o = 0 ;
ivec sel_start ;
ivec sel_end ;
ivec sel_counts ;
ivec sel_min ;
ivec sel_max ;
Octant* ray_start_node ;
bool have_ray_start_node = false ;
ivec ray_start_vec ;
int ray_start_orientation = 0 ;




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
    {-1, 1, -1},    // +ve Y is 'up', -ve X is 'right'
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


void clear_selection()
{
    havesel = false ;
    havesel_end = false ;
}


void set_sel_start()
{
    int NS = world.gridsize ;
    havesel = true ;
    havesel_end = false ;
    havenewcube = false ; 
    if (havetarget)
    {
        sel_start   = ifront ;
        sel_end     = ifront ;
        sel_start[orientation>>1] += NS*(orientation%2) ;
        sel_end[orientation>>1] += NS*(orientation%2) ;
    }
    else
    {
        sel_start   = corner ;
        sel_end     = corner ;
    }
    sel_counts = vec(1,1,1) ;
    sel_o = orientation ;
    sel_size = world.gridsize ;
    printf("\nselection starting at %d  %d  %d\n", sel_start.x, sel_start.y, sel_start.z ) ; 

/*
    sprintf( geom_msgs[geom_msgs_num], "NEW SELECTION at %d %d %d", sel_end.x, sel_end.y, sel_end.z ) ; geom_msgs_num++ ;
*/
}


// sprintf(geom_msgs[geom_msgs_num], "selection counts: %d %d ", sel_counts.x, sel_counts.y) ; geom_msgs_num++ ;
void set_sel_end()
{
    int NS = world.gridsize ;
    if (!havesel || orientation!=sel_o ) 
    {
        set_sel_start() ;
        return ;
    }
    havenewcube = false ; // reset potential set of new cubes
    havesel_end = true ;

    // Here we say if we're pointing at something, then the place we are targeting 
    // is obtained from where our ray was last pointing. Otherwise, we say our 
    // target area will be where the ray touched the edge of the world. 
    if (havetarget) { sel_end = ifront ; sel_end[orientation>>1] += NS*(orientation%2) ; }
    else { sel_end = corner ; }
    sel_size = world.gridsize ;
    loopi(3)
    {
        sel_counts[i] = (( max(sel_start[i], sel_end[i]) - min(sel_start[i], sel_end[i]) ) >> w.gridscale ) + 1 ;
    }
}
// FIXME: this should all be done with ints. 
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
void draw_corner_squarei(
    ivec c,        // corner; lowest values for the coords not in the orientation vector (see function description for info)
    int size, 
    int _o
    )
{
    glBegin( GL_LINE_LOOP ) ;
        glVertex3iv( c.v ) ; c[X(_o)] += size ;  // bottom-right (could be bottom left but it's symmetrical)
        glVertex3iv( c.v ) ; c[Y(_o)] += size ;  // top-right
        glVertex3iv( c.v ) ; c[X(_o)] -= size ;  // top-left
        glVertex3iv( c.v ) ;
    glEnd() ;
}

void draw_corner_square(
    vec& _corner,        // lowest values for the coords not in the orientation vector (see function description for info)
    int size, 
    int _o
    )
{
    ivec c(_corner) ;
    glBegin( GL_LINE_LOOP ) ;
        glVertex3iv( c.v ) ; c[X(_o)] += size ;  // bottom-right
        glVertex3iv( c.v ) ; c[Y(_o)] += size ;  // top-right
        glVertex3iv( c.v ) ; c[X(_o)] -= size ;  // top-left
        glVertex3iv( c.v ) ;
    glEnd() ;
}

void draw_corner_cubei(
    ivec _corner,
    int size
    )
{
    // first two squares cornered at origin 
    draw_corner_squarei( _corner, size, 0) ; 
    draw_corner_squarei( _corner, size, 2) ; 
    // third square cornered at origin.x + size 
    corner[0] += size ; 
    draw_corner_squarei( _corner, size, 1) ;
    corner[0] -= size ; corner[1] += size ; 
    draw_corner_squarei( _corner, size, 3) ;
}

void draw_corner_cube(
    vec & _corner,
    int size
    )
{
    // first two squares cornered at origin 
    draw_corner_square( _corner, size, 0) ; 
    draw_corner_square( _corner, size, 2) ; 
    // third square cornered at origin.x + size 
    corner[0] += size ; 
    draw_corner_square( _corner, size, 1) ;
    corner[0] -= size ; corner[1] += size ; 
    draw_corner_square( _corner, size, 3) ;
}

/*
    This draws a square which frames a possible extrusion base. 

    What's an extrusion base? It's a place from which new geometry 
    can be pulled out. Any interior boundary of a world can be 
    used to do this, as well as any face on a node which is not 
    coplanar to a world boundary. 

*/
void draw_highlight()
{
    glColor3f( 1, 1, 0 ) ;
    int GS = world.gridscale ;
    int NS = world.gridsize ;

    ivec drawn_corner ;
    if (havetarget)
    {
        drawn_corner = ifront ;
        loopj(3) { drawn_corner[j] = (drawn_corner[j] >> GS) << GS ; }
        drawn_corner[orientation>>1] += NS*(orientation%2) ;
        if (havesel)
        {
            drawn_corner[orientation>>1] += Dz(orientation)*NS/40; /* draw_corner_squarei( drawn_corner, //2<<(world.scale-1), world.gridsize, orientation) ;*/
        }
        else
        {
            drawn_corner[orientation>>1] += Dz(orientation)*NS/100 ; /* draw_corner_squarei( drawn_corner, //2<<(world.scale-1), world.gridsize, orientation) ;*/
        }
    } 
    // from ray hitting world boundary 
    else { drawn_corner = corner ; }
    draw_corner_squarei( 
        drawn_corner, 
        world.gridsize, 
        orientation 
        ) ;
}


void draw_sel_start()
{
    if ( !havesel ) return ;
    glLineWidth( 5.0 ) ;
    glColor3f( 1, 0, 0 ) ;
    ivec v( sel_start ) ;
    v[sel_o>>1] += Dz(sel_o)*1 ;
    draw_corner_squarei( v, sel_size, sel_o) ;
    glLineWidth( 1.0 ) ;
}

void draw_sel_end()
{
    if ( !havesel_end ) return ;
    glLineWidth( 5.0 ) ;
    glColor3f( 0, 8, 1 ) ;
    ivec v( sel_end ) ;
    v[sel_o>>1] += Dz(sel_o)*1 ;
    draw_corner_squarei( v, sel_size, sel_o) ;
    glLineWidth( 1.0 ) ;
}

/*
    Green square shows which part of the grid our selection ray enters the 
    world, if we're positioned outside and looking in.
*/
void draw_ray_start_node()
{
    glColor3f(0.f,1.f,0.f) ;
    if (have_ray_start_node)
    {
        vec v(ray_start_vec.v) ;
        draw_corner_square( v, world.gridsize, ray_start_orientation) ;
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
}


/*
    Draws a selection highlight atop one of the faces of a node. 

*/
void draw_selection_face(
    ivec corner,
    int size,
    int orientation
    )
{
}


/*
    NS is node size. 

*/
void render_selection(ivec corner, int NS)
{
    draw_corner_cubei( corner, NS) ;
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
#define bds bounds


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

Possible speed optimization: replace double with float? 
Some cases might be able to use the reduced precision. 
*/
void RayHitPlane( vec& pos, vec ray, plane& pl, float* t )
{
    double numerator = 0, denominator = 0 ;

    numerator   = - pl.offset - ( pos.dot( pl.v) ) ;
    denominator =        camdir.dot( pl.v )  ;
    if ( denominator != 0 ) { *t = ( numerator ) / ( denominator ) ;  }
    // if denominator in plane equation is 0, that means vector parallel to plane. 
    else  { *t = 0 ; }
}


/*
    Intersect a ray with a plane, and provide the point where that intersection 
    happens. 


Possible speed optimization: replace double with float? 
Some cases might be able to use the reduced precision. 
*/
vec RayHitPlanePos( vec& pos, vec ray, plane& pl, float* t )
{
    double num = 0, den = 0 ; // numerator and denominator

    num = - pl.offset - ( pos.dot( pl.v) ) ;
    den = camdir.dot( pl.v )  ;
    if ( den != 0 ) { *t = ( num ) / ( den ) ;  }
    else  { *t = 0 ; }
    vec hitpos = pos.add(ray.mul(*t)) ;
    return hitpos ;
}

/*
    Output: 
        - t, the parameter that multiplies the advancing ray. if it's positive, 
          then something was hit. If it's negative, then nothing was hit. 
        - loc, the location where the the ray hit
        - normal - the plane normal vector of the surface we hit. Good to calculate 
          subsequent velocity vector adjustments when doing collision detection.

    This assumes the 'world' in question is the default world. 

    When multiple worlds exist, such as several large vehicles (spaceships 
    most likely) and perhaps also some planetary or asteroid surface, 
    then there is a default world and other worlds that can be used.  This 
    function assumes only the default world is available. 

*/
float RayHitWorld(vec pos, vec ray, float max_t, vec* loc, vec* normal)
{
    // RAY SHOOTING THROUGH THE WORLD! ZAP!
    int WS = world.size ;
    havetarget = false ;
    int steps = 0 ;

    int Nscale = 0; // tracks the scale of a node
    int NS = 0;     // tracks the size of a node 
    int i = 0 ; // Which axis dominates the ray's movement into the next node

vec ds = vec(0) ; // tracks distances from ray front to next node planes

vec dir = ray ;     // preserve this direction with length 1
vec f = pos ;       // ray front, f
// FIXME: why do I add ray to f at the start? This probably leads to the bugs I was having! 
f.add(ray) ;        // f is now at ray front
ivec ifr ;          // int vector to track which node a ray front is in. ifr == 'int front'
ivec ic ;           // int vector to track the corner of the node we're in. ic == 'int corner'

float r = 0.f ; float d = 0.f ; float t = 0.f ; 

    while (!havetarget)
    {
        if (steps>50) 
        {
            t=0.f ;  
            break ; 
        }                  // Kill runaway loops
        loopj(3) {ifr[j] = (int)f[j] ;} // place, more or less, ifr at rayfront (imprecision from converting float to int)
        // Snap to inside of node we're looking at. Unless we're just starting off - then we need to first move to a boundary 
        if (steps>0) 
        {
            ifr[i] += (ray[i]>=0?1:-1) ;    // snap 'int ray front' to inside of next node. 
            if ((ifr[i]>=WS&&ray[i]>0) || (ifr[i]<=0&&ray[i]<0)) // exit world
            {
                t=0.f ; break ;
            }  // If this adjustment brings us out of the world - we're done
        } ; 
        Octant* oct = findNode(ifr, &NS, &Nscale) ; // Find out what tree node encloses this point
        ic = ifr ;
        if ( oct->has_geometry() )                  // Target acquired. 
        { 
            *loc = f ;
            return t ; 
        }
        loopj(3) { ic[j] = (ic[j] >> Nscale) << Nscale ; }    // Now ic is right on the node corner. 
        // Distances to next plane intersections
        loopj(3) { ds[j] = (ray[j]>=0?(float(ic[j]+NS)-f[j]):(f[j]-float(ic[j]))) ; }

        // Which plane are we going to hit next, given our position and the ray orientation? 
        if (fabs(ray.x*ds.y) > fabs(ray.y*ds.x)) { r = ray.x ; d = ds.x ; i = 0 ; }
        else { r = ray.y ; d = ds.y ; i = 1 ; }
        if ((fabs(ray.z*d) > fabs(r*ds.z))) { r = ray.z ; d = ds.z ; i = 2 ; }
        t += fabs(d/r) ;        // divisions are minimized 
        if (t>max_t)            // not hitting anything - use initial multiplier
        {
            return max_t ;
        } // finished; we only wanted to go as far as max_t would take the ray from pos to pos+t*ray. 
        f = pos ;

        f.add(ray.mul(t)) ;  // move ray front to new plane
        ray = dir ;          // reset ray to length 1

        steps++ ;
    }
    return t ;
}

/*
    Inputs to this function: 
        - camera position (used for ray origin)
        - camera direction (used for ray direction)
        - world.gridscale - at what size scale are we currently editing

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
    vec pos = camera.pos ;

    int gridscale = world.gridscale ; // maximum size of a selection square is half the world size. Else we wouldn't know! 
    //int gridscale = world.gridscale ; // maximum size of a selection square is half the world size. Else we wouldn't know! 
    geom_msgs_num2 = 0 ; 

    // Disgusting bit tricks just to make some lines more concise. 
    #define hittingplane (hitplanes>>(3*i))&0x07

    vec fcorner ;

    // TARGET FINDER PHASE 1: near world planes (if we're outside the world). 
    loopi(3)
    {
        hitplanes |= ( camdir[i]<0 ? 2*i : 2*i+1 ) << (3*i) ;
    }
    min_t = FLT_MAX ;

    have_ray_start_node = false ;

    vec dir ;


    // Probably obsolete: 
    //if (front[b[i][0]] < wp[2*i].offset && front[b[i][0]] > 0  &&    //    front[b[i][1]] < wp[2*i].offset && front[b[i][1]] > 0
    if (camera.inworld(world))
    {
        front = pos ;
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

                    int f1 = front[bds[i][0]] ; 
                    int f2 = front[bds[i][1]] ; 
                    int w = wp[2*i].offset ;

                    if ( f1 > 0 && f1 < w && f2 > 0 && f2 < w )
                    {
                        orientation = (hittingplane) ; min_t = t ;
                        fcorner = vec(front) ; // sel_path[0] = -1 ;
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

    // sprintf(geom_msgs[geom_msgs_num], "update_editor: point at %d %d %d", icorner[0], icorner[1], icorner[2]) ; geom_msgs_num++ ;



    // TARGET FINDER PHASE 2: Compute the first node our ray starts tracking 

    //sprintf(geom_msgs[geom_msgs_num], "update_editor: point at %d %d %d", ) ; geom_msgs_num++ ;


    // TARGET FINDER PHASE 2: in-world content. 
    /*  When this block begins, we know that 'oct' is a non-null node which contains 
        either the camera or the ray front where it hits the world from the outside. 

        In other words, it is the place where the ray 'begins' inside the world. 

        So we need to check from this point forward whether the ray hits geometry or 
        entities. 

        Parameters: */
    
    
    // pos is float vector of where we are
    front = pos ;
    vec ray = camdir ;
    dir = camdir ;
    ray = dir ;              // reset ray to length 1
    vec rayfront ;

    if (have_ray_start_node ) // Redundant (should already be done) but hey life is made for fun. 
    {
        front.add(ray.mul(min_t)) ; 
        rayfront = front ;
    }
    else    // If we aren't aiming at the world exterior, it's because we're inside the world. 
            // Ray front therefore starts at our position. 
    {
        rayfront = pos ;
    }
    ray = dir ;              // reset ray to length 1

    
    loopj(3) 
    {
        if (rayfront[j]<0) { rayfront[j]=0;}
    }

    // ray_start_node = oct ;
    ray_start_vec = icorner ;
    ray_start_orientation = orientation ;

    float r = 0.f ;
    float d = 0.f ;
    t = 0.f ; // divisions are minimized 
    int i = 0 ; // here we'll use i to track which axis is used to change nodes: 0:X, 1:Y, 2:Z. 

    int steps = 0 ;
    int NS = 0 ; // target node size

    //sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "numchildren = %d", numchildren) ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "position : %.2f %.2f %.2f", pos.x, pos.y, pos.z) ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "ray = %.4f %.4f %.4f", ray.x, ray.y, ray.z ) ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "********** RAY START **********") ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "  penetration plane: %s   (orientation=%d)", plane_names[i], orientation) ; geom_msgs_num2++ ;
    //sprintf( geom_msgs2[geom_msgs_num2], "  rayfront: %.2f %2.f %.2f", rayfront.x, rayfront.y, rayfront.z); geom_msgs_num2++ ;
    /*
            if ( (icorner[0]<=0 && ray.x<0) || (ray.x>=0 && icorner[0]>=WS) ||
                 (icorner[1]<=0 && ray.y<0) || (ray.y>=0 && icorner[1]>=WS) ||
                 (icorner[2]<=0 && ray.z<0) || (ray.z>=0 && icorner[2]>=WS) )
            { 
                break ; 
            }
    */

    // RAY SHOOTING THROUGH THE WORLD! ZAP!
    int WS = world.size ;
    havetarget = false ;
    if (have_ray_start_node || camera.inworld(world))
    {
        int Nscale = 0; 
        i = (orientation>>1) ; // Which axis dominates the ray's movement into the next node

        vec ds = vec(0) ; 
        // sprintf( geom_msgs2[geom_msgs_num2], "HAVE TARGET") ; geom_msgs_num2++ ;
        while (!havetarget)
        {
            if (steps>50) { break ; }                  // Kill runaway loops
            loopj(3) {ifront[j] = (int)rayfront[j] ;} // place, more or less, ifront at rayfront (imprecision from converting float to int)
            ifront[i] += (ray[i]>=0?1:-1) ; // Snap to inside of node we're looking at. This deliberately takes us off node boundaries. 
            if ((ifront[i]>=WS&&ray[i]>0) || (ifront[i]<=0&&ray[i]<0)) 
            {
                
    sprintf(geom_msgs2[geom_msgs_num2], "RAY LEAVING WORLD") ; geom_msgs_num2++ ;
                break ;
            }  // If this adjustment brings us out of the world - we're done

            Octant* oct = findNode(ifront, &NS, &Nscale) ;                 // Find out what tree node encloses this point

            icorner = ifront ;
            loopj(3) { icorner[j] = (icorner[j] >> Nscale) << Nscale ; }    // Now icorner is right on the node corner. 
//FIXME MOVE TO DEBUG    sprintf(geom_msgs2[geom_msgs_num2], "RAY NODE at %d %d %d (Node %s)", 
//        icorner.x, icorner.y, icorner.z,
 //       (oct->has_geometry())?("with geometry"):("NOT NULL")) ; geom_msgs_num2++ ;
            
            if ( oct->has_geometry() ) 
            { 
    sprintf(geom_msgs2[geom_msgs_num2], "HAVE TARGET: icorner at %d %d %d", icorner[0], icorner[1], icorner[2]) ; geom_msgs_num2++ ;
                havetarget = true ; break ; 
            }      // Target acquired. 
            vec f = rayfront ; r = 0.f ; d = 0.f ; t = 0.f ; 

            // Distances to next plane intersections
            loopj(3) { ds[j] = (ray[j]>=0?(float(icorner[j]+NS)-f[j]):(f[j]-float(icorner[j]))) ; }
            if (fabs(ray.x*ds.y) > fabs(ray.y*ds.x)){ r = ray.x ; d = ds.x ; i = 0 ; }
            else{ r = ray.y ; d = ds.y ; i = 1 ; }
            if ((fabs(ray.z*d) > fabs(r*ds.z))){ r = ray.z ; d = ds.z ; i = 2 ; }
            t = fabs(d/r) ;             // divisions are minimized 
            rayfront.add(ray.mul(t)) ;  // move ray to new plane
            ray = dir ;                 // reset ray to length 1

            steps++ ;
        }
    } // end if (have_ray_start_node)

    int s = world.gridscale ;
    loopj(3) { ifront[j] = (ifront[j] >> s) << s ; }    // Now icorner is right on the node corner. 
//    ifront[i] += Dz(orientation)*world.gridsize ;
 //   ifront[i] += Dz(orientation) ;
    // glBegin(GL_LINES) ;
    // draw_corner_cube( fc, NS) ;
    // glEnd() ;
    /*
        loopi(3)
        {
            hitplanes |= ( camdir[i]<0 ? 2*i : 2*i+1 ) << (3*i) ;
        }
    //    loopj(3)
    //    {
    //    }
        loopi(3)
        {
            hitplanes |= ( camdir[i]>0 ? 2*i : 2*i+1 ) << (3*i) ;
        }
    */


    hitplanes = ( dir[i]>0 ? 2*i : 2*i+1 ) << (3*i) ;
    orientation = hittingplane ;
    if (havetarget)
    { 
        sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "NODE TARGETED. selection corner at: %d %d %d    (%d steps) ORIENTATION=%d", 
            icorner.x, icorner.y, icorner.z, steps, orientation) ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
    }

        
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
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

    // If we're not hitting any part of the world's geometry, then we want to know 
    // which part of the world we might be extruding from. 

    if (!havetarget)
    {
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
                    if ( front[bds[i][0]] < wp[2*i].offset && front[bds[i][0]] > 0  &&    
                         front[bds[i][1]] < wp[2*i].offset && front[bds[i][1]] > 0
                       )
                    {
                        orientation = (hittingplane) ;
                        min_t = t ;
                        corner = vec(front) ;
                        // sel_path[0] = -1 ;
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
    } // end if (!havetarget)


    /*
        At this point we should have one of the two following: 

            - a target inside the world - geometry or an entity
            or
            - a cursor framing a piece of the world boundary. 

        In the case of a boundary-touching cursor, we can extract 
        geometry from that without problems. 

        In the case of a world target, we need now to compute where 
        the extrusion base should be located. This is somewhat easy, 
        but requires taking into account a couple of design choices. 
        
        For instance, we need our extrusion base to always have the same
        orientation as the plane where our ray front enters a node.  This
        requires special treatment in both the case where the editing gridsize
        is bigger than the node we're aiming at, and in the case where our
        gridsize is smaller than the node we're aiming at. 

    */

    if (havetarget)
    {
    }


//    FIXME make sure you know which corners are which at this point

//            if ( !(O%2) )
 //           {
  //              NC[Z(O)] += wp[O][Z(O)] * (sel_size) * sel_counts[Z(O)] ;
   //         }

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
    // int childcount ;
    char idx ; // The index that will refer to this within a vertex array. 

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
//        while ( hashvectors[hash].set && hashvectors[hash].v!=pos && hash<1024 ) 
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
            // hashvectors[i].childcount = 0 ;
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
    Function: clear_geom 

    Used by the delete_subtree function. 

    As a process is recycling a tree, it calls 
    this function on its Geom element so that 
    the memory can then be discarded without 
    loose ends. 

    Purpose: to release the OpenGL buffer objects that this 
    node is using. 
*/
void clear_geom( Octant* oct ) 
{
    printf("\nclearing a geom") ;
    if (!oct) return ;          // FIXME: in professional code, an error would be logged here
    if (!oct->geom) return  ;   // FIXME: in professional code, an error would be logged here
    glDeleteBuffers(1,&(oct->geom->vertVBOid)) ;
    glDeleteBuffers(1,&(oct->geom->texVBOid)) ;
    glDeleteBuffers(1,&(oct->geom->colorVBOid)) ;
    worldgeometry.removeobj(oct->geom) ;
    delete oct->geom ;
    oct->geom = NULL ;
}

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


// FIXME: use clear_geom( oct ) ; where appropriate. 
    // This is a depth-first descent down the tree. Can't delete a node before its children 
    // are gone, after all. 
    while (d>=0)
    {
        if ( CN->geom )
        {
            clear_geom( CN ) ; // recycle this node's geom
        }
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
        else
        {
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
//FIXME: reclaim any VA sets and GL IDs that might be in this tree. 
// if (CN->geom) ...
                delete [] CN->children ;
                CN->children = NULL ;
                //printf("\ndeleting 8 children. \n") ; 
            }
            else if (CN->has_geometry())
            {
                numchildren-- ;
            }
            idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
        }
        else
        {
            break ;
        }
    }   // end while d>=0
}


// Finds the smallest node enclosing the supplied integer vector. 

/*
    Function: findNode

    Purpose: 
        locate a leaf node that encloses a particular point. If the point given
        is on the border between two nodes, the node returned is the one for
        which the border is at its minimum boundaries in X, Y or Z. This
        follows from the fact that a corner vertex always uniquely refers to
        the node for which that corner is the min X, Y, Z value touching the
        node. 

    Inputs: 
        An int vector representing a location of interest. 

    Outputs: 
        The node which encloses that point. 
        The size of the returned node. (optional)
        The scale of the returned node (optional) 

    Possible speed optimization: 

        Preserve a path of nodes to the point of interest.  If the next point
        of search is inside any of the ancestors of the current target, then we
        might be able to find more quickly the parent to start the descent from
        on the next search. 

*/
Octant* findNode(ivec at, int* out_size, int* nScale) // , int* out_size) 
{
    int CGS = world.scale ;
    int i = 0 ;
    Octant* oct = &world.root ;

    if (nScale != NULL) { *nScale = CGS ; }
    while (CGS>2)
    {
        if ( oct->children ) 
        {
            i = octastep(at.x,at.y,at.z,CGS) ;
            oct = &oct->children[i] ;
            if (nScale != NULL) { *nScale = CGS ; }
        }
        else { break ; }
        CGS-- ;
    }

    //if (out_size!=NULL) { *out_size = 2<<CGS ; }
    if (out_size!=NULL) 
    if (!oct)
    {
        *out_size = 0 ;
    }
    else
    {
        *out_size = 2<<CGS ;
    }

    return oct ;
}

static Geom hello ; // This is a place holder to give a non-null value to the GEOM_NEED_UPDATE pointer. 
static Geom* GEOM_NEED_UPDATE = &hello ; // dummy used to signal that a node needs its geom rebuilt. 

/*
    Function: flagPathForUpdate. 

    This function takes a sequence of nodes where every node at position 
    n+1 is the child of the node at position n, and goes up this sequence 
    and marks nodes as requiring updates, if they haven't been marked 
    already. 

    The mark is left by setting a node's geom equal to the GEOM_NEED_UPDATE 
    point above. The function's task is finished when it reaches the root 
    of this tree (depth 0) or if it encounters a node that was already 
    marked for update. 

    Assumption: any node on the path from depth _depth to 0 is non-null and
    those above depth _depth have children.  These nodes do not necessarily
    have a non-null geom, however. 
    
*/
void flagPathForUpdate( Octant** path, int _depth) 
{
    printf("\nflagging path...") ;
    int d = _depth ;
    while (d>=0)
    {
        printf(" %d...", d) ;
        if (path[d]->geom==GEOM_NEED_UPDATE) 
        {
            // If we reach here it means this node has already been marked. No 
            // need to continue up the tree. 
            break ;
        }

        // Since we're marking this path for update it means any geometry is going 
        // to be rebuilt. We therefore recycle any existing geometry on the path. 
        if (path[d]->geom)
        {
            printf("\nclearing a geom") ;
            clear_geom(path[d]) ;
        }
        path[d]->geom = GEOM_NEED_UPDATE ;
//        printf("\nNode depth %d successfully flagged for update.\n", d) ;

        d-- ;
    }
    printf("\ndone.") ;
}
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
    // vectormap vmap ;                // used to record new nodes and then to construct and update vector arrays

    // define a range of cubes that will be created, bounded by 
    // sel_min and sel_max. These will bound the same rectangle 
    // as sel_start and sel_end, but with easier-to-compute properties. 
    loopi(3)
    {
        sel_min[i] = min( sel_end[i], sel_start[i] ) ;
        sel_max[i] = max( sel_end[i], sel_start[i] ) ;
    }


    DEBUGTRACE(("\n EXTRUDE BEGINS \n")) ;
    // --------------------------------------------
    // PHASE 1: Create tree nodes 
    // --------------------------------------------

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





    First thing we need to know when extruding: whether we are extruding into 
    an available volume. If we are, then we can asses what already exists 
    there, and touches it, that might have to be deleted, have a faced obscured, 
    or be subdivided. 

    Things that become affected belong to nodes that root their own subtree. 
    Affected nodes use their Geom records to hand off the resources they've 
    been using. In a nice metaphor for how I think we should try to live, 
    the willingness to let go exists because there is assurance that more 
    will come. 

    So an affected subtree glDeleteBuffers'es its vertVBOid and texVBOid. 

    Then it either deletes or subdivides its subtree. 

    Note: nodes that only touch the new volume need to be flagged as 
    needing update as well. This is somewhat uncomfortable, but it's not 
    something the system can't handle. Maybe we can give it a switchable 
    state so that when we want precision, it's off, and when we want 
    speed and already know we're spending the resources to do it, we turn 
    it off. 

    */

//DEBUGTRACE(("\n ********** EXTRUDE - PHASE 1 (node creation) ********** \n")) ;

    Octant* path[20] = {NULL} ;
    {
        // extruding inwards (not extruding)
        if ( in )
        {
            havenewcube = false ;
            return ; // FIXME: this should result in deletion
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
                //NC[Z(O)] += wp[O][Z(O)] * (sel_size) * sel_counts[Z(O)] ;
                NC[Z(O)] += wp[O][Z(O)] * (sel_size) ;
            }
            // If the new cube coordinates do not exceed world 
            // limits, then we have a new cube. 
            havenewcube = true ;

            Octant* oct = &world.root ;
            path[0] = oct ;
            int depth = 0 ; // first child of root

            int x_count = 0 ;
            int y_count = 0 ;

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
                        i = octastep(NC.x,NC.y,NC.z,CGS) ;
                        if ( !oct->children ) 
                        {
                            new_octants(oct) ;
                            if (!oct->children)
                            {
                                printf("ERROR -- ASSIGNING CHILDREN TO A NODE! ") ;
                            }
                        }
                        // sel_path[depth] = i ;
                        oct = &oct->children[i] ;
                        depth++ ;
                        path[depth] = oct ;
                        CGS-- ;
                    }

                    if (oct)
                    {
                        if (oct->children)
                        {
                            printf("\ndeleting a subtree\n") ;
                            delete_subtree( oct ) ;
                        }
//                        printf("\nFrom depth %d-1 creating new material node with child %d\n", depth, i) ;
//                        printf("\ntrying to flag path for update") ;
                        oct->set_all_edges() ;
                        // Record the fact that from root to this leaf node, we need to rebuild 
                        // geometry. 
                        flagPathForUpdate( path, depth) ;
                        numchildren++ ;
                    }
                    // Reset variables for any subsequent new nodes. 
                    CGS = WS ; 
                    depth = 0 ;
// possible optimization: instead of going back up to the root, only go back up as far as necessary
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
// FIXME collect total number of triangles 
printf("\nafter an extrusion numchildren = %d\n", numchildren) ;

    int new_z = sel_start[Z(sel_o)]+Dz(O)*GS ;
    // prepare for next extrusion if it doesn't get out of the world 
    // FIXME: make it so that if it DOES get out of the world, no other 
    // extrusion attempts is made until the selection changes.
    if ( new_z >= 0 && new_z <= world.size)
    {
        sel_start[sel_o>>1] += GS*Dz(O) ;
        sel_end  [sel_o>>1] += GS*Dz(O) ;
    }
//    sprintf(geom_msgs[geom_msgs_num], "After a geometry update sel_start Z is %d ", sel_start[sel_o>>1]) ; geom_msgs_num++ ;
// CASE: the cursor is not set on anything; this can happpen if we are outside 
// world boundaries and pointing the cursor at any part of the world. 
//      else if ( sel_path[0] == -2 )
//      {
//          DEBUGTRACE(("\nCannot extrude when no selection is in view. \n")) ; 
//      }
//      ivec counts(1) ; 
// min one cube 

    // --------------------------------------------
    // PHASE 2: Produce leaf vertex counts (lvc) so 
    // that in the final phase vertex array sets can be 
    // assigned to the right nodes. 
    // --------------------------------------------

    // Iteration is over all nodes which have their geom==GEOM_NEED_UPDATE. 
    // zzz
    // local variables. 
    int32_t d = 0 ;             // depth
    Octant* CN = &world.root ;  // current node 
    Octant* CC = NULL ;         // current child 

    // path
    // Octant* path[20] = {NULL} ;
    int32_t idxs[20] ;   // Used to track path indexes. -1 means unused. 
    loopi(20) {idxs[i] = -1 ;}
    loopi(20) {path[i]=NULL ;}
    path[0] = CN ;
    idxs[0] = 0 ;
    d = 0 ;

// FIXME:  instead of changing vc when something HAS geometry (which alters its geometry!!!), 
// we need to instead check: 
//DEBUGTRACE(("\n ********** EXTRUDE - PHASE 2 (face visibility marking and lvc processing) ********** \n")) ;


printf("\n ANALYZE BEGINS \n") ;
AnalyzeGeometry(&world.root, vec(0,0,0), world.scale) ;

AssignNewVBOs(&world.root, vec(0,0,0),world.scale) ;
// --------------------------------------------
// PHASE 3: Produce vertex array sets
// --------------------------------------------
// DEBUGTRACE(("\n ********** EXTRUDE - PHASE 3 (vertex array set creation) ********* \n")) ;

if (0)
if (1)
{
printf("\n ------------------------------ \n") ;
printf("\nVBO CONSTRUCTION BEGINS \n") ;

    int32_t SI = world.scale ;  // Size increment scale. Used to compute offsets from node corners. 
    int32_t incr = 0 ;
    int32_t yesorno = 0 ;
    ivec pos( 0, 0, 0 ) ;
    bool useThisNode = false ;

    while (d>=0)
    {

        // First figure out whether we will create the geom for this node. 
        //if ( CN->geom==GEOM_NEED_UPDATE && CN->children )
        if ( CN->geom==GEOM_NEED_UPDATE )
        {
            useThisNode = true ;
            if ( CN->children )
            {
                if (CN->vc<=256 && CN->vc>0)
                {
                    printf("\nchecking to see if I (depth %d) will hold a geom...\n",d) ;
                    useThisNode = true ;
                    if (CN->children)
                    {
                        int c = CN->vc ;
                        CC = &CN->children[0] ; 
                        useThisNode = true ;

                        // Is any child sufficient to hold all geometry under me?
                        loopi(8)
                        {
                            printf("\nChecking child vc: %d (my vc=%d) (depth %d)\n", CC->vc, CN->vc, idxs[d]) ;
                            if (CC->vc==c)
                            {
                                printf("\n so we're not using me! ") ;
                                useThisNode = false ;
                                break ;
                            }
                            CC++ ;
                        }
                    }
                }


            }   // end if have children

            if (!useThisNode)
            {
                /*
                    If the vertex count under this node is small enough, then:
                        - If this node has a higher vertex count than any one of 
                          its children, then it will host the geometry. 

                        - If any of the chidlren have the same vertex count, it means 
                          that the geometry will be held within that child node. 
                */
                if (idxs[d]<8)
                {
                    // We're going down to a child, marked relative to its parent by idxs[d]. 
                    loopi(3) {
                        incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ;
                        pos.v[i] += yesorno * incr ;
                    }

                    CC = &CN->children[idxs[d]] ; 
                    if ( CC->geom==GEOM_NEED_UPDATE )
                    {
                        printf("\ndepth %d we switch to a child\n", d) ;
                        CN = CC ;
                        d++ ;
                        path[d] = CN ;
                        idxs[d] = 0 ; // Start the children at this level. 
                        continue ;
                    }
                }
            }
            else
            //if (useThisNode)
            {
                printf("\n\tmaking vbo for %d nodes at tree located at %d %d %d. (from depth %d)", CN->vc, pos.x, pos.y, pos.z, d) ; 
//                makeSubtreeVBO( CN, pos, SI-d) ; // FIXME: move this to phase 3 
            }
            
        }   // end if GEOM_NEED_UPDATE
        else
        {
            // we have a lone child that will handle its geometry
//            makeSubtreeVBO( CN, pos, SI-d) ; // FIXME: move this to phase 3 
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        if (d<0)    // Past the root? Then we're done. 
        {   
            break ;
        }
        CN = path[d] ;

        loopi(3) {
            incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ;
            pos.v[i] -= yesorno * incr ;
        }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    } // end while d>=0

}


} // end extrude 


/*
    This function traverses the tree, following nodes that are marked for 
    update. 

    When it finds a node that has vc<=256, and has a larger vc than all 
    its children, then it makes a vbo from it. 
*/
void AssignNewVBOs(Octant* tree, ivec in_corner, int scale)
{
    int32_t d = 0 ;             // depth
    int32_t S = scale ;             // Scale of this world
    Octant* CN = &world.root ;  // current node 
    Octant* CC = NULL ;         // current child 
    int32_t idxs[20] ;   // Used to track path indexes. -1 means unused. 
    loopi(20) {idxs[i] = -1 ;}
    idxs[0] = 0 ;
    Octant* path[20] ;   // Used to track path indexes. -1 means unused. 
    loopi(20) {path[i]=NULL ;}
    path[0] = CN ; 
    int32_t yesorno = 0 ;
    int32_t incr = 0 ;
    ivec pos = in_corner ;

//printf("\nAssignNewVBOs starting with pos %d %d %d\n", (int)pos.x, (int)pos.y, (int)pos.z) ;

    while (d>=0)
    {
        if ( CN->geom==GEOM_NEED_UPDATE )
        {
            if ( CN->children )
            {
                // Ok we have children. Either we make a geom here and now, or we keep going down our children. 
                Octant* CC = &CN->children[0] ;
                bool useThisNode = false ;
                if (CN->vc<=256)
                {
                    useThisNode = true ;
                    loopi(8)
                    {
                        if (CC->vc==CN->vc) { useThisNode = false ; break ; }
                        CC++ ;
                    }
                }
                if (useThisNode)
                {
                    printf("\n\tmaking vbo for %d nodes at tree located at %d %d %d. (from depth %d)", CN->vc, pos.x, pos.y, pos.z, d) ; 
                    if (CN->children)
                    {
                        loopi(8)
                        {
                            printf("\n child %d: %d verts",i, CN->children[i].vc) ;
                        }
                    }
                    makeSubtreeVBO( CN, pos, S-d) ; // FIXME: move this to phase 3 
                }
                else if (idxs[d]<8)
                {
                    // We're going down to a child, marked relative to its parent by idxs[d]. 
                    //if (CN->children[idxs[d]].geom==GEOM_NEED_UPDATE)
                //    if (CN->children[idxs[d]].geom)
                    if (CN->children)
                    {
                        incr = (1<<(S-d)) ; 
                        loopi(3) { yesorno = ((idxs[d]>>i)&1) ; pos.v[i] += yesorno * incr ; }
//printf("\nAssignNewVBOs current pos is %d %d %d\n", (int)pos.x, (int)pos.y, (int)pos.z) ;
                        CN = &CN->children[idxs[d]] ; 
                 //       clear_geom(CN) ;
                        d++ ;
                        path[d] = CN ;
                        idxs[d] = 0 ; // Start the children at this level. 

                        continue ;
                    }
                }
            }
            // If we don't have children, then maybe we have geometry.
            else
            {
                // If we get to here, then it's time to build this. 
                if ( CN->has_geometry() )
                {
// printf("\nMaking vbo for solid node \n") ;
                    makeSubtreeVBO( CN, pos, S-d) ; // FIXME: move this to phase 3 
                }
            }
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ; 
        // Past the root? Then we're done. 
        if (d<0) { break ; }
        CN = path[d] ;

        loopi(3) {
            incr = (1<<(S-d)) ; yesorno = ((idxs[d]>>i)&1) ;
            pos.v[i] -= yesorno * incr ;
        }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    } // end while d>=0
}
/*
    Function: AnalyzeGeometry

        ----------------
        Quick version: 

    This function visits nodes that are marked for update. It figures out for
    each such node which faces are visible, and counts the number of vertices
    that will be used to draw that node. 

        ----------------
        Longer version: 

    This function can work on an entire world or on a subtree of it. Subtrees 
    are trees, after all. 

    Anyway, this function takes a geometry tree and visits all nodes that are
    marked as needing a geometry update (the need for a geometry update is
    indicated by the geom pointer set to the GEOM_NEED_UPDATE pointer. ). When
    it reaches leaf nodes of updated portions of the tree, it figures out which
    faces of those nodes are visible. 

    Once the visible faces of a node have been identified, the lvc of that node 
    is assigned. lvc stands for leaf vertex count, or the number of vertices 
    required to specify the triangles that make the visible faces of this node. 

    The way by which face visibility is recorded is by assigning a tex id to
    every face. In the case of a visible face, an id from 1 to 255 is used. In
    the case of an invisible face, id 0 is used. 

    Detailed procedure: 

        - traverse tree iteratively
        - for each node: 
            - if geom==GEOM_NEED_UPDATE, process each face for visibility

    Parameters: 

        - tree      ->  could be the world root or any other tree or subtree
        - corner    ->  the all-mins corner of this tree
        - scale     ->  this is the size scale of the cube enclosing this tree. 
*/
void AnalyzeGeometry(Octant* tree, ivec corner, int scale)
{
    int32_t S = scale ;  // Size increment scale. Used to compute offsets from node corners. 
    int32_t incr = 0 ;
    int32_t yesorno = 0 ;

    Octant* CN = tree ;
    Octant* CC = NULL ; // used for 'current child'
    Octant* path[20] = {NULL} ;
    int32_t idxs[20] ;   // idxs[d] tracks which child of node path[d] is being used, if any. 

    loopi(20) { idxs[i] = 0 ; }
    loopi(20) { path[i] = NULL ; }
    path[0] = CN ; // This is and always will be the root. 
    int32_t d = 0 ;     // We start at the root. 
    ivec pos = corner ;

//if (0) // lol
    while (d>=0) 
    {
        if ( CN->children && CN->geom==GEOM_NEED_UPDATE)
        {
            if (idxs[d]<8)
            {
                // We're going down to a child, marked relative to its parent by idxs[d]. 
                loopi(3) {
                    incr = (1<<(S-d)) ; yesorno = ((idxs[d]>>i)&1) ;
                    pos.v[i] += yesorno * incr ;
                }
                //printf("\nanalyzing: going down the tree (depth %d) - position = %d %d %d\n", //   S-d, pos.x, pos.y, pos.z //  ) ;
                //printf("\nFrom depth %d looking at child %d\n",d, idxs[d]) ;
                CN = &CN->children[idxs[d]] ; 
//                if (CN->children) { printf("\n\tThis guy has children \n") ; }
//                if (CN->has_geometry()) { printf("\n\tThis guy has geometry\n") ; }
                d++ ;
                path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 
                continue ;
            }
            // If we're done doing this node's children, then we will add up their lvc's. 
            else
            {
                int sum = 0 ;
                CC = &CN->children[0] ;
                loopi(8)
                {
                    sum += CC->vc ;
                    CC++ ;
                }
                CN->vc = sum ;
//                printf("\nNon-leaf Node with lvc total = %d", CN->vc) ;
            }
        }
        else if (CN->children)
        {
//            printf("\n\tnode at depth %d with children, doesn't need update\n", S-d) ;
        }
        // If we don't have children, then maybe we have geometry.
        else
        {
//printf("\n\tnode without children: do we have geometry? %s\n", (CN->has_geometry())?("yes"):("no")) ;
            if ( CN->has_geometry() )
            {
//                printf("\n\tsolid node at depth %d\n", S-d) ;

                ivec pcorner ;          // probe corner
                Octant* anode ;         // probe node
                int NS = 1<<(S-d+1) ;  // present Node size. 
                int ns = 0 ;            // neighbor size

                // face visibility
                int count = 0 ;
                CN->vc = 0 ;
//            if (0)
                for (int face=0;face<6;face++)
                {
                    int O = face ;
                    pcorner = pos ;
                    /* We position the probe vector (pcorner) to the inside of the node adjacent in the direction of the current face.  Then we retrieve from the tree the node at that location.  Once we have 'anode', or the node adjacent to the current face, we know whether it is solid, and whether it is bigger than us.  */
                    pcorner[face>>1] = pos[face>>1]+Dz(face)*(1 + (face%2)*(NS)) ; 
//      printf("\nduring analysis: check for node at location %d %d %d \n", //         pcorner.x, //        pcorner.y, //       pcorner.z //      ) ;
                    anode = findNode(pcorner, &ns) ;
                    pcorner = pos ;
                    /* When do we skip drawing a face?  - when an adjacent node is at least as big - when the face can only be seen from outside world limits (this might change if we want to be able to have multiple worlds.  Q. How do we calculate that first one, 'there is an at-least-as-big neighbor covering the face'?  A.  -> you take this node's corner, and generate with it a point that lies just outside the face concerned.  face i.  corner[i>>1]+=Dz(i)*(1 + (i%2)*(NS)) -> you find the node located there -> if that node is solid, and is bigger than this one, then we know we're skipping this face.  If our node extends beyond what its neighbor can cover of that face */
//printf("\ndo we have a node here? %s (size %d)\n", //(anode)?("yes"):("no"), ns //                        ) ;
//if ( anode->has_geometry() ) { printf("\nanalysis: face %d has neighbor\n", face) ; }
                    CN->tex[face] = 0 ; // setting this to 'null' in case we don't want to see this face. 
                    if ( !anode->has_geometry() || ns<NS ) 
                    {
                        // texture slot assignment
                        CN->tex[face] = 1 ;// Current texture or default==1
                        CN->vc += 6 ;
//                        printf("\nAnalysis: node face with 6 more vertices \n") ;
                        count++ ;
                    } // end if neighbor is not in the way of this face
//else { printf("\nhiding a face. \n") ; CN->tex[face] = 0 ; // Current texture or default (==1) 
//}
                } // end for (int face=0;face<6;face++)
            } // end if ( CN->has_geometry() )
//printf("\n\tAnalysis finished with a node: lvc = %d\n", CN->vc) ;
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        if (d<0)    // Past the root? Then we're done. 
        {   
            break ;
        }
        CN = path[d] ;

        loopi(3) {
            incr = (1<<(S-d)) ; yesorno = ((idxs[d]>>i)&1) ;
            pos.v[i] -= yesorno * incr ;
        }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 

    }  // end while d>=0
}
/*
    Make a VBO from a subtree. 

    It makes the 'parent' parameter Octant the host of a Geom which describes 
    the geometry under this parent. 
    

    Parameters: 
        parent - the node that will be at the top of this Geom. 

        corner - the all-mins corner of the cube containing this volume of space. 

        NS - the scale of the parent


*/

// FIXME! :) this is for TEMPORARY testing purposes only. 
#define MAX_VERTS 1000000
vec allvertices[1000000] ;
vec allcolors[1000000] ;
unsigned int allVertsVBO = 0 ;
unsigned int allColorsVBO = 0 ;
int numVerts = 0 ;

// Dim colors so that untextured geometry looks like ass
float mult = 0.6f ;
vec thecolors[10] = {
  vec(mult*1.0, mult*1.0, mult*1.0) , 
  vec(mult*0.0, mult*1.0, mult*0.0) ,
  vec(mult*1.0, mult*0.0, mult*1.0) , 
  vec(mult*1.0, mult*1.0, mult*0.0) ,
  vec(mult*1.0, mult*0.0, mult*0.0) , 
  vec(mult*0.5, mult*0.5, mult*1.0) ,
  vec(mult*0.5, mult*0.5, mult*0.0) , 
  vec(mult*0.0, mult*0.5, mult*0.8) ,
  vec(mult*1.0, mult*0.0, mult*0.5) , 
  vec(mult*0.5, mult*1.0, mult*0.5)
} ;


/*
    Inputs: 

        parameters: 
            parent -> the node that roots this subtree
            in_corner -> the all-mins corner of this root node
            NS -> the size of the node that contains this subtree. 

        sel_min -> least-valued corner of the selection in XYZ 
        sel_max -> most-valued corner of the selection in XYZ
        sel_o -> 


        A way to understand what globals are available here is to 
        know that while geometry is being created (at least during 
        edit mode), we can be sure that the active selection will 
        not change. 

        This function is called on a node's interior when no more than 256
        vertices will describe the geometry. 

*/
void makeSubtreeVBO(Octant* root, ivec in_corner, int _NS)
{
// printf("\nmakeSubtreeVBO starting with base corner = %d %d %d\n", in_corner.x, in_corner.y, in_corner.z) ;
    if (root==NULL) return ;
    Octant* CN = root ;
    if (CN->geom==NULL||CN->geom==GEOM_NEED_UPDATE) 
    {
        CN->geom = new Geom() ;
        printf("\nWHAT ARE YOU DOING FOOL\n") ;
    }
    else
    {
        printf("\nLol we have all the Geoms we need. \n") ;
    }
    int d = 0 ;
    Octant* path[20] ;
    int32_t idxs[20] ;
    ivec pos = in_corner ;

    vec* verts = CN->geom->vertices ;
    vec* colors = CN->geom->colors ;
    int numverts = 0 ;

    path[0] = CN ;
    loopi(20) {idxs[i] = 0 ;}

    // Used to decide, every time we switch nodes, whether a particular node 
    // coordinate should increment. 
    bool incornot = false ;  

    // Size increment. Used with tree-depth to localize our position to corners. 
    int32_t SI = _NS ;   
    int32_t incr = 0 ;

    /*
        FIXME: REMOVE THIS! TESTING ONLY. 

        size of vec: 3 * size of float. 
        triangle: 3 vecs = 9*size of float. 
        100 verts can make 98 triangles at most, but 33 at least. 
    */
    while (d>=0)
    {
        if ( CN->children )
        {
//        printf("\n\t\t processing a node NEEDING UPDATE with children \n") ;
            // If we have children, then maybe we have a Geom. If we do, we'll know if it needs 
            // rebuilding if it has been set equal to the GEOM_NEED_UPDATE pointer. 
            if (idxs[d]<8)
            {
                // We're going down to a child, marked relative to its parent by idxs[d]. 
                incr = (1<<(SI-d)) ;            // This can be replaced with NS and NS reduced by half or increased by twice as needed. 
                loopi(3) 
                { 
                    incornot = ((idxs[d]>>i)&1) ;   // I pray Satan forgives me for being this brazen about using such nonsense code
                    pos.v[i] += incornot * incr ; 
                }
                CN = &CN->children[idxs[d]] ; 
                d++ ; 
                path[d] = CN ; 
                idxs[d] = 0 ; // Start the children at this level. 


// still evaluating
                if (CN->geom==GEOM_NEED_UPDATE)
                {
                    CN->geom = NULL ;
                }


                continue ;
            }
        }
        else if (CN->children)
        {
            printf("\n\t\t node with chidren NOT NEEDING UPDATE\n") ;
        }
        // If we don't have children, then maybe we have geometry.
        else
        {
            if ( CN->has_geometry() ) // If we have geometry, that has to go into a VBO
            {
                ivec pcorner ;          // probe corner
                Octant* pnode ;         // probe node
                int NS = 1<<(SI-d+1) ;  // Node size. 
                int ns = 0 ;            // neighbor size

                int colorNow = numverts%17 ;
//printf("\n group getting vertices ...................") ;
                int tui=numverts ;
                for (int face=0;face<6;face++)
                {
                    // If this face is visible, then make some triangles for it. 
                    if (CN->tex[face]>0)
                    {
                        int O = face ;
                        pcorner = pos ; 
// printf("\npreparing a corner = %d %d %d", pos.x, pos.y, pos.z) ;
                        
                        int zoffset = Dz(face)*NS*(face%2) ;
                        pcorner[Z(O)] += zoffset ;


                        int32_t x = X(O) ; // in vectors, this x is now the index for the coordinate pointed on the x axis. 
                        int32_t y = Y(O) ;
                        int32_t z = Z(O) ;
                        
                        // ASSIGN TRIANGLE VERTICES HERE
                        /*
                            How triangles are made! Now are you starting to suspect that 
                            we need (at minimum) to start coding within html-aware programs? 
                            Depending on whether X(O) grows in the positive or negative for a 
                            given face, the triangles to be drawn are like this: 

                            A  B    A  B
                            *--*    *--*
                            | /|    |\ |
                            |/ |    | \|
                            *--*    *--*
                            D  C    D  C

                            The first one is for when X grows in the positive, the second is for 
                            when X grows in the negative. 

                                                                        probably not: Compute A, B, C then D. 
                            In case 1, we compute vertices BAD and DCB. 
                            In case 2, we compute vertices ADC and CBA
                        */

                        // Faces 0, 3, 4 - where 'X' grows in the negative direction. 
                        if (((face)&1)==((face>>1)&1))
                        {
                            // Triangle ADC
                            //colors[numverts]   = thecolors[colorNow%10] ;
                            colors[numverts]   = vec(1,1,1) ;
                            verts[numverts][x] = pcorner[x] + NS ;
                            verts[numverts][y] = pcorner[y] + NS ;
                            verts[numverts][z] = pcorner[z] ; 

                            //colors[numverts+1]   = thecolors[colorNow%10] ;
                            colors[numverts+1]   = vec(1,0,0) ;
                            verts[numverts+1][x] = pcorner[x] + NS ;
                            verts[numverts+1][y] = pcorner[y] ;
                            verts[numverts+1][z] = pcorner[z] ; 

                            printf("\nvertex D at: %d %d %d\n",
                                (int)verts[numverts+1][x],
                                (int)verts[numverts+1][y],
                                (int)verts[numverts+1][z]
                                ) ;

                            colors[numverts+2]   = thecolors[colorNow%10] ;
                            colors[numverts+2]   = vec(1,1,1) ;
                            verts[numverts+2][x] = pcorner[x] ;
                            verts[numverts+2][y] = pcorner[y] ;
                            verts[numverts+2][z] = pcorner[z] ; 

                            // Triangle CBA
                            //colors[numverts+3]   = thecolors[colorNow%10] ;
                            colors[numverts+3]   = vec(1,1,1) ;
                            verts[numverts+3][x] = pcorner[x] ;
                            verts[numverts+3][y] = pcorner[y] ;
                            verts[numverts+3][z] = pcorner[z] ; 

                            //colors[numverts+4]   = thecolors[colorNow%10] ;
                            colors[numverts+4]   = vec(1,1,1) ;
                            verts[numverts+4][x] = pcorner[x] ;
                            verts[numverts+4][y] = pcorner[y] + NS ;
                            verts[numverts+4][z] = pcorner[z] ; 

                            //colors[numverts+5]   = thecolors[colorNow%10] ;
                            colors[numverts+5]   = vec(1,1,1) ;
                            verts[numverts+5][x] = pcorner[x] + NS ;
                            verts[numverts+5][y] = pcorner[y] + NS ;
                            verts[numverts+5][z] = pcorner[z] ; 
                        }
                        // Faces 1, 2, 5
                        else 
                        {
                            // Triangle BAD
                            //colors[numverts]   = thecolors[colorNow%10] ;
                            colors[numverts]   = vec(1,1,1) ;
                            verts[numverts][x] = pcorner[x] + NS ;
                            verts[numverts][y] = pcorner[y] + NS ;
                            verts[numverts][z] = pcorner[z] ; 

                            //colors[numverts+1]   = thecolors[colorNow%10] ;
                            colors[numverts+1]   = vec(1,1,1) ;
                            verts[numverts+1][x] = pcorner[x] ;
                            verts[numverts+1][y] = pcorner[y] + NS ;
                            verts[numverts+1][z] = pcorner[z] ; 

                            //colors[numverts+2]   = thecolors[colorNow%10] ;
                            colors[numverts+2]   = vec(1,1,1) ;
                            verts[numverts+2][x] = pcorner[x] ;
                            verts[numverts+2][y] = pcorner[y] ;
                            verts[numverts+2][z] = pcorner[z] ; 

                            // Triangle DCB
                            //colors[numverts+3]   = thecolors[colorNow%10] ;
                            colors[numverts+3]   = vec(1,1,1) ;
                            verts[numverts+3][x] = pcorner[x] ;
                            verts[numverts+3][y] = pcorner[y] ;
                            verts[numverts+3][z] = pcorner[z] ; 

                            //colors[numverts+4]   = thecolors[colorNow%10] ;
                            colors[numverts+4]   = vec(1,1,1) ;
                            verts[numverts+4][x] = pcorner[x] + NS ;
                            verts[numverts+4][y] = pcorner[y] ;
                            verts[numverts+4][z] = pcorner[z] ; 

                            //colors[numverts+5]   = thecolors[colorNow%10] ;
                            colors[numverts+5]   = vec(1,1,1) ;
                            verts[numverts+5][x] = pcorner[x] + NS ;
                            verts[numverts+5][y] = pcorner[y] + NS ;
                            verts[numverts+5][z] = pcorner[z] ; 
                        }
                        numverts += 6 ;    //  Two new triangles for this face means 6 new vertices
                    } // end if (CN->tex[face]>0) (if face is visible)
                } // end loop over 6 faces
            } // end if has geometry 
        }
        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;

        if (d<0)
        {
            break ;
        }
        CN = path[d] ;
        // reset our corner monitor 
        incr = (1<<(SI-d)) ;
        loopi(3) 
        { 
            incornot = ((idxs[d]>>i)&1) ; 
            pos.v[i] -= incornot * incr ; 
        }

        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    }                   // end while d>=0

    glDeleteBuffers( 1, &(CN->geom->vertVBOid))  ; // clear existing VBO
    glGenBuffers( 1, &(CN->geom->vertVBOid)) ;                            // Get A Valid Name
    glBindBuffer( GL_ARRAY_BUFFER, CN->geom->vertVBOid) ;            // Bind The Buffer
    glBufferData( GL_ARRAY_BUFFER, numverts*sizeof(vec), verts, GL_STATIC_DRAW );

    glDeleteBuffers( 1, &(CN->geom->colorVBOid))  ; // clear existing VBO
    glGenBuffers( 1, &(CN->geom->colorVBOid));                            // Get A Valid Name
    glBindBuffer( GL_ARRAY_BUFFER, (CN->geom->colorVBOid));            // Bind The Buffer
    glBufferData( GL_ARRAY_BUFFER, numverts*sizeof(vec), colors, GL_STATIC_DRAW );


    CN->geom->numverts = numverts ;

printf("\n\tDone building a geom on a node. numverts = %d. ", numverts) ;   // FIXME: add removal of updated geoms here
printf("\n\tNode says vc = %d", CN->vc) ;                                   // FIXME: add removal of updated geoms here

worldgeometry.removeobj(CN->geom) ; // In case it's already in there FIXME FIXME: worldgeometry should be a map
worldgeometry.add(CN->geom) ;

//printf("\nMAKE VBO Complete. ") ;
printf("\nnumber of verts = %d. Number of verts according to node: %d", 
numverts, CN->geom->numverts//, //colors[numverts].x, //colors[numverts].y, //colors[numverts].z
) ;
printf("\n---------------------------------") ;
printf("\n") ;
printf("\n") ;
printf("\n------------------------------------------------------------------") ;

printf("\n") ;
printf("\n") ;
printf("\ncontents of worldgeometry: ") ;

loopv(worldgeometry)
{
    Geom* geom = worldgeometry[i] ;
    printf("\n vbo %d: (%d vertices)", i, geom->numverts) ;
}


/*
    loopj(geom->numverts)
    {

        printf("\n vertices: %f %f %f", 
            geom->vertices[j].x,
            geom->vertices[j].y,
            geom->vertices[j].z
            ) ;
        printf("\n colors: %f %f %f", 
            geom->colors[j].x,
            geom->colors[j].y,
            geom->colors[j].z
            ) ;
    }
*/




//printf("\nHello\n");
    // glBufferData( GL_ARRAY_BUFFER, numVerts*sizeof(vec), allcolors, GL_STATIC_DRAW );
   // glBufferData( GL_ARRAY_BUFFER, numVerts*9*sizeof(float), allcolors, GL_STATIC_DRAW );
    //glBufferData( GL_ARRAY_BUFFER, numVerts*9*sizeof(float), allcolors, GL_STATIC_DRAW );
    // glBufferData( GL_ARRAY_BUFFER, numVerts*3*sizeof(float), allcolors, GL_STATIC_DRAW );
    //glBufferData( GL_ARRAY_BUFFER, numVerts*3*sizeof(float), allvertices, GL_STATIC_DRAW );

    // glBufferData( GL_ARRAY_BUFFER, 100*3*sizeof(float), allvertices, GL_STATIC_DRAW );

/*
    sprintf(geom_msgs[geom_msgs_num], "colors element:  %f %f %f", 
        colors[0].x, colors[0].y, colors[0].z) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "colors element:  %f %f %f", 
        colors[1].x, colors[1].y, colors[1].z) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "colors element:  %f %f %f", 
        colors[2].x, colors[2].y, colors[2].z) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "colors element:  %f %f %f", 
        colors[3].x, colors[3].y, colors[3].z) ; geom_msgs_num++ ;

    sprintf(geom_msgs[geom_msgs_num], "allcolors element:  %f %f %f", 
        allcolors[0].x, allcolors[0].y, allcolors[0].z) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "allcolors element:  %f %f %f", 
        allcolors[1].x, allcolors[1].y, allcolors[1].z) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "allcolors element:  %f %f %f", 
        allcolors[2].x, allcolors[2].y, allcolors[2].z) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "allcolors element:  %f %f %f", 
        allcolors[3].x, allcolors[3].y, allcolors[3].z) ; geom_msgs_num++ ;
*/

//    sprintf(geom_msgs[geom_msgs_num], "After geometry creation numVerts = %d", numVerts) ; geom_msgs_num++ ;
//    sprintf(geom_msgs[geom_msgs_num], "allvertices = %.2f %.2f %.2f  %.2f %.2f %.2f  %.2f %.2f %.2f  ", 
 //       allvertices[0].x, allvertices[0].y, allvertices[0].z, 
  //      allvertices[1].x, allvertices[1].y, allvertices[1].z, 
   //     allvertices[2].x, allvertices[2].y, allvertices[2].z 
    //    ) ; geom_msgs_num++ ;
}


/*
    draw_new_octs: 

    Visit every tree node and draw a point in the middle of nodes containing 
    geometry. 

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
    d = 0 ;     // We start at the root. 
    ivec pos( 0, 0, 0 );

    //geom_msgs_num = 0 ;
    int pointcount = 0 ;

    glPointSize(4.0f) ;
    glColor3f(1.0f, 1.0f, 0.0f) ;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// REPLACE ME AND MOVE ME! 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (allVertsVBO>0 && allColorsVBO>0)
    {
        glEnableClientState( GL_VERTEX_ARRAY );                        // Enable Vertex Arrays
        glEnableClientState( GL_COLOR_ARRAY );                        // Enable Vertex Arrays

        glBindBuffer(GL_ARRAY_BUFFER, allVertsVBO );
        glVertexPointer( 3, GL_FLOAT,  0, (char *) NULL );        // Set The Vertex Pointer To The Vertex Buffer

        glBindBuffer(GL_ARRAY_BUFFER, allColorsVBO );
        glColorPointer( 3, GL_FLOAT,  0, (char *) NULL );        // Set The Vertex Pointer To The Vertex Buffer

        // DRAW LIKE AN ALMIGHTY GOD
        // glDrawArrays( GL_TRIANGLES, 0, numVerts/3);    // Draw All Of The Triangles At Once
        // glEnable( GL_DEPTH_TEST ) ;
        glEnable( GL_CULL_FACE ) ;
        glCullFace( GL_BACK ) ;
        glFrontFace( GL_CCW ) ;
        //glFrontFace( GL_CW ) ;
        glDrawArrays( GL_TRIANGLES, 0, numVerts);    // Draw All Of The Triangles At Once
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Disable Pointers
        glDisableClientState( GL_VERTEX_ARRAY );                    // Disable Vertex Arrays
        glDisableClientState( GL_COLOR_ARRAY );                        // Enable Vertex Arrays

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// REPLACE ME AND MOVE ME! 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
    glBegin( GL_POINTS ) ;
*/

/* tree traversal below is 54 lines */
/*
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
                incr = (1<<(SI-d)) ;

                loopi(3) {
                    pos.v[i] += (incr);
                }

                glVertex3iv(pos.v) ;

                loopi(3) {
                    pos.v[i] -= (incr);
                }
            }
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        if (d<0)    // Past the root? Then we're done. 
        {   
            break ;
        }
        CN = path[d] ;

        loopi(3) {
            incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ;
            pos.v[i] -= yesorno * incr ;
        }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    }                   // end while d>=0
    glEnd() ;
*/

}   // end draw_new_octs()

void World::initialize()
{
    world.gridscale = 10 ;
    world.gridsize = 1<<10 ; // In our fake world, this is about 16m. 

    printf("\n\n****************************WORLD SIZE = %d************************************\n\n", world.size ) ;

    // world.root = 0 ;
}

void drawworld()
{
    draw_new_octs() ;

    // world geometry
static int yes = 0 ;
        yes++ ;
    // Now for every geometric set in our world, render! 
    loopv(worldgeometry)
    {
//printf("Here is the shit. %d", i) ;
        Geom* g = worldgeometry[i] ;

        if (yes==60)
        {
            printf("\nHALLO %d worldgeometry has size %d\n", yes, worldgeometry.length()) ;
        }
        if (g->vertVBOid>0 && g->colorVBOid>0)
        {
if (yes==60)
{
    yes = 0 ;
}
//            printf("Here is the shit. ") ;
            glEnableClientState( GL_VERTEX_ARRAY );                        // Enable Vertex Arrays
            glEnableClientState( GL_COLOR_ARRAY );                        // Enable Vertex Arrays

            glBindBuffer(GL_ARRAY_BUFFER, g->vertVBOid);
            glVertexPointer( 3, GL_FLOAT,  0, (char *) NULL );        // Set The Vertex Pointer To The Vertex Buffer

            glBindBuffer(GL_ARRAY_BUFFER, g->colorVBOid);
            glColorPointer( 3, GL_FLOAT,  0, (char *) NULL );        // Set The Vertex Pointer To The Vertex Buffer

            // DRAW LIKE AN ALMIGHTY GOD
            // glDrawArrays( GL_TRIANGLES, 0, numVerts/3);    // Draw All Of The Triangles At Once
            // glEnable( GL_DEPTH_TEST ) ;
            glEnable( GL_CULL_FACE ) ;
            glCullFace( GL_BACK ) ;
            glFrontFace( GL_CCW ) ;
            //glFrontFace( GL_CW ) ;
            glDrawArrays( GL_TRIANGLES, 0, g->numverts);    // Draw All Of The Triangles At Once
            // glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Disable Pointers
            glDisableClientState( GL_VERTEX_ARRAY );                    // Disable Vertex Arrays
            glDisableClientState( GL_COLOR_ARRAY );                        // Enable Vertex Arrays

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}




//__m128 a ;
//int b ;
//float af[4] = { 101.25, 200.75,300.5, 400.5 };
//a = _mm_loadu_ps(af);
//b = _mm_cvtt_ss2si(a);





