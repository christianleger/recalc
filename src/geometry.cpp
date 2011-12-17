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

//------------------------------------------------------------------------
//                  GEOMETRY SUPPORT FUNCTIONS  (octree, memory, etc.)
//------------------------------------------------------------------------
// This is equivalent to 2^15 units, or 32,768

#define WORLD_SCALE 15

// here 'world scale' really means the largest piece out of the world, which 
// is one of the 8 cubes that divide the world
             // default 15   size: 2^15       max gridsize: 15
World world = { WORLD_SCALE, 2<<WORLD_SCALE , WORLD_SCALE-1 } ;
#define w world

bool aiming_at_world ;

bool hit_world = false ;

int orientation = 0 ;

#define o orientation 
#define octstep(x,y,z,scale) ((z>>(scale))&1)<<2 | ((y>>(scale))&1)<<1 | ((x>>(scale))&1)
// corner determines the selection: it is the minimum in X,Y,Z of a box 
// of size gridsize*(xCount,yCount,zCount). 
vec corner( 0, 0, 0 ) ;

Octant::Octant()
{
    children = NULL ;
    geom = NULL ;
}
void new_octant( Octant& oct )
{
    oct.children = new Octant[8] ;
    return ;
}   
//------------------------------------------------------------------------
//                  EDITING VARIABLES AND FUNCTIONS
//------------------------------------------------------------------------
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
//float direction_multipliers[6][2] = 
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
                int size,       /* size of world box */
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

*/
void draw_corner_square(
    vec& _corner,        // lowest values for the coords not in the orientation vector (see function description for info)
    int size, 
    int _o
    )
{
    vec c(_corner.v) ;
// FIXME FIXME FIXME FIXME

    glBegin( GL_LINE_LOOP ) ;
        glVertex3fv( c.v ) ; c[X(_o)] += size ;  // bottom-right
        glVertex3fv( c.v ) ; c[Y(_o)] += size ;  // top-right
        glVertex3fv( c.v ) ; c[X(_o)] -= size ;  // top-left
        glVertex3fv( c.v ) ;
    glEnd() ;
}

void draw_corner_cube(
    vec & _corner,
    int size
    )
{

    vec corner = _corner ;
    // first two squares cornered at origin 
    draw_corner_square( corner, size, 0) ; 
    draw_corner_square( corner, size, 2) ; 
    // third square cornered at origin.x + size 
    //corner[0] += size ; 
    //draw_corner_square( corner, size, 1) ;
    //corner[0] -= size ; corner[1] += size ; 
    //draw_corner_square( corner, size, 3) ;
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

void draw_world_box()
{
    int half = world.size>>1 ;

    vec center( 0, half, half ) ;

    glDisable( GL_DEPTH_TEST ) ; 

    draw_square(center, world.size, 0) ; center[0] += half ; center[1] += half ;
    draw_square(center, world.size, 3) ; center[0] += half ; center[1] -= half ;
    draw_square(center, world.size, 1) ; center[0] -= half ; center[1] -= half ;
    draw_square(center, world.size, 2) ;
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
    //geom_msgs_num = 0 ; 

    // The variable hitplanes encodes which three axis-aligned planes are 
    // always possible hits for a ray going through an octree. 
    //
    // Later, hitplanes can be used to quickly express plane indexes into the 
    // world_normals array. 
    loopi(3)
    {
        hitplanes |= ( camdir[i]<0 ? 2*i+1 : 2*i ) << (3*i) ;
    }
    
    // First see if we're hitting something in the tree 
    {
    }

    // This is the calculation to determine which plane, 
    // positive or negative (see world_planes array), is 
    // to be looked up. This approach is taken because 
    // we chose to record the set of potentially hit planes
    // using a single int, 'hitplanes'. 
    #define hittingplane (hitplanes>>(3*i))&0x07
    // are we aiming at the world? 
    min_t = FLT_MAX ;
    int i = 0 ;

    // If we hit nothing in the world, we find out which part of the grid 
    // frontier we're hitting. 

    // Are we hitting a far plane?
    if (!hitworld)
    {
    for (i=0;i<3;i++)
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
        // we only truncate the variables which aren't the major 
        // axis for the current orienatation plane. If we did, it might 
        // give us a smaller value than we need. 
        if (!( (orientation==2*j) || (orientation==2*j+1) ))
        {
            corner[j] = (((int)(corner[j])) >> gridscale ) << gridscale ;
        }
    }
    }




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

    int O = (sel_o) ; // orientation
    int WS = world.scale ;
    int WGSZ = world.gridsize ;
    int WGSC = world.gridscale ;
    int CGS = WS ; // current grid scale
    int i = 0 ;
    ivec pos(0) ;   // used to compute vertices that go into our vbo_array
    vectormap vmap ; // used to record new nodes and then to construct and update vector arrays

    // define a range of cubes that will be created, bounded by 
    // sel_min and sel_max. These will bound the same rectangle 
    // as sel_start and sel_end, but with easier to compute properties. 
    loopi(3)
    {
        sel_min[i] = min( sel_end[i], sel_start[i] ) ;
        sel_max[i] = max( sel_end[i], sel_start[i] ) ;
    }

    // Where the extrusion originates is affected by what the 
    // selection cursor is currently pointed at. 
    // Are we pointed at something within the tree? 
    if ( sel_path[0] >= 0 )
    {
    }
    // Else we pointing at a world boundary. 
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
            newcubes_new = true ;


            // The check is true if our orientation is even. 
            if ( !(O%2) )
            {
                NC[Z(O)] += wp[O][Z(O)] * (sel_size) * sel_counts[Z(O)] ;
            }
            // If the new cube coordinates do not exceed world 
            // limits, then we have a new cube. 
            havenewcube = true ;

            Octant* oct = &world.root ;
            int depth = 0 ; // first child of root

            // make our way to the desired node 
            while (CGS>=WGSC)
            {
                if ( !oct->children ) 
                { 
                    printf("\n  allocating 8 children ") ; 
                    oct->children = new Octant[8] ; 
                }
                i = octastep(NC.x,NC.y,NC.z,CGS) ;
                sel_path[depth] = i ;
                oct = &oct->children[i] ;

                loopj(3)
                {
                    pos[j] |= ((i>>j)&1)<<(CGS) ; // using the new child index, incr the coordinates to our position 
                }

                depth++ ;
                CGS-- ;
            }
            //printf("\n Created new position vector: %d %d %d", pos.x, pos.y, pos.z) ; 
            //sprintf(geom_msgs[geom_msgs_num], "octastep=%d", sel_path[depth]) ; geom_msgs_num++ ;
            //printf("\noctastep=%d  depth=%d  sel_path[depth]=%d", sel_path[depth], depth, sel_path[depth]) ; 
            //sprintf(geom_msgs[geom_msgs_num], "depth=%d. CGS=%d", depth, CGS) ; geom_msgs_num++ ;
            //printf("depth=%d. CGS=%d", depth, CGS) ; 
            /*
                At this point, oct is pointing to the octant. 

                First, we create a geom. 
            */
            if (oct)
            {
                printf("\n  allocating a geom ") ; 
                //oct->geom = new OctExt() ;
                oct->edges[0] = 0 ;

                vmap.AddHashVector(pos) ;
                int k = vmap.HashFindVec(pos) ;

                sprintf(geom_msgs[geom_msgs_num], "Created new position vector: %d %d %d", pos.x, pos.y, pos.z) ; geom_msgs_num++ ;
                sprintf(geom_msgs[geom_msgs_num], "New vector in hashmap at: %d", k) ; geom_msgs_num++ ;
                // Assign pos to new vbo_verts and then reset pos. 


                loopi(3) pos[i] = 0 ;
            }

            /*
            if (world.root.children==NULL)
            {
                new_octant( world.root ) ;
                if (world.root.children)
                {
                    printf("\nworld root's children allocated. ") ; 
                }
            }
            else
            {
                printf("\n children not allowed, = %d. ", *(int*)world.root.children) ; 
            }
            */
        }
    }
    else if ( sel_path[0] == -2 )
    {
        printf("\nCannot extrude when no selection is in view. \n") ; 
    }
    ivec counts(1) ; 
    // min one cube 

    /* step: populate the geometry tree with the newly created cubes, replacing 
       (and therefore deleting) anything that inside the volume of new cubes. 

        Populating the tree: 
            - For every cube of size==world.gridsize in our selection, create a 
              child.
    */

        // for every potential child we walk from the root to the path that 
        // takes us to this child, to know his ancestry. 

    // step: generate all triangle vertices from the current set of new cubes. 


    // next step: determine which faces of our new cubes are visible 

}

/*
*/
bool use_dl ;
void draw_newcubes()
{

    /* FIXME: TEMPORARY: TODO: nein nein nein */ 
    /*
        Render little bits to show where the current contents of the tree is 
    */ 
    /* 
        Steps: 
                Iterate through the tree. 

                Whenever there are children, we go down. 
                Whenever there is a node with geometry, we draw points 
                for its eight corners. 
    */
    Octant* oct = &world.root ;
    int     c[20] = {0} ;    // sequence of child indexes
    Octant* octpath[20] = {NULL} ;  // sequence of octants 
    octpath[0] = oct ;
    int d = 0 ;
    int scale = world.scale ;
    int x = 0, y = 0, z = 0 ;

    glColor3f(1,1,1) ; 
    glBegin( GL_POINTS ) ; 
    int i=0 ;


    while ( d>= 0 )
    {
        // case: we're done with the children (array c)of this node, so we back up
        if (c[d]>=8)
        {
            // time to go up one
            d-- ;
            scale++ ;
            if (d>=0)
            {
                oct = octpath[d] ;
                c[d]++ ;
            }
        }
        // case: we go to the next child of this node
        else
        {
            i = c[d] ; // which child are we? 

            if (oct->geom)
            {
                for (int j=0;j<d;j++)
                {
                    x |= (((c[j]))   &1)<<(world.scale-j) ;
                    y |= (((c[j])>>1)&1)<<(world.scale-j) ;
                    z |= (((c[j])>>2)&1)<<(world.scale-j) ;
                }
                //printf("\n child #%d corner at %d, %d, %d ", c[d-1], x, y, z) ; // c[d-1], using index used from parent to get here 
                glVertex3f(x, y, z) ;
                x = y = z = 0 ; 
                // This means next iteration we're moving back up (to go to siblings, we must go up. 
                // And we can't go down, since having geom means being a leaf.) 
                c[d] = 8 ; 
            }

            if (oct->children)
            {
                oct = &oct->children[i] ; 
                d++ ;
                scale-- ;
                c[d] = -1 ; // reset: it will be 0 after this if-else block. 
                octpath[d] = oct ;
            }
            // next child 
            c[d]++ ;
        }
    }
    glEnd() ; 
    if ( !havenewcube ) return ;

    vec new_corner( newcube.v ) ;

    int orien = sel_o ;
    bool end = false ;

    static int gl_newcubes = 0 ;

/*
    If we're creating new new cubes (previous new cubes no 
    longer newest), then we create a new display list for 
    their drawing. 
*/
if (!use_dl)
    newcubes_new = true ;

    if ( newcubes_new )
    //if ( 1 )
    {

if (use_dl)
{
        newcubes_new = false ; // only need to go in here once
        glDeleteLists( gl_newcubes, 1 ) ;
        gl_newcubes  = glGenLists(1) ; 
        glNewList(gl_newcubes, GL_COMPILE);
}
        // FIXME: this code belongs in the geometry extrusion section  
        // It is only here while rendering must rely on unavailable geometry. 
        loopi(sel_counts[Y(orien)])
        {
            loopj(sel_counts[X(orien)])
            {


                draw_corner_cube(
                    new_corner,
                    sel_size
                    ) ; 

                new_corner[X(orien)] += ((sel_size)) ;
            }
            new_corner[X(orien)] = newcube[X(orien)] ;
            new_corner[Y(orien)] += ((sel_size));
        }

if (use_dl)
{
        glEndList() ;
}
    }
    else
    {
        glCallList( gl_newcubes ) ;
    }

    // vertex arrays ...
    //glEnableClientState() ; 
    //glDisableClientState() ; 


}

void World::initialize()
{
    world.gridscale = 10 ;
    world.gridsize = 1<<10 ;

    // world.root = 0 ;
}







