/*
    File: geometry.h 



    Contains the data type definitions that are used for world geometry 
    construction and use. 


*/

// Handy and cute. I swear I found it by hand (took little while) but it is indeed to be found 
// in sauer's code. 
#define octastep(x,y,z,scale)  (((((z)>>(scale))&1)<<2) | ((((y)>>(scale))&1)<<1) | (((x)>>(scale))&1))

struct Geom
{
    // Texture tex_slot ;
} ;

#include "stdint.h"

struct Octant
{
    Octant* children ;                                          // 4/8 bytes 
    Geom* geom ;                                              // 4/8 bytes

    // This gross structure allows us to use the 'edges' fields 
    // for the leaf vertex count, which is only needed by nodes 
    // that do not have geometry; ie, non-leaf nodes. 
    union 
    {
        struct 
        {
            int32_t c ;         // leaf vertex count            // 4 bytes
            int32_t int2 ;      // unused                       // 4 bytes
            int32_t int3 ;      // unused                       // 4 bytes
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
    //inline 
    bool has_geometry() ;
/*
    {
        return (
                (edge_check[0]==MAX_INT) &&
                (edge_check[1]==MAX_INT) &&
                (edge_check[2]==MAX_INT)
               ) ;
    }*/

//printf("\n set_all_edges: edges of this octant located at %d", (int)&edges[0]) ;
//printf("\n set_all_edges: edges[%d] = %d", (i) , edges[i]) ;
    //inline 
    void set_all_edges(int in_value) ;
/*
    {
        printf("\n setting all edges with d=%d\n", in_value) ;
        loopi(12)
        {
            edges[i] = in_value ;
        }
    }
*/

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


