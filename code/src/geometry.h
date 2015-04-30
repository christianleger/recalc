/*
    File: geometry.h 



    Contains the data type definitions that are used for world geometry 
    construction and use. 


*/

// This trick is inevitable after you stare at the binary representation of the 
// eight children of an octree long enough :-) 
#define octastep(x,y,z,scale)  (((((z)>>(scale))&1)<<2) | ((((y)>>(scale))&1)<<1) | (((x)>>(scale))&1))


/*

---> Triangle Sets <---
---> Triangle Sets <---
---> Triangle Sets <---

So as previously established, we want groups of 256 vertices or less to be the basic 
block of volume and structure, as far as rendering goes. The tree is processed so that 
once geometry has been altered, every new or rebuilt area is built out of subtrees that 
enclose 256 vertices or less. A node will know if its parent has too many vertices under 
it to be host to be host



When a geom gets created, 

When the geom gets deleted, 

glDeleteBuffers with geoVBOid

and 

glDeleteBuffers with texVBOid

Textures will have to come later. They will consist of a principle, light, and height-mapping
surfaces, and will have a set of shaders that can be used on any surface. 


*/
/////////////////////////////////////////////////////////////////////////////////////////////
#define VERTS_PER_GEOM 1024
//#define VERTS_PER_GEOM 4096
//#define VERTS_PER_GEOM 8192
struct Geom
{
    int frontguard ;
    //  Texture tex_slot ;
    //  vertex ID given by and used with OpenGL for any VBOs used by this Geom's owner. 
    unsigned int vertVBOid ; 
    //  texture ID given by and used with OpenGL for any VBOs used by this Geom's owner. 
    unsigned int texVBOid ; 
    unsigned int colorVBOid; 

    ivec vertices[VERTS_PER_GEOM] ;        // maybe 85 triangles
    vec texcoords[VERTS_PER_GEOM] ;        // maybe 85 triangles

    ushort idxs[768] ;
    //  char indexes[256] ;      // Implement this when your highness isn't too busy doing 'work' on something else. 
    //unsigned char numverts ;
    ushort numverts ;

    Geom()
    {
        vertVBOid = 0 ;
        colorVBOid = 0 ;
        texVBOid = 0 ;
        numverts = 0 ;
    }
} ;

/////////////////////////////////////////////////////////////////////////////////////////////
/*
    For any node that is not built from only simple geometry, or that contains 
    entities, this struct is used to store the extra stuff. 
*/
/////////////////////////////////////////////////////////////////////////////////////////////
struct Stuff
{
    Geom* geom ;

     //vector<Entity*>  ents ;
     //vector<Sound*>   sounds ;
     //vector<Scripts*> scripts ;
} ;


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#include "stdint.h"

struct Octant
{
    Octant* children ;                       // 4/8 bytes 
    Stuff* stuff ;                           // 4/8 bytes
    Geom* geom ;                             // 4/8 bytes

    // TODO: put the geom into the stuff. This isn't costly, since geoms are
    // only manipulated during geometry update operations, which are either manual or asynchronous. 

    // This structure allows us to use the 'edges' data location for other purposes than 
    // counting geometric parameters. 
    // for the leaf vertex count, which is only needed by nodes 
    // that do not have geometry; ie, non-leaf nodes. 
    union 
    {
        uchar    edges[12] ;                                    // 12 bytes 
        uint32_t edgegroups[3] ; // alternate named used to quickly check if a node has geometry // 12 bytes
    } ;
    #define eg edgegroups

    char tex[6] ;          // 6 bytes  // Tex slot IDs
    uint16_t pvs ;         // 2 bytes. Links this node to the pvs for this and nearby locations. 
    uint32_t vc ;          // 4 bytes. Vertex count or subtree vertex count in the case of non-leaf nodes. 

    // the different material types a node can have
    enum 
    {
        EMPTY = 0,  // default for non-leaf nodes. 
        AIR,
        GLASS, 
        STONE,
        METAL
    } ;


    /* 4 bytes. 
        Bit meanings: 
            0: need update
            1: solid
            2: full
    */

    // Flags
    #define NEEDS_UPDATE        0x00000001
    #define SOLID               0x00000002
    #define FULL                0x00000004

    // Functional Types
    #define POWERCABLE          0x00000008
    #define DATACABLE           0x00000010

    // Orientations: using face indexes as the endpoints of 'back and front'. 
    // '01' means 0 is the back and 1 is the front. 
    #define ORIENTATION01       0x00000100
    #define ORIENTATION10       0x00000200
    #define ORIENTATION23       0x00000400
    #define ORIENTATION32       0x00000800
    #define ORIENTATION45       0x00001000
    #define ORIENTATION54       0x00002000

    // Faces flat or not: we use this to decide which faces have normals not 
    // perpendicular to cube faces. 0 means face is flat, and 1 means face nonflat. 
    #define FACE_0_FLAT         0x00100000
    #define FACE_1_FLAT         0x00200000
    #define FACE_2_FLAT         0x00400000
    #define FACE_3_FLAT         0x00800000
    #define FACE_4_FLAT         0x01000000
    #define FACE_5_FLAT         0x02000000
    // Face diagonals: when set for a particular face, tells us that this face's diagonal 
    // starts from the face origin. Unset means transverse diagonal instead. Saves time in normals computation. 
    #define FACE_0_DIAG_0       0x04000000
    #define FACE_1_DIAG_0       0x08000000
    #define FACE_2_DIAG_0       0x10000000
    #define FACE_3_DIAG_0       0x20000000
    #define FACE_4_DIAG_0       0x40000000
    #define FACE_5_DIAG_0       0x80000000
    
    // Flags for material type
    int32_t flags ;

    Octant() ;

    bool has_geometry() ;
    bool has_children() ;
    bool IsSolid() { return flags & SOLID ; }
    bool IsFull()  { return flags & FULL ; }
    bool FaceIsFull(int face) ;
    bool FaceHiddenBy(int face, Octant* node) ;


    bool needsupdate() { return flags & NEEDS_UPDATE ; }

    void setneedsupdate() { flags |= NEEDS_UPDATE ; }

    void doneupdate() { flags ^= NEEDS_UPDATE ; }

    void clear_geometry() ;
    void clear_children() ;
    void clear_geom() ;
    void clear_all() ;

    void set_edges(const uchar* in_values) ;
    void set_all_edges(int in_value) ;

    bool FaceNonFlat(int face) { return ((flags>>(24+face))&1) ; }
    void SetFaceDiagonal(int face, bool set) { flags ^= (face&(set<<(face*26))) ;}
    bool GetFaceDiagonal(int face) { return (flags>>(face+26))&0x1 ;} ;
} ;


void new_octant( Octant& oct ) ;

struct World
{
    int scale ;
    int size ;      // 2^scale
    int gridscale ;  
    int gridsize ;  // variable. 

    void initialize() ; 
    void SaveToFile(const char* name) ; 
    void LoadFromFile(const char* name) ;
    int SetNode(const uchar* name) ;

    Octant root ;
} ;


