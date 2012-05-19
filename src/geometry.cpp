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
extern Engine engine ;

//------------------------------------------------------------------------
//                  GLOBAL VARIABLES
//------------------------------------------------------------------------

//------------------------
//  OPTIONS
//------------------------
bool usetextures = false ; // for testing, we might want to set this to false



// player/user control and status
//vec  front ;
//vec  pos ;
vec  camdir ;

// main renderable geometry data
//vector<Geom*> worldgeoms ;

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
Octant* FindNode(ivec at, int* out_size, int* nscale) ; 
Octant* FindSizedNode(ivec at, int size) ; 
Octant* FindGeom(ivec at, int* out_size, int* nscale) ; 

int numchildren = 0 ;

// The main high-level functions of the geometry module
void update_editor() ;
void extrude( void* _in ) ;
void FlagSubtreeForUpdate( Octant* parent) ;
void FlagPathForUpdate( Octant** path, int depth) ;
void SubdivideOctant( Octant* oct ) ;
bool CubeInsideWorld( 
    ivec cp /* corner position*/, 
    World& w /* world to check against */
    ) ;

bool FaceCovered(Octant* node, int face) ;
void AnalyzeGeometry(Octant* tree, ivec corner, int scale) ;
void AssignNewVBOs(Octant* tree, ivec corner, int scale) ;
void makeSubtreeVBO(Octant* parent, ivec corner, int NS) ;
void UploadGeomToGfx(Geom* g) ;
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

static Geom hello ; // This is a placeholder to give a non-null value to the GEOM_NEED_UPDATE pointer. 
static Geom* GEOM_NEED_UPDATE = &hello ; // dummy used to signal that a node needs its geom rebuilt. 


// create a node with nothing in it
Octant::Octant()
{
    children = NULL ;
    geom = GEOM_NEED_UPDATE ;
    set_all_edges(0) ;
    vc = 0 ;
    flags = EMPTY ;
    loopi(6) tex[i] = -1 ;
}

bool Octant::has_geometry() 
{
    return (
        (edgegroups[0]!=0) &&
        (edgegroups[1]!=0) &&
        (edgegroups[2]!=0)
        ) ;
}

void Octant::clear_geometry()
{
    loopi(3) { edgegroups[i] = 0 ;} 
}
void Octant::clear_children()
{
    delete [] children ;
    children = NULL ;
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

void Octant::clear_all()
{
    if ( geom )
    {
        clear_geom() ; // recycle this node's geom
    }
    if (children)
    {
        clear_children() ;
    }
    if (has_geometry())
    {
        clear_geometry() ; numchildren-- ;
    }
    loopi(6) tex[i] = -1 ;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------
//                  EDITING VARIABLES AND FUNCTIONS
//------------------------------------------------------------------------

int orientation = 0 ;
#define o orientation 

int targetsize ;
ivec ifront ;   // used to remember the location where a ray hits a node
ivec icorner ;
vec corner( 0, 0, 0 ) ;

bool havetarget = false ;
bool havesel = false ;
bool havesel_end = false ;

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

Geom* currentgeom = NULL ;

// About the next two arrays and groups of defines: 
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
int orientation_indexes[6][3] = 
{
    {1,2,0}, 
    {1,2,0}, 
    {0,2,1}, 
    {0,2,1}, 
    {0,1,2}, 
    {0,1,2} 
} ; 
#define X(_o) orientation_indexes[_o][0]
#define Y(_o) orientation_indexes[_o][1]
#define Z(_o) orientation_indexes[_o][2]


float direction_multipliers[6][3] = 
{
    {-1, 1, -1}, 
    { 1, 1,  1}, 
    { 1, 1, -1}, 
    {-1, 1,  1}, 
    {-1, 1, -1},    // +ve Y is 'up', -ve X is 'right'
    { 1, 1,  1}
} ; 
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
    if (havetarget)
    {
        sel_start   = ifront ;
        sel_end     = ifront ;
        sel_start[o>>1] += NS*(o%2) ;
        sel_end[o>>1] += NS*(o%2) ;
    }
    else
    {
        sel_start   = corner ;
        sel_end     = corner ;
    }
    sel_counts = vec(1,1,1) ;
    sel_o = o ;
    sel_size = world.gridsize ;

    printf("\nselection starting at %d  %d  %d\n", sel_start.x, sel_start.y, sel_start.z ) ; 
}


void set_sel_end()
{
    int NS = world.gridsize ;
    if (!havesel || o!=sel_o ) 
    {
        set_sel_start() ;
        return ;
    }
    havesel_end = true ;

    // Here we say if we're pointing at something, then the place we are targeting 
    // is obtained from where our ray was last pointing. Otherwise, we say our 
    // target area will be where the ray touched the edge of the world. 
    if (havetarget) { sel_end = ifront ; sel_end[o>>1] += NS*(o%2) ; }
    else { sel_end = corner ; }
    sel_size = world.gridsize ;
    loopi(3)
    {
        sel_counts[i] = (( max(sel_start[i], sel_end[i]) - min(sel_start[i], sel_end[i]) ) >> w.gridscale ) + 1 ;
    }
}


// Oriented square centered on a point
void draw_square(
    ivec & center    /* center */,
    int size,       /* size of box */
    int _o           /* orientation */
    )
{
    ivec c(center) ;
    c[X(_o)] -= Dx(_o)*(size>>1) ;
    c[Y(_o)] -= Dy(_o)*(size>>1) ; // now positioned at 'bottom-left' corner. 

    glColor3f( 1, 1, 0 ) ;

    glBegin( GL_LINE_LOOP ) ;
        glVertex3iv( c.v ) ; c[X(_o)] += Dx(_o)*size ;  // bottom-right
        glVertex3iv( c.v ) ; c[Y(_o)] += Dy(_o)*size ;  // top-right
        glVertex3iv( c.v ) ; c[X(_o)] -= Dx(_o)*size ;  // top-left
        glVertex3iv( c.v ) ;
    glEnd() ;
}


/*
    Draw a square that takes 'corner' as its lower-left corner. 

    Integer version. 

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
    draw_corner_squarei( c, size, _o ) ;
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
        drawn_corner[o>>1] += NS*(o%2) ;
        if (havesel) { // lifts the highlight off-surface so that the selection highlight isn't rendered at the same spot. 
            drawn_corner[o>>1] += Dz(o)*NS/40; 
        }
        else {
            drawn_corner[o>>1] += Dz(o)*NS/100 ; // no selection - less lift
        }
    } 
    // from ray hitting world boundary 
    else { drawn_corner = corner ; }
    draw_corner_squarei( drawn_corner, world.gridsize, o) ;
}


void draw_sel_start()
{
    if ( !havesel ) return ;
    glLineWidth( 5.0 ) ;
    glColor3f( 1, 0, 0 ) ;
    ivec v( sel_start ) ;
    v[sel_o>>1] += Dz(sel_o)*10 ; // lets our square highlight float at the right distance from the cube's corner
    draw_corner_squarei( v, sel_size, sel_o) ;
    glLineWidth( 1.0 ) ;
}


void draw_sel_end()
{
    if ( !havesel_end ) return ;
    glLineWidth( 5.0 ) ;
    glColor3f( 0, 1, 1 ) ;
    ivec v( sel_end ) ;
    v[sel_o>>1] += Dz(sel_o)*10 ; // lets our square highlight float at the right distance from the cube's corner
    draw_corner_squarei( v, sel_size, sel_o) ;
    glLineWidth( 1.0 ) ;
}

void draw_selection() 
{
    if ( !havesel ) return ;
    ivec deltas(
        abs(sel_start.x - sel_end.x),
        abs(sel_start.y - sel_end.y),
        abs(sel_start.z - sel_end.z)
        ) ;
    ivec c( // c == corner
        min(sel_start.x,sel_end.x),
        min(sel_start.y,sel_end.y),
        min(sel_start.z,sel_end.z)
        ) ;

        c[Z(sel_o)] += Dz(sel_o)*10 ;

    glColor3f( 1, 1, 1 ) ;
    glBegin( GL_LINE_LOOP ) ;
        glVertex3iv( c.v ) ; c[X(sel_o)] += deltas[X(sel_o)]+sel_size;  // bottom-right (could be bottom left but it's symmetrical)
        glVertex3iv( c.v ) ; c[Y(sel_o)] += deltas[Y(sel_o)]+sel_size;  // top-right
        glVertex3iv( c.v ) ; c[X(sel_o)] -= deltas[X(sel_o)]+sel_size;  // top-left
        glVertex3iv( c.v ) ;
    glEnd() ;
    draw_sel_start() ;
    draw_sel_end() ;
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
    ivec center( 0, half, half ) ;
    draw_square(center, world.size, 0) ; center[0] += half ; center[1] += half ;
    draw_square(center, world.size, 3) ; center[0] += half ; center[1] -= half ;
    draw_square(center, world.size, 1) ; center[0] -= half ; center[1] -= half ;
    draw_square(center, world.size, 2) ;
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
    denominator =              camdir.dot( pl.v )  ;
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
    den =              camdir.dot( pl.v )  ;
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

//    f.add(ray) ;        // f is now at ray front

    ivec ifr ;          // int vector to track which node a ray front is in. ifr == 'int front'
    ivec ic ;           // int vector to track the corner of the node we're in. ic == 'int corner'

    float r = 0.f ; // rate
    float d = 0.f ; float t = 0.f ; 

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
                //t=0.f ; 
                break ;
            }  // If this adjustment brings us out of the world - we're done
        } ; 
        Octant* oct = FindNode(ifr, &NS, &Nscale) ; // Find out what tree node encloses this point
        if ( oct->has_geometry() )                  // Target acquired. 
        { 
            *loc = f ;
            return t ; 
        }
        ic = ifr ;
        loopj(3) { ic[j] = (ic[j] >> Nscale) << Nscale ; }    // Now ic is right on the node corner. 
        // Distances to next plane intersections
        loopj(3) { ds[j] = (ray[j]>=0?(float(ic[j]+NS)-f[j]):(f[j]-float(ic[j]))) ; }

        // Which plane are we going to hit next, given our position and the ray orientation? 
        if (fabs(ray.x*ds.y) > fabs(ray.y*ds.x)) 
        { r = ray.x ; d = ds.x ; i = 0 ; }
        else 
        { r = ray.y ; d = ds.y ; i = 1 ; }
        if ((fabs(ray.z*d) > fabs(r*ds.z))) 
        { r = ray.z ; d = ds.z ; i = 2 ; }

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
    vec front ; // ray front


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
                        o = (hittingplane) ; min_t = t ;
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
                if (!( (o==2*j) || (o==2*j+1) ))
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
    ray_start_orientation = o ;

    float r = 0.f ;
    float d = 0.f ;
    t = 0.f ; // divisions are minimized 
    int i = 0 ; // here we'll use i to track which axis is used to change nodes: 0:X, 1:Y, 2:Z. 

    int steps = 0 ;
    int NS = 0 ; // target node size
    int Nscale = 0; 

    // RAY SHOOTING THROUGH THE WORLD! ZAP!
    int WS = world.size ;
    havetarget = false ;

    Octant* aim = NULL ;
    if (have_ray_start_node || camera.inworld(world))
    {
        i = (o>>1) ; // Which axis dominates the ray's movement into the next node

        vec ds = vec(0) ; 
        while (!havetarget)
        {
            if (steps>50) { break ; }                  // Kill runaway loops. Bigger than 50 needed to be able to point to large distances. 
            loopj(3) {ifront[j] = (int)rayfront[j] ;} // place, more or less, ifront at rayfront (imprecision from converting float to int)
            ifront[i] += (ray[i]>=0?1:-1) ; // Snap to inside of node we're looking at. This deliberately takes us off node boundaries. 
            if ((ifront[i]>=WS&&ray[i]>0) || (ifront[i]<=0&&ray[i]<0)) 
            {
                
sprintf(geom_msgs2[geom_msgs_num2], "RAY LEAVING WORLD") ; geom_msgs_num2++ ;
                break ;
            }  // If this adjustment brings us out of the world - we're done

            Octant* oct = FindNode(ifront, &NS, &Nscale) ;                 // Find out what tree node encloses this point

            icorner = ifront ;
            loopj(3) { icorner[j] = (icorner[j] >> Nscale) << Nscale ; }    // Now icorner is right on the node corner. 

            if ( oct->has_geometry() ) 
            { 
                havetarget = true ; 
                aim = oct ;
                targetsize = NS ; 
sprintf(geom_msgs2[geom_msgs_num2], "HAVE TARGET: icorner at %d %d %d targetsize=%d", icorner[0], icorner[1], icorner[2], targetsize) ; geom_msgs_num2++ ;
                break ; 
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


    hitplanes = ( dir[i]>0 ? 2*i : 2*i+1 ) << (3*i) ;
    o = hittingplane ;

    // If we're pointing at something and we want to do something with it, now's the time to do it. 
    if (havetarget)
    { 
//FindGeom(ifront, &NS, &Nscale) ; 
        sprintf( geom_msgs2[geom_msgs_num2], "") ; geom_msgs_num2++ ;
        sprintf( geom_msgs2[geom_msgs_num2], "NODE TARGETED. selection corner at: %d %d %d    (%d steps) ORIENTATION=%d", 
        icorner.x, icorner.y, icorner.z, steps, o) ; geom_msgs_num2++ ;
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
                        o = (hittingplane) ;
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
                if (!( (o==2*j) || (o==2*j+1) ))
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
sprintf(geom_msgs2[geom_msgs_num2], "NUMBER OF GEOMS = %d", worldgeometry.length()) ; geom_msgs_num2++ ;
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
} // end update_editor 



/*
    Function: clear_geom 

    Used by the deletesubtree function. 

    As a process is recycling a tree, it calls 
    this function on its Geom element so that 
    the memory can then be discarded without 
    loose ends. 

    Purpose: to release the OpenGL buffer objects that this 
    node is using. 
*/
void Octant::clear_geom() 
{
    //if (!oct) return ;          // FIXME: in professional code, an error would be logged here
    if (!geom) return  ;   // FIXME: in professional code, an error would be logged here
    if (geom==GEOM_NEED_UPDATE) return  ;   // FIXME: in professional code, an error would be logged here

    if (glIsBuffer((geom->vertVBOid))) { glDeleteBuffers(1,&(geom->vertVBOid)) ;}
    if (glIsBuffer((geom->texVBOid))) { glDeleteBuffers(1,&(geom->texVBOid)) ;}
    if (glIsBuffer((geom->colorVBOid))) { glDeleteBuffers(1,&(geom->colorVBOid)) ;}

    worldgeometry.removeobj(geom) ;
    delete geom ;
    geom = NULL ;
}

/*
    Function: deletesubtree. 

    Purpose: to remove all descendants and their contents a node may have. 
    
    It always proceeding depth-first and then upwards by child index to find
    leaves with no children.

    Typical usage: when content needs to be removed from a portion 
    of the tree, it can be removed by using the nodes whose combined 
    volume exactly encompasses this content. 

*/
void deletesubtree(Octant* in_oct)
{
    int32_t d = 0 ;             // depth
    Octant* CN = in_oct ;       // current node 
    Octant* path[20] = {NULL} ;
    path[0] = CN ; // This is and always will be the root. 
    int32_t idxs[20] = { 0 } ;   // idxs[d] tracks which child of node path[d] is being used, if any. 
//    loopi(20) { idxs[i] = 0 ; }

    d = 0 ; // We start at the root. 
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
        
        if (CN)
        {
            CN->clear_all() ;
            CN->geom = GEOM_NEED_UPDATE ;
        }
        path[d] = NULL ;

        // Done with last node; go back up the tree. 
        d-- ;
        if (d<0) { break ; }
        CN = path[d] ;
        idxs[d]++ ; 
    }
}


// Finds the smallest node enclosing the supplied integer vector. 

/*
    Function: FindNode

    Purpose: 
        Locate a leaf node that contains the given point. 
        
        If the point given
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
Octant* FindNode(ivec at, int* out_size=NULL, int* nScale=NULL) // , int* out_size) 
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

    if (out_size!=NULL) 
    if (!oct) { *out_size = 0 ; }
    else { *out_size = 2<<CGS ; }

    return oct ;
}

/*
    Finds a node no smaller than size in_size. 

*/
Octant* FindSizedNode(ivec at, int in_size)
{
    int32_t CGS = world.scale ;
    int32_t i = 0 ;
    Octant* oct = &world.root ;

    while ((2<<CGS)>in_size)
    {
        if ( oct->children ) 
        {
            i = octastep(at.x,at.y,at.z,CGS) ;
            oct = &oct->children[i] ;
        }
        else { break ; }
        CGS-- ;
    }

//printf("\nDone finding node. Node is%s null", oct==NULL?(""):(" not")) ;
    return oct ;
}

/*
    Finds the geom, if any, that contains the point
*/
Octant* FindGeom(ivec at, int* out_size, int* nscale) //, int* out_size=NULL) ;
{
    int CS = world.scale ;
    int i = 0 ;
    Octant* oct = &world.root ;

    if (nscale != NULL) { *nscale = CS ; }
    while (CS>2)
    {
        if ( oct->geom )
        {
             currentgeom = oct->geom ;
		        break ; 
        }

        if ( oct->children ) 
		{
		    i = octastep(at.x,at.y,at.z,CS) ;
		    oct = &oct->children[i] ;
		    if (nscale != NULL) { *nscale = CS ; }
		    CS-- ;
		}
    }

    if (out_size!=NULL) 
    if (!oct) { *out_size = 0 ; }
    else { *out_size = 2<<CS ; }

    return oct ;
}

/*
    This function marks a subtree as requiring update. 

    It is used when a node is being flagged for update, and 
    is seen to possess its own geom - which means all nodes 
    under this node need to be taken into account on the 
    next pass to reconstruct the geometry under this node. 

*/
void FlagSubtreeForUpdate( Octant* parent) 
{
    Octant* CN = parent ;
    Octant* path[20] ;
    int idxs[20] ;
    loopi(20) { path[i] = NULL ;}
    path[0] = CN ;
    loopi(20) { idxs[i] = 0 ;}

    int d = 0 ;

    while (d>=0) // Here starts iterative traversal
    {
        if ( CN->children )
        {
            if (idxs[d]<8)
            {
                CN = &CN->children[idxs[d]] ; 
                d++ ; path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 
                continue ;
            }
        }

        if ( CN->geom != NULL )
        {
            if (CN->geom != GEOM_NEED_UPDATE )
            {
                CN->clear_geom() ;
            }
        }
        CN->geom = GEOM_NEED_UPDATE ;
        d-- ;
        if (d<0) { break ; }    // Past the root? Then we're done. 
        CN = path[d] ;          // else go up tree
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    }   // end while d>=0
}


/*
    Function: FlagPathForUpdate. 

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
void FlagPathForUpdate( Octant** path, int _depth) 
{
    int d = _depth ;
    while (d>=0) // Question: what events prevent us from stopping the course upwards when we encounter a GEOM_NEED_UPDATE? 
    {
        if (path[d]->geom != NULL)
        {
            if (path[d]->geom!=GEOM_NEED_UPDATE)
            {
                FlagSubtreeForUpdate( path[d] ) ;
                path[d]->clear_geom() ;
            }
        }
        path[d]->geom = GEOM_NEED_UPDATE ;
        d-- ;
    }
}

/*
    Function: SubdivideOctant
    
    What's happening? 
    
    This is used to take a solid node and break it into 8 or fewer (depending 
    on which volume of its enclosing cube a node occupies) nodes which 
    give the same overall shape and outside textures. 
    
    A typical usage of this function is when a certain cube of space needs 
    to be cleared, and this space happens to be inside a larger node. In 
    order that the entire larger node isn't lost, we subdivide it until only 
    the space being targeted will be removed. 
    
    Parameters: 
        c is the corner of the parent node. 
        oct is the node that will be subdiviced. 
*/
static int facechildindexes[6][4] = 
{
    {0,2,4,6}, 
    {1,3,5,7}, 
    {0,1,4,5}, 
    {2,3,6,7}, 
    {0,1,2,3},
    {4,5,6,7} 
} ;
#define fci facechildindexes

void SubdivideOctant( Octant** path , int depth )
{
    Octant* oct = path[depth] ;
    // give children to this guy. 
    new_octants( oct ) ;
    
    // High-level description: 
    // Using this guy's shape and the textures that it has, give the children 
    // their respective properties so that the outward appearance doesn't change. 
    
    // Steps: 
    // 1. Gather the eight vertices of this cube. 
    Octant* CC = &(oct->children[0]) ;
    loopi(8)
    {
        CC->set_all_edges() ; 
//        loopj(6) { CC->tex[j] = -1 ;}
        numchildren++ ;
        CC++ ;
    }
    // 2. Using interpolation to find the mid-face vertices, generate the 5 new 
    // vertices per face needed to specify the faces of the children. 
    // 3. Build the children's faces with the accumulated vertices. 
         //   int j = 0 ; //  j = //j = ((1%2)<<(face/2)) ; // int j = 1 ; j = ((1%2)<<(face/2)) ; 

    for (int face=0;face<6;face++)
    {
        int k = oct->tex[face] ;
       
        oct->children[fci[face][0]].tex[face] = k ;
        oct->children[fci[face][1]].tex[face] = k ;
        oct->children[fci[face][2]].tex[face] = k ;
        oct->children[fci[face][3]].tex[face] = k ;

        /*
        oct->children[fci[face][0]].tex[face] = -1 ;
        oct->children[fci[face][1]].tex[face] = -1 ;
        oct->children[fci[face][2]].tex[face] = -1 ;
        oct->children[fci[face][3]].tex[face] = -1 ;
        */
    }
    // 4. assign to all the faces concerned the tex slots currently used 
    // by the parent node. 
    // 5. flag this subtree as needing update. 

    // cancel the parent's geometry at this level, now that the children have it. 
    oct->clear_geometry() ;
}

bool CubeInsideWorld( 
    ivec cp /* corner position*/, 
    World& w /* world to check against */
    )
{
    bool insideworld = true ;
    loopi(3) { if (cp[i]<0 || cp[i]==w.size) { insideworld = false ; break ; } }
    return insideworld ;
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
void extrude( void * _in )
{
    if (!havesel) return ;

    bool in = *(bool *)_in ;

    int O = (sel_o) ;               // orientation
    int x = X(O) ;    int y = Y(O) ;    int z = Z(O) ;
    int WS = world.scale ;
    int GS = world.gridsize ;
    int WGSc = world.gridscale ;
    int CGS = WS ;                  // current grid scale
    // vectormap vmap ;                // used to record new nodes and then to construct and update vector arrays

    // define a range of cubes that will be created, bounded by 
    // sel_min and sel_max. These will bound the same rectangle 
    // as sel_start and sel_end, but with easier-to-compute properties. 
    loopi(3)
    {
        sel_min[i] = min( sel_end[i], sel_start[i] ) ;
        sel_max[i] = max( sel_end[i], sel_start[i] ) ;
    }

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


    Octant* path[20] = {NULL} ;

    NC = sel_min ; // newcube

    // We will cover a larger range of space than just our selection - so that neighboring 
    // elements will know if they are supposed to update themselves (when faces become 
    // visible or obscured as a result of an extrusion or deletion operation). 
    int x_count = -1 ;
    int y_count = -1 ;

    Octant* oct = &world.root ;
    path[0] = oct ;
    int depth = 0 ; // first child of root

    // ---------------------------------------------------------------------------
    // DELETION
    // ---------------------------------------------------------------------------


    if ( in )
    {
        if ( (O%2) ){NC[z] -= wp[O][z] * (sel_size) ;}
        if (!CubeInsideWorld(NC, world)) { return ; }
        NC[x] -= 1 ;
        NC[y] -= 1 ;
        ivec NCstart = NC ;

        while ( y_count < sel_counts[y]+1)
        {
            while ( x_count < sel_counts[x]+1)
            {
                bool inside = false ;
                if ( (x_count>=0 && x_count<sel_counts[x]) &&
                     (y_count>=0 && y_count<sel_counts[y]) )
                {
                    inside = true ;
                }
                
                while (CGS>=WGSc)
                {
                    // If we're not done going deeper and we hit geometry, we need to subdivide. 
                    if (!oct->children )
                    {
                        if (inside && oct->has_geometry() )
                        {
                            SubdivideOctant( path, depth ) ;
                        }
                        else { break ; }
                    }

                    int i = octastep(NC.x,NC.y,NC.z,CGS) ;
                    oct = &oct->children[i] ;
                    depth++ ;
                    path[depth] = oct ;
                    CGS-- ;
                }
                
                //if (oct)
                if (inside)
                {
                 //   FlagSubtreeForUpdate(oct) ;
                    if (oct->children){deletesubtree( oct ) ;}
                    if (oct->has_geometry()){oct->clear_geometry() ;}
                }
                FlagPathForUpdate( path, depth) ;
                
                // reset node tracker
                CGS = WS ; 
                depth = 0 ;
                oct = &world.root ;

                // Move to next R^3 position for a new node 
                NC[x] += GS ;
                x_count ++ ;
            } // end while ( x_count < sel_counts[X(O)])
            x_count = -1 ;
            y_count ++ ;
            NC[x] = NCstart[x] ;
            NC[y] += GS ;
        } // end while ( y_count < sel_counts.y )
        
//printf("\nNext phase in life. . ") ;
        
        // Now do a similar process of flagging for update (no tree modifications 
        // after this point) for the surface that is ahead of the deletion area.
        NCstart[x] += 1 ;
        NCstart[y] += 1 ;
        NCstart[z] -= Dz(O)*GS ;
        NC = NCstart ;
        x_count = 0 ;
        y_count = 0 ;

        while ( y_count < sel_counts[y])
        {
            while ( x_count < sel_counts[x])
            {
                while (CGS>=WGSc)
                {
                    if (!oct->children)
                    {
                        break ;
                    }
                    int i = octastep(NC.x,NC.y,NC.z,CGS) ;
                    oct = &oct->children[i] ;
                    depth++ ;
                    path[depth] = oct ;
                    CGS-- ;
                }
                FlagPathForUpdate( path, depth) ;

                CGS = WS ; 
                depth = 0 ;
                oct = &world.root ;

                NC[x] += GS ;
                x_count ++ ;
            }
            x_count = 0 ;
            y_count ++ ;
            NC[x] = NCstart[x] ;
            NC[y] += GS ;
        } // end while ( y_count < sel_counts.y )
        
        // Move selection highlights to match new additions. 
        sel_end  [sel_o>>1] -= GS*Dz(O) ;
        sel_start[sel_o>>1] = sel_end  [sel_o>>1] ; // GS*Dz(O) ;

    }   // end if deleting nodes 
    // ---------------------------------------------------------------------------
    // EXTRUSION
    // ---------------------------------------------------------------------------
    else
    {
        // 'pop out' from reference point if orientation is even
        // geometry from coordinates where to do so means to decrement our coords. 
        if ( !(O%2) ){NC[z] += wp[O][z] * (sel_size) ;}
        // If the new cube coordinates exceed world limits, then we forget it
        if (!CubeInsideWorld(NC, world)){return ;}
        NC[x] -= 1 ;
        NC[y] -= 1 ;
        ivec NCstart = NC ;
        while ( y_count < sel_counts[y]+1 )
        {
            while ( x_count < sel_counts[x]+1 )
            {
                bool inside = false ;
                if ((x_count>=0 && x_count<sel_counts[x])&&
                    (y_count>=0 && y_count<sel_counts[y]))
                {
                    inside = true ;
                }
                while (CGS>=WGSc)
                {
                    int i = octastep(NC.x,NC.y,NC.z,CGS) ;
                    if ( !oct->children )
                    {
                        if (inside) 
                        { 
                            new_octants(oct) ;
                        }
                        else{break ;}
                        
                    }
                    oct = &oct->children[i] ;
                    depth++ ;
                    path[depth] = oct ;
                    CGS-- ;
                }
                
                if (oct){
                    if (inside){
                        if (oct->children){deletesubtree( oct ) ;}
                        oct->set_all_edges() ; numchildren++ ;
                    }
                    FlagPathForUpdate( path, depth) ;
                }
                // Reset variables for any subsequent new nodes. 
                CGS = WS ; 
                depth = 0 ;
// possible optimization: instead of going back up to the root, only go back up as far as necessary
// 7 times out of 8 (ok, maybe 4.5 out of 8) the trip would be super short. 
                oct = &world.root ; 

                // Move to next position for a new node 
                NC[x] += GS ;
                x_count ++ ;
            } // end while ( x_count < sel_counts.x )
            x_count = -1 ;
            y_count ++ ;
            NC[x] = NCstart[x] ;
            NC[y] += GS ;
        } 
        
        // Now flag the area that potentially just got obscured. 
        x_count = 0 ;
        y_count = 0 ;
        NCstart[x] += 1 ;
        NCstart[y] += 1 ;
        NCstart[z] -= Dz(O)*GS ;
        NC = NCstart ;
        
        while ( y_count < sel_counts[y] )
        {
            while ( x_count < sel_counts[x] )
            {
                while (CGS>=WGSc)
                {
                    int i = octastep(NC.x,NC.y,NC.z,CGS) ;
                    if ( !oct->children ){break ;}
                    oct = &oct->children[i] ;
                    depth++ ;
                        path[depth] = oct ;
                        CGS-- ;
                    }
                    FlagPathForUpdate( path, depth) ;
                    CGS = WS ; 
                    depth = 0 ;
                    oct = &world.root ; 

                    NC[x] += GS ;
                    x_count ++ ;
            }
            x_count = 0 ;
            y_count ++ ;
            NC[x] = NCstart[x] ;
            NC[y] += GS ;
        }
        // Move selection highlights to match new additions. 
        sel_start[sel_o>>1] += GS*Dz(O) ;
        sel_end  [sel_o>>1] += GS*Dz(O) ;
            
    }   // end extrusion

    // Identify pieces that need to be rebuilt
    AnalyzeGeometry(&world.root, vec(0,0,0), world.scale) ;

    // Rebuild updated geometry
    AssignNewVBOs(&world.root, vec(0,0,0),world.scale) ;

// FIXME monitor total number of triangles 
} // end extrude 



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

    while (d>=0) 
    {
        if ( CN->geom==GEOM_NEED_UPDATE)
        {
            if ( CN->children )
            {
                if (idxs[d]<8)
                {
                    // We're going down to a child, marked relative to its parent by idxs[d]. 
                    loopi(3) {
                        incr = (1<<(S-d)) ; yesorno = ((idxs[d]>>i)&1) ;
                        pos.v[i] += yesorno * incr ;
                    }
                    CN = &CN->children[idxs[d]] ; 
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
                    loopi(8) { sum += CC->vc ; CC++ ; }
                    CN->vc = sum ;
                }
            }
            // If we don't have children, then maybe we have geometry.
            else
            {
                if ( CN->has_geometry() )
                {
                    ivec pcorner ;          // probe corner
                    Octant* anode ;         // probe node
                    int NS = 1<<(S-d+1) ;   // present Node size. 
//                    int ns = 0 ;            // neighbor size

//--------------------------------------------------------------------------------------
                    // face visibility
                    CN->vc = 0 ;
                    for (int face=0;face<6;face++)
                    {
                        int O = face ;
                        pcorner = pos ;
                        /* We position the probe vector (pcorner) to the inside of the node adjacent in the direction of the current face.  Then we retrieve from the tree the node at that location.  Once we have 'anode', or the node adjacent to the current face, we know whether it is solid, and whether it is bigger than us.  */
                        pcorner[face>>1] = pos[face>>1]+Dz(face)*(1 + (face%2)*(NS)) ; 
                        if (pcorner.x>world.size||pcorner.x<0||
                            pcorner.y>world.size||pcorner.y<0||
                            pcorner.z>world.size||pcorner.z<0)
                        {
                            CN->tex[face] = -1 ;
                            continue ;
                        }
                        //anode = FindNode(pcorner, &ns) ;
                        anode = FindSizedNode(pcorner, NS) ;


                        // If the adjacent node is bigger than us and has geometry - don't show this face. 
                        // FIXME: add checking if the face is flush to our face. 
                        if ( anode->has_geometry() ) 
                        {
                            // avoid this face! 
                            CN->tex[face] = -1 ;
                        }
                        else
                        {
//printf("\nProcessing node at %d %d %d", pcorner.x, pcorner.y, pcorner.z) ;
                            if 
                            (
//                    ( !anode->has_geometry() )
//                    &&
                                    /* 
                                    FIXME - 
                                   If we want to see whether face 1 is covered, we 
                                   check face 0, the face opposite to this one, in 
                                   our neighbor node. Each of the number pairs that match 
                                   a face and its neighbor-opposite share the property that 
                                   all bits but the first one (2^0) are the same. So to switch 
                                   between either of any pair, we just flip the smallest bit. 
                                    0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc. 
                                    - FIXME
                                    */
                                ( !FaceCovered(anode, (face&0x6|((~face)&0x1))) ) 
                            )
                            {
                                // FIXME: add a check for whether a face is covered even though it is covered by 
                                // smaller nodes. 

                                //  texture slot assignment: Current texture or default==1
                                //  printf("\n assigning %d as current face tex. ", engine.texids[engine.tex]) ;
                                //  CN->tex[face] = engine.texids[engine.tex] ;
                                //if (CN->tex[face] == -1)  // setting this to 'null' in case we don't want to see this face. 
                                {
                                    CN->tex[face] = engine.tex ;
                                }
                                CN->vc += 6 ;
                            }   // end if neighbor is not in the way of this face
                            

                        }   // end for (int face=0;face<6;face++)

                        pcorner = pos ; // do we need to reset this here?

                    }   // end for every face
//--------------------------------------------------------------------------------------
                }           // end if ( CN->has_geometry() )
            }               // end if not have children
        }                   // end if need update

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ; 
        // Past the root? Then we're done. 
        if (d<0) { break ; }
        CN = path[d] ;

        loopi(3) {incr = (1<<(S-d)) ; yesorno = ((idxs[d]>>i)&1) ;pos.v[i] -= yesorno * incr ;}
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 

    }  // end while d>=0
}


/*
    This function traverses the tree, following nodes that are marked for 
    update. 

    When it finds a node that has vc<=256, and has a larger vc than all 
    its children, then it makes a vbo from it. 
*/
void AssignNewVBOs(Octant* tree, ivec in_corner, int scale)
{
    int32_t d = 0 ;                         // depth
    int32_t S = scale ;                 // Scale of this world
    Octant* CN = &world.root ;    // current node 
    Octant* CC = NULL ;                 // current child 

    int32_t idxs[20] ;   // Used to track path indexes. -1 means unused. 
    loopi(20) {idxs[i] = 0 ;} idxs[0] = 0 ;
    Octant* path[20] ;   // Used to track path indexes. -1 means unused. 
    loopi(20) {path[i]=NULL ;} path[0] = CN ; 

    // These are used to state positions which tell us where things are located when we call makeSubtreeVBO.
    int32_t yesorno = 0 ;
    int32_t incr = 0 ;
    ivec pos = in_corner ;

    while (d>=0)
    {
        if ( CN->geom==GEOM_NEED_UPDATE )
        {
            if ( CN->children && CN->vc>0)
            {
                // Ok we have children. Either we make a geom here and now, or we keep going down our children. 
                Octant* CC = &CN->children[0] ;
                bool useThisNode = false ;
                //if (CN->vc<=256)
                if (CN->vc<=256)
                {
                    useThisNode = true ;
                    loopi(8)
                    {
                        if (CC->vc==CN->vc) // If a child holds all the same geometry as me, then that child can host that geometry
                        { useThisNode = false ; break ; }
                        CC++ ;
                    }
                }

                if (useThisNode)
                {
                    makeSubtreeVBO( CN, pos, S-d) ; // FIXME: move this to phase 3 
                }
                else 
                if (idxs[d]<8) // We go down the tree if this node has a child that contains everything (which is the case if useThisNode was false)
                {
                    incr = (1<<(S-d)) ; 
                    loopi(3) { yesorno = ((idxs[d]>>i)&1) ; pos.v[i] += yesorno * incr ; }
                    
                    CN = &CN->children[idxs[d]] ; 
                    d++ ;
                    path[d] = CN ;
                    idxs[d] = 0 ; // Start the children at this level. 
                    continue ;
                }
            } // end if children 
            else // If we don't have children, then maybe we have geometry.
            {
                // If we get to here, then it's time to build this. 
                if ( CN->has_geometry() )
                {
                    makeSubtreeVBO( CN, pos, S-d) ; // FIXME: move this to phase 3 
                }
            }
        } // end if ( CN->geom==GEOM_NEED_UPDATE )

        // At this point, we're done with the present node, so we cancel its need for update if it's there. 
        // These last lines of the while loop make up the 'going up the tree' action. 
        if (CN->geom == GEOM_NEED_UPDATE) { CN->geom = NULL ; }

        // back up the tree
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


// FIXME! do we need a max? how do we decide what it is? 
#define MAX_VERTS 1000000

// Dim colors so that untextured geometry looks like ass
//float mult = 0.6f ;
float mult = 1 ;
vec colors[10] = {
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

static int colorNow = 0 ;

/*
    It makes the 'parent' parameter Octant the host of a Geom which describes 
    the geometry under this parent. 
    
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
extern uint texid ;
void makeSubtreeVBO(Octant* root, ivec in_corner, int _NS)
{
    Octant* CN = root ;
    if (CN->geom==NULL||CN->geom==GEOM_NEED_UPDATE) 
    {
        CN->geom = new Geom() ;
    }
    
    int d = 0 ;
    Octant* path[20] ;
    int32_t idxs[20] ;
    ivec pos = in_corner ;

//    vec* verts = CN->geom->vertices ;
    ivec* verts = CN->geom->vertices ;
    vec* colors = CN->geom->colors ;
    vec* tex = CN->geom->texcoords ;

    int t = 0 ;
    int numverts = 0 ;
    int nv = 0 ;
    int numnodes = 0 ;

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
    colorNow++ ;
    colorNow = colorNow % 10 ;
    while (d>=0)
    {
        if ( CN->children )
        {
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

                // new node: discard any geometry it holds
//                clearx_geom( CN ) ; if (CN->geom==GEOM_NEED_UPDATE) { CN->geom = NULL ;}
                d++ ; 
                path[d] = CN ; 
                idxs[d] = 0 ; // Start the children at this level. 

                continue ;
            }
        }
        // If we don't have children, then maybe we have geometry.
        else
        {
            if ( CN->has_geometry() ) // If we have geometry, that has to go into a VBO
            {
                numnodes ++ ;

                ivec pcorner ;          // probe corner
                Octant* pnode ;         // probe node
                int NS = 1<<(SI-d+1) ;  // Node size. 
                int ns = 0 ;            // neighbor size

                // A cube is defined by 8 vertices. 
                // The faces of the cube are defined by which of these vertices
                // they are constructed with, and here we have the indexes that 
                // refer to these vertices. 
                static int faceindexes[] =
                {
                    0, 1, 2, 3, // face 0 CCW from origin
                    4, 5, 6, 7, // face 1 CCW from face bottom-left
                    7, 6, 1, 0, // face 2
                    3, 2, 5, 4, // face 3
                    0, 3, 4, 7, // face 4
                    6, 5, 2, 1, // face 5
                } ;
                #define fi faceindexes
                static ivec corners[8] ;
                //FIXME: only compute corners if at least one face needs to be drawn
                //FIXME: when edge computations are properly implemented, set corner values to 
                // reflect edge sizes. (this will allow non-cube geometry - yippee!)
                
               
                for (int face=0;face<6;face++)
                {
                    if (CN->tex[face]>=0)
                    {
                        pcorner = pos ; 
                        corners[0] = pcorner ; 
                        corners[1] = pcorner ; corners[1][2] += NS ;
                        corners[2] = pcorner ; corners[2][1] += NS ; corners[2][2] += NS ; 
                        corners[3] = pcorner ; corners[3][1] += NS ; 

                        corners[4] = pcorner ; corners[4][0] += NS ; corners[4][1] += NS ; 
                        corners[5] = pcorner ; corners[5][0] += NS ; corners[5][1] += NS ; corners[5][2] += NS ; 
                        corners[6] = pcorner ; corners[6][0] += NS ; corners[6][2] += NS ;
                        corners[7] = pcorner ; corners[7][0] += NS ;
                        break ;
                    }
                }


                for (int face=0;face<6;face++)
                {
                    // If this face is visible, then make some triangles for it. 
                    t = CN->tex[face] ;
                    if (t>=0)
                    {
                        float tc = ((float)t+0.5)/(float)engine.numtex ;
                        // Face Builder. 
                        // Every face is defined with: 
                        /*
                            A ++++ D
                              + /+
                              +/ +
                            B ++++ C

                            (Here the sequence ABCD is used to define the four corners 
                            of a texture that will be used to paint this face.)
                            
                            Triangle 1: vertices = ABD  texcoords: (0,0) (0,1) (1,0)  
                            Triangle 2: vertices = DBC  texcoords: (1,0) (0,1) (1,0)  
                        */

                        float x0 = (float)corners[fi[face*4+2]][X(face)]/128 ; // scaled so that the full-size texture is 128 units tall and wide. 
                        float x1 = (float)corners[fi[face*4]][X(face)]/128 ;
                        float y0 = (float)corners[fi[face*4+2]][Y(face)]/128 ;
                        float y1 = (float)corners[fi[face*4]][Y(face)]/128 ;


                        // triangle 1 
                        tex[nv]   = vec(x0,-y0,tc) ; verts[nv]   = corners[fi[face*4+2]] ;
                        tex[nv+1] = vec(x0,-y1,tc) ; verts[nv+1] = corners[fi[face*4+3]] ;
                        tex[nv+2] = vec(x1,-y0,tc) ; verts[nv+2] = corners[fi[face*4+1]] ;
                        
                        // triangle 2 
                        tex[nv+3] = vec(x1,-y0,tc) ; verts[nv+3] = corners[fi[face*4+1]] ;
                        tex[nv+4] = vec(x0,-y1,tc) ; verts[nv+4] = corners[fi[face*4+3]] ;
                        tex[nv+5] = vec(x1,-y1,tc) ; verts[nv+5] = corners[fi[face*4]] ;
                        
                        nv += 6 ;    //  Two new triangles for this face means 6 new vertices
                    } // end if (CN->tex[face]>0) (if face is visible)
                } // end loop over 6 faces
            } // end if has geometry 
        }

        // up the tree
        path[d] = NULL ;
        d-- ;
        if (d<0) { break ; }
        // reset our corner position
        incr = (1<<(SI-d)) ;
        loopi(3) { incornot = ((idxs[d]>>i)&1) ; pos.v[i] -= incornot * incr ; }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
        CN = path[d] ;
    }// end while d>=0
    
    //CN->geom->numverts = numverts ;
    CN->geom->numverts = nv ;

    if (!glIsBuffer(CN->geom->vertVBOid)) { glGenBuffers( 1, &(CN->geom->vertVBOid)) ; }
    glBindBuffer( GL_ARRAY_BUFFER, CN->geom->vertVBOid) ;
    glBufferData( GL_ARRAY_BUFFER, nv*sizeof(vec), verts, GL_STATIC_DRAW );

    if (!glIsBuffer(CN->geom->texVBOid)) { glGenBuffers( 1, &(CN->geom->texVBOid)) ; }
    glBindBuffer( GL_ARRAY_BUFFER, (CN->geom->texVBOid));            // Bind The Buffer
    glBufferData( GL_ARRAY_BUFFER, nv*sizeof(vec), tex, GL_STATIC_DRAW );

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    worldgeometry.add(CN->geom) ;
}


// Array: face indexes. 
// Specifies which four indexes turn a given face of a an octree node into a 
// quadtree. 
int faceidxs[24] = 
{
    0, 2, 4, 6, // face 0
    1, 3, 5, 7, // face 1
    0, 1, 4, 5, // face 2
    2, 3, 6, 7, // face 3
    0, 1, 2, 3, // face 4
    4, 5, 6, 7  // face 5
} ;

/*  
    Performs a face-restricted traversal of the tree. If 'face' 
    corresponds to the cube side whose normal is the x axis, 
    then only nodes in the higher-x set of 4 children get 
    traversed. This actually turns the octree into a sort 
    of quadtree, for a given face of the octree. 
*/
bool FaceCovered(Octant* node, int face)
{
    int32_t d = 0 ;

    int idxs[20] ;
    loopi(20) {idxs[i]=0 ;}
    Octant* path[20] ;
    loopi(20) {path[i]=NULL ;}

    Octant* CN = node ;
    path[0] = CN ;
    vec pos(0,0,0) ;

//    return false ;

    while (d>=0)
    {
        if ( CN->children )
        {
            if (idxs[d]<4)
            {
 //           printf("\nFaceCovered: checking a child.") ;
                CN = &CN->children[fci[face][idxs[d]]] ; 
                d++ ;
                path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 
                continue ;
            }
        }
        else
        {
            // If the face is not flat against its opposite, then we can see the 
            // opposite. 
            if ( !CN->has_geometry() )
            {
                return false ;
            }
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        if (d<0) {   break ; } // Past the root? Then we're done. 
        CN = path[d] ;
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    }   // end while d>=0

    // If we did not encounter leaf nodes on this face which lacked geometry, then the face is covered. 
    return true ;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// REPLACE ME AND MOVE ME! 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* tree traversal below is 29 lines */
/*
    while (d>=0)
    {
        if ( CN->children )
        {
            if (idxs[d]<8)
            {
                // We're going down to a child, marked relative to its parent by idxs[d]. 
                loopi(3) {incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ;pos.v[i] += yesorno * incr ;}
                CN = &CN->children[idxs[d]] ; 
                d++ ;
                path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 
                continue ;
            }
        }
        // If we don't have children, then maybe we have geometry.
        else { if ( CN->has_geometry() ) { } }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        if (d<0) {   break ; } // Past the root? Then we're done. 
        CN = path[d] ;
        loopi(3) { incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ; pos.v[i] -= yesorno * incr ; }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    } // end while d>=0
*/


void World::initialize()
{
    world.gridscale = 10 ;
    world.gridsize = 1<<10 ; // In our fake world, this is about 16m. 

    printf("\n\n****************************WORLD SIZE = %d************************************\n\n", world.size ) ;
}


bool showpolys = false ;
void drawworld()
{
    // Now for every geometric set in our world, render! 

    //--------------------------------------------------------------------
    // FIXME: encapsulate this block into a render state set or shader or something. 
    //glEnable( GL_TEXTURE_2D ) ;
    //glBindTexture(GL_TEXTURE_2D, texid ) ;
    glEnable( GL_TEXTURE_3D ) ;
extern GLuint surfacetex ;
    glBindTexture(GL_TEXTURE_3D, surfacetex) ;
    
    bool showVBOs = false ;
    glColor3f(1,1,1) ;

    glEnableClientState( GL_VERTEX_ARRAY );                       // Enable Vertex Arrays
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );                // Enable Vertex Arrays

    glEnable( GL_CULL_FACE ) ;
    glCullFace( GL_BACK ) ;
    glFrontFace( GL_CCW ) ;
    glDepthFunc( GL_LESS ) ;    // Only replace a pixel if new pixel is nearer

    //---------------------------------------------------------------------
    loopv(worldgeometry)
    {
        Geom* g = worldgeometry[i] ;

        if (g->vertVBOid>0 && g->texVBOid)
        {
            // FIXME: eventually it will useful (and fun) to color-code the different VBOs, so we retain 
            // the ability to inspect the extent of individual VBOs. 
            if (showpolys) { glColor3fv(colors[i%10].v) ; }

            glBindBuffer(GL_ARRAY_BUFFER, g->vertVBOid) ;
            glVertexPointer( 3, GL_INT,  0, (char *) NULL);        // Set The Vertex Pointer To The Vertex Buffer

            glBindBuffer(GL_ARRAY_BUFFER, g->texVBOid) ;
            glTexCoordPointer( 3, GL_FLOAT,  0, (char *) NULL) ;

            glDrawArrays( GL_TRIANGLES, 0, g->numverts);    // Draw All Of The Triangles At Once
        }// end if VBO ID's valid
    }// end looping over worldgeometry
    //---------------------------------------------------------------------

    // if show triangles 
    if (showpolys&&1)
    {
        glDepthFunc( GL_LEQUAL ) ;

        if (showVBOs) 
        { 
            // new color
        }
        else { glColor4f(0,0,0,1) ; }

        glPolygonMode( GL_FRONT, GL_LINE ) ;
        glPolygonOffset( 0.5, 1) ;
        glLineWidth(3) ;
//        glDisable( GL_DEPTH_TEST ) ;

            loopv(worldgeometry)
            {
//                glColor3fv(colors[i%10].v) ;
                glColor3fv(vec(0,0,0).v) ;
                Geom* g = worldgeometry[i] ;

                ///if (g->vertVBOid>0 && g->colorVBOid>0 && g->texVBOid)
                //if (g->vertVBOid>0 && g->texVBOid)
                if (g->vertVBOid>0)
                {
                    glBindBuffer(GL_ARRAY_BUFFER, g->vertVBOid);
                    glVertexPointer( 3, GL_INT,  0, (char *) NULL);        // Set The Vertex Pointer To The Vertex Buffer
                    glDrawArrays( GL_TRIANGLES, 0, g->numverts);    // Draw All Of The Triangles At Once
                }// end if VBO ID's valid
            }// end looping over worldgeometry

        glPolygonMode( GL_FRONT, GL_FILL ) ;
        glPolygonOffset( 0, 0) ;
        glLineWidth(1) ;
        glEnable( GL_DEPTH_TEST ) ;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
// */


    // Get out of this rendering state 
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glDisable( GL_TEXTURE_2D ) ;
    glDisable( GL_TEXTURE_3D ) ;

    glColor3f(0,0,1) ;
    glLineWidth(3) ;
    glBegin( GL_LINES ) ;

    ivec hello = ifront ;

if (0)
{
for (int face=0;face<6;face++)
{
    ivec pcorner = ifront ;
    pcorner[face>>1] = ifront[face>>1]+Dz(face)*(100 + (face%2)*(targetsize)) ; 

    hello = pcorner ;

    loopi(3)
    {
        hello[i] -= 640 ; glVertex3iv( hello.v ) ; 
        hello[i] += 1280 ; glVertex3iv( hello.v ) ; 
        hello[i] -= 640 ; 
        
        ivec art = hello ;
        
        glColor3f(1,0,0) ;
        glVertex3iv( art.v ) ; 
        art[X(face)] += targetsize/2 ;
        art[Y(face)] += targetsize/2 ;
        glVertex3iv( art.v ) ; 
        
        glColor3f(0,0,1) ;
    }
}
}


/*
    hello[1] -= 1280 ; glVertex3iv( hello.v ) ; 
    hello[1] += 2560 ; glVertex3iv( hello.v ) ; 
    hello[1] -= 1280 ; 
    hello[2] -= 1280 ; glVertex3iv( hello.v ) ; 
    hello[2] += 2560 ; glVertex3iv( hello.v ) ; 
*/

    glEnd() ;
    glLineWidth(1) ;
}


/*
    Used to re-send a geom's data to the graphics card. 
*/
void SendGeomToGfx(Geom* g)
{
    if ( glIsBuffer(g->vertVBOid)  && 
         glIsBuffer(g->texVBOid)   )
    {
        glBindBuffer( GL_ARRAY_BUFFER, g->vertVBOid) ;
        glBufferData( GL_ARRAY_BUFFER, g->numverts*sizeof(vec), g->vertices, GL_STATIC_DRAW ) ;

        glBindBuffer( GL_ARRAY_BUFFER, g->texVBOid) ;
        glBufferData( GL_ARRAY_BUFFER, g->numverts*sizeof(vec), g->texcoords, GL_STATIC_DRAW ) ;

        glBindBuffer( GL_ARRAY_BUFFER, 0) ;             
    }
}

// Conversion from float to int - is this fast? 
//__m128 a ;
//int b ;
//float af[4] = { 101.25, 200.75,300.5, 400.5 };
//a = _mm_loadu_ps(af);
//b = _mm_cvtt_ss2si(a);

/*
else
{
    // If execution ever reaches here, it should break your house, break your mind,
    // shake the Earth and shatter the ground. 
}
*/
