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
    needing update. This is done by setting the nodes needs_update flag. 

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

    --------------
    Terminology 
    --------------

    prime corner: 
        The minimum-value point on the border of a cube. This means X, Y and Z
        are minimum. Example: (0,0,0) is the prime corner of every cube that
        shares a vertex with the world origin. 
        
        At a given grid size, every point whose coordinates are a multiple of
        the grid size is the prime corner to a single cube. 

        Just to be clear, even though many points are on the border of up to 
        8 cubes, every point is only the prime corner of the cube whose other 
        points are all at higher values than this corner's XYZ values. 

        A prime corner is therefore a unique identifier/specifier of a
        particular cube, at a given grid size. 

        When new cubes are created, or old ones are destroyed, they are 
        specified by their prime corner. 
        
*/


//------------------------------------------------------------------------
// INCLUDES
//------------------------------------------------------------------------
#include "recalc.h"


//------------------------------------------------------------------------
// EXTERNAL OBJECTS
//------------------------------------------------------------------------
extern Camera camera ;
extern Engine engine ;
#define e engine

//------------------------------------------------------------------------
// GLOBAL VARIABLES
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// OPTIONS
//------------------------------------------------------------------------



//------------------------------------------------------------------------
// player/user control and status
//------------------------------------------------------------------------
//vec  camdir ;

////////////////////////////////////////////////////////////////////////////////
// GLOBAL GEOMETRY DATA
////////////////////////////////////////////////////////////////////////////////
vector<Geom*> worldgeometry ;

vector<Geom*>& GetWorldGeometry() // worldgeometry
{
    return worldgeometry ;
}


#define numstars 10000
ivec stars[numstars] ;
GLuint starsVBO = 0 ;

////////////////////////////////////////////////////////////////////////////////
// GEOMETRY MESSAGES
////////////////////////////////////////////////////////////////////////////////
int geom_msgs_num = 0 ;
char geom_msgs[100][256] ;

////////////////////////////////////////////////////////////////////////////////
// GEOMETRY SUPPORT FUNCTIONS  (octree, memory, etc.)
////////////////////////////////////////////////////////////////////////////////
Octant* FindNode(ivec at, int* out_size, int* nscale) ; 
Octant* FindSizedNode(ivec at, int size) ; 
Octant* FindGeom(ivec at, int* out_size, int* nscale) ; 

int numchildren = 0 ;

// The main high-level functions of the geometry module
void update_editor() ;
void deletesubtree(Octant* in_oct) ;
void modify( void* _in ) ;
void FlagSubtreeForUpdate( Octant* parent) ;
void FlagPathForUpdate( Octant** path, int depth) ;
void SubdivideOctant( Octant* oct ) ;
bool const PointInsideWorld ( 
    ivec cp /* corner position*/, 
    World& w /* world to check against */
    ) ;

bool FaceCovered(Octant* node, int face) ;
void AnalyzeGeometry(Octant* tree, ivec corner, int scale) ;
void AssignNewVBOs(Octant* tree, ivec corner, int scale) ;
void BuildSubtreeGeometry(Octant* parent, ivec corner, int NS) ;
void UploadGeomToGfx(Geom* g) ;

////////////////////////////////////////////////////////////////////////////////
//      The World and its Dimensions
////////////////////////////////////////////////////////////////////////////////
/*  
    A world is a cube containing 8 cubes of size 2^WORLD_SCALE. 

    64 units represent a meter. 

    Max size: 21
    Normal size: 10-15, equivalent to 16-1024 meters. Default scale is 15, or 1024 meters. 
*/
#define WORLD_SCALE 15

// SOME OF THESE VALUES ARE OVERRIDDEN DURING INITIALIZATION ROUTINES
World world = 
{ 
    WORLD_SCALE,        // scale: default 15   
    2<<WORLD_SCALE ,    // size: 2<<15=65536    (total size equals 2^16)
    WORLD_SCALE,        // gridscale (scale used to size things while editing)
    2<<(WORLD_SCALE-1)  // gridsize (size of new nodes when editing) max gridsize: 2^15
} ;
#define w world

////////////////////////////////////////////////////////////////////////////////
// GEOMETRY HELPER ARRAYS AND MACROS 
////////////////////////////////////////////////////////////////////////////////

/*
    About the next two arrays and groups of defines: 
    This stuff makes us able to pretend that any axis-aligned plane is just 
    the plain old XY plane, with X increasing to the right and Y increasing upwards. 

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

////////////////////////////////////////////////////////////////////////////////
//      Octant member and auxiliary functions
////////////////////////////////////////////////////////////////////////////////
bool aiming_at_world ;
bool hit_world = false ;

// create a node with nothing in it
Octant::Octant()
{
    children = NULL ;
    stuff = NULL ;
    geom = NULL ;
    vc = 0 ;

    setneedsupdate() ;
    clear_geometry() ;
    flags = EMPTY | NEEDS_UPDATE ;
    
    tex[0] = -1 ; tex[1] = -1 ; tex[2] = -1 ;
    tex[3] = -1 ; tex[4] = -1 ; tex[5] = -1 ;
}

bool Octant::has_geometry() 
{
    return ( (edgegroups[0]!=0) ||
             (edgegroups[1]!=0) ||
             (edgegroups[2]!=0) ) ;
}


// A cube is defined by 8 vertices. 
// The faces of the cube are defined by which of these vertices
// they are constructed with, and here we have the indexes that 
// refer to these vertices. 
static int faceindexes[][4] =
{
    {0, 1, 2, 3}, // face 0 CCW from origin looking +ve X-ward
    {4, 5, 6, 7}, // face 1 CCW from bottom-right looking -ve X-ward
    {7, 6, 1, 0}, // face 2
    {3, 2, 5, 4}, // face 3
    {0, 3, 4, 7}, // face 4
    {6, 5, 2, 1}  // face 5
} ;
#define fi faceindexes

static int newfaceindexes[][4] = 
{
    {0, 4, 6, 2},   // face 0
    {3, 7, 5, 1}, 
    {1, 5, 4, 0}, 
    {2, 6, 7, 3}, 
    {2, 3, 1, 0}, 
    {5, 7, 6, 4}    // face 5
} ;
#define nfi newfaceindexes

static int cornerstooctindexes[] =
{
    0, 4, 6, 2, 3, 7, 5, 1
} ;
static int octindexestocorners[] =
{
    0, 7, 3, 4, 1, 6, 2, 5
} ;
#define oitc octindexestocorners

/*
    FaceIsFull:
        
        returns true if a given face of a node is a perfect square which 
        extends the full height and width of the node. 

        TODO: anything to change here? 
        Let's see.... which nibs go with which face? (can we do it algorithmically
        or do we need another lookup table?)

    The face is full if all edge values for the four corners of this face are 
    maximum. 

    Because of the way the cube's corners are defined, 0 through 7, on the faces 
    (for details see the docs), we know that for even faces have a diagonal along 
    X=Y whose two vertices are even-numbered and the other two vertices are odd-
    numbered. This allows us to save a little bit of time when producing corner 
    coordinates from the edge values, when we don't want all 8 of the vertices. 
*/
bool Octant::FaceIsFull(int face)
{
    // First we check if the node is solid. If it is, then the answer is yes right away. 
    if ( (edgegroups[0] == 0xFFFFFFFF) &&
         (edgegroups[1] == 0xFFFFFFFF) &&
         (edgegroups[2] == 0xFFFFFFFF) ) { return true ; }
 
    // If we're still here, we check if the values representing 
    // the corner coordinates for the desired face are all max. 
    loopi(4)
    {
        // TODO: replace fi with nfi, then remove fi, then rename nfi to fi! :-D 
        int nib = fi[face][i] ;
        int shift = 0 ;
        if (nib&0x1) shift = 4 ; else shift = 0 ;
        int x = (edges[   nib>>1]    >> shift)&0xF ;  
        int y = (edges[4+(nib>>1)]   >> shift)&0xF ;  
        int z = (edges[8+(nib>>1)]   >> shift)&0xF ;  
        if ( x < 15 || y < 15 || z < 15 ) return false ;
    }
    return true ;
}





/*
printf("\n face %d - corner %d: opposite = %d", face, c[i], (c[i] ^ (0x1<<coord))) ;
printf("\n shift: %d   oshift: %d", shift, oshift) ;
printf("\n x value: %d  opp x value: %d", (eg[x]>>shift)&0xF, (node->eg[x]>>oshift)&0xF) ;
printf("\n y value: %d  opp y value: %d", (eg[y]>>shift)&0xF, (node->eg[y]>>oshift)&0xF) ;
printf("\n z value: %d  opp z value: %d", (eg[z]>>shift)&0xF, (node->eg[z]>>oshift)&0xF) ;
*/
bool Octant::FaceHiddenBy(int face, Octant* node) 
{
    // Full adjacent cubes of course don't show opposite faces
    if (eg[0]==node->eg[0] && 
        eg[1]==node->eg[1] && 
        eg[2]==node->eg[2] && 
        eg[0]==0xFFFFFFFF  && 
        eg[1]==0xFFFFFFFF  && 
        eg[2]==0xFFFFFFFF) { return true ; }

    // What about bent cubes? Need to check if one face spills out of the other. 
    int* c = &nfi[face][0] ;

    int coord = face>>1 ;
    int x = X(face) ;
    int y = Y(face) ;
    int z = Z(face) ;

    loopi(4)
    {
        int shift = (4*c[i]) ;                  // bitshift to get our current corner
        int oshift = 4*(c[i] ^ (0x1<<coord)) ;  // shift to get opposite corner

        // If faces are 'unstuck' then they both show. 
        if ( ((( eg[z]      >>shift)  & 0xF ) < 0xF ) || 
             ((( node->eg[z]>>oshift) & 0xF ) < 0xF ))   { return false ; }

        if (((eg[x]>>shift)&0xF) > ((node->eg[x]>>(oshift))&0xF) || 
            ((eg[y]>>shift)&0xF) > ((node->eg[y]>>(oshift))&0xF) )  { return false ; }
    }
    return true ;
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
    For the general case of setting edges to specific values, 
    always use a 12-byte buffer. 
*/
void Octant::set_edges(const uchar* in_values)
{   
    // Don't know if this is faster than a 12-byte memcpy. Tell me if you know. Even better, show me if you can. 
    edgegroups[0] = *((int*)(&in_values[0])) ;
    edgegroups[1] = *((int*)(&in_values[1])) ;
    edgegroups[2] = *((int*)(&in_values[2])) ;
}

/*
    No arguments means make this solid. 
    Having an argument means set edges to that value. 
    TODO: right now we're not doing anything with the parameter!
*/
void Octant::set_all_edges(int in_value=255)
{
    eg[0] = 0xFFFFFFFF ;
    eg[1] = 0xFFFFFFFF ;
    eg[2] = 0xFFFFFFFF ;
//    loopi(12) { edges[i] = in_value ; }
    flags |= SOLID | FULL ;
}

void new_octants( Octant* oct )
{
    oct->children = new Octant[8] ;
    return ;
}

void Octant::clear_all()
{
    if ( geom )           { clear_geom()     ; }
    if ( children )       { clear_children() ; }
    if ( has_geometry() ) { clear_geometry() ; }
    vc = 0 ;
    /*
        TODO: FIXME: woa woa, woa. Are you assigning negative values to something that is a char? 
        Don't I want a uchar for this? What are you doing? Heh. 

        Suggestion: use 0 as nothing, and count tex IDs from 1? .. nah, that'd be weird. 
    */
    tex[0] = -1 ; tex[1] = -1 ; tex[2] = -1 ;
    tex[3] = -1 ; tex[4] = -1 ; tex[5] = -1 ;
}

//------------------------------------------------------------------------
//                  EDITING VARIABLES AND FUNCTIONS
//------------------------------------------------------------------------

int orientation = 0 ;
#define o orientation 

ivec ifront ;           // used to remember the location where a ray hits a node
ivec icorner ;          // the prime corner of the node or world boundary square we are currently pointing at. 
Octant* aim = NULL ;    // currently aimed-at node. 
ivec hitcorner ;        // specifies which corner our ray hits closest to (0-3 counter-clockwise from bottom-right). 
int facecorner = 0 ;

bool shaping = false ;  // tracks whether we are molding & sculpting cubes, or deleting and creating them. 

ivec farcorner ( 0, 0, 0 ) ; // tracks the integer-truncated point reached by the editor aiming ray

bool havetarget = false ;
bool havesel = false ;
bool havesel_end = false ;

int sel_grid_size = 0 ;
int sel_o = 0 ;         // Face orientation (0-5) of aim ray entry face into current target node
int sel_start_o = 0 ;   // orientation of selection start
int sel_end_o = 0 ;     // orientation of selection end

/*
    sel_start and sel_end record the prime face corners of selections. 
    sel_min and sel_max together record the bounding box of a selection volume. 
*/
ivec sel_start ;    
ivec sel_min ;
ivec sel_end ;
ivec sel_lim[2] ;
ivec sel_first ;
ivec sel_counts ;

Octant* ray_start_node ;

// If our edit-mode ray hits the world from the outside, here is what we track it with. 
bool    have_ray_start_node = false ;
ivec    ray_start_vec ;
int     ray_start_orientation = 0 ;

Geom* currentgeom = NULL ;

/*
    The numbers in this array refer to nibbles. 
    
    The 'edges' array in every node (12 bytes in all) contains the 24 half-edge 
    corner values (which can represent either X, Y or Z values of coordinates 
    on a face, depending on the face we are looking at the 
    This array relates the indexes for the 'edges' array found in nodes to the 
    face-oriented Z coordinates of each corner of each face. See references
    about the coordinate systems in use to figure out the meaning and origin 
    of these numbers. 
    (email me to ask for this documentation if you cannot find it)
    
    The first index into this array (the major one) refers to the current face of 
    a node we are dealing with. 
    
    The second index into the array is which quadrant of the face we are using, 
    where quadrants can be 00, 01, 10 or 11. The first digit represents 
    face-oriented X, and the second one Y. A 0 means the coordinate is at a 
    min and a 1 means the coord is at a max. 
    
    Definitely refer to the coordinate systems defined for cube faces if you want 
    to understand this. 
    
   TODO: dump this. Apparently not needed! 
*/
int face_corners_to_edge_values_indexes[6][4] = 
{
    {0,    3,    9,    6}, 
    {21,  18,  12,  15}, 
    {1,    4,    22,  19}, 
    {10,  7,    13,  16}, 
    {5,    8,    20,  17}, 
    {2,    11,  23,  14}
} ;
#define fcei face_corners_to_edge_values_indexes

void clear_selection()
{
    havesel = false ;
    havesel_end = false ;
}

bool selnonempty = false ;
void set_sel_start()
{
    int GS = world.gridsize ;
    havesel = true ;
    havesel_end = false ;

    // If we're pointing at geometry
    if (havetarget)
    {
        sel_start   = ifront ; 
        sel_start[o>>1] += GS*(o%2) ;   // This positions sel_start on the face we're currently looking at
        sel_lim[0] = ifront ;
        sel_lim[1] = ifront ;
        sel_lim[1][o>>1] += GS ;
        sel_first = ifront ;
        selnonempty = true ;
    }
    // If we're pointing at a world boundary
    else
    {
        sel_start   = farcorner ;
        sel_end     = farcorner ;
        sel_lim[0] = farcorner ;
        sel_lim[1] = farcorner ;
        sel_first = farcorner ;
        selnonempty = false ;
    }

    sel_counts = vec(1,1,1) ;
    sel_o = o ;
    sel_start_o = o ;
    sel_grid_size = world.gridsize ;

    printf("\nselection starting at %d  %d  %d\n", sel_start.x, sel_start.y, sel_start.z ) ; 
}


void set_sel_end()
{
    int GS = world.gridsize ;
    //if (!havesel || o!=sel_o ) 
    if (!havesel)
    {
        set_sel_start() ;
        return ;
    }
    havesel_end = true ;

    sel_o = o ;
    sel_end_o = o ;

    // We point at geometry or we point at world boundary
    if (havetarget) 
    { 
        sel_end = ifront ; sel_end[o>>1] += GS*(o%2) ; 
        loopi(3) sel_lim[0][i] = min(ifront[i], sel_first[i]) ;
        loopi(3) sel_lim[1][i] = max(ifront[i], sel_first[i]) ;
        sel_lim[1][o>>1] += GS ; 
    }
    else 
    { 
        sel_end = farcorner ; 
        loopi(3) sel_lim[0][i] = min(sel_end[i], sel_first[i]) ;
        loopi(3) sel_lim[1][i] = max(sel_end[i], sel_first[i]) ;

        if (selnonempty)
            sel_lim[1][o>>1] += GS ; 
    }

    sel_grid_size = world.gridsize ;

    loopi(3) 
    {   
        sel_counts[i] = (( max(sel_start[i], sel_end[i]) - min(sel_start[i], sel_end[i]) ) >> w.gridscale ) + 1 ; 
    }
}


// Oriented square centered on a point
void draw_square(
    ivec & center    /* center */,
    int size,        /* size of box */
    int _o           /* orientation */
    )
{
    ivec c(center) ;
    c[X(_o)] -= Dx(_o)*(size>>1) ;
    c[Y(_o)] -= Dy(_o)*(size>>1) ; // now positioned at (origin) corner. 

    //glColor3f( 1, 1, 0 ) ;
    glColor4f( 1, 1, 0, 1 ) ;

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
    ivec c,        // corner; min coord values in the selection plane 
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


/*
    Draws a cube whose minimum corner is at _corner. 
*/
void draw_corner_cubei(
    ivec _corner,
    int size
    )
{
    // first two squares cornered at origin 
    draw_corner_squarei( _corner, size, 0) ; 
    draw_corner_squarei( _corner, size, 2) ; 
    
    // third square cornered at origin.x + size 
    _corner[0] += size ; 
    draw_corner_squarei( _corner, size, 1) ;
    _corner[0] -= size ; _corner[1] += size ; 
    draw_corner_squarei( _corner, size, 3) ;
}


/*
    FUNCTION: 
        draw_corner_cube

    DESCRIPTION: 
        This function draws an axis-aligned cube of the requested size, based
        on a reference point given by the parameter 'corner', which defines 
        the min X,Y and Z values of the cube. 

*/
void draw_corner_cube(
    vec & in_corner,
    int size
    )
{
    vec c = in_corner ;
    // first two squares cornered at origin 
    draw_corner_square( c, size, 0) ; 
    draw_corner_square( c, size, 2) ; 
    // third square cornered at origin.x + size 
    c[0] += size ; 
    draw_corner_square( c, size, 1) ;
    c[0] -= size ; c[1] += size ; 
    draw_corner_square( c, size, 3) ;
}


/*
    FUNCTION:
        draw_edit_cursor


    DESCRIPTION:
        This function draws a square which frames the node which we are
        currently looking at. 

*/
void draw_edit_cursor()
{
    glColor3f( 0, 1, 0 ) ;
    glLineWidth( 2.5f ) ;
    int GS = world.gridscale ;
    int NS = world.gridsize ;

    ivec drawn_corner ;
    if (havetarget)
    {
        drawn_corner = ifront ;
        loopj(3) { drawn_corner[j] = ((drawn_corner[j] >> GS) << GS) + 1 ; }
        drawn_corner[o>>1] += NS*(o%2) ;
        drawn_corner[o>>1] += Dz(o)*NS/(GS*6); 
    } 
    // from ray hitting world boundary 
    else { drawn_corner = farcorner ; }
    draw_corner_squarei( drawn_corner, world.gridsize-2, o) ;
    glLineWidth( 1.0f ) ;
}


void draw_sel_start()
{
    if ( !havesel ) return ;
    int GS = world.gridscale ;
    glLineWidth( 5.0 ) ;
    glColor3f( 0, 1, 0 ) ;
    ivec v( sel_start ) ;
    v[sel_start_o>>1] += Dz(sel_start_o)*GS/2 ; // lets our square highlight float at the right distance from the cube's corner
    draw_corner_squarei( v, sel_grid_size, sel_start_o) ;
    glLineWidth( 1.0 ) ;
}


void draw_sel_end()
{
    if ( !havesel_end ) return ;
    int GS = world.gridscale ;
    glLineWidth( 5.0 ) ;
    glColor3f( 0, 1, 1 ) ;
    ivec v( sel_end ) ;
    v[sel_o>>1] += Dz(sel_o)*GS/2 ; // lets our square highlight float at the right distance from the cube's corner
    draw_corner_squarei( v, sel_grid_size, sel_o) ;
    glLineWidth( 1.0 ) ;
}


/*
    FUNCTION: 
        draw_selection

    DESCRIPTION:
        This function renders a grid that highlights the currently selected 
        geometry. This geometry might be a single node, or several. 

    NOTES: 

        A selection is defined by the points sel_start and sel_end. 

*/
void draw_selection() 
{
    if ( !havesel ) return ;
    int GS = world.gridscale ;

    ivec smin = sel_lim[0] ;
    ivec smax = sel_lim[1] ;
   
    smax[X(sel_o)] += sel_grid_size ;
    smax[Y(sel_o)] += sel_grid_size ;
    
    ivec delta ;
    delta[X(sel_o)] = abs(smax[X(sel_o)] - smin[X(sel_o)]) ;
    delta[Y(sel_o)] = abs(smax[Y(sel_o)] - smin[Y(sel_o)]) ;
    delta[Z(sel_o)] = abs(smax[Z(sel_o)] - smin[Z(sel_o)]) ;

    ivec c( sel_lim[0] ) ;
    ivec savec = c ;

    // Draw the selection box outline
    c = sel_lim[0] ;
    glLineWidth(3.0) ;
    glColor4f( 1, 0, 0, 1) ;
    glEnable(GL_DEPTH_TEST) ;

    // Draw a 'somewhat squished' line loop hugging each face
    loopi(6)
    {
        c = savec ;
        c[Z(i)] += ((i%2)*(delta[Z(i)] + (0)))+ Dz(i)*(GS+2) ;
        glBegin( GL_LINE_LOOP ) ;
            c[X(i)] += 2 ; c[Y(i)] += 2 ;
            glVertex3iv( c.v ) ; c[Y(i)] += delta[Y(i)]-4 ;
            glVertex3iv( c.v ) ; c[X(i)] += delta[X(i)]-4 ;
            glVertex3iv( c.v ) ; c[Y(i)] -= delta[Y(i)]-4 ;
            glVertex3iv( c.v ) ; c[X(i)] -= delta[X(i)]-4 ;
            glVertex3iv( c.v ) ; 
        glEnd() ;
    }
//    c = savec ;
    c = ( sel_lim[0] ) ;
   
    // TODO: experiment with glDepthRange to make the selection face 
    // always clearly visible (when in line of sight) without having to raise it 
    // too much. 
//        c[Z(i)] += ((i%2)*(delta[Z(i)] + (0)))+ Dz(i)*(GS+2) ;

// TODO: adjust the separation between the selection face and geometry by view distance. 
// The larger the distance (calculated when edit aim ray is computed), the greater 
// the separation. This is to improve artifacts we get from improper depth-range 
// management which we haven't had time to investigate yet. Urghs. 
    c[Z(sel_o)] = sel_lim[sel_o%2][Z(sel_o)] + Dz(sel_o)*((GS*4)) ;
    
    // Now draw the selection face
//glDepthRange(-1, 0.99) ;
//glDepthRange(0.500, 0.51) ;
//glDepthRange(0, 1) ;
    glColor4f( 1, 1, 1, 0.5) ;
    glDisable( GL_CULL_FACE ) ;
    glBegin( GL_QUADS ) ;

        glVertex3iv( c.v ) ; c[Y(sel_o)] += delta[Y(sel_o)];// + sel_grid_size ;  
        glVertex3iv( c.v ) ; c[X(sel_o)] += delta[X(sel_o)];// + sel_grid_size ;  
        glVertex3iv( c.v ) ; c[Y(sel_o)] -= delta[Y(sel_o)];// + sel_grid_size ;  
        glVertex3iv( c.v ) ; 

    glEnd() ;
    
    glEnable( GL_CULL_FACE ) ;
    glDisable(GL_DEPTH_TEST) ;

//    draw_sel_start() ;
 //   draw_sel_end() ;

    // selection bbox spanner. For debug/testing, and disabled normally. 
    /* glLineWidth(5.0) ;
    glBegin(GL_LINES) ;
        glColor3f(1,0,0) ; glVertex3iv(smin.v) ;
        glColor3f(0,1,0) ; glVertex3iv(smax.v) ;
    glEnd() ; */

    glLineWidth(1.0) ;
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

TODO: is this needed for anything? 
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

    TODO: FIXME: if ever we implement changing world size, this needs to be updated. 
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


char plane_names[3][2] = { "X", "Y", "Z" } ;    // TODO: delete this :-) 

// TODO: check if this is redundant, given our X(orientation) stuff. 
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
    denominator = ray.dot( pl.v )  ;
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
    den =                 ray.dot( pl.v )  ;
    if ( den != 0 ) { *t = ( num ) / ( den ) ;  }
    else  { *t = 0 ; }
    vec hitpos = pos.add(ray.mul(*t)) ;
    return hitpos ;
}


/*
    Inputs: 
        pos - start of ray
        ray - direction of ray
        node - node into which ray is casting
        max_t - the maximum factor by which ray can be extend from pos

    Outputs: 

        return_t - the result of the computation: how much the ray is extended
        to hit the node. 

        true - if the ray hits the node's planes within the node's volume. 
        false - otherwise. 

    TODO: this is the next thing in life for you to do. 


    Overall procedure: 

        This node is not a solic block. Therefore some of its planes 
*/
bool RayHitNodePlanes(vec pos, vec ray, Octant* node, float max_t, float& return_t)
{

    // For each face on this node, if the faces of that node are visible, then
    // assign the corners of that face to be computed. 

    loopi(6)
    {
    }
    return false ;
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

    TODO: 
        this needs both a massive cleanup and also partitioning into a few parts, 
        including the dumping of ray-hit-axis-plane and ray-hit-plane into functions. 

        Also, variable names need to be clarified. too many variables differing only by 
        one letter or something. 
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

    ivec ifr ;          // int vector to track which node a ray front is in. ifr == 'int ray front'
    ivec ic ;           // int vector to track the corner of the node we're in. ic == 'int corner'

    float r = 0.f ; // rate
    float d = 0.f ; float t = 0.f ; 

    while (!havetarget)
    {
        if (steps>100) 
        {
            t=0.f ;  
            break ; // Kill runaway loops
        }

        loopj(3) {ifr[j] = (int)f[j] ;} // place, more or less, ifr at rayfront (imprecision from converting float to int)

        // Snap to inside of node we're looking at. Unless we're just starting off - then we need to first move to a boundary 
        if (steps>0) 
        {
            ifr[i] += ( ray[i] >= 0?1:-1 ) ;    // snap 'int ray front' to inside of next node.  
            if ((ifr[i]>= WS && ray[i]>0) || 
                (ifr[i]<= 0  && ray[i]<0)) { break ;  } // ray exited world
        } ; 

        Octant* oct = FindNode(ifr, &NS, &Nscale) ; // Find out what node encloses this point

        if ( oct->has_geometry() )                  // Potential target acquired. 
        { 
            // If the node is a full cube, then we know we hit it, so we return this result. 
            if (oct->IsFull())
            {
                *loc = f ;
                return t ; 
            }
            // Otherwise we have a shaped cube and need to check against its planes. 
            else
            {
                float other_t = t ;
                if (RayHitNodePlanes(pos, vec(ray), oct, max_t, other_t))
                {
                    return other_t ;
                }
            }
        }

        ic = ifr ;

        // Snap ic to node prime corner
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
       
        // prepare a repeat of this process
        f = pos ;
        f.add(ray.mul(t)) ;  // move ray front to new plane
        ray = dir ;          // reset ray to length 1
        steps++ ;
    }
    return t ;
}


/*
    Prepares a set of messages for use by the rendering system if debugging 
    information is desired by the user. 


*/
void update_editor_info()
{
}


/*
    Inputs to this function: 
        - camera position (used for ray origin)
        - camera direction (used for ray direction)
        - world.gridscale - at what size scale are we currently editing

    Outputs of this function: 

        - which, if any, world geometry is currently highlighted. 

        - which, if any, world planes are currently selected. 

    TODO: 
        same as for RayHitWorld: needs cleanup, partitioning, and variable 
        name revision. 
*/
vec rf ;
bool updateeditor = true ;
void update_editor()
{
    if (!updateeditor) return ;
    //bool advancing = true ;
    bool hitworld = false ;
    int hitplanes = 0 ;

    float t = 0 ;             // this records the parameter in the equation P' = P + tV to find where a ray touches a plane. 
    float min_t = 0 ;

    //pos = camera.pos ;
    vec camdir = camera.dir ;
    vec pos = camera.pos ;

    int gridscale = world.gridscale ; // maximum size of a selection square is half the world size. Else we wouldn't know! 
    //int gridscale = world.gridscale ; // maximum size of a selection square is half the world size. Else we wouldn't know! 
    geom_msgs_num = 0 ; 

    // Disgusting bit tricks just to make some lines more concise. 
    #define hittingplane (hitplanes>>(3*i))&0x07

    // Used to track where our ray front is 
    vec fcorner ;

    // ---------------------------------------------------------------------------------
    // TARGET FINDER PHASE 1: near world planes (if we're outside the world). 
    // ---------------------------------------------------------------------------------
    loopi(3)
    {
        hitplanes |= ( camdir[i]<0 ? 2*i : 2*i+1 ) << (3*i) ;
    }
    min_t = FLT_MAX ;

    have_ray_start_node = false ;

    vec dir ;
    vec front ; // ray front


    // Is our viewpoint inside the world? 
    if (camera.inworld(world))
    {
        front = pos ;
    }
    // If not, then we might be aiming at the world, which we will want to highlight
    else
    {
        loopi(3)
        {
            RayHitPlane( pos, camdir, wp[ hittingplane ], &t ) ; 
            if (t<min_t && t>0)
            {
                front = pos ;
                dir = camdir ;
                front.add(dir.mul(t)) ; // this works because dir (like camdir) has length 1. 

                int f1 = front[bds[i][0]] ; 
                int f2 = front[bds[i][1]] ; 
                int w = wp[2*i].offset ;

                // Here we check that the point where we hit the plane is
                // inside world limits. 
                if ( f1 > 0 && f1 < w && 
                     f2 > 0 && f2 < w )
                {
                    o = (hittingplane) ; min_t = t ;
                    fcorner = vec(front) ; 
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
            if (!( (o==2*j) || 
                   (o==2*j+1) )) { icorner[j] = (((int)(fcorner[j])) >> gridscale ) << gridscale ; } 
            // This nastiness is so that we are always flat against grid boundaries
            else { icorner[j] = floor( fcorner[j] ) ; }
        }
    }

    // ---------------------------------------------------------------------------------
    // TARGET FINDER PHASE 2: in-world content. 
    // ---------------------------------------------------------------------------------
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
    // If we aren't aiming at the world exterior, it's because we're inside the world. 
    // Ray front therefore starts at our position. 
    else { rayfront = pos ; }
    
    ray = dir ;              // reset ray to length 1
    
    loopj(3) { if (rayfront[j]<0) { rayfront[j]=0 ; } }

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

    // TODO FIXME: replace this with a function. A RayHitWorld function. 
    if (have_ray_start_node || camera.inworld(world))
    {
        i = (o>>1) ; // Which axis dominates the ray's movement into the next node

        vec ds = vec(0) ; 
        while (!havetarget)
        {
            if (steps>50) { break ; }                   // Kill runaway loops. Bigger than 50 needed to be able to point to large distances. 
            loopj(3) {ifront[j] = (int)rayfront[j] ;}   // place, more or less, ifront at rayfront (imprecision from converting float to int)
            ifront[i] += (ray[i]>=0?1:-1) ; // Snap to inside of node we're looking at. This deliberately takes us off node boundaries. 

            if ((ifront[i]>=WS&&ray[i]>0) || (ifront[i]<=0&&ray[i]<0)) 
            {
                sprintf(geom_msgs[geom_msgs_num], 
                    "RAY HITTING WORLD BOUNDARY") ; geom_msgs_num++ ;
                break ;
            }  // If this adjustment brings us out of the world - we're done

            Octant* oct = FindNode(ifront, &NS, &Nscale) ;                 // Find out what tree node encloses this point

            icorner = ifront ;
            loopj(3) { icorner[j] = (icorner[j] >> Nscale) << Nscale ; }    // Now icorner is right on the node corner. 

            if ( oct->has_geometry() ) 
            { 
                havetarget = true ; 
                aim = oct ;
                sprintf( geom_msgs[geom_msgs_num], 
                    "TARGET UNDER CURSOR: icorner at %d %d %d size = %d", 
                    icorner[0], icorner[1], icorner[2], NS) ; geom_msgs_num++ ;
                break ; // Target acquired. 
            }   
            vec f = rayfront ; r = 0.f ; d = 0.f ; t = 0.f ; 

            // Distances to next plane intersections
            loopj(3) { ds[j] = (ray[j]>=0?(float(icorner[j]+NS)-f[j]):(f[j]-float(icorner[j]))) ; }
            if (fabs(ray.x*ds.y) > fabs(ray.y*ds.x)){ r = ray.x ; d = ds.x ; i = 0 ; }
            else{ r = ray.y ; d = ds.y ; i = 1 ; }
            if ((fabs(ray.z*d) > fabs(r*ds.z))){ r = ray.z ; d = ds.z ; i = 2 ; }
            t = fabs(d/r) ;             // divisions are hopefully minimized 
            rayfront.add(ray.mul(t)) ;  // move ray to new plane
            ray = dir ;                 // reset ray to length 1

            steps++ ;
        } // end while !havetarget
    } // end if (have_ray_start_node)



    int gs = world.gridscale ;

    hitcorner = ifront ; // record where our ray front is, on the grid. 
    loopj(3) { ifront[j] = (ifront[j] >> gs) << gs ; }    // Now icorner is right on the node corner. 

    hitplanes = ( dir[i]>0 ? 2*i : 2*i+1 ) << (3*i) ;
    o = hittingplane ;  // o stands for orientation class. 

    // If we're pointing at something and we want to do something with it, now's the time to do it. 
    if (havetarget)
    { 
        sprintf( geom_msgs[geom_msgs_num], 
            "NODE TARGETED. selection corner at: %d %d %d    (%d steps) ORIENTATION=%d", 
            icorner.x, icorner.y, icorner.z, steps, o) ; geom_msgs_num++ ;
    }
        
    // ---------------------------------------------------------------------------------
    // TARGET FINDER PHASE 3: far world planes. 
    // ---------------------------------------------------------------------------------
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
        
        if (!havetarget)
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
                        farcorner = vec(front) ;
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
                if (!( (o==2*j) || 
                       (o==2*j+1) )) { farcorner[j] = ((farcorner[j]) >> gridscale ) << gridscale ; }
                // This nastiness is so that we are always flat against grid boundaries
//UGGGGGG else { farcorner[j] = floor( farcorner[j] + .5f ) ; }
                else { farcorner[j] = floor( farcorner[j] + 1 ) ; }

/*
sprintf( geom_msgs[geom_msgs_num], 
    "  farcorner %d = %d", 
    j, farcorner[j]
    ) ; geom_msgs_num++ ;
*/
            }
        }
        sprintf( geom_msgs[geom_msgs_num], 
            "  hittingplane = %d", 
	    o
	    ) ; geom_msgs_num++ ;
        sprintf( geom_msgs[geom_msgs_num], 
            "  farcorner = %d %d %d", 
            farcorner.x, farcorner.y, farcorner.z ) ; geom_msgs_num++ ;
        sprintf( geom_msgs[geom_msgs_num], 
            " icorner = %d %d %d", 
            icorner.x, icorner.y, icorner.z ) ; geom_msgs_num++ ;
            
    } // end if (!havetarget)

//    sprintf(geom_msgs[geom_msgs_num], "") ; geom_msgs_num++ ;
//    sprintf(geom_msgs[geom_msgs_num], "") ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "NUMBER OF GEOMS = %d", worldgeometry.length()) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], " corner hit: %d  translation: %d", facecorner, oitc[facecorner]) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "IFRONT:    %d %d %d", ifront[0], ifront[1], ifront[2]) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "SEL_START: %d %d %d", sel_start[0], sel_start[1], sel_start[2]) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "SEL_END:   %d %d %d", sel_end[0], sel_end[1], sel_end[2]) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "SEL_LIM0:   %d %d %d", sel_lim[0][0], sel_lim[0][1], sel_lim[0][2]) ; geom_msgs_num++ ;
    sprintf(geom_msgs[geom_msgs_num], "SEL_LIM1:   %d %d %d", sel_lim[1][0], sel_lim[1][1], sel_lim[1][2]) ; geom_msgs_num++ ;
// sprintf(geom_msgs[geom_msgs_num], "update_editor: point at %d %d %d", icorner[0], icorner[1], icorner[2]) ; geom_msgs_num++ ;

    /*
        At this point we should have one of the two following: 

            - a target inside the world - geometry or an entity
            or
            - a cursor framing a piece of the world boundary, at ray entry or exit or both. 

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

extern bool mbutton1down ;
    if (mbutton1down) { set_sel_end() ; }
    
    ComputeAimedCorner() ;

    rf = rayfront ;
} // end update_editor 


/*
*/
void draw_rayfront()
{
    glDisable( GL_DEPTH_TEST ) ;
        glPointSize(10.0) ;
        glBegin(GL_POINTS) ;
            glVertex3fv(rf.v) ;
        glEnd() ;
    glEnable( GL_DEPTH_TEST ) ;
}


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
    if (!geom) return  ;            // TODO: log error. 
    if (needsupdate()) return  ;    // TODO: log error. 

    glDeleteBuffersARB(1,&(geom->vertVBOid)) ;
    // TODO: normal data? 
    glDeleteBuffers(1,&(geom->texVBOid)) ;

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
    Octant* CN = in_oct ;           // the root of the subtree to delete
    Octant* path[20] = {NULL} ;
    loopi(20) { path[i] = NULL ; }
    path[0] = CN ; 
    int32_t idxs[20] ;   // idxs[d] tracks which child of node path[d] is being used, if any. 
    loopi(20) { idxs[i] = 0 ; }

    int32_t d = 0 ;             // depth
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

        CN->clear_all() ;
        
        // Done with last node; go back up the tree. 
        path[d] = NULL ;
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
        Returns the node which encloses that point. 
        Assigns the size of the returned node. (optional)
        Assigns the scale of the returned node (optional) 

    Possible speed optimization: 
        Preserve a path of nodes to the point of interest.  If the next point
        of search is inside any of the ancestors of the current target, then we
        might be able to find more quickly the parent to start the descent from
        on the next search. 
*/
Octant* FindNode(ivec at, int* out_size=NULL, int* nScale=NULL)
{
    int WS = world.scale ;
    int i = 0 ;
    Octant* oct = &world.root ;

    while (WS>0)
    {
        if ( oct->children ) 
        {
            i = octastep(at.x,at.y,at.z,WS) ;
            oct = &oct->children[i] ;
        }
        else { break ; }
        WS-- ;
    }

    // If relevant pointers provided, assign size and scale data. 
    if (oct) 
    {
        if (out_size) { *out_size = 2<<WS ; }
        if (nScale)   { *nScale   = WS+1  ; }
    }
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
    return oct ;
}


/*
    DESCRIPTION:
        Given a point in space, retrieve all the nodes that contain this point, 
        from root to the leaf node encompassing it. 

        FIXME: this code is possibly incomplete and definitely untested. (even if it looks reasonable)
*/
void GetNodePath(ivec p, Octant** path)
{
    Octant* oct = &world.root ;
    int GS = world.gridscale ;
    int WS = world.scale ;
    int depth = 0 ;
    int i = 0 ;

    while (depth<WS)
    {
        path[depth] = oct ;
        if ( oct->children ) 
        {
            i = octastep(p.x,p.y,p.z,WS) ;
            oct = &oct->children[i] ;
        }
        else { break ; }
        depth++ ;
    }
    // Fill up the rest with NULL
    while (depth<WS)
    {
        path[depth] = NULL ;
        depth++ ;
    }
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

    if (out_size) { if (oct) { *out_size = 2<<CS ; } }

    return oct ;
}

/*
    This function marks a subtree as requiring update. 

    It is used when a node is being flagged for update, and 
    is seen to possess its own geom - which means all nodes 
    under this node need to be taken into account on the 
    next pass to reconstruct the geometry under this node. 

    TODO: find if this function or its current usage do more work
    than they should. 

*/
void FlagSubtreeForUpdate( Octant* parent) 
{
    Octant* CN = parent ;
    Octant* path[20] = {NULL} ;
    loopi(20) { path[i] = NULL ; }
    path[0] = CN ;
    int32_t idxs[20] ;
    loopi(20) { idxs[i] = 0 ; }

    int32_t d = 0 ;
    while (d>=0) // Here starts iterative traversal
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
        CN->clear_geom() ;
        CN->setneedsupdate() ; 

        path[d] = NULL ;
        d-- ;
        if (d<0) { break ; }    // Past the root? Then we're done. 
        CN = path[d] ;          // else go up tree
        idxs[d]++ ;             // Next time we visit this node, it'll be next child. 
    }   // end while d>=0
}


/*
    Function: FlagPathForUpdate. 

    This function takes a sequence of nodes where every node at position 
    n+1 is the child of the node at position n, and goes up this sequence 
    and marks nodes as requiring updates, if they haven't been marked 
    already. 

    The mark is left by setting a node's flag to needs_update. The function's
    task is finished when it reaches the root of this tree (depth 0) or if it
    encounters a node that was already marked for update. 

    Assumption: any node on the path from depth _depth to 0 is non-null and
    those above depth _depth have children.  These nodes do not necessarily
    have a non-null geom, however. 
    
*/
void FlagPathForUpdate( Octant** path, int depth) 
{
    int d = depth ;
    while (d>=0) // Question: what events prevent us from stopping the course upwards when we encounter a needs_update? 
    {
        if (path[d]->geom)
        {
            /*
                Every node on this path should be flagged for update, so if we 
                find one that hasn't been flagged, flag it and its subtree. 
            */
            if (!path[d]->needsupdate()) { FlagSubtreeForUpdate( path[d] ) ; }
        }
        path[d]->setneedsupdate() ; 
        d-- ;
    }
}


// Array: face child indexes. 
// Specifies which four indexes specify the children of a given face. 
// Makes it possible to use an octree node as a quadtree node. 
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

void SubdivideOctant( Octant** path , int depth )
{
    // TODO: have this function preserve the shape of cubes being subdivided, 
    // to the extent possible with our current shaped cube setup. 
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
    }
    // 4. assign to all the faces concerned the tex slots currently used 
    // by the parent node. 
    // 5. flag this subtree as needing update. 

    // cancel the parent's geometry at this level, now that the children have it. 
    oct->clear_geometry() ;
}

bool const PointInsideWorld( 
    ivec cp     /* corner position*/, 
    World& w    /* world to check against */
    ) 
{
    loopi(3) { if (cp[i]<0 || cp[i]==w.size) { return false ; } }
    return true ;
}


/*
    We locate the nib we want by using the fact that the edges member of Octant 
    holds first the X coordinates (8 nibbles in a row) followed by the Y coords 
    and then the Z. 

    TODO: new coord system. 
*/
void MoveNodeCorner(Octant* node, int face, int corneridx, bool in)
{
    int coord = face>>1 ;   // X, Y or Z? 
    int shift = 4*corneridx ;
    int eg = node->eg[coord] ;
    int mask = ~(0xF<< shift) ;
    int neweg = eg & mask ;     // Zero out the part we're updating. 
    int val = (eg >> shift)&0xF ;

    // TODO: prevent val from getting too smal if the opposite face's value 
    // currently matches val in Z. 
    if (in) { if (val>7)  { val-- ; } }
    else    { if (val<15) { val++ ; } }

    neweg += (val<<shift) ;
    node->eg[coord] = neweg ;

    // node becomes 'unfull' is edges are reduced
    if (val<15) { node->flags ^= FULL ; }

    // TODO: set the face diagonals flags. This requires taking the potential normals 
    // on this face, and whether they have a negative or positive angle. 
    loopi(6) if (node->FaceNonFlat(i))
    {
        // Compute normals of both face triangles

        // if normals form a negative angle then do something!
    }


    // If all edges are now full, we are full. 
    if (node->eg[0] == 0xFFFFFFFF &&
        node->eg[1] == 0xFFFFFFFF &&
        node->eg[2] == 0xFFFFFFFF ) { node->flags |= FULL ; }

    return ;
}


/*
    UNDER CONSTRUCTION: this is a revision of the modify function to 
    make it more concise and deduplicate some logic. Not yet finished or 
    optimal in any way other than code reduction. 
*/
void modify( void * _in )
{
    if (!havesel) return ;          
    
    bool in = *(bool *)_in ;

    int O = (sel_o) ;                   // principal orientation plane of our current selection 
    int x = X(O), y = Y(O), z = Z(O) ;  // define coordinate system for active selection plane  

    int WSc     =   world.scale ;       // power of two that defines world size
    int GS      =   world.gridsize ;    // current size of cube units 
    int WGSc    =   world.gridscale ;   // current editor grid scale
    int CGS     =   WSc ;               // current grid scale while following a path down the tree

    Octant* oct = &world.root ;
    Octant* path[20] = {NULL} ; loopi(20) { path[i] = NULL ;}
    int d = 0 ;     // depth; root has depth 0 
    path[0] = oct ;

    // NC = "newcube" is the (moving) prime corner of our selection volume. 
    // Every creation, shaping or deletion operation uses this as the identifier
    // For finding nodes. 
    // TODO: eventually, switch to sel_lim instead of sel_start and sel_end. But only eventually, since 
    // for now it works fine. The goal will be to simplify to only one method of tracking selections. 
    ivec NC ;
    loopi(3) { NC[i] = min( sel_start[i], sel_end[i] ) ; }

    // Depending on our current operation, set our control variables.  
    bool creating = false ; bool deleting = false ;
    if (!shaping) { if ( in ) { deleting = true ;} else { creating = true ;} }

    // If deleting, our newcube's prime corner is going to be 'into' the geometry, 
    // meaning that our local-coord Z is decrementing. For extrusion (creating), 
    // Z comes up. For shaping, the selection must start at the same place as for deleting. 
    if (deleting||shaping) 
                  { if (  (O%2) ){NC[z] -= wp[O][z] * (sel_grid_size) ;} }
    if (creating) { if ( !(O%2) ){NC[z] += wp[O][z] * (sel_grid_size) ;} }

    // If the new prime corner specifies a cube outside our world, finish. 
    if (!PointInsideWorld(NC, world)) { return ; }

    int lowerx  = -1 ; int lowery  = -1 ;
    int x_count = -1 ; int y_count = -1 ;
    int upperx  =  1 ; int uppery  =  1 ;
    NC[x] -= 1 ;
    NC[y] -= 1 ;
    ivec NCstart = NC ; // save NC so we can get back to it for every row

    // When are we done? When we've covered every X and Y for both the
    // starting and ending layer. 
    bool done = false ; 
    bool modifyinglayer = true ; 

    //--------------------------------------------------------------------------
    while( !done ) 
    {
        done = true ; 
        while ( y_count < sel_counts[y]+upperx )
        {
            while ( x_count < sel_counts[x]+uppery )
            {
                bool inside = false ;   // Tracks whether we are inside our modification zone
                if ( (x_count>=0 && x_count<sel_counts[x]) &&
                     (y_count>=0 && y_count<sel_counts[y]) ) { inside = true ; }
               
                // Path to node 
                while (CGS>=WGSc)
                {
                    // TODO: generalize to interchangeable modification functions. 
                    if (!oct->children )
                    {
                        // Subdivision to only delete selected node(s). Surrounding geometry preserved. 
                        if (deleting||shaping) // TODO: maybe replace with 'removing'
                        { 
                            if (inside && modifyinglayer && oct->has_geometry()) { SubdivideOctant( path, d ) ; } 
                            else { break ; } 
                        }
                        // New nodes need their ancestors to exist. 
                        else if (creating) // TODO: maybe replace with 'adding'
                        { 
                            if (inside && modifyinglayer) { new_octants(oct) ; } 
                            else { break ; } 
                        }
                    }

                    // Go further down tree
                    int i = octastep(NC.x,NC.y,NC.z,CGS) ;
                    oct = &oct->children[i] ;
                    d++ ;
                    path[d] = oct ;
                    CGS-- ;
                }

                // Cube Modification Happens Here
                if (inside && modifyinglayer)
                {
                    // Create a solid node
                    if (creating)       
                    {
                        if (oct->children) { deletesubtree( oct ) ; }
                        oct->set_all_edges() ; 
                        numchildren++ ; // TODO: do we need this for anything? 
                    }
                    // Delete the current node and everything in it!
                    else if (deleting)  { deletesubtree( oct ) ; } 
                    // Bending cubes. 
                    else if (shaping) { MoveNodeCorner(oct, o, facecorner, in) ; }
                } 
                // -------------------------------------------------------------
               
                // Order a refresh to visible geometry for the path to this node
                if (oct) FlagPathForUpdate( path, d ) ;
                
                // Return to top of octtree
                CGS = WSc ; 
                d = 0 ; 
                oct = &world.root ;

                // Next node in this row
                NC[x] += GS ; 
                x_count ++ ;
            } // end while ( x_count < sel_counts[x]+upperx )

            x_count = lowerx ;      // Reset horizontal position
            NC[x] = NCstart[x] ;    
            y_count ++ ;            // Next vertical position
            NC[y] += GS ;
        } // end while ( y_count < sel_counts[y]+uppery )

        // ---------------------------------------------------------------------
        // Logic to adjust whether we move up-layer or down-layer...
        // TODO: this block is pretty abominable. Should improve if doable. 
        if (!shaping)
        {   
            done = false ;
            int layerinc = -1 ;
            if (modifyinglayer) { modifyinglayer = false ; }
            else                { done = true ; }

            // Moving to next layer 
            x_count = lowerx ;
            y_count = lowery ;
            NCstart[z] += Dz(O)*GS*layerinc ;
            NC = NCstart ;
        }
        else { done = true ; } // If we were shaping, we only have the first layer to deal with. 
        // ---------------------------------------------------------------------

    } // end while( !done ) 
    
    //--------------------------------------------------------------------------
    // Move selection highlights to match new additions. 
    if (!shaping)
    {
        int inc = 1 ;

        if (deleting) { inc = -1 ; }

        sel_end  [sel_o>>1] += GS*Dz(O)*inc ;
        sel_start[sel_o>>1] = sel_end[sel_o>>1] ; // GS*Dz(O) ;
       
        sel_lim[sel_o%2][sel_o>>1] += GS*Dz(O)*inc ;
        //if ((  (sel_o%2)     && sel_lim[sel_o%2][sel_o>>1]<world.size)||
         //   ((((sel_o+1)%2)) && sel_lim[(sel_o+1)%2][sel_o>>1]>0))
        if ((  (sel_lim[sel_o%2][sel_o>>1] > 0) && (sel_lim[sel_o%2][sel_o>>1] < world.size)  ))
        {
            /*
                As sel_lim[0] and sel_lim[1] are the extremes of a non-zero-volume 
                selection, they can't both be in the place of the selection face. 
                This block adjusts for this. 

                First: determine if we're in the 'plusses' or 'minuses' (odd vs. 
                even numbered faces)
                Next: 

                    extracting from odd faces means upper limit has to shrink
                    extracting from even faces means lower limit has to grow

                    adding to odd faces means upper limit has to grow
                    adding to even faces means lower limit has to shrink

            */
            if (sel_lim[sel_o%2][sel_o>>1]==sel_lim[(sel_o+1)%2][sel_o>>1])
            {
                sel_lim[(sel_o+1)%2][sel_o>>1] += GS*Dz(O)*inc ;
            }
        }
    }

    // Identify pieces that need to be rebuilt
    // TODO: FIXME: make this a world class function
    AnalyzeGeometry(&world.root, vec(0,0,0), world.scale) ;

    // Rebuild updated geometry
    // TODO: FIXME: make this a world class function
    AssignNewVBOs(&world.root, vec(0,0,0),world.scale) ;
    //--------------------------------------------------------------------------

} // end modify 


/*
    DESCRIPTION: 
    (TODO: make the pushing behavior apply to a region of selected corners. )
        When we are pointing at a node, the cursor is closer to one of the
        corners.  This corner is the corner that will be pushed, if there is
        room to push that corner in this node. 

        The work for this will likely be done in the modify() function. 
*/
void PushCorner() 
{
    shaping = true ;
    bool inwards = true ;
    modify(&inwards) ;
    shaping = false ;
}


/*
    DESCRIPTION: 
    (TODO: make the pulling behavior apply to a region of selected corners. )
        When we are pointing at a node, the cursor is closer to one of the
        corners.  This corner is the corner that will be pulled, if there is
        room to pull that corner in this node. 

    How do we know what corner we're pointing at? We use two pieces of data for
    this: the node currently aimed at by the edit cursor ('aim', defined near
    the top of the file) and the ifront vector, which identifies which XY
    integer coordinates are closest to the edit cursor.

    aim is used to modify the node's geometry values once 

    Start at the last editor ray that was shot through the world. 
*/
void PullCorner() 
{
    shaping = true ;
    bool inwards = false ;
    modify(&inwards) ;
    shaping = false ;
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
    indicated by the node's flag's NEEDS_UPDATE flag being set) When it reaches
    leaf nodes of portions of the tree flagged NEEDS_UPDATE, it figures out which faces of
    those nodes are visible. 

    Once the visible faces of a node have been identified, the lvc of that node 
    is assigned. lvc stands for leaf vertex count, or the number of vertices 
    required to specify the triangles that make the visible faces of this node. 

    The way by which face visibility is recorded is by assigning a tex id to
    every face. In the case of a visible face, an id from 1 to 255 is used. In
    the case of an invisible face, id 0 is used. 

    Detailed procedure: 

        - traverse tree iteratively
        - for each node: 
            - if geom needs update, process each face for visibility

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

    char idxs[20] ;   // idxs[d] tracks which child of node path[d] is being used, if any. 
    loopi(20) { idxs[i] = 0 ; }
    
    Octant* path[20] = {NULL} ;
//    loopi(20) { path[i] = NULL ; }

    path[0] = CN ; // This is and always will be the root. 

    int32_t d = 0 ;     // We start at the root. 
    ivec pos = vec(0,0,0) ;

    while (d>=0) 
    {
        if ( CN->needsupdate()) 
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
                CN->vc = 0 ;
                if ( CN->has_geometry() )
                {
                    ivec pcorner ;          // probe corner
                    Octant* anode ;         // probe node
                    int NS = 1<<(S-d+1) ;   // present Node size. 
                    
                    // face visibility
                    for (int face=0;face<6;face++)
                    {
                        int O = face ;
                        pcorner = pos ;
                        /* 
                            We position the probe vector (pcorner) just inside
                            of the node adjacent in the direction of the
                            current face.  Then we retrieve from the tree the
                            node at that location.  Once we have 'anode', or
                            the node adjacent to the current face, we know
                            whether it is solid, and whether it is bigger than
                            us.  
                        */
                        pcorner[face>>1] = pos[face>>1]+Dz(face)*(1 + (face%2)*(NS)) ; 

                        if ( pcorner.x>world.size || pcorner.x<0 ||
                             pcorner.y>world.size || pcorner.y<0 ||
                             pcorner.z>world.size || pcorner.z<0 )
                        {
                            CN->tex[face] = -1 ;    // Any face that is only visible from outside our world is removed. 
                            continue ;
                        }

                        anode = FindSizedNode(pcorner, NS) ;

                            /* 
                                Opposite face numbers only differ in their first
                                bit (2^0), hence the flipping operation done here
                                to get a face's opposite. 

                                FIXME: I know that a shaped cube doesn't
                                register as solid by smaller cubes, even if
                                they are against a flat face of and hidden by
                                shaped cube. 
                            */
                            bool showface = false ;
                            if (!anode->has_children())
                            {
                                if (anode->has_geometry()) 
                                {
                                    showface = !CN->FaceHiddenBy(face, anode) ;
                                }
                                else { showface = true ; }
                            }
                            else
                            {
                                showface = !FaceCovered(anode, (face&0x6|((~face)&0x1))) ;
                            }
                            if (showface)
                            {
                            // FIXME: add a check for whether a face is covered
                            // even though it is covered by smaller nodes. 
                                if (CN->tex[face] == -1)  
                                {
                                    CN->tex[face] = engine.activetex ;
                                }
                                CN->vc += 6 ;
                            }   // end if neighbor is not in the way of this face
                            else { CN->tex[face] = -1 ; }
                        pcorner = pos ; // do we need to reset this here?
                    }   // end for every face
                }   // end if ( CN->has_geometry() )
            }   // end if not have children
        }   // end if need update

        // going up the tree. 
        path[d] = NULL ;
        d-- ; 
        
        if (d<0) { break ; }    // Past the root? Then we're done. 
        CN = path[d] ;

        // adjust position of reference vector
        loopi(3) {incr = (1<<(S-d)) ; yesorno = ((idxs[d]>>i)&1) ;pos.v[i] -= yesorno * incr ;}
        idxs[d]++ ;   // Next time we visit this node's children, it'll be next child. 
    }  // end while d>=0
}


/*
    This function traverses the tree, following nodes that are marked for 
    update. 

    When it finds a node that has vc<=VERTS_PER_GEOM, and has a larger vc than all 
    its children, then it makes a vbo from it. 
*/
void AssignNewVBOs(Octant* tree, ivec in_corner, int scale)
{

    int32_t d = 0 ;               // depth
    int32_t S = scale ;           // Scale of this world
    Octant* CN = &world.root ;    // current node 
    Octant* CC = NULL ;           // current child 

    int32_t idxs[20] ;   // path indexes. -1 means unused. 
    loopi(20) {idxs[i] = 0 ;} idxs[0] = 0 ;

    Octant* path[20] ;   // path Octants. 
    loopi(20) {path[i]=NULL ;} path[0] = CN ; 

    // These are used to state positions which tell us where things are located when we call BuildSubtreeGeometry.
    int32_t yesorno = 0 ;
    int32_t incr = 0 ;
    ivec pos = in_corner ;

    while (d>=0)
    {
        if ( CN->needsupdate()) 
        {
            if ( CN->children && CN->vc>0)
            {
                // Ok we have children. Either we make a geom here and now, or we keep going down our children. 
                Octant* CC = &CN->children[0] ;
                bool useThisNode = false ;
                
                if ( CN->vc<=VERTS_PER_GEOM ) //&& CN->vc>=VERTS_PER_GEOM)
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
                    BuildSubtreeGeometry( CN, pos, S-d) ; // FIXME: move this to phase 3 
                }
                else 
                {
                    if (CN->geom || CN->needsupdate())
                    {
                        // TODO: figure out if this here is needed or if it's already done in FlagSubtreeForUpdate
                        CN->clear_geom() ;
                    }
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
                }
            } // end if have children and contain geometry
            else // If we don't have children, then maybe we have geometry.
            {
                // If we get to here, then it's time to build this. 
                if ( CN->has_geometry() )
                {
                    BuildSubtreeGeometry( CN, pos, S-d) ; // FIXME: move this to phase 3 
                }
            }
        } // end if ( CN->needsupdate() )

        // At this point, we're done with the present node, so we cancel its need for update if it's there. 
        // These last lines of the while loop make up the 'going up the tree' action. 
        if (CN->needsupdate()) { CN->doneupdate() ; }

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

/*
    DESCRIPTION: 
        Creates a single VBO with the geometry that is contained by a particular 
        node. 

    It makes the 'parent' parameter Octant the host of a Geom which describes 
    the geometry under this parent. 
    
    Inputs: 

        parameters: 
            parent -> the node that roots this subtree
            in_corner -> the all-mins corner of this root node
            NS -> the size of the node that contains this subtree. 

        sel_o -> 


        A way to understand what globals are available here is to 
        know that while geometry is being created (at least during 
        edit mode), we can be sure that the active selection will 
        not change. 

        This function is called on a node's interior when no more than VERTS_PER_GEOM
        vertices will describe the geometry. 

*/
void BuildSubtreeGeometry(Octant* root, ivec in_corner, int _NS)
{
    Octant* CN = root ;
    // The geom is held by the node at the root of this subtree. 
    if (CN->geom==NULL||CN->needsupdate()) { CN->geom = new Geom() ; }
    
    int d = 0 ;
    Octant* path[20] ;
    int32_t idxs[20] ;
    ivec pos = in_corner ;

    ivec* verts         = CN->geom->vertices ;
    vec* tex            = CN->geom->texcoords ;
//    vec* texatlascoords = CN->geom->texcoords ;
    ushort* elements    = CN->geom->idxs ;


    int t = 0 ;
    int nv = 0 ;

    path[0]=NULL ;  //loopi(20) {path[i]=NULL ;} TODO: if you feel confident, remove these comments. 
    idxs[0] = 0 ;   //loopi(20) {idxs[i] = 0 ;}
    path[0] = CN ;

    int32_t facecount = 0 ;
    struct veclookup
    {
        ivec v ;            // R^3 coordinates
        vec t ;             // texture coordinates
        ushort i ;          // index
        UT_hash_handle hh ; // hashing handle

        // TODO: see if this hash table is fast at all. 
        // Example: try sauer's hash table (spending no more than 5 minutes trying to see if you can figure it out
        // Example: try a quick homebrew dilly-o and see what kinds of results I can get from that
    } ;

    struct veclookup* vecidxs = NULL ;
    uint32_t numidxs = 0 ;
    uint32_t numuniqueidxs = 0 ;    // the number of distinct indexes seen so far
    uint32_t idx = 0 ;
    
    // When using uthash, we use the v and t from struct veclookup to create a unique key. 
    uint32_t keylen = sizeof(ivec) + sizeof(vec) ; 

    // TODO: replace with pointer to this Geom's indexes list
    ushort indexes[VERTS_PER_GEOM] ; indexes[0] = 0 ;

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
            // If we have children, then maybe we have a Geom. If we do, we'll know if it needs 
            // rebuilding if its NEEDS_UPDATE flag has been set. 
            if (idxs[d]<8)
            {
                // We're going down to a child, marked relative to its parent by idxs[d]. 
                incr = (1<<(SI-d)) ; // This can be replaced with NS and NS reduced by half or increased by twice as needed. 
                loopi(3) 
                { 
                    incornot = ((idxs[d]>>i)&1) ;   // I pray Satan forgives me for being this brazen about using such nonsense code
                    pos.v[i] += incornot * incr ; 
                }
                CN = &CN->children[idxs[d]] ; 

                // new node: discard any geometry it holds
                d++ ; 
                path[d] = CN ; 
                idxs[d] = 0 ; // Start the children at this level. 

                continue ;
            }
        }
        // If we don't have children, then maybe we have geometry.
        else
        {
            if ( CN->has_geometry() ) // If we have geometry, we put it into the VBO. 
            {
//                ivec pcorner ;          // probe corner
                Octant* pnode ;         // probe node
                int NS = 1<<(SI-d+1) ;  // Node size. 
                int ns = 0 ;            // neighbor size
                int nsfrac = NS>>3 ;    // nsfrac is a 1/8th fraction of the node size. 

// TODO: fun times: profile with this static and not. Does 
// stack frame mean more likely in cache and any faster? 
                //static ivec corners[8] ;
                ivec corners[8] ;
                
                bool havevisibleface = false ; 
                loopi(6) { if (CN->tex[i]>=0) { havevisibleface = true ; break ; } }
    
// TODO: maybe profile this code and a looped version (which would require some
// index arrays), to see which is more gooder.
                if (havevisibleface)
                {
                    //pcorner = pos ; 
                    // v0
                    char x = CN->eg[0] & 0xF ; char y = CN->eg[1] & 0xF ; char z = CN->eg[2] & 0xF ;
//printf("");
                    corners[0] = pos ; //pcorner ; 
                    corners[0].x += (15-x)*nsfrac ; corners[0].y += (15-y)*nsfrac ; corners[0].z += (15-z)*nsfrac ; 

                    // v1
                    x = (CN->eg[0]>>4) & 0xF ;  y = (CN->eg[1]>>4) & 0xF ;  z = (CN->eg[2]>>4) & 0xF ;
                    corners[1] = pos ; //pcorner ; 
                    corners[1].x += (x-7)*nsfrac ; corners[1].y += (15-y)*nsfrac ; corners[1].z += (15-z)*nsfrac ; 

                    // v2
                    x = (CN->eg[0]>>8) & 0xF ;  y = (CN->eg[1]>>8) & 0xF ;  z = (CN->eg[2]>>8) & 0xF ;
                    corners[2] = pos ; //pcorner ; 
                    corners[2].x += (15-x)*nsfrac ; corners[2].y += (y-7)*nsfrac ; corners[2].z += (15-z)*nsfrac ; 

                    // v3
                    x = (CN->eg[0]>>12) & 0xF ;  y = (CN->eg[1]>>12) & 0xF ;  z = (CN->eg[2]>>12) & 0xF ;
                    corners[3] = pos ; //pcorner ; 
                    corners[3].x += (x-7)*nsfrac ; corners[3].y += (y-7)*nsfrac ; corners[3].z += (15-z)*nsfrac ; 

                    // v4
                    x = (CN->eg[0]>>16) & 0xF ;  y = (CN->eg[1]>>16) & 0xF ;  z = (CN->eg[2]>>16) & 0xF ;
                    corners[4] = pos ; //pcorner ; 
                    corners[4].x += (15-x)*nsfrac ; corners[4].y += (15-y)*nsfrac ; corners[4].z += (z-7)*nsfrac ; 

                    // v5
                    x = (CN->eg[0]>>20) & 0xF ;  y = (CN->eg[1]>>20) & 0xF ;  z = (CN->eg[2]>>20) & 0xF ;
                    corners[5] = pos ; //pcorner ; 
                    corners[5].x += (x-7)*nsfrac ; corners[5].y += (15-y)*nsfrac ; corners[5].z += (z-7)*nsfrac ; 

                    // v6
                    x = (CN->eg[0]>>24) & 0xF ;  y = (CN->eg[1]>>24) & 0xF ;  z = (CN->eg[2]>>24) & 0xF ;
                    corners[6] = pos ; //pcorner ; 
                    corners[6].x += (15-x)*nsfrac ; corners[6].y += (y-7)*nsfrac ; corners[6].z += (z-7)*nsfrac ; 

                    // v7
                    x = (CN->eg[0]>>28) & 0xF ;  y = (CN->eg[1]>>28) & 0xF ;  z = (CN->eg[2]>>28) & 0xF ;
                    corners[7] = pos ; //pcorner ; 
                    corners[7].x += (x-7)*nsfrac ; corners[7].y += (y-7)*nsfrac ; corners[7].z += (z-7)*nsfrac ; 

                }

                for (int face=0;face<6;face++)
                {
                    // If this face is visible, then make some triangles for it. 
                    t = CN->tex[face] ;
                    if (t>=0)
                    {
                        facecount++ ;
                        //float tc = ((float)t+0.5)/(float)engine.numtex ;
                        float tc = t ;
                        // Face Builder. 
                        // Every face is defined with: 
                        /*
                            A ++++ D
                              + /+
                              +/ +
                            B ++++ C

                            Or a mirror image, where the diagonal is flipped. 

                            (Here the sequence ABCD is used to define the four corners 
                            of a texture that will be used to paint this face.)
                            
                            Triangle 1: vertices = ABD  texcoords: (0,0) (0,1) (1,0)  
                            Triangle 2: vertices = DBC  texcoords: (1,0) (0,1) (1,0)  

                        */
                        // FIXME: TODO: set up texture slots where different textures have different scale, 
                        //              orientation, noise functions or secondary textures, transformations, etc. 

                        // TODO: review this to accomodate moving cube corners.  
                        #define DIV 1/256   // How much space in world a whole texture occupies. 256 world units = 4m. 
                        float x0 = (float)corners[nfi[face][2]][X(face)]*DIV ; // scaled so that the full-size texture is 128 units tall and wide. 
                        float x1 = (float)corners[nfi[face][0]][X(face)]*DIV ;
                        float y0 = (float)corners[nfi[face][2]][Y(face)]*DIV ;
                        float y1 = (float)corners[nfi[face][0]][Y(face)]*DIV ;

                        // If we're using a texture atlas, we have to adjust texture offsets
                        if (e.texatlas)
                        {
                            #define TEX_ATLAS_UNIT 512/4096 // 1/8th
                            #define ROW_SIZE 512/8  // 32 normally

//                        yfraction = ((t - (t%tarc)) * 512 / 8 )
//zzz
                            float xo = ( ( t % e.texatlasrowcount ) * e.texatlastilesize ) * TEX_ATLAS_UNIT ; // X offset into texture atlas
                            float yo = (t - (t % e.texatlasrowcount )) * 32 ; // TODO BAD MAN HARD CODED LIKELY VARIABLE NUMBER! // Y offset into texture atlas

//printf ("\n tex offsets: %f, %f", xo, yo)  ;
// We need two parts: 
/*
    1 - An offset into the atlas (xo, yo)
    2 - T

*/

                            // Converts extents from one-texture to atlas extents (where the coord needs to fall within the sub texture in the atlas)
//if (face==1)
{
//printf ("\n extents before: (%f, %f), (%f, %f)", x0, y0, x1, y1)  ;
                            x0 = xo + (x0-floor(x0))*TEX_ATLAS_UNIT ; //512.0 ;
                            x1 = xo + (x1-floor(x1)+1)*TEX_ATLAS_UNIT ; //512.0 ;
                            y0 = yo + (y0-floor(y0)+1)*TEX_ATLAS_UNIT ; //512.0 ;
                            y1 = yo + (y1-floor(y1))*TEX_ATLAS_UNIT ; //512.0 ;
//printf ("\n extents after:  (%f, %f), (%f, %f)", x0, y0, x1, y1)  ;
}
                        }

                        // Which diagonal? Chosen so that nodes are never concave. This simplifies
                        // the computation of collisions somewhat. 
                        //TODO: replace for texarray case  - if (0) // diagonal orientation 1
                        if (e.texatlas)
                        {
                            // Ok regala tex coords conversion to tex atlas. 
                            // We have to simulate pattern repeat even though the current texture is lying next to a different 
                            // texture in the atlas. 

                            // Example: coords (0,256) will map to 1.0 in texture. No problem. But 320 (5m, or 256 + 64) should 
                            // fall on 1.25 which would map to 0.25 in regular repeat if using a whole texture. 
                            // Now, though, we only have a region in the atlas defined by the size of the atlas, the size of the 
                            // subtextures, and the index of our current texture.

                            // tc

// TODO Decision point: whether to go 012, 230, or 130, 231 for triangles. TODO


                            // triangle 1 
                            tex[nv]   = vec(x0,y0,tc) ; verts[nv]   = corners[nfi[face][2]] ;
                            tex[nv+1] = vec(x0,y1,tc) ; verts[nv+1] = corners[nfi[face][3]] ;
                            tex[nv+2] = vec(x1,y0,tc) ; verts[nv+2] = corners[nfi[face][1]] ;
                            
                            // triangle 2 
                            tex[nv+3] = vec(x1,y0,tc) ; verts[nv+3] = corners[nfi[face][1]] ;
                            tex[nv+4] = vec(x0,y1,tc) ; verts[nv+4] = corners[nfi[face][3]] ;
                            tex[nv+5] = vec(x1,y1,tc) ; verts[nv+5] = corners[nfi[face][0]] ;
/* 
                            // triangle 1 
                            tex[nv]   = vec(x0,-y0,tc) ; verts[nv]   = corners[nfi[face][2]] ;
                            tex[nv+1] = vec(x0,-y1,tc) ; verts[nv+1] = corners[nfi[face][3]] ;
                            tex[nv+2] = vec(x1,-y0,tc) ; verts[nv+2] = corners[nfi[face][1]] ;
                            
                            // triangle 2 
                            tex[nv+3] = vec(x1,-y0,tc) ; verts[nv+3] = corners[nfi[face][1]] ;
                            tex[nv+4] = vec(x0,-y1,tc) ; verts[nv+4] = corners[nfi[face][3]] ;
                            tex[nv+5] = vec(x1,-y1,tc) ; verts[nv+5] = corners[nfi[face][0]] ;
*/
                        }
                        else // or if face diagonals are to be flipped
                        {
                            // triangle 1 
                            tex[nv]   = vec(x0,-y0,tc) ; verts[nv]   = corners[nfi[face][2]] ;
                            tex[nv+1] = vec(x0,-y1,tc) ; verts[nv+1] = corners[nfi[face][3]] ;
                            tex[nv+2] = vec(x1,-y1,tc) ; verts[nv+2] = corners[nfi[face][0]] ;
                            
                            // triangle 2 
                            tex[nv+3] = vec(x1,-y1,tc) ; verts[nv+3] = corners[nfi[face][0]] ;
                            tex[nv+4] = vec(x1,-y0,tc) ; verts[nv+4] = corners[nfi[face][1]] ;
                            tex[nv+5] = vec(x0,-y0,tc) ; verts[nv+5] = corners[nfi[face][2]] ;
                        }
                        
                        // Vertex index creation
                        // TODO: answer this question: why not 4 verts instead of 6? A face 
                        // is fully determined by 4 distinct (vert,tex,norm) tuples. 
                        loopi(6)
                        {
                            // look for verts[nv+i]
                            struct veclookup* result = NULL ;
                            struct veclookup lookup ;

                            lookup.v = verts[nv+i] ;
                            lookup.t = tex[nv+i] ;

                            HASH_FIND(hh, vecidxs, &lookup.v, keylen, result) ;
                            // vert found - fetch index
                            if (result) { idx = result->i ; }
                            // vert not found - add and give it newest index
                            else 
                            {
                                idx = numuniqueidxs ;
                                numuniqueidxs++ ;
                                lookup.i = idx ;
                                HASH_ADD(hh, vecidxs, v, keylen, &lookup) ; 
                                // TODO: add this to an OpenGL indexes list. 
                            }
                            result = NULL ;

                            // add index to index array. 
                            indexes[numidxs] = idx ;
                            numidxs ++ ;
                        }   // Done index generation 
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
    } // end while d>=0

    CN->geom->numverts = nv ;

    // TODO: generalize attributes buffer loading steps
    // Question: TODO: FIXME: should we check whether an ID is a buffer already 
    // rather than always deleting first? 
    if (!glIsBuffer(CN->geom->vertVBOid)) { glGenBuffers( 1, &(CN->geom->vertVBOid)) ; }
    glBindBuffer( GL_ARRAY_BUFFER, CN->geom->vertVBOid) ;
    glBufferData( GL_ARRAY_BUFFER, nv*sizeof(vec), verts, GL_STATIC_DRAW );

    if (!glIsBuffer(CN->geom->texVBOid)) { glGenBuffers( 1, &(CN->geom->texVBOid)) ; }
    glBindBuffer( GL_ARRAY_BUFFER, (CN->geom->texVBOid));            // Bind The Buffer
// if (e.texatlas) { // TODO: if we're keeping 3 tex coords per vertex for simpler code, then remove other stuff. If we have 
// money one day, we can fix this. 
//glBufferData( GL_ARRAY_BUFFER, nv*sizeof(vec2), tex, GL_STATIC_DRAW );  // 2D coords when using tex atlas
    glBufferData( GL_ARRAY_BUFFER, nv*sizeof(vec), tex, GL_STATIC_DRAW );  // 2D coords when using tex atlas
//    } else {
//       glBufferData( GL_ARRAY_BUFFER, nv*sizeof(vec), tex, GL_STATIC_DRAW );   // 3D coords when using tex array
//  }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    worldgeometry.add(CN->geom) ;
}


/*  
    Performs a face-restricted traversal of the tree. If 'face' 
    corresponds to the cube side whose normal is the +ve x axis, 
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

    while (d>=0)
    {
        if ( CN->children )
        {
            if (idxs[d]<4)
            {
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
            if ( CN->has_geometry() )
            {
                /* TODO: refine the logic so that instead of just making a face 
                    visible if its opposite isn't full/flat, make it so that our 
                    face is visible if any of its corners peek out of the other 
                    face's coverage. IMPORTANT! Otherwise having large irregular 
                    surfaces will create huge numbers of unseen and unneeded faces. 
                    So, TODO, TODO< TODO< TODO< TODO< TODO< TODO< TODO< TODO< TODO< 
                    So, TODO, TODO< TODO< TODO< TODO< TODO< TODO< TODO< TODO< TODO< 
                */
                if ( !CN->FaceIsFull(face) ) { return false ; }
            }
            else { return false ; }
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        if (d<0) {   break ; } // Past the root? Then we're done. (otherwise we're backing up into memory we don't own! yikes.) 
        CN = path[d] ;
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    }   // end while d>=0

    // If we did not encounter leaf nodes on this face which lacked geometry, then the face is covered. 
    return true ;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// REPLACE ME AND MOVE ME! 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    int32_t d = 0 ;
    int idxs[20] ;      loopi(20) {idxs[i]=0 ;}
    Octant* path[20] ;  loopi(20) {path[i]=NULL ;}

    Octant* CN = node ;
    path[0] = CN ;
    vec pos(0,0,0) ;
    while (d>=0)
    {
        if ( CN->children )
        {
            if (idxs[d]<8)
            {
                loopi(3) {incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ;pos.v[i] += yesorno * incr ;}
                CN = &CN->children[idxs[d]] ; 
                d++ ;
                path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 
                continue ;
            }
        }
        else { if ( CN->has_geometry() ) { } }

        path[d] = NULL ;
        d-- ;
        if (d<0) { break ; } // Past the root? We're done. 
        CN = path[d] ;
        loopi(3) { incr = (1<<(SI-d)) ; yesorno = ((idxs[d]>>i)&1) ; pos.v[i] -= yesorno * incr ; }
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    } // end while d>=0
*/


#include <time.h>
#include <sys/time.h>


void World::initialize()
{
    world.gridscale = 10 ;
    world.gridsize = 1<<10 ; // In our fake world, this is about 16m. 
    printf("\n\n****************************WORLD SIZE = %d************************************\n\n", world.size ) ;

    // Some testing data initialized

// TODO: non-null in windows only
#ifdef WIN32
//    time_t t ;
 //   srand ( time(&t) ); // I initially tried using NULL for the time_t argument, but that caused occasional hangs!
#else
// TODO: null in linux only
    timeval t ;
    //srand ( time() ); // I initially tried using NULL for the time_t argument, but that caused occasional hangs!
 //   gettimeofday(&t, NULL); // I initially tried using NULL for the time_t argument, but that caused occasional hangs!
//    srand ( t.tv_usec ); // I initially tried using NULL for the time_t argument, but that caused occasional hangs!
#endif

    // What are you doing!
    loopi(50)
    {
    srand ( time(NULL) ); // I initially tried using NULL for the time_t argument, but that caused occasional hangs!
    //srand ( t.tv_usec ); // I initially tried using NULL for the time_t argument, but that caused occasional hangs!
    }
// TODO: fix the number of star buffers according to this GPU's buffer size (which could be less than 10000)
//       and also, move this to somewhere more logical. 

// TODO: this code for some reason really doesn't work in windows. Let's fix
// thix when we have a chance. 

    loopi(numstars)
    {
        stars[i][0] = (rand() % (2<<19)) - (2<<18) ;
        stars[i][1] = (rand() % (2<<19)) - (2<<18) ;
        stars[i][2] = (rand() % (2<<19)) - (2<<18) ;

        if (
            abs(stars[i][0]) < (2<<16) &&
            abs(stars[i][1]) < (2<<16) &&
            abs(stars[i][2]) < (2<<16)
           )
        {
            int which = rand()%3 ;
            while (stars[i][which] < (2<<16)) 
            {
                stars[i][which] *= 2 ;
            }
        }

    }
/*
*/
    
    glGenBuffers( 1, &starsVBO ) ;
    glBindBuffer( GL_ARRAY_BUFFER, starsVBO ) ;
    glBufferData( GL_ARRAY_BUFFER, numstars*sizeof(vec), stars, GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 ) ;
}


bool showpolys = false ;

void ComputeAimedCorner() ;
/*
*/
void DrawWorld()
{
    return ;
    ComputeAimedCorner() ;
    return ;


    // Now for every geometric set in our world, render! 
/*
*/
}   // end DrawWorld 


/*  
    Don't ask... but if you must, then do! As is often the case, the goal of 
    this function is a million times simpler than the implementation: 
        
        To identify which one (of 8 possibilities) of a cube's corners we 
        are currently pointing at. The corners are identified the same 
        as octree children: xyz=0,0,0 means index 0, and indexes grow 
        along x, then y, then z. 


*/
void ComputeAimedCorner()
{
    int x = hitcorner[X(o)] ;
    int xdir = 1 ; // pointing right to start with
    int y = hitcorner[Y(o)] ;
    int ydir = 1 ; // pointing up to start with

    int gs = world.gridsize ;
    int gs_2 = gs >> 1 ;
    int sc = world.gridscale ;

    facecorner = 0 ;
    if (hitcorner.x%gs>gs_2) { facecorner |= 0x1 ; }
    if (hitcorner.y%gs>gs_2) { facecorner |= 0x2 ; }
    if (hitcorner.z%gs>gs_2) { facecorner |= 0x4 ; }
    hitcorner[X(o)] = ((hitcorner[X(o)] >> sc) << sc) + sc ;
    hitcorner[Y(o)] = ((hitcorner[Y(o)] >> sc) << sc) + sc ;
    hitcorner[Z(o)] +=  Dz(o)*10 ;
    if (x%gs > gs_2) { hitcorner[X(o)] += gs - (2*sc) ; xdir = -1 ; }
    if (y%gs > gs_2) { hitcorner[Y(o)] += gs - (2*sc) ; ydir = -1 ; }

    // draw the corner aimed at
    // TODO: move to a function which only renders if: 
        // we are editing
        // we aren't inside geometry. 
    ivec vert = hitcorner ;
    glLineWidth(3.0) ;
    glColor3f(0,0,1) ;
        glBegin( GL_LINES ) ;
            glVertex3iv(vert.v) ;  vert[X(o)] += 5*sc*xdir ;
            glVertex3iv(vert.v) ;  vert = hitcorner ;
            glVertex3iv(vert.v) ;  vert[Y(o)] += 5*sc*ydir ;
            glVertex3iv(vert.v) ;  

            //glVertex3iv(hitcorner.v) ; 
            //glVertex3iv(hitcorner.v) ; hitcorner[Z(o)] += Dz(o)*500 ;
            //glVertex3iv(hitcorner.v) ; dunno what this one is for. 
        glEnd( ) ;
    glLineWidth(1.0) ;

// -----------------------------------------------------------------------------

}   // end DrawWorld 

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


/*
*/
void LoadWorld(const char* name)
{
    // First clear current world. 
    deletesubtree(&world.root) ;

    //world.LoadFromFile("data/maps_by_users/map.txt") ;
    char filename[256] ;
    bool maploaded = false ;

    // First try main maps
    //sprintf(filename, "data/user_maps/%s", name) ;
    //maploaded = world.LoadFromFile("data/user_maps/map.txt") ;

    if (!maploaded)
    {
        // Next try user maps
     //   sprintf(filename, "data/maps/%s", name) ;
      //  maploaded = world.LoadFromFile("data/user_maps/map.txt") ;
    }

    
    AnalyzeGeometry(&world.root, vec(0,0,0), world.scale) ;

    AssignNewVBOs(&world.root, vec(0,0,0),world.scale) ;
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
    // burn the Earth and shatter the world. 
}
*/

void drawStars() 
{
    //--------------------------------------------------------------------------
    // Stars
    //--------------------------------------------------------------------------
    //return ;    // TODO: fix this; it breaks rendering other non-shader things
    glDisable( GL_CULL_FACE ) ;
    glEnableClientState( GL_VERTEX_ARRAY );                       // Enable Vertex Arrays
   
    glPushMatrix() ;
    glMatrixMode( GL_PROJECTION ) ;
    glPushMatrix() ;
    glLoadIdentity() ;
    float ratio = (float)engine.current_w / (float)engine.current_h ;
    gluPerspective( (GLfloat)engine.fov, ratio, 10.0f, 2000000.0f ) ;
    
    glPointSize( 1.8 ) ;
    glBindBuffer(GL_ARRAY_BUFFER, starsVBO ) ;
    glVertexPointer( 3, GL_INT,  0, (char *) NULL);        // Set The Vertex Pointer To The Vertex Buffer
    glDrawArrays( GL_POINTS, 0, numstars);    // Draw All Of The Triangles At Once
    
    //gluPerspective( (GLfloat)engine.fov, ratio, 10.0f, 150000.0f ) ;
    glPopMatrix() ;
    glMatrixMode( GL_MODELVIEW ) ;
    glPopMatrix() ;
    //glLoadIdentity() ;
    glDisableClientState( GL_VERTEX_ARRAY );                       // Enable Vertex Arrays
    glBindBuffer(GL_ARRAY_BUFFER, 0 ) ;
    //--------------------------------------------------------------------------
}
