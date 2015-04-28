


#include "recalc.h"



/*
    Saves the world to a file of name name.

    Method used: Traverse the whole tree iteratively. 
  
    data schema: 
    ----Version 01----
        Nv                  // Version number for this data file
        Ns                  // The number of sections described in this file
                            // Possible sections: 
                            //      - geometry
                            //      - entities
                            //      - future additions to the map format
        L_p                 // Length of path to this node
        P1                  // Path indexes 1-n, held in 3-bit chunks
        P2
        ...
        Pn

        M                   // Faces mask: bits 0-5 (counted from the right-most bit) indicate which faces are visible

        e0
        t0                  // The edge values followed by the tex values, for each face
        ...
        e5
        t5                  

    More succinctly: [ L P1 ... Pn M t0 ... t5 e0 ... e5 ] 
    size: 8-15 bytes Not huge! 

    ----End Version 01----

*/

// Yay a 1mb buffer to do writes with. Who knows how good this is :-) 
#define MAX_BUFFER_SIZE 1000000
// Max node size (as recorded to file) is sizeof(edges[3]+tex[12])+max_depth = 24+16 = 16 bytes. 
#define MAX_NODE_SIZE 16
uchar byte_buffer[MAX_BUFFER_SIZE] ;
int byte_buffer_index ;

// From these two lines you'll guess that for any one time period, the buffer 
// is only used for saving or for loading, not both. 
#define save_buffer byte_buffer
#define sb save_buffer
//#define sbi byte_buffer_index 

#define load_buffer byte_buffer
#define lb load_buffer
//#define lbi byte_buffer_index 
void World::SaveToFile(const char* name)
{
    char d = 0 ;
    uchar idxs[20] ;
    loopi(20) {idxs[i]=0 ;}
    Octant* path[20] ;
    loopi(20) {path[i]=NULL ;}
    Octant* CN = &root ;
    path[0] = CN ;

    int32_t sbi = 0 ;
//    int32_t s = 24 ; // size of data from start of 'edges' to end of 'tex'. 

    FILE* f = fopen(name, "wb") ;
    if (!f)
    {
        printf("\n WORLD::SAVE ==> FAILED TO OPEN FILE %s; ABORTING SAVE OPERATION", name) ; // FIXME: this is a log event
        return ;
    }

    // Header
    sb[0] = 1 ;     // Version of this data file. 
    sb[1] = 1 ;     // number of sections in this data file. 
    sbi+=2 ;

    while (d>=0)
    {
        // Flush buffer to file if it's filled up
        if (sbi+100>=MAX_BUFFER_SIZE)   // Definitely stop filling buffer ahead of possibly reaching the end. 
        {
            fwrite(sb, 1, sbi, f) ; // write all sbi bytes currently in buffer
            sbi = 0 ;
        }

        if ( CN->children )
        {
            if (idxs[d]<8)
            {
                // We're going down to a child, marked relative to its parent by idxs[d]. 
                CN = &CN->children[idxs[d]] ; 
                d++ ;
                path[d] = CN ;
                idxs[d] = 0 ; // Start the children at this level. 
                continue ;
            }
        }
        // If we don't have children, then maybe we have geometry.
        else 
        { 
            // add this node to the save buffer
            if ( CN->has_geometry() ) 
            { 
                // check that at least one face is visible. 
                bool visible = false ;
                uchar facemask = 0 ;
                int nodesize = sbi ;

                loopi(6) { if (CN->tex[i] > -1) { facemask |= (1<<i); } }
                // only write data for this node if it's visible. 
                sb[sbi] = d ; // path length
                sbi++ ;

                int64_t paths = 0 ; 

                ////////////////////////////////////////////////////////////////////////////////
                // Save path in 3bit steps into a int64_t. Max path length: 21 
                //  (permitting world scale of 21, or size 2^21 = 2^15 * 2^6    = 1km * 64! Shit. 
                ////////////////////////////////////////////////////////////////////////////////
                
                // number of bytes needed to encode this many 3-bit indexes. 
                int numbytes = ((d*3)>>3) + (((d*3)&(0x7)) ? (1) : (0))  ;   // ssshh make into macro if too ugly to look at

                // squeeze path info into a single 64bit element. 
                loopi(d) { paths |= (((int64_t)idxs[i])<<(61-i*3)) ; }  // Gross but true. 
                // Copy 64bit's bytes into save buffer
                loopi(numbytes)
                {
                    // Yes this is gross. It packs a int64 into consecutive bytes. Ignores alignment. 
                    uchar c = ((uchar)((paths>>(56-8*i)))) ;    
                    sb[sbi+i] = c ;
                }
                sbi += numbytes ;

                // faces visibility info 
                sb[sbi] = facemask ;
                sbi++ ;

                if (facemask)   // facemask nonzero means this node has at least one visible face
                {
                    // Now for every visible face, record its edge values and tex value
                    //if (facemask)
                    {
                        // TODO: optimization: if a block only shows one face or one face and its opposite, 
                        // only record the int32 that describes them. 
                        // TODO: if you do this, then the read operation has to take that into account. 
                        loopi(6)
                        {
//                            if (facemask&(1<<i))
                            {
                                // 3 bytes - 2 for edges, 1 for tex value
                                sb[sbi]     = CN->edges[2*i] ;
                                sb[sbi+1]   = CN->edges[2*i+1] ;
                                sb[sbi+2]   = CN->tex[i] ;
                                sbi += 3 ;
                            }
                        }
                    }
                    //else
                    {
                        // a blank facemask value means this node is solid but invisible. Great compression 
                        // because no need to store edges or tex ids. 
                    }

                    nodesize = sbi - nodesize ;

                    // number of path indexes to this node (this also saves the 
                    // node's position in the world). This could easily be 
                    // compressed, but would make for shit code. I will only do 
                    // this if it proves to be desirable to save a lot of space. 
                }
                else    // a node that has substance but isn't visible? cool, we just store the path facemask, and material ID. 
                {
                    // TODO: add anything, like material type, that also belongs here. 
                    // sbi += ?
                }
            }
        }

        // These last lines of the while loop make up the 'going up the tree' action. 
        path[d] = NULL ;
        d-- ;
        if (d<0) {   break ; } // Past the root? Then we're done. 
        CN = path[d] ;
        idxs[d]++ ;   // Next time we visit this node, it'll be next child. 
    } // end while d>=0

    // Now put any remaining data into file and wrap things up. 
    sb[sbi] = 255 ;   // end of geometry marker
    sbi++ ;
    fwrite(sb, 1, sbi, f) ; // write all sbi bytes currently 

    printf("\n save done. wrote %d bytes. ", sbi) ;
    sbi = 0 ;   // pointless
    
    fclose(f) ;
    return ;
}


// values for the different sections of a data file
#define HEADER 0
#define GEOMETRY 1
#define ENTITIES 2
/*
    World :: LoadFromFile

    How things are loaded: 
        world size - tells us the size of the root node
        world scale - tells us the scale of the root node. 
        number of nodes. 
        every node:
            edges, face tex values, path to this node.

        eventually: 
        number of entites
        every entity
*/
void World::LoadFromFile(const char* name)
{
    int lbi = 0 ;   // index into file read buffer
    int count = 0 ;
    int bns = 0 ;   // used to store the node's binary size

    uchar d = 0 ;    // depth of a node's path
    int max = 0 ;

    int version = 0 ;   // TODO: when do we give a version? When we want, or when things are user-usable. 
    int numsections = 0 ;
    int currentsection = 0 ;

    FILE* f = fopen(name, "rb") ;
    if (!f)     { printf("\n FAILED TO OPEN FILE %s; ABORTING LOAD OPERATION", name) ; // FIXME: this is a log event 
    return ;    }

    printf("\n\t Load World From File... \n") ;

    int readsize = 0 ;
    while( !feof(f) )
    {
        // Read part: 
        count = fread(&lb[lbi], 1, MAX_BUFFER_SIZE, f ) ;

        lbi = 0 ;   // Load Buffer Index = lbi. 

        // If current contents of buffer will exceed its capacity on reading another, then use this content 
        // to create our tree. 
        //if (lbi+MAX_NODE_SIZE>=MAX_BUFFER_SIZE)
        while(count>lbi)
        {
            if (currentsection==HEADER)
            {
                version = lb[lbi] ; 
                lbi++ ;
                printf("\n Load - File version: %d \n", version ) ;

                if (count>lbi) 
                {
                    numsections = lb[lbi] ; 
                    lbi++ ;
                    printf("\n Load - This file has %d section(s). \n", numsections) ;
                    if (numsections>0)
                    {
                        currentsection = 1 ;
                    }
                }
                continue ;
                
                //break ;
            }
            else if ( currentsection == GEOMETRY )
            {
                // Alright, now read all the geometry we can from this current buffer. 

                if (count>lbi) 
                {
                    printf("\n Load - Current section = GEOMETRY \n") ;
                    while(count>lbi) // still have bytes to read
                    {
                        int nodesize = lbi ;
                        uint pathlength = lb[lbi] ;

                        if (pathlength==255)  // This is the condition that means we have reached the end of geometry data (since no node has path length 0).
                        {
                            printf("\n Load - end of geometry. Moving to next section. \n") ;
                            currentsection++ ;
                            break ;
                        }

                        lbi += SetNode(&lb[lbi]) ; // insert node into the tree, and learn how big this node was to advance our read index. 
                        nodesize = lbi - nodesize ;
                        readsize += nodesize ;

                        /*
                        if (count>lbi)
                        {
                            printf("\n still have to read from buffer because count=%d and lbi=%d", count, lbi) ;
                        }
                        */
                    }
                    continue ;
                }
            }
            else if ( currentsection == ENTITIES )
            {
                if ( numsections < currentsection )
                {   
                    break ;
                }

                if (count>lbi)
                {
                }

                break ;                             // temporary
            }
            else if ( currentsection == ENTITIES )
            {
            }

        }   // End while (count>lbi)
        printf("\nDone reading data file. \n") ;

    }   // End while (!feof(f))

//if (feof(f)) { break ; }
//lbi += count ;

//d = lb[lbi] ;

// Part 2: node path
//ignore = fread(&lb[lbi], d, 1, f ) ;
//if (feof(f)) { break ; }
//lbi += d ;

    // Now use anything left in the buffer 
    max = lbi ;
    lbi = 0 ;
    while (lbi<max)
    {
//  bns = SetNode(Octant* world, &lb[lbi]) ; lbi += bns ;
        lbi += size + d ;
    }

    // TODO: add to log
    fclose(f) ;
    printf("\nLoad File closed.\n") ;

    // AnalyzeGeometry of this tree

    // Create VBOs for this tree. 
}



extern void new_octants( Octant* oct ) ;
















#define CHECKPATH \
    printf("\n path: ") ; \
    loopi(pathlength)   \
    {   \
        printf(" %lld", ( ( paths >> (  61-i*3  ) ) &0x7 )     ) ;    \
    }   \
    printf("\n paths = %d", ((char)(paths>>61))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>58))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>55))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>52))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>49))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>46))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>43))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>40))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>37))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>34))&0x07) ; \
    printf("\n paths = %d", ((char)(paths>>31))&0x07) ; \
    printf("\n paths = %ld", paths) ;   \
    printf("\n paths = %lld", paths&0xff00000000000000 ) ;  \

/*
*/











/*
    A bunch of bytes define a node. This node is going to get added to our 
    tree. 
*/
int World::SetNode(const uchar* loadbuf) 
{
    #define buf loadbuf
    Octant* CN = &root ;            // start at world root 
    uchar d = loadbuf[0] ;
    uchar depth = 0 ;
    int nodesize ;                  // Size in bytes of this. 
  
    
    int lbi = 0 ;       // Load loadbuffer index

//printf("\nSetNode----------------------------------------------\n") ;
    
    int pathlength = loadbuf[lbi] ; lbi++ ;
    int numbits = pathlength*3 ;
    int numbytes = ((numbits)>>3) + (((numbits)&(0x7)) ? (1) : (0))  ;   // ssshh make into macro if too ugly to look at

    // Unpack the byte's path
    int64_t paths = 0 ;
    loopi(numbytes)
    {
        paths |= (((int64_t)(loadbuf[lbi+i]))&0x0ff)<<(56-8*i) ; // Horrible but true 
    }
    lbi += numbytes ;
    // CHECKPATH ;
  
    while (depth < d)
    {
        CN->setneedsupdate() ; 
        if (!CN->children)
        {
            new_octants( CN ) ;
        }
        // FIXME: path indexes from bits 
        // set current node to the next child in the path
        //int hello =     ( paths   >> (  61-d*3  ) ) &0x7  ;
        char hello =     (((char)( paths   >> (  61-depth*3  ) ))&0x07 ) ;  // Don't ask how long it took to get correct syntax here. 
//        printf("\n (depth=%d) new node: moving to child %d", depth, hello) ;
        //CN = &CN->children[     ( paths   >> (  61-d*3  ) ) &0x7 ] ;
        //CN = &CN->children[     ( paths   >> (  61-d*3  ) ) &0x7 ] ;
        CN = &CN->children[  hello ] ;

        depth++ ;
    }
//    now we're at the right depth. 
//    set this leaf node's attributes to those provided here. 

    uchar facemask = loadbuf[lbi] ;
    lbi++ ;

//    printf("\n facemask = %d", facemask) ;

//    printf("\nMap load: facemask\n") ;
//    if (facemask)
    {
//        printf("\nload: face showing. edge values: \n") ;
        loopi(6)
        {
            if (facemask&(1<<i))
            {
                // Record edge and face tex values.
                CN->edges[2*i]      = loadbuf[lbi ] ;
                CN->edges[2*i+1]    = loadbuf[lbi + 1] ;
                CN->tex[i]          = loadbuf[lbi + 2] ;
                lbi += 3 ;
            }
            else // invisible faces have full edges
            {
                CN->edges[2*i]      = 0xff ;
                CN->edges[2*i+1]    = 0xff ;
                CN->tex[i]          = -1 ;
            }
        }
    }
//    else    
//    {
        // facemask blank means solid, invisible node
//        printf("\nface NOT showing. edge values: \n") ;
//        loopi(6)
//        {
//                printf("\n egdes: %d", CN->edges[2*i]) ;
//                printf("\n egdes: %d", CN->edges[2*i+1]) ;
//                CN->edges[2*i]      = 0xff ;
//                CN->edges[2*i+1]    = 0xff ;
//                CN->tex[i]          = -1 ;
//        }
//    }

    return lbi ;    // nodesize
}


