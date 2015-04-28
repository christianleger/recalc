/*

*/ 
 
#include "recalc.h"

extern Engine engine ;
#define e engine
extern Camera camera ;
 
#define BUFFER_OFFSET(i) ((void*)(i))
 
//Vertex, tex0
//
//SIZE : 4+4+4 +4+4 = 4*6 = 20 bytes
//It's better to make it multiple of 32
//32-20 = 12 bytes (of garbage should be added)
//12/4 = 3 floats should be added
struct TVertex_VT
{
	float	x, y, z;
	float	s0, t0, p0;
	float	padding[2];
};
 
struct vert_vcnt
{
	float	x, y, z ;       // 12 bytes
    float   nx, ny, nz ;    // 12 bytes
	float   s0, t0, p0 ;    // 12 bytes
//    uint color ;            // 4 bytes         == 40 bytes. 
//    float padding[11] ;   TODO: see if there's any perf improvement with padding included
};
 
	//float	padding[4];
 
 
 
//Globals
 
//A quad
GLushort	quad_idxs[60];
vert_vcnt   quad_vtxs[60];
 
 
//1 VAO for the quad
//1 VAO for the triangle
GLuint VAOID[2];

//1 IBO for the quad (Index Buffer Object)
//1 IBO for the triangle
GLuint IBOID[2];

//1 IBO for the quad (Vertex Buffer Object)
//1 IBO for the triangle
GLuint VBOID[2];
 
//1 shader for the quad
//1 shader for the triangle
 
int ProjectionModelviewMatrix_Loc[2];		//The location of ProjectionModelviewMatrix in the shaders
GLuint Get_P_MV_Matrix_Uniform(int ID /*= 0*/)
{
    return ProjectionModelviewMatrix_Loc[ID] ;
}

int TexSampler[2] ;
GLuint Get_MainTexSampler()
{
    return TexSampler[0] ;
}

int MainColor[2] ;
GLuint GetColor(int ID)
{
    return MainColor[ID] ;
}
int atlasScale ; // uniform for texture atlas scaling. 
int GetAtlasScaleUniform() 
{
    return atlasScale ; // uniform for texture atlas scaling. 
}
 
// loadFile - loads text file into char* fname
// allocates memory - so need to delete after use
// size of file returned in fSize

#define MAX_SHADER_SIZE 5000
char filecontents[2][MAX_SHADER_SIZE] ;                       // LoL this will crash if source is more than 4999 bytes. Leuls. 
char* loadShaderFile(const char *fname, int sindex)
{
    FILE* f = NULL ;

    f = fopen(fname, "r") ;

    int count = fread(&filecontents[sindex], 1, 5000, f) ;

    if (count>MAX_SHADER_SIZE)
    {
        // TODO: fail, log, complain and quit here. 
    }
    filecontents[sindex][count] = '\0' ; 
    fclose(f) ;
   
   return filecontents[sindex] ;
}
 
 
// printShaderInfoLog
// From OpenGL Shading Language 3rd Edition, p215-216
// Display (hopefully) useful error messages if shader fails to compile
void printShaderInfoLog(GLint shader)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar *infoLog;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0)
	{
		infoLog = new GLchar[infoLogLen];
		glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
        printf("\nSHADER INFO LOG: %s\n", infoLog) ;
		delete [] infoLog;
	}
}
 
 
void InitGLStates()
{
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(TRUE);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFFFFFFFF);
	glStencilFunc(GL_EQUAL, 0x00000000, 0x00000001);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	//glEnable(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);
	glClearColor(1.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glClearStencil(0);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DITHER);
	glActiveTexture(GL_TEXTURE0);
}
 
GLuint	ShaderProgram[2];
GLuint	VertexShader[2];
GLuint	FragmentShader[2];

/*
*/
GLuint GetShader(int ID/*= 0*/) 
{
    return ShaderProgram[ID] ;
}

bool shaderloaded = false ;

int LoadShader(
    const char *pfilePath_vs,   // Vertex Shader
    const char *pfilePath_fs,   // Fragment Shader
    bool bindNormal, 
    bool bindTexCoord0, 
    bool bindColor, 
    GLuint &shaderProgram, 
    GLuint &vertexShader, 
    GLuint &fragmentShader
    )
{
	shaderProgram=0;
	vertexShader=0;
	fragmentShader=0;
 
	// load shaders & get length of each
	int vlen = 0 ;
	int flen = 0 ;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);	

	const char* vertexShaderCStr = loadShaderFile(pfilePath_vs, 0);
	const char* fragmentShaderCStr = loadShaderFile(pfilePath_fs, 1);

    if (vertexShaderCStr)   { vlen = strlen(vertexShaderCStr) ; }
    if (fragmentShaderCStr) { flen = strlen(fragmentShaderCStr) ; }

	glShaderSource(vertexShader, 1, (const GLchar **)&vertexShaderCStr, &vlen);
	glShaderSource(fragmentShader, 1, (const GLchar **)&fragmentShaderCStr, &flen);
 
	GLint compiled = 0 ;

    // VERTEX SHADER
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
	if(compiled==FALSE)
	{
        printf("\nVERTEX SHADER FAILED !\n") ;
        printf("\n shader info log: ") ;
        printShaderInfoLog(vertexShader);
 
		glDeleteShader(vertexShader);
		vertexShader=0;
		glDeleteShader(fragmentShader);
		fragmentShader=0;
 
		return -1;
	}
    else
    {
//        printf("\nVERTEX SHADER OK! MESSAGES: \n") ;
		printShaderInfoLog(vertexShader);
    }
 
    // FRAGMENT SHADER
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
	if(compiled==FALSE)
	{
        printf("\nFRAGMENT SHADER FAILED !\n") ;
		printShaderInfoLog(fragmentShader);
 
		glDeleteShader(vertexShader);
		vertexShader=0;
		glDeleteShader(fragmentShader);
		fragmentShader=0;
 
		return -1;
	}
    else
    {
        //printf("\nFRAGMENT SHADER OK! MESSAGES: \n") ;
		printShaderInfoLog(fragmentShader);
    }
 
	shaderProgram = glCreateProgram();
    printf("\nSHADER.CPP:: new shader program %d", shaderProgram ) ;


	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
 
                        glBindAttribLocation(shaderProgram, 0, "InVertex");
	if(bindTexCoord0)   glBindAttribLocation(shaderProgram, 1, "InTexCoord0");
	//if(bindNormal)	    glBindAttribLocation(shaderProgram, 2, "InNormal");
	//if(bindColor)	    glBindAttribLocation(shaderProgram, 3, "InColor");

    /*  THE GREAT LINK OH MY GOD    */
	glLinkProgram(shaderProgram);
 
	GLint IsLinked;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, (GLint *)&IsLinked);
	if(IsLinked==FALSE)
	{
        printf("\nPROGREM RODING SHADER PROGRUM\n") ;
 
		GLint maxLength;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
		if(maxLength>0)
		{
			// char *pLinkInfoLog = new char[maxLength];
            char LinkInfoLog[512] ;
            // why use 'new' when a failure of a shader should just die an ignominious death? >:-D 
			glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, LinkInfoLog);

            printf("\nREASONS: \n%s\n", LinkInfoLog) ;

			//delete [] pLinkInfoLog;
		}
 
		glDetachShader(shaderProgram, vertexShader);
		glDetachShader(shaderProgram, fragmentShader);
		glDeleteShader(vertexShader);
		vertexShader=0;
		glDeleteShader(fragmentShader);
		fragmentShader=0;
		glDeleteProgram(shaderProgram);
		shaderProgram=0;
 
		return -1;
	}
    else
    {
        shaderloaded = true ;

        //////printf("\nGREAT SUCCESS BUILDING SHADER PROGRUM!\n") ;

        //printf("\n Shader progrum number = %d \n", shaderProgram) ;
    }
 
	return 1;		//Success
}

/*

*/
void loadShaderFiles(const char* vertfile, const char* fragfile, int shaderID)
{
    //printf("\n *************************************************") ;
    //printf("\n *************** LOADING SHADERS FILES ***********") ;
    //printf("\n *************** LOADING SHADERS FILES ***********") ;
    //printf("\n *******          vert name: %s        ", vertfile) ;
    //printf("\n *******          frag name: %s        ", fragfile) ;
    //printf("\n *************************************************") ;



	int result = LoadShader(
        vertfile, 
        fragfile, 
        false,  // use normals? 
        true,   // use tex coords? 
        true,   // use colors? 
        ShaderProgram[shaderID], 
        VertexShader[shaderID], 
        FragmentShader[shaderID]) ;

    glUseProgram(ShaderProgram[shaderID]) ;
    
    //printf("\n\n ************ UNIFORM LOCATIONS ******************" ) ;
    //printf("\n shader program = %d", ShaderProgram[shaderID]) ;
    
if (e.texarray && shaderID==0)
{
    ProjectionModelviewMatrix_Loc[shaderID] =   glGetUniformLocation(ShaderProgram[shaderID], "ProjectionModelviewMatrix");
    TexSampler[shaderID]                    =   glGetUniformLocation(ShaderProgram[shaderID], "texatlas");
    //MainColor[shaderID]                     =   glGetUniformLocation(ShaderProgram[shaderID], "colorX");
}
//if (e.texarray && shaderID==0)
//{
//    ProjectionModelviewMatrix_Loc[shaderID] =   glGetUniformLocation(ShaderProgram[shaderID], "ProjectionModelviewMatrix");
//    //TexSampler[shaderID]                    =   glGetUniformLocation(ShaderProgram[shaderID], "texatlas");
//    MainColor[shaderID]                     =   glGetUniformLocation(ShaderProgram[shaderID], "colorX");
//}
//    MainColor[shaderID]                     =   glGetUniformLocation(ShaderProgram[shaderID], "colorX");
//TODO: 
//if (shaderID==0)
//if (e.texatlas)
else
{
printf("\nSHADER.CPP: loadShaderFiles: running texatlas branch to get uniform locations. ") ;

                    ProjectionModelviewMatrix_Loc[shaderID] =   glGetUniformLocation(ShaderProgram[shaderID], "ProjectionModelviewMatrix");
if (shaderID==0)    TexSampler[shaderID]                    =   glGetUniformLocation(ShaderProgram[shaderID], "texatlas");
if (shaderID==1)    MainColor[shaderID]                     =   glGetUniformLocation(ShaderProgram[shaderID], "colorX");
if (shaderID==0)    atlasScale                              =   glGetUniformLocation(ShaderProgram[shaderID], "atlasScale");
}


printf("\n----------------------------------------------------") ;
printf("\nSHADER.CPP::loadShaderFiles: uniform values for shader program %d are: ", ShaderProgram[shaderID]) ;
    printf("\n projectionmodelviewmatrix = %d", ProjectionModelviewMatrix_Loc[shaderID]) ;
    printf("\n texture sampler = %d", TexSampler[shaderID]) ;
    printf("\n color uniform = %d", MainColor[shaderID]) ;
printf("\n----------------------------------------------------") ;
    
    
    glUseProgram(0) ;
    
    ////////////////////////////////////////////////////////////////////////////////
    // BS diagnostic code lifted from a dead gypsy
    ////////////////////////////////////////////////////////////////////////////////
    GLint nUniforms, maxLen;

    glGetProgramiv( ShaderProgram[shaderID], GL_ACTIVE_UNIFORM_MAX_LENGTH,
                  &maxLen);
    glGetProgramiv( ShaderProgram[shaderID], GL_ACTIVE_UNIFORMS,
                  &nUniforms);
    // Allocate space to store each uniform variable's name.
    GLchar name[1000] ;

    //Retrieve and print information about each active uniform using glGetActiveUniform and glGetUniformLocation.
    GLint size, location;
    GLsizei written;
    GLenum type;

    /*
    debug: show active uniforms
    */ 
    for( int i = 0; i < nUniforms; ++i ) {
        glGetActiveUniform( 
            ShaderProgram[shaderID], i, maxLen, &written,
            &size, &type, name );
        location = glGetUniformLocation(ShaderProgram[shaderID], name);
        //printf(" %-8d | %s\n", location, name);
    }


    if (result==-1) { printf("\n---- ERROR: Unable to load shader ----\n") ; }
    else { printf("\n---- SHADER FUNCTIONAL! ----\n") ; }

	int glversion[2];
    printf("\n OpenGL Information: \n") ;
	
    //This is the new way for getting the GL version.
	//It returns integers. Much better than the old glGetString(GL_VERSION).
    #ifdef RECALC_TEXARRAY
        glGetIntegerv(GL_MAJOR_VERSION, &glversion[0]);
        glGetIntegerv(GL_MINOR_VERSION, &glversion[1]);
   #endif
    printf("\nOpenGL major version = %d\n", glversion[0]) ;
    printf("\nOpenGL minor version = %d\n", glversion[1]) ;
}


/*
    A shader is comprised of a vertex and fragment program. 

    Use of any vertex program with any fragment program is 
    currently not in use. 
*/
void ActivateShader(const char* name, int shaderID)
{
    printf("\nSHADER.CPP::ActivateShader: being called with name=%s and shaderID=%d", name, shaderID) ;
    char vertname[64] ;     
    char fragname[64] ;     // Appends the _v and _f suffixes to glsl file names. 

    if (e.texarray)
    {
        sprintf(vertname, "data/shaders/%s_v.15.glsl", name) ;// Appends the _v and _f suffixes to glsl file names. 
        sprintf(fragname, "data/shaders/%s_f.15.glsl", name) ;// Appends the _v and _f suffixes to glsl file names. 
    }
    else
    {
        sprintf(vertname, "data/shaders/%s_v.12.glsl", name) ;// Appends the _v and _f suffixes to glsl file names. 
        sprintf(fragname, "data/shaders/%s_f.12.glsl", name) ;// Appends the _v and _f suffixes to glsl file names. 
    }

    printf("\n Loading vertex shader: %s\n", vertname) ;
    printf("\n Loading fragment shader: %s\n", fragname) ;

    loadShaderFiles(vertname, fragname, shaderID) ;
}


bool geometrycreated = false ; 
void CreateGeometry()
{
    geometrycreated = true ;

    ///////////////////////////////////////////////////////////////////////////////////
    // GEOMETRY CONSTRUCTION
    ///////////////////////////////////////////////////////////////////////////////////
    // First quad attributes
	quad_vtxs[0].x=0.0f;
	quad_vtxs[0].y=0.0f;
	quad_vtxs[0].z= 0 ;// -100.9f;
	quad_vtxs[0].s0 = 0 ;   // 
	quad_vtxs[0].t0 = 1.0f ;   // 
	quad_vtxs[0].p0 = 0 ;   // 
//	quad_vtxs[0].color=0xFFFFFFFF;
 
	quad_vtxs[1].x=100.0f;
	quad_vtxs[1].y=0.0f;
	quad_vtxs[1].z= 0 ;// -100.9f;
	quad_vtxs[1].s0 = 1.0f ;   // 
	quad_vtxs[1].t0 = 1.0f ;   // 
	quad_vtxs[1].p0 = 0 ;   // 
//	quad_vtxs[1].color=0xFFFF0000;
 
	quad_vtxs[2].x=0.0f;
	quad_vtxs[2].y=100.5f;
	quad_vtxs[2].z= 0 ;// -100.9f;
	quad_vtxs[2].s0 = 0 ;   // 
	quad_vtxs[2].t0 = 0 ;   // 
	quad_vtxs[2].p0 = 0 ;   // 
//	quad_vtxs[2].color=0xFF00FF00;
 
	quad_vtxs[3].x=100.0f;
	quad_vtxs[3].y=100.5f;
	quad_vtxs[3].z= 0 ;// -100.9f;
	quad_vtxs[3].s0 = 1.0f ;   // 
	quad_vtxs[3].t0 = 0 ;   // 
	quad_vtxs[3].p0 = 0 ;   // 
//	quad_vtxs[3].color=0xFF0000FF;

    // Second quad attributes
	quad_vtxs[4].x=0.0f + 50.0f ;
	quad_vtxs[4].y=0.0f;
	quad_vtxs[4].z= 0 ;         // -100.9f;
	quad_vtxs[4].s0 = 0 ;       // 
	quad_vtxs[4].t0 = 1.0f ;    // 
	quad_vtxs[4].p0 = 3.0f ;       // 2nd texture in the texture array
//	quad_vtxs[4].color=0xFFFFFFFF;

	quad_vtxs[5].x=100.0f + 50.0f ;
	quad_vtxs[5].y=0.0f;
	quad_vtxs[5].z= 0 ;         // -100.9f;
	quad_vtxs[5].s0 = 1.0f ;    // 
	quad_vtxs[5].t0 = 1.0f ;    // 
	quad_vtxs[5].p0 = 3.0f ;       // 
//	quad_vtxs[5].color=0xFFFF0000;
 
	quad_vtxs[6].x=0.0f + 50.0f ;
	quad_vtxs[6].y=100.5f;
	quad_vtxs[6].z= 0 ;         // -100.9f;
	quad_vtxs[6].s0 = 0 ;       // 
	quad_vtxs[6].t0 = 0 ;       // 
	quad_vtxs[6].p0 = 3.0f ;       // 
//	quad_vtxs[6].color=0xFF00FF00;
 
	quad_vtxs[7].x=100.0f + 50.0f ;
	quad_vtxs[7].y=100.5f;
	quad_vtxs[7].z= 0 ;         // -100.9f;
	quad_vtxs[7].s0 = 1.0f ;    // 
	quad_vtxs[7].t0 = 0 ;       // 
	quad_vtxs[7].p0 = 3.0f ;       // 
//	quad_vtxs[7].color=0xFF0000FF;
 
	quad_idxs[0]=0;
	quad_idxs[1]=1;
	quad_idxs[2]=2;
	quad_idxs[3]=2;
	quad_idxs[4]=1;
	quad_idxs[5]=3;

	quad_idxs[6]=4;
	quad_idxs[7]=5;
	quad_idxs[8]=6;
	quad_idxs[9]=6;
	quad_idxs[10]=5;
	quad_idxs[11]=7;

    ///////////////////////////////////////////////////////////////////////////////////
    // Now for some black, black, black black magic. 
    ///////////////////////////////////////////////////////////////////////////////////
 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
 
    // -------------------------------------------------------------------------
	// Vertex data
	glGenBuffers(1, &VBOID[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOID[0]);
	glBufferData(GL_ARRAY_BUFFER, 8*sizeof(vert_vcnt), &quad_vtxs[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // -------------------------------------------------------------------------
    // Index Data
	glGenBuffers(1, &IBOID[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOID[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 12*sizeof(GLushort), &quad_idxs[0], GL_STATIC_DRAW) ;


// TODO: replace with something that still compiles when glGenVertexArrays is not available. 
#ifdef RECALC_TEXARRAY
	//  VAO for the quad *********************
	glGenVertexArrays(1, &VAOID[0]);
	glBindVertexArray(VAOID[0]);
    // -------------------------------------------------------------------------
    // Vertex Data
	glBindBuffer(GL_ARRAY_BUFFER, VBOID[0]);
	glVertexAttribPointer(    // vertices
        //0, 3, GL_FLOAT, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(0)
        0, 3, GL_FLOAT, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(0)
        ) ;
//	glVertexAttribPointer(    // normals
 //       //1, 3, GL_FLOAT, GL_TRUE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*3)
  //      1, 3, GL_FLOAT, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*3)
   //     ) ;
	glVertexAttribPointer(    // textures
        //2, 3, GL_FLOAT, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*6)
        1, 2, GL_FLOAT, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*6)
        ) ;// TODO: different size of texture coords for texarray/texatlas
//	glVertexAttribPointer(    // textures
        //2, 3, GL_FLOAT, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*6)
//       2, 3, GL_FLOAT, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*6)
//        ) ;
//	glVertexAttribPointer(    // colors
 //       //3, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*9)
  //      3, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vert_vcnt), BUFFER_OFFSET(sizeof(float)*9)
   //     );

	glEnableVertexAttribArray(0) ; // vertices
//  glEnableVertexAttribArray(1) ; // normals   // glDisableVertexAttribArray(1);
//glEnableVertexAttribArray(2) ; // textures
	glEnableVertexAttribArray(1) ; // textures
//printf("\n\n\nLLLLLLLLLLLLLLLLLLLLLLLLLLOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\n\n\n") ;
    // -------------------------------------------------------------------------
    // Return to neutral state
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
#endif
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    // -------------------------------------------------------------------------

// If ultralogging - record this in log
//    printf("\nCOMPLETED SETTING UP BUFFERS AND ATTRIB LOCATIONS\n") ;
    return ; 
}


/* 
    zzz

    Main rendering function of the world. 
    TODO: move to render.cpp, when you separate shader creation and management 
    from use. 
*/
void render_shader_01()
{
    //--------------------------------------------------------------------------------------------------
    if (!shaderloaded) { return ; }  // Should die here unless testing


return ;
    // TODO: remove this when world rendering has replaced it. 


//    if (!geometrycreated) { CreateGeometry() ; printf("\nGEOMETRY CREATED. \n") ; }

    // WHITE OPAQUE
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f ) ;

    // Projection matrix
    glmatrixf p_mtx ;

    // Modelview matrix
    glmatrixf mv_mtx ;

    p_mtx.identity() ;
    mv_mtx.identity() ;
 
    // Texturing 
    
    //extern GLuint surfacetex ; 
    //glBindTexture(GL_TEXTURE_3D, surfacetex) ;
   // zzzzzzzzzzzzz 


extern GLuint texture2d ; 
extern GLuint textureatlas ; 

    // TODO: replace texture binding with a wrapper function which 
    // switches between texture array and texture atlas as needed. 
 //   if ()
    {
//        glBindTexture(GL_TEXTURE_2D_ARRAY, texture2d ) ;
#ifdef holyshit
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture2d ) ;
#endif
    }
  //  else
    {
    }

    // Enable desired shader
    glUseProgram(ShaderProgram[0]);

#ifdef holyshit
    glBindVertexArray(VAOID[0]);
#endif
//--------------------------------------------------------------------------------------------------
    
    
    p_mtx.identity() ;
    /*
        TODO: replace the aspect arg with a pre-calc'd value. 
    */
    p_mtx.perspective(engine.fov, (float)engine.current_w/(float)engine.current_h, 10.0, 300000.0 ) ;
    mv_mtx.identity() ;

    mv_mtx.translate( -camera.pos.x, -camera.pos.y, -(camera.pos.z) ) ;
    mv_mtx.rotate_around_x( -M_PI/2.0 ) ;
    mv_mtx.rotate_around_y( -(camera.yaw/10.0)*M_PI/180.0 ) ;
    mv_mtx.rotate_around_x( -(camera.pitch/10.0)*M_PI/180.0 ) ;
    p_mtx.mul(mv_mtx) ;

	glUniformMatrix4fv( ProjectionModelviewMatrix_Loc[0], 1, FALSE, p_mtx.v) ;
	glUniform1i( TexSampler[0], 0) ;
	glDrawRangeElements(GL_TRIANGLES, 0, 5, 6, GL_UNSIGNED_SHORT, NULL);    // First quad, first texture
    CheckGlError(); 


    //	float	x, y, z ;       // 12 bytes
    //  float   nx, ny, nz ;    // 12 bytes
    //	float   s0, t0, p0 ;    // 12 bytes
    //--------------------------------------------------------------------------------------------------
   
    // First Quad
    p_mtx.perspective(90.0, (float)engine.current_w/(float)engine.current_h, 10, 300000 ) ;
    mv_mtx.identity() ;
    mv_mtx.translate(200,0,-100) ;
    p_mtx.mul(mv_mtx) ; // mv_mtx.mul(p_mtx) ;
	glUniformMatrix4fv( ProjectionModelviewMatrix_Loc[0], 1, FALSE, p_mtx.v) ;
	glUniform1i( TexSampler[0], 0) ;
//	glDrawRangeElements(GL_TRIANGLES, 0, 5, 6, GL_UNSIGNED_SHORT, NULL);
    CheckGlError(); 
    glUseProgram(0);
    /*
    */
/*
    // Second Quad
    p_mtx.perspective(90.0, (float)engine.current_w/(float)engine.current_h, 10, 300000 ) ;
    mv_mtx.translate(-100,0,-100) ;   // A further translation down the Z axis
    p_mtx.mul(mv_mtx) ;
   
	glUniformMatrix4fv( ProjectionModelviewMatrix_Loc[0], 1, FALSE, p_mtx.v) ;
	glUniform1i( TexSampler[0], 0) ;
	//glDrawRangeElements(GL_TRIANGLES, 6, 11, 6, GL_UNSIGNED_SHORT, NULL);
    //glDrawRangeElements(GL_TRIANGLES, 0, 5, 6, GL_UNSIGNED_SHORT, NULL);
    CheckGlError(); 
*/

//--------------------------------------------------------------------------------------------------
    // Return to Fixed Functionality Pipeline
#ifdef holyshit
    glBindVertexArray(0);
#endif
	glUseProgram(0);


    CheckGlError(); 
    return ;
}


void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}


/*
*/
void Shader_Exit_Function(int value)
{
#ifdef holyshit
	glBindVertexArray(0);
#endif
	
    
    glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
 
	glUseProgram(0);
 
	glDeleteBuffers(1, &IBOID[0]);
	glDeleteBuffers(1, &IBOID[1]);
	glDeleteBuffers(1, &VBOID[0]);
	glDeleteBuffers(1, &IBOID[1]);

    for (int shaderID=0;shaderID<2;shaderID++)
    {
        glDetachShader(ShaderProgram[shaderID], VertexShader[shaderID]);
        glDetachShader(ShaderProgram[shaderID], FragmentShader[shaderID]);
        glDeleteShader(VertexShader[shaderID]);
        glDeleteShader(FragmentShader[shaderID]);
        glDeleteProgram(ShaderProgram[shaderID]);
     
        glDetachShader(ShaderProgram[shaderID], VertexShader[shaderID]);
        glDetachShader(ShaderProgram[shaderID], FragmentShader[shaderID]);
        glDeleteShader(VertexShader[shaderID]);
        glDeleteShader(FragmentShader[shaderID]);
        glDeleteProgram(ShaderProgram[shaderID]);
    }

}


/*
*/
void Get_P_MV_Matrices(float* p,float* mv)
{
    glGetFloatv(GL_PROJECTION_MATRIX, p) ;
    glGetFloatv(GL_MODELVIEW_MATRIX, mv) ;
}


/*
*/
void Show_P_MV_Matrices()
{
    float p[16] ;
    float mv[16] ;

    Get_P_MV_Matrices(p, mv) ;

    printf("\nModelviews: \n") ;
    loopi(4) { printf("ffp  | %f  %f  %f  %f\n", mv[i], mv[i+4], mv[i+8], mv[i+12]) ; }

    printf("\n--------\n") ;

    printf("\nProjections: \n") ;
    loopi(4) { printf("ffp  | %f  %f  %f  %f\n", p[i], p[i+4], p[i+8], p[i+12]) ; }
}


/*
*/
void init_shaders()
{
    printf("\n[SHADER::init_shaders] called... ") ;

	glActiveTexture(GL_TEXTURE0);
#ifdef holyshit
	glBindVertexArray(0);
#endif
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    ActivateShader("default", 0) ; // Main shader for rendering textured geometry
//TODO: reinstate    ActivateShader("frame", 1) ;   // Shows the frame outline of geometry
    ActivateShader("frame", 1) ;   // Shows the frame outline of geometry

//    glgetuniformlocation
//    glGetUniformLocation(ShaderProgram[shaderID], "ProjectionModelviewMatrix");
    //loadShaderFiles() ;
    //loadShaderFiles() ;
    printf("\n-----------------------------------------------------------------------------------------") ;
    printf("\n [SHADERS]: init_shaders executed. ") ;
//    printf("\n SHADER.CPP::loadShaderFiles: uniform values for shader program %d are: ", ShaderProgram[shaderID]) ;
    printf("\n projectionmodelviewmatrix = %d", ProjectionModelviewMatrix_Loc[0]) ;
    printf("\n texture sampler = %d", TexSampler[0]) ;
    printf("\n color uniform = %d", MainColor[0]) ;
    printf("\n-----------------------------------------------------------------------------------------") ;


    printf("done. ") ;
}


