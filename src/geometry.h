/*
    File: geometry.h 



    Contains the data type definitions that are used for world geometry 
    construction and use. 


*/

#define octastep(x,y,z,scale)  (((((z)>>(scale))&1)<<2) | ((((y)>>(scale))&1)<<1) | (((x)>>(scale))&1))

struct OctExt
{
    // Texture tex_slot ;
} ;

struct Octant
{
    Octant* children ; 
    OctExt* geom ;
    char edges[6] ;
    // FIXME: replace with a proper texture struct 
    GLuint textures[6] ;


    Octant() ;
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
    int gridsize ;  // 2^engine.gridscale

    void initialize() ; 

    Octant root ;
} ;


