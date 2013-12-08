

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

/*
    Some details inspired by the sauerbraten model. But adapted to our needs!

*/
struct texslot
{
    unsigned int glID ;

    short height ;
    short width ;

    void* data ;    // Where the information for the pixels is held
}
#endif  // __TEXTURE_H__
