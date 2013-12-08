

#include "recalc.h"


////////////////////////////////////////////////
// Main switch on this system
////////////////////////////////////////////////

// TODO: reassign externs to getters or something. 
extern bool textenabled ;

extern Camera camera ;
extern Engine engine ;
#define e engine
extern Area area ;
extern vector<Font*> fonts ;
extern void prstrend() ;
extern void prstrstart() ;

void render_menu2() ;


// Render System messages
int render_msgs_num = 0 ;
char render_msgs[100][256] ;
/*
    Report errors from the OpenGL subsystem. 

    TODO: add in a mechanism to identify the string that should be associated 
    with a given error code. 
*/
void CheckGlError()
{
    int err = glGetError() ;
    if (err)
    {
        // FIXME / TODO : implement logging system. 
        //printf("\n\nGL ERROR IS %d\n\n", (err)) ; 
        printf("\n\nGL ERROR IS %x\n\n", (err)) ; 
    }
}


/*
*/
void MoveGlViewPoint()
{
    glMatrixMode( GL_MODELVIEW ) ; 
    glLoadIdentity() ; 
    glRotatef( -camera.pitch/10 , 1, 0, 0 ) ; 
    glRotatef( -camera.yaw/10 ,   0, 1, 0 ) ; 
    glRotatef( -90 ,              1, 0, 0 ) ; 
    glTranslatef( -camera.pos.x, -camera.pos.y, -(camera.pos.z) ) ;
}


/*
    FUNCTION: 
        render_ortho_begin

    DESCRIPTION:
        Sets up the perspective matrix so that the rendering coordinate 
        system is a 2D grid the height and width of the application window. 

        X grows towards the right. 
        Y grows upwards. 
*/
void render_ortho_begin()
{
    // we assume the matrix mode is always GL_MODELVIEW
    glPushMatrix() ; 
    glLoadIdentity() ; 
    glMatrixMode( GL_PROJECTION ) ;
    glPushMatrix() ; 
    glLoadIdentity() ; 
    glOrtho( 0, engine.current_w, 0, engine.current_h, -100, 1000 ) ; 
    glMatrixMode( GL_MODELVIEW ) ; 
}


// Returns to state prior to engaging the 2D space. 
void render_ortho_end()
{
    glPopMatrix() ; 
    glMatrixMode( GL_PROJECTION ) ;
    glPopMatrix() ; 
    glMatrixMode( GL_MODELVIEW ) ; 
}




// Colors - Dim colors so that untextured geometry looks like ass
float mult = 1.0f ;
vec colors[10] = {
  vec(mult*0.0, mult*0.3, mult*0.0) ,   // dark green 
  vec(mult*0.0, mult*0.6, mult*0.3) ,   // dark green 
  vec(mult*0.0, mult*0.6, mult*0.3) ,   // dark green 
  vec(mult*0.0, mult*0.0, mult*0.3) ,   // dark blue
  vec(mult*0.0, mult*0.0, mult*0.6) ,   // dark purple
  vec(mult*0.0, mult*0.3, mult*0.6) ,   // dark purple
  vec(mult*0.2, mult*0.5, mult*0.0) ,   // medium green
  vec(mult*0.2, mult*0.0, mult*0.5) ,   // medium blue
  vec(mult*0.0, mult*0.5, mult*0.5) ,   // medium purple
  vec(mult*0.2, mult*0.5, mult*0.5)     // medium grey
} ;

/**
 *  SetVertAttribsGLSL12
 *
 */
void SetVertAttribsGLSL12(
    vector<Geom*> wg // = GetWorldGeometry() ;
    )
{
}

/**
 *  SetVertAttribsGLSL15
 *
 */
void SetVertAttribsGLSL15(
    vector<Geom*> wg // = GetWorldGeometry() ;
    )
{
}

/**
 *  setVertexAttribs 
 *  
 *  This pointer is set to point to either one of the above two functions, to 
 *  give us an attribute-setting function which matches the GLSL version we're using.  
 *  
 */
void (*setVertexAttribs)( 
    vector<Geom*> wg // = GetWorldGeometry() ;
    ) = NULL ;

void render_world( )
{
    // FFP rendering
    MoveGlViewPoint() ;
    drawStars() ;
    
    // Proper matrices
    glmatrixf p ;   // Projection matrix
    glmatrixf mv ;  // ModelView matrix
    p.identity() ;
    mv.identity() ;

    p.perspective(
        engine.fov, 
        (float)engine.current_w/(float)engine.current_h,
        10.0, 
        300000.0 
        ) ;

    float nearfar[2] ; glGetFloatv(GL_DEPTH_RANGE, nearfar) ;
//       printf("\n gldepthrange: near=%f  far=%f", nearfar[0], nearfar[1]) ;
//       glDepthRange(0, 100.0) ;

    //mv.translate( -camera.pos.x-(1<<16), -camera.pos.y, -camera.pos.z ) ;
    mv.translate( -camera.pos.x, -camera.pos.y, -camera.pos.z ) ;
    mv.rotate_around_x( -M_PI/2.0 ) ;
    mv.rotate_around_y( -(camera.yaw/10.0)*M_PI/180.0 ) ;
    mv.rotate_around_x( -(camera.pitch/10.0)*M_PI/180.0 ) ;
    p.mul(mv) ;

    
//  glEnableVertexAttribArray(1) ;  // normal coords 

    // Draw Style - FILLED OR POLYGON? 
    // Textures

    // Attributes 
    vector<Geom*> wg = GetWorldGeometry() ;

    //TODO: tweak polygon offset to improve contrast between cursor/selection and geometry. 

    // Go
extern bool showpolys ;
//    if (!showpolys)
vec4 colour(1,1,1,1) ;  // this should be white
    glColor4f( 1.0, 1.0, 1.0, 1.0 ) ;
    if (wg.length()>0)
    {
        glEnable( GL_CULL_FACE ) ;
        glEnable( GL_DEPTH_TEST ) ;
        glCullFace( GL_BACK ) ;
        glFrontFace( GL_CCW ) ;
        glEnable( GL_POLYGON_OFFSET_LINE ) ;
        glPolygonMode( GL_FRONT, GL_FILL ) ; // or glPolygonMode( GL_FRONT, GL_LINE ) ;
        glPolygonOffset( 1.18, 1.18 ) ;

#ifdef RECALC_TEXARRAY
        glEnable( GL_TEXTURE_2D_ARRAY );
        glBindTexture( GL_TEXTURE_2D_ARRAY, GetMainTextures() ) ;
#else
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, GetMainTextures() ) ;
#endif
        
        glUseProgram(GetShader(0)) ;
        glEnableVertexAttribArray(0) ;  // vert coords 
        glEnableVertexAttribArray(1) ;  // tex coords

        loopv(wg)
        {
            { glColor3fv(colors[i%10].v) ; }
            glUniformMatrix4fv(Get_P_MV_Matrix_Uniform(0), 1, FALSE, p.v) ;
            glUniform1i(Get_MainTexSampler(), 0) ;
            glUniform1f( GetAtlasScaleUniform(), 4) ; 

            // Specify attribute arrays, according to current rendering more (old or modern shaders)
            glBindBuffer( GL_ARRAY_BUFFER, wg[i]->vertVBOid ) ;
            glVertexAttribPointer( 0, 3, GL_INT, GL_FALSE, 0, NULL) ;       // vertex pointer
            
            glBindBuffer( GL_ARRAY_BUFFER, wg[i]->texVBOid ) ;
            if (e.texarray) {
                glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, NULL ) ;    // texture pointer
            } else if (e.texatlas){
                glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, NULL ) ;    // texture pointer
            }

            // normal pointer
            //glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 0, NULL) ;       // vertex pointer
            // ...
            
            glDrawArrays( GL_TRIANGLES, 0, wg[i]->numverts ) ;
        }

//        glDisable( GL_TEXTURE_2D_ARRAY ) ;
    }

    if (showpolys)
    {
//        glDisable( GL_DEPTH_TEST ) ;

//        glDisable( GL_CULL_FACE ) ;
//printf("wattup") ;
        glBindTexture( GL_TEXTURE_2D, 0 ) ;
        glDisable( GL_TEXTURE_2D ) ;
        glDepthFunc( GL_LEQUAL ) ;    // Only replace a pixel if new pixel is nearer
        //glPolygonMode( GL_FRONT, GL_LINE ) ;
        glPolygonMode( GL_FRONT, GL_LINE ) ;
        glEnable( GL_POLYGON_OFFSET_LINE ) ;
//        glPolygonOffset( -0.10, -0.10) ;
glPolygonOffset( 0, 0) ;
        glLineWidth(2.5) ;

        glUseProgram(GetShader(1)) ;
        glEnableVertexAttribArray(0) ;  // vert coords 

        vec4 colour(1,1,1,1) ;  // this should be white
        loopv(wg)
        {
            
            loopj(3) colour[j] = colors[i%10][j] ;
            int col = GetColor(1) ;
            glUniform4fv( col, 1, colour.v) ; 
            glUniformMatrix4fv(Get_P_MV_Matrix_Uniform(1), 1, FALSE, p.v) ;
            
//            glGetUniformfv(GetShader(1), GetColor(1), colour.v) ;

            glBindBuffer( GL_ARRAY_BUFFER, wg[i]->vertVBOid ) ;
            glVertexAttribPointer( 0, 3, GL_INT, GL_FALSE, 0, NULL) ;

            glDrawArrays( GL_TRIANGLES, 0, wg[i]->numverts ) ;
        }

        glPolygonMode( GL_FRONT, GL_FILL ) ; // or glPolygonMode( GL_FRONT, GL_LINE ) ;
        glDisable( GL_POLYGON_OFFSET_LINE ) ;
        glDepthFunc( GL_LESS ) ;    // Only replace a pixel if new pixel is nearer
        glLineWidth(1.0) ;
    }

    glColor4f( 1.0, 1.0, 1.0, 1.0 ) ;

    glDisableVertexAttribArray(0) ;
    glDisableVertexAttribArray(1) ;
    //--------------------------------------------------------------------------

    // Back to FFP
    glUseProgram(0) ;
    glBindBuffer( GL_ARRAY_BUFFER, 0 ) ;
#ifdef RECALC_TEXARRAY
    glDisable( GL_TEXTURE_2D_ARRAY ) ;
    glBindTexture( GL_TEXTURE_2D_ARRAY, 0 ) ;
#else
    glDisable( GL_TEXTURE_2D ) ;
    glBindTexture( GL_TEXTURE_2D, 0 ) ;
#endif
}


// mouselook cursor - what's highlights what's directly ahead of the camera. 
/*
    TODO: integrate this into the whole HFD/menu sequence of rendering. 

*/
void draw_cursor()
{
    // Yes this is way too much work for such a small amout of stuff. 
    // Should be folded into another function that justifies all this. 
    glMatrixMode( GL_PROJECTION ) ; 
    glPushMatrix() ;

    glLoadIdentity() ;
    glOrtho( 0, engine.current_w, 0, engine.current_h, -1, 1000 ) ; 
    glMatrixMode( GL_MODELVIEW ) ; 

    glPushMatrix() ;
    glLoadIdentity() ;

    float v1[3] = { engine.current_w/2, engine.current_h/2, 0 } ; 

    glPointSize( 10.0 ) ;

    glBegin( GL_LINES ) ;
                           v1[0] -= 10 ; 
        glVertex3fv( v1 ); v1[0] += 20 ; 
        glVertex3fv( v1 ); v1[0] -= 10 ; v1[1] -= 10 ; 
        glVertex3fv( v1 );               v1[1] += 20 ; 
        glVertex3fv( v1 ); 
    glEnd() ;

    glPopMatrix() ;
    glMatrixMode( GL_PROJECTION ) ; 
    glPopMatrix() ;
    glMatrixMode( GL_MODELVIEW ) ; 
}

extern void draw_world_box() ;
extern void draw_square() ;
extern bool hit_world ;


void render_editor()
{
    float v1[3] = { 0, 0, 0 } ; 

    // Before we draw a frame we must find out what we are looking at
    extern void update_editor() ;
    update_editor() ;
   
    if ( hit_world ) { glColor3f( .75, .75, 0) ; }
    glColor4f( 1, 1, 0, 1) ; 

    // ------------------------------------------------------------
    // FFP transformations 
    glMatrixMode( GL_MODELVIEW ) ; 
    glLoadIdentity() ; 
    // set camera perspective : normalize to make the XY plane horizontal, and the Z axis be up-down. 
    glRotatef( -camera.pitch/10 , 1, 0, 0 ) ; 
    glRotatef( -camera.yaw/10 ,   0, 1, 0 ) ; 
    glRotatef( -90 ,              1, 0, 0 ) ; 
    glTranslatef( -camera.pos.x, -camera.pos.y, -(camera.pos.z) ) ;
    // ------------------------------------------------------------

    // If we're aiming at the world, then we draw a helpful box 
    // around it. 

//glLineWidth( 1.5f ) ;
glLineWidth( 1.0f ) ;
glColor4f( 1, 1, 0, 1) ; 
    
    glColor4f(1,1,1,1) ;
    draw_world_box() ;

    glDisable( GL_DEPTH_TEST ) ;
    // draw a little dot or cross or crosshair. 
    draw_cursor() ;
    
    extern void draw_edit_cursor() ; // FIXME lol externs everywhere. 
    draw_edit_cursor() ;

    draw_selection() ;
    extern void draw_rayfront() ;
    draw_rayfront() ;
    glEnable( GL_DEPTH_TEST ) ;
    
    // green square that shows where on world boundary ray is entering world, 
    // if camera is looking at world from outside. 
    extern void draw_ray_start_node() ;
    draw_ray_start_node() ;

    render_menu2() ;
}

void render_tester()
{
// glEnable( GL_DEPTH_TEST ) ;

    float v1[3] = { 0, 50, 20 } ; 

//extern void render_test_001() ;
//render_test_001() ;

    //extern void render_shader_01() ;
    render_shader_01() ;

glMatrixMode( GL_PROJECTION ) ;
glPushMatrix() ; 
glLoadIdentity() ;
glMatrixMode( GL_MODELVIEW ) ;
glPushMatrix() ; 
glLoadIdentity() ;



// Completely exit custom shader mode
glDisable( GL_TEXTURE_2D ) ;
glBindTexture(GL_TEXTURE_2D, 0) ;
glUseProgram(0) ;   // Use basic, FFP-emulating shader. 
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

glDisable( GL_CULL_FACE ) ;
glDisable( GL_DEPTH_TEST ) ;
//glEnable( GL_DEPTH_TEST ) ;

glColor4f(1,1,1,.4) ;
    static vec verts[6] = 
    {
        vec(0,0,-100), 
        vec(100,0,-100), 
        vec(100,100,-100), 
        vec(100,0,-100), 
        vec(100,0,-100),
        vec(100,0,-100)
    };

    //-------------------------------------------------------------------------
    // Stage 1a: render FFP (attribs from vertex pointers) 
    
    glEnableClientState( GL_VERTEX_ARRAY ) ;
    glVertexPointer(
        3,              // size of every vertex (3 for 3 coordinates per vertex)
        GL_FLOAT,       // data type
        0,              // stride: space between consecutive elements of the same attribute (0 when the array is for one attribute only)
        verts           // vertex buffer (or where this particular attribute starts in an array)
        ) ;
    glDrawArrays(
        GL_TRIANGLES, 
        0,              // start index in enable array
        150               // number of verts to render
        ) ;    
    glDisableClientState( GL_VERTEX_ARRAY ) ;
    
    // Stage 1b: render FFP (attribs from vertex and tex coord pointers) 
    //-------------------------------------------------------------------------

/*

    glEnableVertexAttribLocation() ;
    glVertexAttribPointer() ;
    
    //-------------------------------------------------------------------------
    
    // Stage 2: render with old shaders (generic attribs, old shaders)
    glVertexAttribPointer(
        0, // index of attrib in shader
        4, GL_FLOAT, GL_FALSE, 
        0, // Stride: how many bytes to skip to next element for this attribute (0 in non-interleaved arrays)
        0); // Offset: 0 for non-interleaved arrays, and element-size*

    glDrawArrays(GL_TRIANGLES, 0, 2) ;
    glDisableVertexAttribArray(0) ;
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // Stage 3: render with modern shaders (generic attribs, modern shaders)
    glVertexAttribPointer(
        0, // index of attrib in shader
        4, GL_FLOAT, GL_FALSE, 
        0, // Stride: how many bytes to skip to next element for this attribute (0 in non-interleaved arrays)
        0);
    glDrawArrays(GL_TRIANGLES, 0, 2) ;
    //-------------------------------------------------------------------------
*/
    GLuint shaderprogram = GetShader(0) ;
    glBindAttribLocation(shaderprogram, 0, "InVertex") ;
    glBindAttribLocation(shaderprogram, 0, "InTexCoord0") ;
    glEnableVertexAttribArray(0) ;
//    glEnableVertexAttribArray(1) ;
    glVertexAttribPointer(
        0,          // attrib index in shader
        3,          // number of elements per attrib (3 coords for 3-element position attribute, or 2 for 2D texture coords)
        GL_FLOAT,   // data type
        GL_FALSE,   // don't normalize
        0,          // zero stride in array-per-attrib setup
        verts       // the data. It would be 0 for VBOs. 
    ) ;
    
//zzz


    glPopMatrix() ;
    glMatrixMode( GL_PROJECTION ) ;
    glPopMatrix() ;
    glMatrixMode( GL_MODELVIEW ) ;
}

extern GLuint mainfontID ;
extern vector<Font*> fonts ;

/*
    TODO FIXME: obviously, shove this into arrays and do it with either 
    per-frame array draws or VBO-based groups where the line(s) being changed 
    may be array-based calls but the rest of the non-editing lines are static. 
    Also switch to shaders whenever you feel like it. 

*/
void drawchar(int x,int _y, char c)
{

    glBegin(GL_QUADS) ;

    int y = _y + 2* fonts[0]->bot[(int)c] ;

//if (1)
//{
        //0
    //glTexCoord3d( 0, 0, c);    
    glTexCoord3d( 0, 0, (float)c/128);    
    glVertex3f( x, y+2*fonts[0]->height[(int)c], 0) ;

        //1 
    //glTexCoord3d( 0, fonts[0]->tcoords[c][1][1], c) ;
    glTexCoord3d( 
        fonts[0]->tcoords[(int)c][1][0],
        fonts[0]->tcoords[(int)c][1][1],
        (float)c/128) ;
    glVertex3f( x, y, 0);

        //2 // bottom right of a letter 
    //glTexCoord3d( fonts[0]->tcoords[c][2][0], fonts[0]->tcoords[c][2][1], c);
    glTexCoord3d( 
        fonts[0]->tcoords[(int)c][2][0],
        fonts[0]->tcoords[(int)c][2][1],
        (float)c/128) ;
    glVertex3f( x+2*fonts[0]->width[(int)c], y, 0);

        //3 
    //glTexCoord3d( fonts[0]->tcoords[c][3][0], 0, c);
    glTexCoord3d(
        fonts[0]->tcoords[(int)c][3][0],
        fonts[0]->tcoords[(int)c][3][1],
        (float)c/128) ;
    glVertex3f( x+2*fonts[0]->width[(int)c], y+2*fonts[0]->height[(int)c], 0);
//}

    glEnd() ;
}

// TODO: replace use of 'surfacetex' 3D texture with either texarray or texatlas. 
void render_menu2()
{
    return ;
    render_ortho_begin() ;
    //if debug 
//    CheckGlError() ; TODO: find error (if it appears here) causing error msg here. 
    glColor3f(1,1,1) ;
    
    extern GLuint surfacetex ;  // TODO: remove! replace with tex array/atlas

    // TODO: replace with render using texture array/texture atlas
    // TODO: allow viewing all loaded textures
    glEnable( GL_TEXTURE_3D ) ;
    glBindTexture(GL_TEXTURE_3D, surfacetex) ; // TODO: remove/replace!
    // texture scrolling! , actually. 
    float f = engine.activetex ;
    f = (f+0.5f) / 3.0f ;   // fuck this shit but this is what you have to do to specify the layer you want in a 3D texture. 
    int x = engine.current_w ;
    if (engine.numtex>0)
    {
        glBegin(GL_QUADS) ;
            glTexCoord3f( 0.0f, 0.0f, f);
            glVertex3f(x - 200.f, 200.0f, 0.0f);

            glTexCoord3f( 0.0f, 1.0f, f);
            glVertex3f(x - 200.0f, 000.0f, 0.0f);

            glTexCoord3f( 1.0f, 1.0f, f);         // when texture is repeating, this makes two get drawn. 
            glVertex3f(x, 000.0f, 0.0f);

            glTexCoord3f( 1.0f, 0.0f, f);
            glVertex3f(x, 200.0f, 0.0f);
        glEnd() ;
    }
    int next = 0 ;
    glColor3f(1,1,1) ;
    glDisable( GL_TEXTURE_3D ) ;
    render_ortho_end() ;
}


void render_menu() 
{
//    render_menu2() ;
    Menu* CurrentMenu = GetCurrentMenu() ;
    int x = 0 ;
    int y = 0 ;
    int h = fonts[0]->_height ;
    char msg[256] ;
    
    render_ortho_begin() ;

    glEnable( GL_TEXTURE_3D ) ;

    // Bluish green. 
    glColor3f( 0, .8, .3) ; 
    prstrstart() ;
    glEnable( GL_BLEND ) ;

//  glEnable( GL_DEPTH_FUNC ) ;
    glTranslatef(450, 3*engine.current_h/4, 0) ;
    glScalef(2, 2, 0) ;
    int currentitem = 0 ;

    if (CurrentMenu->numMenuItems>0)
    {
        for (int i=CurrentMenu->LowestVisible; i<CurrentMenu->numMenuItems; i++)
        {
            if (CurrentMenu->CurrentMenuItem==i) { currentitem = i ;}
            
            sprintf(msg, "%s (i=%d)", CurrentMenu->items[i].name, i) ;
            prstr( 0.f, -i*h, msg) ;
        }
    }

//    glDisable( GL_BLEND ) ;
    prstrend() ;

    //glDisable( GL_TEXTURE_2D ) ;
   /* 
    glMatrixMode(GL_PROJECTION) ;
    glLoadIdentity() ;
    glMatrixMode(GL_MODELVIEW) ;
    glLoadIdentity() ;
    glUseProgram(0) ;
    glBindBuffer(GL_ARRAY_BUFFER, 0) ;
    glDisable( GL_TEXTURE_2D ) ;
    glDisable( GL_TEXTURE_3D ) ;
    glBindTexture( GL_TEXTURE_2D, 0) ;
    */
    // zzz
//    glCullFace( GL_BACK ) ;
 //   glFrontFace( GL_CCW ) ;

    glDisable( GL_CULL_FACE ) ;
    glDisable(GL_DEPTH_TEST) ;
    glColor4f(0.9f,0.9f,0.9f,1) ;
    
    // Draw highlight around current menu selection
    int civp = currentitem*16 ;  // current item vertical position 
    glColor4f(0.5f,0.9f,0.9f,1) ;
    
    glLineWidth(1.0) ;
    //glEnable( GL_LINE_SMOOTH ) ;
    glDisable( GL_LINE_SMOOTH ) ;
    glBegin( GL_LINE_LOOP ) ;
        glVertex3f(-10, h-civp-2, 10) ; glVertex3f(200, h-civp-2, 10) ; glColor3f(1,0,0) ;
        glVertex3f(200, h-19-civp, 10) ; glVertex3f(-10, h-19-civp, 10) ;
     //   glVertex3f(280, h+2-civp, 10) ; //glVertex3f(50, h-20-civp, 10) ;
    glEnd() ;

/*    glBegin( GL_LINES ) ;
        glVertex3f( -50, h+2-civp, 10 ) ;
        glVertex3f( 10000, 0, 10 ) ;
        glVertex3f( -50, 100, 10 ) ;
        glVertex3f( 10000, 100, 10 ) ;
        glVertex3f( -50, -100, 10 ) ;
        glVertex3f( 10000, -100, 10 ) ;
    glEnd() ;*/


    glColor4f(0.5f,0.9f,0.9f,1) ;
/*
    static vec verts[6] =
    {
        vec(0,0,-10),
        vec(100,0,-10),
        vec(100,10,-10),
        vec(100,10,-10),
        vec(0,100,-10),
        vec(0,0,-10)
    };

    //-------------------------------------------------------------------------
    // Stage 1a: render FFP (attribs from vertex pointers) 
    glEnableClientState( GL_VERTEX_ARRAY ) ;
    glVertexPointer(
        3,              // size of every vertex (3 for 3 coordinates per vertex)
        GL_FLOAT,       // data type
        0,              // stride: space between consecutive elements of the same attribute (0 when the array is for one attribute only)
        verts           // vertex buffer (or where this particular attribute starts in an array)
        ) ;
    glDrawArrays(
        GL_TRIANGLES,
        0,              // start index in enable array
        6               // number of verts to render
        ) ;
    glDisableClientState( GL_VERTEX_ARRAY ) ;
*/

    render_ortho_end() ;
}


/*
    Function: render_info

    Purpose: to show everything we want to be watching at a particular time. 

    Framerates, polygons rendered, entity counts and types, inputs received, 
    vertex array counts, frame times, operations counts (operations like 
    physics frame, world render, menu render, console execute) , event counts, 
    operation durations (GL context changes, frames, render frames, physics frames, etc.).

    Possible other information: 

        - console contents 

        - geometry updates 

        - input updates 

        - graphics system stats 

    FIXME: add a translucent rectangle so that text is visible against 
    most backgrounds. 
*/
void render_info()
{
    if (!textenabled) { return ; }

    int i = 10 ; // Number of lines above bottom of screen. Leave room for command entry line. 
    int j = 0 ;

    #define next_line ((i++)+1)     // is that the stupidest thing to do or just effective? 
    int height = fonts[0]->_height ;

    render_ortho_begin() ;

        prstrstart() ;

        int width = 0 ;
        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM INFO SYSTEM
        ////////////////////////////////////////////////////////////////////////////////
        char info_msg[256] ;
        sprintf(info_msg, "height=%d ", height) ;
        prstr( 60.f, 0 - next_line*height, "info messages here") ; 

//        glTranslatef(0, 3*engine.current_h/4, 0) ;
        glTranslatef(0, engine.current_h, 0) ;

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM MAIN
        ////////////////////////////////////////////////////////////////////////////////
        char main_title[] = "+++++ main messages:----" ; 
        width = scrstrlen(main_title) ;
        glColor4f(0,0,0.2,1) ;
        prquad( 10.f, -i*height-2, width, height ) ;
        glColor4f(0.5,0.5,1,1) ;
        prstr( 10.f, - next_line*height, main_title) ;
        
        extern int main_msgs_num ;
        extern char main_msgs[100][256] ;
        j = 0 ;
        while (j<main_msgs_num)
        {
            width = scrstrlen(main_msgs[j]) ;
            glColor4f(0,0,0.4,.80) ;
            prquad( 10.f, -i*height-2, width, height ) ;
            glColor4f(0,0.8,0.8,.80) ;
            prstr( 10, - next_line*height, main_msgs[j]) ; j++ ;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM INPUT SYSTEM
        ////////////////////////////////////////////////////////////////////////////////
        char input_title[] = "+++++ input system:----" ; // j++ ;
        width = scrstrlen(input_title) ;
        glColor4f(0.3f,0,0.3f,1) ;
        //prquad( 10.f, -i*height-2, width, height, vec4(0.3,0.0,0.3,0.8)) ;
        prquad( 10.f, -i*height-2, width, height ) ;
        glColor4f(8,0,8,0.8) ;
        prstr( 10.f, - next_line*height, input_title) ; // j++ ;
        
        extern int input_msgs_num ;
        extern char input_msgs[100][256] ;
        extern void update_input_messages() ;
        update_input_messages() ;
      
        j = 0 ;
        while (j<input_msgs_num)
        {
            width = scrstrlen(input_msgs[j]) ;
            glColor4f(0.2,0,0.2,.80) ;
            prquad( 10.f, -i*height-2, width, height ) ;
            glColor4f(0.9,0,0.9,1) ;
            prstr( 10.f, - next_line*height, input_msgs[j]) ; j++ ;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM GEOMETRY
        ////////////////////////////////////////////////////////////////////////////////
        // PER ACTION messages
        char geo_title[] = "+++++ geometry messages:----" ; // j++ ;
        width = scrstrlen(geo_title) ;
        glColor4f(0,0,0,1) ;
        prquad( 10.f, -i*height-2, width, height ) ;
        glColor4f(0,1,1,1) ;
        prstr( 10.f, - next_line*height, geo_title) ; // j++ ;

        extern int geom_msgs_num ;
        extern char geom_msgs[100][256] ;
        j = 0 ;
        while (j<geom_msgs_num)
        {
            width = scrstrlen(geom_msgs[j]) ;
            glColor4f(0,0,0,.80) ;
            prquad( 10.f, -i*height-2, width, height ) ;
            glColor4f(0,1,1,1) ;
            prstr( 10.f, - next_line*height, geom_msgs[j]) ; j++ ;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM PHYSICS
        ////////////////////////////////////////////////////////////////////////////////
        char phys_title[] = "+++++ physics messages:----" ; 
        width = scrstrlen(phys_title) ;
        glColor4f(0,0,0.2,1) ;
        prquad( 10.f, -i*height-2, width, height ) ;
        glColor4f(0.5,0.5,1,1) ;
        prstr( 10.f, - next_line*height, phys_title) ;
        
        extern int phys_msgs_num ;
        extern char phys_msgs[100][256] ;
        j = 0 ;
        while (j<phys_msgs_num)
        {
            width = scrstrlen(phys_msgs[j]) ;
            glColor4f(0,0,0.4,.80) ;
            prquad( 10.f, -i*height-2, width, height ) ;
            glColor4f(0.8,0,0.8,1) ;
            prstr( 10, - next_line*height, phys_msgs[j]) ; j++ ;
        }
        ////////////////////////////////////////////////////////////////////////////////
        //                      MESSAGES FROM RENDERING
        ////////////////////////////////////////////////////////////////////////////////
        char render_title[] = "+++++ render messages:----" ; 
        width = scrstrlen(render_title) ;
        glColor4f(0,0,0.2,1) ;
        prquad( 10.f, -i*height-2, width, height ) ;
        glColor4f(0.5,0.5,1,1) ;
        prstr( 10.f, - next_line*height, render_title) ;
        
        extern int render_msgs_num ;
        extern char render_msgs[100][256] ;
        j = 0 ;
        while (j<render_msgs_num)
        {
            width = scrstrlen(render_msgs[j]) ;
            glColor4f(0,0,0.4,.80) ;
            prquad( 10.f, -i*height-2, width, height ) ;
            glColor4f(0.8,0,0.8,1) ;
            prstr( 10, - next_line*height, render_msgs[j]) ; j++ ;
        }

// glTranslatef(0, -3*engine.current_h/4, 0) ;
// prstr( 10, 0, "HAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAALLO!!") ;
////////////////////////////////////////////////////////////////////////////////
        prstrend() ;

        int h = next_line*height ;
        glColor3f(0.5f,0.5f,0.9f) ;
        glBegin( GL_LINES ) ;
            glVertex3f(50, h, 0) ;
            glVertex3f(500, h, 0) ;
        glEnd() ;

    render_ortho_end() ;
//    CheckGlError() ; TODO: find error (if it appears here) causing error msg here. 
        
/*      
    DEBUG ONLY
    // PER FRAME messages
    extern int geom_msgs_num2 ;
    extern char geom_msgs2[100][256] ;
    // messages from geometry 
    j = 0 ;
    while (j<geom_msgs_num2)
    {
        prstr( 10, - next_line*height, geom_msgs2[j]) ; j++ ;
    }
*/

}


extern Console console ;
#define c console
/*
    Simple algorithm: 

    For the number of console lines that are visible on the screen, 
    starting at the line which is the 'cursor' position, show lines. 

    Start at the top of the screen, defined by Y = engine.current_h

    FIXME: add a translucent rectangle behind console so that it's 
    visible against most backgrounds. 
*/
void render_console()
{
    // TODO: compute the overall size of the console, and then draw a rectangle 
    // of the proper size behind it. 
   
    render_ortho_begin() ;
    glColor4f(0.1, 0.1, 0.1, 0.75) ;
    glLoadIdentity() ;
    glBegin( GL_QUADS ) ;
        glVertex2i(50, 30) ;
        glVertex2i(engine.current_w - 50, 30) ;
        glVertex2i(engine.current_w - 50, engine.current_h-30) ;
        glVertex2i(50,  engine.current_h-30) ;
    glEnd() ;

    glDepthFunc( GL_LEQUAL ) ;
    
    glColor4f(0.6, 0.6, 0.6, 1.0) ;
    // Position console contents to the top of the screen  
    extern void prstrstart() ;
    prstrstart() ;
    glTranslatef(53, engine.current_h-fonts[0]->_height, 0 ) ;
    
    for (int i=0 ; i<CONSOLE_V_SIZE ; i++)
    {
        prstr( 0, -(i+1)*fonts[0]->_height, c.lines[i] ) ;
    }

    // Console entry buffer
    glLoadIdentity() ; 
    prstr( 53, 2*fonts[0]->_height, c.line_buffer ) ;

    prstrend() ;

    // Cursor 
    glColor3f(1,1,1) ;
    float cursor_pos_x = c.current_line_pix_len ;
    float v[3] = { cursor_pos_x+50, 1.5*fonts[0]->_height, 0 } ;
    glBegin( GL_LINES ) ;
        glVertex3fv( v ) ; v[0] += fonts[0]->_width ;
        glVertex3fv( v ) ;
    glEnd() ;

    render_ortho_end() ;
    CheckGlError() ;
}


// The proper syntax for this came to me in a waking dream after many missteps and realizing googling wasn't enough...
vector<void (*)(void)> visuals ;
/*
    FUNCTION: 
        render_visuals

    DESCRIPTION: 
        This function calls in sequence all the currently registered 
        rendering functions. 

    EXAMPLE USAGE: 
        The world-render function may be called first, followed by the 
        menu-render function, if a game is being played and the menu 
        has been activated. 

        To alter the visuals array, we can do the following: 

        - add elements at the end, which means things will get rendered 
          last. This is useful for interface overlays. 

        - add elements at the beginning, which has things render earlier. 
          This is useful for things like a skybox, or in edit mode, the 
          world boundary frame. 

    NOTES
        Every mode found in Recalc (main menu, play, edit, interface) knows
        what to do to activate itself and either pause or work with the
        currently enabled render set. This is similar to how input subsystems
        are managed. 

*/
void render_visuals() 
{
    loopi(visuals.length())
    {
        visuals[i]() ;
    }
}

/*
    DESCRIPTION: 
        Adds a visual to be drawn at the start of our render set. 

    EXAMPLES: 
        skybox, world edit frame, starscape
*/
void add_first_visual(void (*new_visual)(void))
{
    visuals.insert(0, new_visual) ;
}

/*
    DESCRIPTION:
        Does a brute-force search (which honestly should be fast since we 
        (as of 2012-10-01) do not expect this list to ever be very long)a
        of a particular function pointer and removes it. 
*/
void remove_visual(void (*new_visual)(void))
{
    visuals.removeobj(new_visual) ;
}

/*
    DESCRIPTION: 
        Removes everything in our visuals list. This is useful for a state 
        that is being enabled which wants to only set its own visuals. 
*/
void remove_all_visuals()
{
    visuals.shrink(0) ;
}


/*
    FUNCTION: 
        init_rendering

    DESCRIPTION: 
        At the start of our application, the default rendering function(s) 
        (or the one specified by a config file) is/are assigned to our 
        visual render function set. These functions will be rendered at all 
        times the application is active. This set can be changed by the 
        application's events. 
*/
void init_rendering()
{
    visuals.add(render_menu) ;

    // TODO: if we're using GLSL 1.2, then set 
    setVertexAttribs = SetVertAttribsGLSL15 ;
    setVertexAttribs = SetVertAttribsGLSL12 ;
}




