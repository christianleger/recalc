/*
    File: geometry.h 



    Contains the data type definitions that are used for world geometry 
    construction and use. 


*/

// Handy. I swear I found it by hand (took a little while turning cube indexes into their constituent bits) 
// but it is indeed to be found in sauer's code. 
#define octastep(x,y,z,scale)  (((((z)>>(scale))&1)<<2) | ((((y)>>(scale))&1)<<1) | (((x)>>(scale))&1))













/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////




/*

VBOs and what they're good for. 

A lot of things you need, actually. We get large numbers of triangles groups 
with a good selectio of textures and we can draw convincingly a huge amount 
of world-making elements. 


*/
/////////////////////////////////////////////////////////////////////////////////////////////
/*

---> Triangle Sets <---
---> Triangle Sets <---
---> Triangle Sets <---

So as previously established, we want groups of 256 vertices or less to be the basic 
block of volume and structure, as far as rendering goes. The tree is processed so that 
once geometry has been altered, every new or rebuilt area is built out of subtrees that 
enclose 256 vertices or less. A node will know if its parent has too many vertices under 
it to be host to 
to be host



When a geom gets created, 

When the geom gets deleted, 

glDeleteBuffers with geoVBOid

and 

glDeleteBuffers with texVBOid

Textures will have to come later. They will consist of a principle, light, and height-mapping
surfaces, and will have a set of shaders that can be used on any surface. 


*/
/////////////////////////////////////////////////////////////////////////////////////////////
struct Geom
{
    // Texture tex_slot ;
    // vertex ID given by and used with OpenGL for any VBOs used by this Geom's owner. 
    unsigned int geoVBOid ; 
    // texture ID given by and used with OpenGL for any VBOs used by this Geom's owner. 
    unsigned int texVBOid ; 


/*

    Does a geom need anything else than geoVBOid or texVBOid? 

    It can be used to tell the renderer whether to use a triangle set or not.
    Using the position of the host node, we know what relative position of this
    volume to the camera's line of sight. 


*/
} ;

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#include "stdint.h"

struct Octant
{
    Octant* children ;                           // 4/8 bytes 
    Geom* geom ;                                 // 4/8 bytes

    // This structure allows us to use the 'edges' data location for other purposes than 
    // counting geometric parameters. 
    // for the leaf vertex count, which is only needed by nodes 
    // that do not have geometry; ie, non-leaf nodes. 
    union 
    {
        struct 
        {
            int32_t c ;         // leaf vertex count (should be 3X(number of triangles) minus number of reused vertices)
            int32_t int2 ;      // unused            
            int32_t int3 ;      // unused            
        } lvc ; 

        uchar    edges[12] ;                                    // 12 bytes 
        uint32_t edge_check[3] ; // used to check if a node has geometry // 12 bytes
    } ;

    // Slot IDs
    short tex[6] ;                                              // 12 bytes


    Octant() ;

//#define MAX_INT ((255<<24)|(255<<16)|(255<<8)|(255))
#define MAX_INT 0xFFFFFFFF
    /*
        Answers whether or not this node has a physical extent. When edge_checks 
        are all zero, it means there is no geometry directly defined by this 
        node (does not by itself preclude the possibility that child nodes 
        define geometry). 
    */
    bool has_geometry() ;
    bool has_children() ;
    void set_all_edges(int in_value) ;

    void test_func_def()
    {
        lvc.c = 0 ;
        lvc.int2 = 0 ; //lalal
    }
} ;



/*
    An allocated octant has children slots and 
    a geometry extension
void new_octant( Octant& oct )
{
    oct.children = new Octant[8] ;
}
*/
void new_octant( Octant& oct ) ;

struct World
{
    int scale ;
    int size ;      // 2^scale
    int gridscale ;  
    int gridsize ;  // variable. 

    void initialize() ; 

    Octant root ;
} ;


