/*
 * Copyright Christian Léger
 * 
 * Inspiration and code bits from the countless selfless contributors from the cyberverse, including folks from 
 *
 * Sauerbraten - NeHe - Valve - ID Software - Lighthouse 3D - 
 *
 * This code is being created with the intention of enhancing humanity's ability to entertain itself 
 * and communicate with itself. The various ways in which this is achieved include: 
 *
 *  - easier creation of 3D worlds and object representations
 *     - easier understanding of the tools which create and animate virtual worlds
 *     - exposure to new ideas for having fun (in educational and self-actualizing    ways)
 *     - exposure to new ideas for thinking and representing technical and symbolic information 
 *     - exposure to new ideas for collaborating with other beings. 
 *     - meshing tools which, before this, were always used separately and toward separate problem domains
 *
 *     All of these are simply coins on a pile; I don't know that anything here will change anything completely, 
 *     but I think it can change everything a little bit, and in doing so contribute momentum to our cultural evolution. 
 * 
 */
 
//#include <time.h>       // used for time measurement function clock_gettime
#include <stdio.h>
#include <stdlib.h>
//#include <GL/gl.h>

//#include <SDL.h>
//#include <SDL_image.h>

#include "recalc.h"

#include <GL/glu.h>


/* all other globals are found in recalc.c */

//#define SCREEN_WIDTH 2600
//#define SCREEN_HEIGHT 900

int default_screen_width = 1200 ; 
int default_screen_height = 800 ; 


/*---------------------------------------------------*/
// External functions 
/*---------------------------------------------------*/
void CheckGlError() ;
void initialize_subsystems() ; 
/*---------------------------------------------------*/
//                  control variables
/*---------------------------------------------------*/
bool testonly = false ;
bool usetextures = true ; // Why the hell not
bool usesound = true ;

/*-----------------------------------------------------------------*/
//                  function prototypes 
/*-----------------------------------------------------------------*/
void readargs( int, char** ) ;
void initSDL( Engine * engine ) ;
void initOpenGL( ) ; 
void resize_window( int width, int height ) ;
/*-----------------------------------------------------------------*/


//extern Engine engine ;
GLuint tex2Did = 0 ;
GLuint surfacetex = 0 ;
GLuint texture2d = 0;

GLuint gsurf = 0 ;
/**
 * Function: load_texture
 *  
 *  TODO: make this function reusable on a per-map basis. Also, manage total number of textures in use  
 *        according to the number of maps active. Also, move this code to the texture.cpp module. 
 *  
 */
void load_textures() 
{
    Engine& engine = GetEngine() ;
    #define e engine
    CheckGlError() ;
    glEnable(GL_TEXTURE_2D) ;

    glGenTextures(1,&GetMainTextures());
    CheckGlError() ;

    // TODO: write code that checks available OpenGL features and if texture array not available, 
    // then use the texture atlas feature. 
    engine.texatlas = true ;
#ifdef holyshit 
    // TEXTURE ARRAY
    glEnable( GL_TEXTURE_2D_ARRAY ) ;
    glBindTexture(GL_TEXTURE_2D_ARRAY,GetMainTextures());
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR ) ;
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,                          // mipmap level
        GL_RGBA,
        512,                        // width
        512,                        // height
        3,                          // number of layers
        0,                          // border
        GL_RGB,                     // pixel type
        GL_UNSIGNED_BYTE,           // pixel channel format
        NULL                        // null because data will be assigned later; now just reserving mem. 
        );
#else
    // TEXTURE ATLAS
    glEnable( GL_TEXTURE_2D ) ;
    glBindTexture(GL_TEXTURE_2D,GetMainTextures());
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR ) ;
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   
    // Get texture size limits for our texture atlas. 
    // We don't necessarily use this size because on some platforms a texture with these dimensions 
    // will exceed available video memory. 
    int maxw = 0 ;
    int maxh = 0 ;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxw);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxh);

    // This is the texture atlas. Will hold all the geometry textures we can use for a particular map. 
    glTexImage2D(
        GL_TEXTURE_2D,  // 'target'
        0,              // Mipmap level
        //GL_RGBA, 
        3, 
        //2048, // TODO: compute a reasonable maximum size for texture atlas. Possibly use both 
        // TODO: ya right, just assume 16mpix X 24 bits = 64mb is ok on most video cards since a long time! If you get paying customers 
        // bitching and we have a pile of money then we'll think about heuristics to fix this deficiency. 
        //2048, // active map texture sets and available video memory to decide on this. Rule of thumb: use up to half video memory. 
        4096, // TODO: compute a reasonable maximum size for texture atlas. Possibly use both 
        4096, // active map texture sets and available video memory to decide on this. Rule of thumb: use up to half video memory. 
        0, 
        GL_RGB, 
        GL_UNSIGNED_BYTE, 
        NULL                // data, NULL means just allocate memory
        ) ;
#endif
    e.texatlassize = 4096 ;
    e.texatlastilesize = 512 ;
    e.texatlasrowcount = 8 ;
    CheckGlError() ;

    //--------------------------------------------------------------------------------
    //    Image Loading Begins
    //--------------------------------------------------------------------------------
    SDL_Surface* data_image ;
    char* data = NULL ;

    /*
        TODO:
            desired logic: for every map that can be activated in the current zone, 
            load all the images needed. Load them up at the resolution we're able 
            to handle on the current hardware, into a texture array or a texture atlas. 
    */


//---------------------------------------------------------------------------------    
// LOAD AN IMAGE
//---------------------------------------------------------------------------------    
    data_image = IMG_Load("data/textures/grid.png") ;
    if (data_image == NULL)
    {
        printf("\nPROBREM LOADING FILE\n") ;
        printf("\nThe error is: %s\n", IMG_GetError()) ;
        Quit(1) ;
    }
    data = (char *)(data_image->pixels) ;
    e.addtex(0) ;
    //---------------------------------------------------------------------------------    
    
#ifdef holyshit
    // Texture array
    glBindTexture(GL_TEXTURE_2D_ARRAY, GetMainTextures()) ;
    glTexSubImage3D( 
        GL_TEXTURE_2D_ARRAY, 0,             // 3D texture, full-resolution (no mipmap)
        0,0,0,                              // offsets 
        data_image->w, data_image->h, 1,    // dimensions for each component
        GL_RGB, GL_UNSIGNED_BYTE,           // pixel format
        data                                // the data
        );
#else
  glEnable(GL_TEXTURE_2D) ;
    int sizex = 512 ;
    int sizey = 512 ;
    glBindTexture(GL_TEXTURE_2D, GetMainTextures()) ;
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0, // mipmap level
        0,0,// x, y (determined by standard size and tex id)
        sizex, sizey,
        GL_RGB, GL_UNSIGNED_BYTE,           // pixel format
        data                                // the data
        ) ;
    printf("\nPost gltexsubimage2d check error: ") ;
    CheckGlError() ;
#endif
    
    CheckGlError() ;
    SDL_FreeSurface(data_image) ;

//---------------------------------------------------------------------------------    
// LOAD AN IMAGE
//---------------------------------------------------------------------------------    
    data_image = IMG_Load("data/textures/morphbrick.jpg") ;
    if (data_image == NULL)
    {
        printf("\nPROBREM LOADING FILE\n") ;
        printf("\nThe error is: %s\n", IMG_GetError()) ;
        Quit(1) ;
    }
    data = (char *)(data_image->pixels) ;
    e.addtex(1) ;

#ifdef holyshit
    // Texture array
    glBindTexture(GL_TEXTURE_2D_ARRAY, GetMainTextures());
    glTexSubImage3D( 
        GL_TEXTURE_2D_ARRAY, 0,             // 3D texture, full-resolution (no mipmap)
        0,0,1,                              // offsets 
        data_image->w, data_image->h, 1,    // dimensions for each component
        GL_RGB, GL_UNSIGNED_BYTE,           // pixel format
        data                                // the data
        );
#else
    // Texture atlas
    glBindTexture(GL_TEXTURE_2D,GetMainTextures());
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0, // mipmap level
        512,0,// x, y (determined by standard size and tex id)
        512, 512,
        GL_RGB, GL_UNSIGNED_BYTE,           // pixel format
        data                                // the data
        ) ;
#endif
    CheckGlError() ;
    SDL_FreeSurface(data_image) ;

//---------------------------------------------------------------------------------    
// LOAD AN IMAGE
//---------------------------------------------------------------------------------    
    data_image = IMG_Load("data/textures/planets.jpg") ;
    if (data_image == NULL)
    {
        printf("\nPROBREM LOADING FILE\n") ;
        printf("\nThe error is: %s\n", IMG_GetError()) ;
        Quit(1) ;
    }
    data = (char *)(data_image->pixels) ;
    e.addtex(2) ;

#ifdef holyshit
    // Texture array
    glBindTexture(GL_TEXTURE_2D_ARRAY, GetMainTextures());
    glTexSubImage3D( 
        GL_TEXTURE_2D_ARRAY, 0,             // 3D texture, full-resolution (no mipmap)
        0,0,2,                              // offsets 
        data_image->w, data_image->h, 1,    // dimensions for each component
        GL_RGB, GL_UNSIGNED_BYTE,           // pixel format
        data                                // the data
        );
#else
    // Texture atlas
    glBindTexture(GL_TEXTURE_2D,GetMainTextures());
    // This is what handles automated mipmapping in old OpenGL!!!!!!!!!!!!!!!!!!!!!!
    // Using it before the last glTexSubImage2D is supposed to do the trick. 
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0, // mipmap level
        1024,0,// x, y (determined by standard size and tex id)
        512,512,// dimensions
        GL_RGB, GL_UNSIGNED_BYTE,           // pixel format
        data                                // the data
        ) ;
#endif
    CheckGlError() ;
    SDL_FreeSurface(data_image) ;

    // Finally, mipmaps
#ifdef holyshit
    glGenerateMipmap( GL_TEXTURE_2D_ARRAY ) ;
#else
//    glGenerateMipmap( GL_TEXTURE_2D ) ;
#endif
    // TODO: add log
}

/*-----------------------------------------------------------------*/
//                  function definitions 
/*-----------------------------------------------------------------*/

void readargs( int argc, char** argv )
{
    // Args
    Engine& e = GetEngine() ;

    // TODO: move to logging/optional output/console
    /* printf("\nNUMBER OF ARGUMENTS: %d\n", argc) ;
    printf("\n ") ;
    loopi(argv) printf("%s ", argv[i]) ;
    */

    int i = 1 ; // first argument is the name of this program (unless this is a lib)
    int args = argc ;
    while (args>0 && i < argc)
    {
        //TODO: also to logging/console - printf("\nFIRST ARG: %s\n", argv[i]) ;
        if (argv[i][0]=='-')
        {
            switch (argv[i][1])
            {
                case 'w':
                {
                    if (argc <= i) {;} // TODO: exit here. Invent proper single-place exit. 
                    e.win_w = atoi(&argv[i+1][0]) ; // zzz
                    printf("\n\t[MAIN]: WATTUP WATTUP ****\n") ;
                    break ;
                }
                case 'f':
                {
                    e.fullscreen = true ;
                    printf("\n\t[MAIN]: ENGINE FULLSCREEN ACTIVATED ****\n") ;
                    break ;
                }
                case 'q':
                {
                    if (strlen(argv[i])>2)
                    {
                        switch (argv[i][2])
                        {
                            case 'm': // disable music
                            {
                                musicoff() ;
                                break ;
                            }
                            case 's': // disable sound effects
                            {
                                sfxoff() ;
                                break ;
                            }
                        }
                    } 
                    else 
                    {
                        soundoff() ;
                    }
                    // TODO: log printf("\nQUIET MODE SELECTED. SOUND SHOULDN'T PLAY. \n") ;
                    break ; 
                }
                case 't':
                {
                    // TODO: log printf("\nTEST ONLY MODE SELECTED. NOW PROBABLY EXITING. \n") ;
                    args = 0 ;
                    testonly = true ;
                    break ;
                }
                default:
                {
                    printf("\nUnhandled argument: %s\n", argv[i]) ;
                    break ;
                }
            }
        }
        else
        {
            printf("\nNot Unhandled argument: '%s' from args)\n", argv[i]) ;
        }
        args-- ;
        i++ ;
    }
}


void initSDL( Engine * engine )
{
    /* prepare SDL */
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
    {
        fprintf( stderr, "Video initialize failed: %s\n", SDL_GetError( ) ) ;
        Quit( 1 ) ;
    }
    
    engine->videoInfo = SDL_GetVideoInfo( ) ;
    if ( !engine->videoInfo )
    {
        fprintf( stderr, "Video query failed: %s\n", SDL_GetError( ) ) ;
        Quit( 1 ) ;
    }

    // record desktop and window resolution for later use 
    engine->desktop_w = engine->videoInfo->current_w; 
    engine->desktop_h = engine->videoInfo->current_h; 
    if (engine->win_w==0) engine->win_w = default_screen_width; 
    if (engine->win_h==0)engine->win_h = default_screen_height; 

    printf("\nDesktop dimensions provided by videoInfo: %d x %d: \n", 
           engine->videoInfo->current_w , 
           engine->videoInfo->current_h 
           ) ; 

    /* the flags to pass to SDL_SetVideoMode */
//    engine->videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    engine->videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    engine->videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    engine->videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */

//    if ( engine->videoInfo->hw_available )
//        engine->videoFlags |= SDL_HWSURFACE;
//    else
//        engine->videoFlags |= SDL_SWSURFACE; // are you kidding me. 

    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ) ;
    
    /* get a SDL surface */
    engine->videoFlagsFS  = engine->videoFlags; 
    engine->videoFlagsFS |= SDL_FULLSCREEN; 

    // FIXME: this is convoluted bullshit // WINDOW
    if ( engine->fullscreen )
    {
        engine->current_w = engine->desktop_w; 
        engine->current_h = engine->desktop_h; 
    }
    else
    {
        engine->current_w = engine->win_w ; //default_screen_width; 
        engine->current_h = engine->win_h ; //default_screen_height; 
    }

    // VISUAL
    printf("\nSetting video mode: \n") ; 
    printf("\n window size: ") ;
    printf("\n %d X %d \n", engine->win_w, engine->win_h) ; 
    printf("\n desktop size: ") ;
    printf("\n %d X %d \n", engine->desktop_w, engine->desktop_h) ; 
    /*
    */
    engine->surface = SDL_SetVideoMode( 
        engine->win_w, 
        engine->win_h, 
        SCREEN_BPP, 
        engine->videoFlags
        //engine->videoFlags
    ) ;
    if (engine->fullscreen)
    {
    engine->surface = SDL_SetVideoMode( 
        engine->current_w, 
        engine->current_h, 
        SCREEN_BPP, 
        //((engine->fullscreen)   ?  engine->videoFlagsFS : engine->videoFlags)
        engine->videoFlagsFS 
    ) ;
    }

    /* Verify there is a surface */
    if ( !engine->surface )
    {
        fprintf( stderr,  "Video mode set failed: %s\n", SDL_GetError( ) ) ;
        Quit( 1 ) ;
    }
    else
    {
        fprintf( stderr,  "GREAT SUCCESS FOR THE COMPUTER GENERATING A SURFACE: %s\n", SDL_GetError( ) ) ;
        //"Messages given by SDL_GetError: %s\n", 
    }

    printf("\n current screen width: %d", engine->win_w ) ; 
    printf("\n current screen height: %d", engine->win_h ) ; 
}


void initOpenGL( )
{
#ifdef WIN32
    glewInit() ;
#endif

    glShadeModel( GL_SMOOTH ) ;
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f ) ;
    glClearDepth( 1.0f ) ;
    
    /* Really Nice Perspective Calculations */
    //glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;

    glEnable( GL_POINT_SMOOTH ) ;
    glEnable( GL_LINE_SMOOTH ) ;
    //glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST ) ;
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST ) ;
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST ) ;
    
    glEnable( GL_DEPTH_TEST ) ;
    glDepthFunc( GL_LESS ) ; // checked. 

    glEnable( GL_BLEND ) ;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ; // checked. 

    // TODO: check if texture arrays are available. If not, it means we shall be 
    // using texture atlases. 

    return ; 
}

/* reset our viewport after a window resize */
void resize_window( int width, int height, int fov )
{
    /* Field of view factor; 1 gives proper proportions in a realistic scene. 
       For instance, a square looks square. 
    */
    GLfloat ratio;
    GLfloat angle ; 
    /* avoid cancelling the universe */
    if ( height == 0 ){   height = 1;  }

    ratio = ( GLfloat )width / ( GLfloat )height;

    printf( "\n attemping to resize window with fov %d \n", fov ) ; 

    printf("\n VIEWPORT SIZE: %dX%d", width, height) ;
    if (fov==-1) { angle = 60 ; }
    else { angle = fov ; }

    /* viewport. */
    glViewport( 0, 0, ( GLsizei )width, ( GLsizei )height ) ;

    /* By default, large viewing volume. Culling in software. */
    glMatrixMode( GL_PROJECTION ) ;
    glLoadIdentity( ) ;

    ////gluPerspective( (GLfloat)angle, ratio, 0.01f, 10000000.0f ) ;
    gluPerspective( (GLfloat)angle, ratio, 10.0f, 300000.0f ) ;
    //gluPerspective( (GLfloat)140, ratio, 10.0f, 300000.0f ) ;
//    gluPerspective( (GLfloat)40, ratio, 10.0f, 300000.0f ) ;
//    gluPerspective( 60.0f, ratio, 0.01f, 50000.0f ) ;

    /* default matrix to manipulate is modelview */
    glMatrixMode( GL_MODELVIEW ) ;
    glLoadIdentity( ) ;

    return ;
}


// Main resources to use 
//zzzextern Engine engine ;
extern Camera camera ; 


int main_msgs_num = 0 ;
char main_msgs[100][256] ;
uint millis = 0 ;
int main( int argc, char **argv )
{
    bool done = false ;

    unsigned long long first_cycle = 0 ;
    unsigned long long delta_cycle = 0 ;
    get_cycle(first_cycle) ;
    printf("\nENGINE STARTING ON CLOCK CYCLE %lld\n", first_cycle) ;

    Engine& engine = GetEngine() ;
    
    if (testonly)
    {

        GLint texSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);

        printf("\n MAX texture size: %d      \n", texSize) ;

    // Testing code. This can run stuff just to evaluate the characteristics of some 
    // code without having to load and run everything. 
    // Move this block further down is some resources are needed to perform some tests. 
    //readconfig("data/config.cfg") ;

       /* 
        // Things to test: 
        // sub-milimeter timing code
        testtiming() ;

        // floating-point to integer conversion
        // TODO: when did you want to do this, anyway? 

        // test a new hash table implementation. 
        testhashtable() ;

        // Whoops haha we leave now because we only wanted to run some arbitrary code ! 
        // get_cycle(delta_cycle) ;
        cycle_delta(first_cycle, delta_cycle) ;
        printf("\nENGINE EXITING AFTER %lld CYCLES\n", delta_cycle) ;

        char* hello = NULL ;

        hello = new char[10+1] ;
        delete hello ;
        */

        Quit(0) ;
    }

    engine.window_active = true ;

    readargs(argc, argv) ; 
    
    initSDL( &engine ) ; // this by itself does a lot: it gives us our window and rendering context
    
    initOpenGL( ) ; CheckGlError() ;    /* OpenGL SUBSYSTEM */
    
    init_scripting() ;

    // Parts of engine - input, geometry, sound, menus, etc. 
    initialize_subsystems() ; 

    // Any reason this can't be done in the previous subsystem initialization? 
    engine.fov = 100 ;
    resize_window( engine.current_w , engine.current_h, engine.fov ) ;
    printf("\n INITIALIZATION: resizing window after SDL init. dimensions are     %d x %d\n", engine.win_w, engine.win_h ) ; 

    // Bookkeeping and timing variables. 
    uint framecount = 0; 
    uint last_millis = 0;
    uint sec_progress = 0 ; 
    int delta = 0 ; 
    int delta_millis = 0 ; 
    int physics_millis = 0 ; 

    millis = SDL_GetTicks() ;
    last_millis = millis ; 

    bool annoying = false ; 
    
    // pump the event queue empty before starting 
    SDL_Event event;
    while ( SDL_PollEvent( &event ) ) ;

//    SDL_WM_GrabInput(SDL_GRAB_ON) ;
    SDL_ShowCursor(SDL_DISABLE) ;         

    //unsigned int last_frame = SDL_GetTicks() ;

    load_textures() ; CheckGlError() ;
    
    /* main loop */
#define FRAME_TIME 5 // 5 gives about 200 fps. Not relevant when using vsync. 
    while ( !done )
    {
        millis = SDL_GetTicks() ;
        delta = millis - last_millis ;
        if (delta<0) {delta =0 ;} // Fun: create a test case that shows this is needed. 
        if ( delta >= 1 ) 
        {
            delta_millis += delta ;
            physics_millis += delta ;
            last_millis = millis ; 
        }

        while ( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
                case SDL_MOUSEMOTION: 
                {
                    if (!engine.paused) 
                    handle_mouse_motion( &event ) ;
                    break ; 
                }

                case SDL_MOUSEBUTTONDOWN:
                {
                    handle_mouse_button_down( &event ) ;
                    break ;
                }
                case SDL_MOUSEBUTTONUP:
                {
                    handle_mouse_button_up( &event ) ;
                    break ;
                }

                case SDL_KEYDOWN: 
                case SDL_KEYUP:         
                {
                    handle_key( &event ) ;
                    break ; 
                }

                // Application master control
                case SDL_QUIT: 
                {
                    Quit( 0 ) ;
                    break ; 
                }

                // Interaction with OS
                case SDL_ACTIVEEVENT:
                {
//SDL_WM_GrabInput( SDL_GRAB_ON ) ;
//return current_commands to whatever last_commands holds
//extern void reset_commands() ;
//reset_commands() ; 
                    SDL_GL_SwapBuffers() ; 
                    break ; 
                }
                default:
                    break ; 
            }
        } // end of event polling 

        // PHYSICS 
        if (!engine.paused)
        //if ( physics_millis >= 1 )
        //if ( 1 )
        if ( physics_millis >= PHYSICS_FRAME_TIME )
        {
            // Reset counter if frame was run
            if ( engine.physics )
            { 
                physics_frame( physics_millis ) ; 
            }

            physics_millis = 0 ;
        }

        if (
               ( delta_millis >= FRAME_TIME-1) &&
               ( engine.window_active ) &&
               ( !engine.paused )
           )
        {
            // TODO: put this inside render_visuals(). 
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

    
glMatrixMode(GL_PROJECTION) ;
glPushMatrix() ;
glLoadIdentity() ;

/*
    GLfloat angle = 90 ; 
    GLfloat ratio = 16.0/9.0 ;
    gluPerspective( (GLfloat)angle, ratio, 10.0f, 300000.0f ) ;
*/

glMatrixMode(GL_MODELVIEW) ;
glPushMatrix() ;
glLoadIdentity() ;

//CheckGlError() ;

/*
    glBindTexture(GL_TEXTURE_2D,0);
    glColor4f(1,0,1,1) ;
    glBegin( GL_LINES ) ;
        glVertex3f(0,0,0) ;
        glVertex3f(0.8,0.8,0) ;
    glEnd() ;
*/



glDisable(GL_CULL_FACE) ;
glDisable(GL_DEPTH_TEST ) ;
glEnable(GL_TEXTURE_2D);
//CheckGlError() ;
glBindTexture(GL_TEXTURE_2D,GetMainTextures());
glBegin( GL_QUADS ) ;
    glTexCoord2f( 0.0f, 1.0f ); glVertex3f( 0.5f, -0.5f, -0.5f );
    glTexCoord2f( 1.0f, 1.0f ); glVertex3f(  1.0f, -0.5f, -0.5f );
    glTexCoord2f( 1.0f, 0.0f ); glVertex3f(  1.0f,  0.5f, -0.5f );
    glTexCoord2f( 0.0f, 0.0f ); glVertex3f( 0.5f,  0.5f, -0.5f );

/*
      glTexCoord2f( 0.0f, 0.125f ); glVertex3f( 0.5f, -0.5f, -0.5f );
      glTexCoord2f( 0.125f, 0.125f ); glVertex3f(  1.0f, -0.5f, -0.5f );
      glTexCoord2f( 0.125f, 0.0f ); glVertex3f(  1.0f,  0.5f, -0.5f );
      glTexCoord2f( 0.0f, 0.0f ); glVertex3f( 0.5f,  0.5f, -0.5f );
*/
/*
*/
      
glEnd() ;
glBindTexture(GL_TEXTURE_2D,0);
glDisable(GL_TEXTURE_2D) ;
glUseProgram(0) ;


//AA

//extern void render_ortho_begin() ;
//render_ortho_begin() ;

//glBindVertexArray(0);
//glDisableVertexAttribArray(0);
//glDisableVertexAttribArray(1);
//glDisableVertexAttribArray(2);
glDisable(GL_CULL_FACE) ;
//glDisable(GL_DEPTH_TEST ) ;
glColor4f(1,1,1,.4) ;
    static vec verts[6] =
    {
        vec(0,0,-0.5),
        vec(10,10,-0.5),
        vec(10,0,-0.5),
        vec(100,10,-0.5),
        vec(10,10,-0.5),
        vec(10,30,-0.5)
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
/*
glBegin( GL_TRIANGLES ) ;
    glVertex3f(0,0,0) ;
    glVertex3f(1,0,0) ;
    glVertex3f(1,1,0) ;
glEnd() ;
*/

/*
glMatrixMode(GL_PROJECTION) ;
glPushMatrix() ;
glLoadIdentity() ;

glMatrixMode(GL_MODELVIEW) ;
glPushMatrix() ;
glLoadIdentity() ;
*/



///////////////

    glDisable( GL_POINT_SMOOTH ) ;
    glDisable( GL_LINE_SMOOTH ) ;
    //glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST ) ;
    //glHint( GL_POINT_SMOOTH_HINT, GL_NICEST ) ;
    glHint( GL_POINT_SMOOTH_HINT, GL_FASTEST ) ;
    glHint( GL_LINE_SMOOTH_HINT, GL_FASTEST ) ;
    
    glDisable( GL_DEPTH_TEST ) ;
    glDepthFunc( GL_LESS ) ; // checked. 

    glEnable( GL_BLEND ) ;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ; // checked. 


glColor4f(1,1,1,1) ;
glTranslatef(0, 0, -10) ;
    glDrawArrays( GL_TRIANGLES, 0, 6) ;
glTranslatef(0, 0, -10) ;
    glDrawArrays( GL_TRIANGLES, 0, 6) ;
glTranslatef(0, 0, -10) ;
    glDrawArrays( GL_TRIANGLES, 0, 6) ;
glTranslatef(0, 0, -10) ;
    glDrawArrays( GL_TRIANGLES, 0, 6) ;
    glDrawArrays( GL_TRIANGLES, 0, 6) ;
    glDisableClientState( GL_VERTEX_ARRAY ) ;
    glDisableClientState( GL_TEXTURE_COORD_ARRAY ) ;
    glDisableClientState( GL_NORMAL_ARRAY ) ;
//AB

//extern void render_ortho_end() ;
//render_ortho_end() ;


/*
*/

glPopMatrix() ;
glMatrixMode(GL_PROJECTION) ;
glPopMatrix() ;
glMatrixMode(GL_MODELVIEW) ;

            if ( engine.testing ) { render_tester() ; }

            if ( engine.rendering ) { render_world() ; }

            if ( engine.editing ) { render_editor() ; }

            if ( engine.menu ) { render_menu() ; }
            
            if ( engine.info ) { render_info() ; }  // Debug and analysis

            if ( engine.console ) { render_console() ; }

            // frame counting 
            SDL_GL_SwapBuffers() ; 
            sec_progress += delta_millis ;
            framecount++; 
            delta_millis = 0 ;
        } // end if delta_millis > FRAME_TIME 
        else
        {
            if ( !engine.window_active )
            {
                delta_millis = 0 ;
                SDL_GL_SwapBuffers() ; 
            }
            SDL_Delay(1) ; 
           // SDL_GL_SwapBuffers() ; 
        }
        // FRAME COUNTING and other 1Hz actions. 
        if ( sec_progress >= 1000 )
        {
            main_msgs_num = 0 ;
            sprintf( main_msgs[0], 
                "  running time: %d. FPS=%d.", 
                SDL_GetTicks()/1000, framecount 
                ) ; main_msgs_num ++ ;

            framecount = 0 ; 
            sec_progress = 0 ; 
        }
    } // end of 'while not done'

    printf("\n Exiting Recalc normally. ") ;
    Quit( 0 ) ;
    return( 0 ) ;
}

// BELOW are miscellaneous conveniences and curiosities.

/*
SDL_WarpMouse(0,0) ;         
SDL_WM_GrabInput(SDL_GRAB_ON) ;
SDL_WM_ToggleFullScreen( surface ) ;
*/
void testee(char * arg)
{
    printf("\nfunction called: %s ", arg) ;
}
void test_command()
{
    Command _c; 

    _c.function = testee; 

    _c.function((char*)"pleasure") ;

    return;
}

