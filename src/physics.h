


/*
    Entities are defined as having a coordinate system with 

    +y forward
    +z up
    +x to the right

    While it doesn't completely matter where an object's 0 is, 
    a good start for people and creatures is for the Entity's origin 
    to be under their center of mass, at the bottom of their feet. This 
    makes a bunch of particular computations simpler. 

    Entities can be creatures or people or projectiles or machines. Or 
    even machine parts or inanimate objects (anything can become a projectile 
    if you get it moving). 

    Even ships are entities at their most abstract - and are treated as 
    such by the high-level dynamic system. 

*/
struct Entity
{
    vec pos ; // where you is at
    
    vec dir ;   // where you is looking at
    vec vel ;   // where your center of mass is moving at
    vec up ;    // if you have an upright direction, where it is pointing at

    float mass ;    // Only kilograms work for us

    float dpitch ;  // speed of rotating around: delta pitch
    float droll ;   // delta roll
    float dyaw ;    // delta yaw

    float xsize ;   // left-right width
    float ysize ;   // back-to-front depth (or length for horizontal things/beings)
    float zsize ;   // height/tallness
} ;



