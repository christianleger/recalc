/*
    File: test.cpp

    This file provides a number of functions whose purpose is to expand our
    comprehension of the computer and software and APIs at our disposal. 
*/
#include "recalc.h"


//#define GL_GLEXT_PROTOTYPES
//#include <GL/glext.h>

//PFNGLGENBUFFERSPROC glGenBuffers = NULL;                  // VBO Name Generation Procedure
//PFNGLBINDBUFFERPROC glBindBuffer = NULL;                  // VBO Bind Procedure
//PFNGLBUFFERDATAPROC glBufferData = NULL;                  // VBO Data Loading Procedure
//PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;            // VBO Deletion Procedure

// GLuint                m_nVertexCount;                                // Vertex Count

// CVert*            m_pVertices;                                // Vertex Data
vec m_vertices[10000] ;
vec m_colors[10000] ;

// CTexCoord*        m_pTexCoords;                                // Texture Coordinates
// CTexCoord*        m_pTexCoords;                                // Texture Coordinates
// unsigned int    m_nTextureId;                                // Texture ID

// Vertex Buffer Object Names
unsigned int    m_vertVBO ;                                // Vertex VBO Name
unsigned int    m_colorVBO ;                                // Vertex VBO Name
// unsigned int    m_nVBOTexCoords;                            // Texture Coordinate VBO Name

bool test_001_initiated = false ;
// populate some fricking vertex arrays. 
void init_test_001()
{

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                        GENERATE DATA
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Triangle 1
        m_vertices[0] = vec(0, 0, 0) ;
        m_vertices[1] = vec(1000, 0, 0) ;
        m_vertices[2] = vec(800, 700, 0) ;
       
        // Triangle 2
        m_vertices[3] = vec(800, 700, 0) ;
        m_vertices[4] = vec(1000, 0, 0) ;
        m_vertices[5] = vec(1800, 500, 0) ;
       
        // Triangle 3
        m_vertices[6] = vec(1800, 500, 0) ;
        m_vertices[7] = vec(1000, 0, 0) ;
        m_vertices[8] = vec(2200, 200, 30) ;
       
        // Triangle 4
        m_vertices[9] = vec(1800, 500, 0) ;
        m_vertices[10] = vec(2200, 200, 30) ;
        m_vertices[11] = vec(2400, 300, 800) ;

        // How many triangles: 
        // 600 vertices where after the first two each new one is another triangle. 
        // So 598 triangles. 
        for (int j=12;j<600;j+=3)
        {
            m_vertices[j] = m_vertices[j-2] ;
            m_vertices[j+1] = m_vertices[j-1] ;
            m_vertices[j+2] = vec(200*j, 0+(j%2?900:0), 20*j) ;
        }
      
      vec colors[10] = 
      {
        vec(1.0, 1.0, 1.0) , vec(0.0, 1.0, 0.0) , 
        vec(1.0, 0.0, 1.0) , vec(1.0, 1.0, 0.0) , 
        vec(1.0, 0.0, 0.0) , vec(0.5, 0.5, 1.0) , 
        vec(0.5, 0.5, 0.0) , vec(0.0, 0.5, 0.8) , 
        vec() , vec() 
      } ;

        m_colors[0] = vec(1,1,1) ;
        m_colors[1] = vec(1,1,1) ;
        m_colors[2] = vec(1,1,1) ;

        m_colors[3] = vec(1,.5,.5) ;
        m_colors[4] = vec(1,.5,.5) ;
        m_colors[5] = vec(1,.5,.5) ;
       
        m_colors[6] = vec(.5,.5,.5) ;
        m_colors[7] = vec(.5,.5,.5) ;
        m_colors[8] = vec(.5,.5,.5) ;
       
        m_colors[9] = vec(.2,.5,.3) ;
        m_colors[10] = vec(.2,.5,.3) ;
        m_colors[11] = vec(.2,.5,.3) ;
       
        for (int j=12;j<600;j++)
        {
            m_colors[j] = colors[j%9] ;
            m_colors[j+1] = colors[j%10] ;
        }
       /*
        m_vertices[3][0] = 1 * i  ;
        m_vertices[3][1] = 1 * 7 *i ;
        m_vertices[3][2] = 1 * 5  ;
        
        m_vertices[4][0] = 1 * i  ;
        m_vertices[4][1] = 1 * 7 *i ;
        m_vertices[4][2] = 1 * 5  ;
        
        m_vertices[5][0] = 1 * i  ;
        m_vertices[5][1] = 1 * 7 *i ;
        m_vertices[5][2] = 1 * 5  ;
        */
   
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                        POPULATE DATA ARRAY
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    /*
    for (int i=0;i<500;i++)
    {
        // int p = pow(-1,i) ;
        m_vertices[i][0] = 1 * i  ;
        m_vertices[i][1] = 1 * 7 *i ;
        m_vertices[i][2] = 1 * 5  ;
    }
    */


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                        GENERATE and FEED BUFFERS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // VERTICES
    glGenBuffers( 1, &m_vertVBO );                            // Get A Valid Name
    glBindBuffer( GL_ARRAY_BUFFER, m_vertVBO );            // Bind The Buffer
    glBufferData( GL_ARRAY_BUFFER, 500*9*sizeof(float), m_vertices, GL_STATIC_DRAW );
    // glBufferData( GL_ARRAY_BUFFER, 500*3*sizeof(float), m_vertices, GL_STATIC_DRAW );

    // COLOR VALUES 
    glGenBuffers( 1, &m_colorVBO );                            // Get A Valid Name
    glBindBuffer( GL_ARRAY_BUFFER, m_colorVBO );            // Bind The Buffer
    glBufferData( GL_ARRAY_BUFFER, 500*9*sizeof(float), m_colors, GL_STATIC_DRAW );
    /*
    */

    // TEXTURE COORDINATES
//    glGenBuffers( 1, &m_nVBOTexCoords );                            // Get A Valid Name
//    glBindBuffer( GL_ARRAY_BUFFER, m_nVBOTexCoords );        // Bind The Buffer
//    glBufferData( GL_ARRAY_BUFFER, m_nVertexCount*2*sizeof(float), m_pTexCoords, GL_STATIC_DRAW );

    //  Our Copy Of The Data Is No Longer Necessary, It Is Safe In The Graphics Card
    //  delete [] m_pVertices; m_pVertices = NULL;
    //  delete [] m_pTexCoords; m_pTexCoords = NULL;
}


void render_test_001()
{
    if ( !test_001_initiated )
    {
        exit(0) ;
        printf("\n\n\n\n\n") ;
        init_test_001() ;
        test_001_initiated = true ;
    }

//    GLuint vboId1 = 0 ;
//    GLuint vboId2 = 0 ;

    // draw 6 quads using offset of index array
// FIXME: DO THISSSSSSSSSSSSSSSS    //glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, 0);

    // bind with 0, so, switch back to normal pointer operation
    //glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Enable Pointers
    glEnableClientState( GL_VERTEX_ARRAY );                        // Enable Vertex Arrays
    glEnableClientState( GL_COLOR_ARRAY );                        // Enable Vertex Arrays
    /*
     */
    // glEnableClientState( GL_TEXTURE_COORD_ARRAY );                // Enable Texture Coord Arrays

    // Set Pointers To Our Data
    

    /*
*/
    glBindBuffer(GL_ARRAY_BUFFER, m_vertVBO );
    glVertexPointer( 
        3,              // size (of vertex)
        GL_FLOAT,       // data type of each data entry
        0,              // stride
        (char *) NULL   // offset - NULL gives zero
        );        // Set The Vertex Pointer To The Vertex Buffer
   
    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO );
    glColorPointer( 3, GL_FLOAT, 0, (char *) NULL );        // Set The Vertex Pointer To The Vertex Buffer
   
    
    // DRAW LIKE AN ALMIGHTY GOD
/*
*/
    glDrawArrays( GL_TRIANGLES, 0, 650 );    // Draw All Of The Triangles At Once

    glPushMatrix() ;

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 500 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glRotatef(80, 0,0,1) ;
    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 500 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glRotatef(80, 0,0,1) ;
    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glTranslatef(0,2000,4000) ;
    glDrawArrays( GL_TRIANGLES, 0, 200 );    // Draw All Of The Triangles At Once

    glPopMatrix() ;
/*
*/

    // glDrawArrays( GL_TRIANGLES, 0, numElements*elementSize );    // Draw All Of The Triangles At Once
    // FIXME: use element arrays



    // Disable buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    
    // Disable Pointers
    glDisableClientState( GL_VERTEX_ARRAY );                    // Disable Vertex Arrays
    glDisableClientState( GL_COLOR_ARRAY );                        // Enable Vertex Arrays
    // glDisableClientState( GL_TEXTURE_COORD_ARRAY );                // Disable Texture Coord Arrays





////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////
/////////////////
/////////////////
/////////////////
/////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////







}



// Present the value of every bit in this particular number. 
// Trying out multiple numbers to gather a sense of the position 
// of the bits at certain values. 
void float32_bits()
{
    float number = 1.f ;

    printf("\n---------------------------------------------------") ; 
    printf("\n  Test: floating point bits ") ; 
    printf("\n---------------------------------------------------") ; 
    printf("\n float of value %f bits:\n", number) ;
    for (int i=0;i<32;i++)
    {
        printf("\nbit #%d = %d", i, 
        
            (((*((int*)(&number)))>>i)  &0x01)

        ) ; 
    }
    printf("\n---------------------------------------------------") ; 

    number = 0.f ;
    printf("\n float of value %f bits:\n", number) ;
    for (int i=0;i<32;i++)
    {
        printf("\nbit #%d = %d", i, 
        
            (((*((int*)(&number)))>>i)  &0x01)

        ) ; 
    }
    printf("\n---------------------------------------------------") ; 

    number = -1.f ;
    printf("\n float of value %f bits:\n", number) ;
    for (int i=0;i<32;i++)
    {
        printf("\nbit #%d = %d", i, 
        
            (((*((int*)(&number)))>>i)  &0x01)

        ) ; 
    }
    printf("\n---------------------------------------------------") ; 

    number = 2.f ;
    printf("\n float of value %f bits:\n", number) ;
    for (int i=0;i<32;i++)
    {
        printf("\nbit #%d = %d", i, 
        
            (((*((int*)(&number)))>>i)  &0x01)

        ) ; 
    }
    printf("\n---------------------------------------------------") ; 

    number = 3.f ;
    printf("\n float of value %f bits:\n", number) ;
    for (int i=0;i<32;i++)
    {
        printf("\nbit #%d = %d", i, 
        
            (((*((int*)(&number)))>>i)  &0x01)

        ) ; 
    }
    printf("\n---------------------------------------------------") ; 

    number = 4.f ;
    printf("\n float of value %f bits:\n", number) ;
    for (int i=0;i<32;i++)
    {
        printf("\nbit #%d = %d", i, 
        
            (((*((int*)(&number)))>>i)  &0x01)

        ) ; 
    }
    printf("\n---------------------------------------------------") ; 

    number = 5.f ;
    printf("\n float of value %f bits:\n", number) ;
    for (int i=0;i<32;i++)
    {
        printf("\nbit #%d = %d", i, 
        
            (((*((int*)(&number)))>>i)  &0x01)

        ) ; 
    }
    printf("\n---------------------------------------------------") ; 

    printf("\nTESTING ROUNDING CONVERSTION FOR LIMITED-RANGE FLOATS: \n") ;

    union iorf
    {
        float f ;
        int32_t i ;
    } ;
    iorf n ;
    n.f = 5.f ;
    char cshift = (n.i>>23)-127 ;
    printf("\n  cshift = %d \n", cshift) ;
    printf("\n  result first part = %d \n", (1<<cshift)) ;
    int res = (n.i) ;
    res = res<<9 ;
    res = res>>(9+23-cshift) ;
    printf("\n  result final part = %d \n", (1<<cshift)+res) ;

}


void init_test_002()
{
}
void render_test_002()
{
}

/*
    Function: initialize_tests

    Purpose: to provide a number of tests which are to be run at the start of 
    the application. This is perfect for console-only, one-off tests. 


*/
void initialize_tests()
{

    // Test: floating point numbers. 
    //float32_bits() ;

    init_test_002() ;
}

