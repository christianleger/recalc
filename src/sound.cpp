// sound.cpp: basic positional sound using sdl_mixer

/*
    Copyright whoever wrote this for Sauer. 

    Copyright Christian Léger for things that differ from the sauer original. 
*/

//#include "engine.h"
#include "recalc.h"

#include "SDL_mixer.h"
#define MAXVOL MIX_MAX_VOLUME

bool nosound = true;
char soundvol = 128 ;
char musicvol = 128 ;

// PROTOTYPES
void closemumble() ;
void startsound() ;
void startmusic(char *name, char *cmd) ;
void resetchannels() ;

#define addOrFind access
struct soundsample
{
    char *name;
    Mix_Chunk *chunk;

    soundsample() : name(NULL) {}
    ~soundsample() { DELETEA(name); }
};

struct soundslot
{
    soundsample *sample;
    int volume, maxuses;
};

struct soundchannel
{ 
    int id;
    bool inuse;
    vec loc; 
    soundslot *slot; 
    // extentity *ent; 
    int *ent; 
    int radius, volume, pan;
    bool dirty;

    soundchannel(int id) : id(id) { reset(); }

    bool hasloc() const { return loc.x >= -1e15f; }
    void clearloc() { loc = vec(-1e16f, -1e16f, -1e16f); }

    void reset()
    {
        inuse = false;
        clearloc();
        slot = NULL;
        ent = NULL;
        radius = 0;
        volume = -1;
        pan = -1;
        dirty = false;
    }
};
vector<soundchannel> channels;
int maxchannels = 0;

soundchannel &newchannel(int n, soundslot *slot, const vec *loc = NULL, /*extentity *ent = NULL,*/ int radius = 0)
{
  //  if(ent)
    {
//        loc = &ent->o;
 //       ent->visible = true;
    }
        printf("\nLALALALALA\n") ;
    if (channels.inrange(n)) 
    {
        printf("\nchannels says 'inrange' for %d\n", n) ;
    }
    while(!channels.inrange(n)) 
    {   
        printf("\nADDING A CHANNEL           \n") ;
        printf("\nADDING A CHANNEL           \n") ;
        channels.add(channels.length()) ;
    } 
    soundchannel &chan = channels[n];
    chan.reset();
    chan.inuse = true;
    if(loc) chan.loc = *loc;
    chan.slot = slot;
    //chan.ent = ent;
    chan.radius = radius;
        // printf("\nNEW CHANNEL NUMBER %d\n", chan) ;
    return chan;
}
/*
*/

void freechannel(int n)
{
    // Note that this can potentially be called from the SDL_mixer audio thread.
    // Be careful of race conditions when checking chan.inuse without locking audio.
    // Can't use Mix_Playing() checks due to bug with looping sounds in SDL_mixer.
    if(!channels.inrange(n) || !channels[n].inuse) return;
    soundchannel &chan = channels[n];
    chan.inuse = false;
    // if(chan.ent) chan.ent->visible = false;
}

void syncchannel(soundchannel &chan)
{
    if(!chan.dirty) return;
    if(!Mix_FadingChannel(chan.id)) Mix_Volume(chan.id, chan.volume);
    Mix_SetPanning(chan.id, 255-chan.pan, chan.pan);
    chan.dirty = false;
}

void stopchannels()
{
    loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(!chan.inuse) continue;
        Mix_HaltChannel(i);
        freechannel(i);
    }
}

void setmusicvol(int musicvol);
//VARFP(soundvol, 0, 255, 255, if(!soundvol) { stopchannels(); setmusicvol(0); });
//VARFP(musicvol, 0, 128, 255, setmusicvol(soundvol ? musicvol : 0));

char *musicfile = NULL, *musicdonecmd = NULL;

Mix_Music *music = NULL;
SDL_RWops *musicrw = NULL;
stream *musicstream = NULL;

void setmusicvol(int musicvol)
{
    if(nosound) return;
    if(music) Mix_VolumeMusic((musicvol*MAXVOL)/255);
}

void stopmusic()
{
    if(nosound) return;
    DELETEA(musicfile);
    DELETEA(musicdonecmd);
    if(music)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
        music = NULL;
    }
    if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }
    DELETEP(musicstream);
}

//VARF(soundchans, 1, 32, 128, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
unsigned int soundchans = 0.f ;
//VARF(soundfreq, 0, MIX_DEFAULT_FREQUENCY, 44100, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
float soundfreq = 0.f ;
//VARF(soundbufferlen, 128, 1024, 4096, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
unsigned int soundbufferlen = 0.f ;
//VARF(soundbufferlen, 128, 2048, 4096, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));

void initsound()
{
    if(Mix_OpenAudio(soundfreq, MIX_DEFAULT_FORMAT, 2, soundbufferlen)<0)
    {
        nosound = true;
        //conoutf(CON_ERROR, "sound init failed (SDL_mixer): %s", (size_t)Mix_GetError());
        DEBUGTRACE(("sound init failed (SDL_mixer): %s", Mix_GetError()));
        return;
    }
	Mix_AllocateChannels(soundchans);	
    Mix_ChannelFinished(freechannel);
    maxchannels = soundchans;
    nosound = false;
}

void musicdone()
{
    if(music) { Mix_HaltMusic(); Mix_FreeMusic(music); music = NULL; }
    if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }

    DELETEP(musicstream);
    DELETEA(musicfile);

    if(!musicdonecmd) return;

    char *cmd = musicdonecmd;
    musicdonecmd = NULL;
//     execute(cmd);
    delete[] cmd;
}

extern stream *openzipfile(const char *name, const char *mode) ;
extern const char *findfile(const char *filename, const char *mode) ;
Mix_Music *loadmusic(const char *name)
{
    if(!musicstream) musicstream = openzipfile(name, "rb");
    if(musicstream)
    {
        if(!musicrw) musicrw = musicstream->rwops();
        if(!musicrw) DELETEP(musicstream);
    }
    if(musicrw) music = Mix_LoadMUS_RW(musicrw);
    else music = Mix_LoadMUS(findfile(name, "rb")); 
    if(!music)
    {
        if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }
        DELETEP(musicstream);
    }
    return music;
}


extern char* path(char *s) ;
void startmusic(char *name, char *cmd)
{
    printf("\nWe are asked to play: %s\n", name) ;

    if(nosound) 
    {
        return;
    }
    stopmusic();
    printf("\nHELLO \n", name) ;
    if(soundvol && musicvol && *name)
    {
        //defformatstring(file)("packages/%s", name);
        defformatstring(file)("%s", name);
        path(file);
        if(loadmusic(file))
        {
            musicvol = 55 ;
            printf("\nMUSIC FILE LOADED %s, musicvol = %d \n", name, (int)(musicvol)) ;
            DELETEA(musicfile);
            DELETEA(musicdonecmd);
            musicfile = newstring(file);
            // if(cmd[0]) musicdonecmd = newstring(cmd);
            // Mix_PlayMusic(music, cmd[0] ? 0 : -1);
            //            Mix_PlayMusic(music, 0 );

            Mix_VolumeMusic(55);
            if(Mix_PlayMusic(music, 1)==-1) 
            {
                printf("Mix_PlayMusic: %s\n", Mix_GetError());
                // well, there's no music, but most games don't break without music...
            }
        Mix_PlayMusic(music, musicdonecmd ? 0 : -1);
            //Mix_VolumeMusic((musicvol*MAXVOL)/255);
            // intret(1); FIXME: THIS IS THE RETURN FOR A CONSOLE COMMAND
        }
        else
        {
            // FIXME: replace  - conoutf(CON_ERROR, "could not play music: %s", file);
            printf("\n\nFUCK NOMUSIC could not play music: %s", file);
            // intret(0); FIXME: THIS IS THE RETURN FOR A CONSOLE COMMAND
        }
    }
}

// FIXME: how do my console commands work? :) 
// COMMANDN(music, startmusic, "ss");

hashtable<const char *, soundsample> samples;
vector<soundslot> gamesounds, mapsounds;

void updatesounds() ;

void startsound()
{
    //newchannel(int n, soundslot *slot, const vec *loc = NULL, /*extentity *ent = NULL,*/ int radius = 0)
    initsound() ;
    resetchannels();
    newchannel(1, &gamesounds[0]) ;
    updatesounds() ;
}
int findsound(const char *name, int vol, vector<soundslot> &sounds)
{
    loopv(sounds)
    {
        if(!strcmp(sounds[i].sample->name, name) && (!vol || sounds[i].volume==vol)) return i;
    }
    return -1;
}

int addsound(const char *name, int vol, int maxuses, vector<soundslot> &sounds)
{
    soundsample *s = samples.addOrFind(name);
    if(!s)
    {
        char *n = newstring(name);
        s = &samples[n];
        s->name = n;
        s->chunk = NULL;
    }
    soundslot *oldsounds = sounds.getbuf();
    int oldlen = sounds.length();
    soundslot &slot = sounds.add();
    // sounds.add() may relocate slot pointers
    if(sounds.getbuf() != oldsounds) loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(chan.inuse && chan.slot >= oldsounds && chan.slot < &oldsounds[oldlen])
            chan.slot = &sounds[chan.slot - oldsounds];
    }
    slot.sample = s;
    slot.volume = vol ? vol : 100;
    slot.maxuses = maxuses;
    return oldlen;
}

int registersound(char *name, int *vol) 
{
    // intret(
    return addsound(name, *vol, 0, gamesounds) ;
      //   ); 
}

// COMMAND(registersound, "si");

void mapsound(char *name, int *vol, int *maxuses) 
{ 
    // intret(
    addsound(name, *vol, *maxuses < 0 ? 0 : max(1, *maxuses), mapsounds) ;
    // ); 
}
// COMMAND(mapsound, "sii");

void resetchannels()
{
    loopv(channels) if(channels[i].inuse) freechannel(i);
    channels.shrink(0);
}

void clear_sound()
{
    closemumble();
    if(nosound) return;
    stopmusic();
    Mix_CloseAudio();
    resetchannels();
    gamesounds.setsize(0);
    mapsounds.setsize(0);
    samples.clear();
}

void clearmapsounds()
{
    loopv(channels) if(channels[i].inuse && channels[i].ent)
    {
        Mix_HaltChannel(i);
        freechannel(i);
    }
    mapsounds.setsize(0);
}

/*
void stopmapsound(extentity *e)
{
    loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(chan.inuse && chan.ent == e)
        {
            Mix_HaltChannel(i);
            freechannel(i);
        }
    }
}
*/


// FIXME: this will be useful once I have my own entities. 
void checkmapsounds()
{
/*
    const vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        extentity &e = *ents[i];
        if(e.type!=ET_SOUND) continue;
        if(camera1->o.dist(e.o) < e.attr2)
        {
            if(!e.visible) playsound(e.attr1, NULL, &e, -1);
        }
        else if(e.visible) stopmapsound(&e);
    }
*/
}

// VAR(stereo, 0, 1, 1);
// bool stereo = true ;
bool stereo = false ;

// VARP(maxsoundradius, 0, 340, 10000);
int maxsoundradius = 50000 ; // FIXME :)

bool updatechannel(soundchannel &chan)
{
    if(!chan.slot) return false;
    int vol = soundvol, pan = 255/2;
    if(chan.hasloc())
    {
        vec v;
        float dist = 0.0f ; // FIXME chan.loc.dist(camera1->o, v);
        int rad = maxsoundradius ;
        if(chan.ent)
        {
        /*
            rad = chan.ent->attr2;
            if(chan.ent->attr3)
            {
                rad -= chan.ent->attr3;
                dist -= chan.ent->attr3;
            }
            */
        }
        else if(chan.radius > 0) rad = maxsoundradius ? min(maxsoundradius, chan.radius) : chan.radius;
        if(rad > 0) vol -= int(clamp(dist/rad, 0.0f, 1.0f)*soundvol); // simple mono distance attenuation
        if(stereo && (v.x != 0 || v.y != 0) && dist>0)
        {
            // v.rotate_around_z(-camera1->yaw*RAD);
            pan = int(255.9f*(0.5f - 0.5f*v.x/v.magnitude2())); // range is from 0 (left) to 255 (right)
        }
    }
    vol = (vol*MAXVOL*chan.slot->volume)/255/255;
    vol = min(vol, MAXVOL);
    if(vol == chan.volume && pan == chan.pan) return false;
    chan.volume = vol;
    chan.pan = pan;
    chan.dirty = true;
    return true;
}  

void updatesounds()
{
    // updatemumble();
    if(nosound) return;
    checkmapsounds();
    int dirty = 0;
    loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(chan.inuse && chan.hasloc() && updatechannel(chan)) dirty++;
    }
    if(dirty)
    {
        SDL_LockAudio(); // workaround for race conditions inside Mix_SetPanning
        loopv(channels) 
        {
            soundchannel &chan = channels[i];
            if(chan.inuse && chan.dirty) syncchannel(chan);
        }
        SDL_UnlockAudio();
    }
    if(music)
    {
        if(!Mix_PlayingMusic()) musicdone();
        else if(Mix_PausedMusic()) Mix_ResumeMusic();
    }
}

// VARP(maxsoundsatonce, 0, 5, 100);
int maxsoundsatonce = 2 ;

// VAR(dbgsound, 0, 0, 1);
bool dbgsound = true ;

static Mix_Chunk *loadwav(const char *name)
{
    Mix_Chunk *c = NULL;
    stream *z = openzipfile(name, "rb");
    if(z)
    {
        SDL_RWops *rw = z->rwops();
        if(rw) 
        {
            c = Mix_LoadWAV_RW(rw, 0);
            SDL_FreeRW(rw);
        }
        delete z;
    }
    if(!c) c = Mix_LoadWAV(findfile(name, "rb"));
    return c;
}

extern unsigned int millis ;

// #error you did not compile me
// /*extentity *ent,*/ 

int playsound( vec* loc, int n, int loops, int fade, int chanid, int radius, int expire)
{   
    // nosound = false ;  // FIXME: put this in a control
    if(nosound || !soundvol) return -1;


    // vector<soundslot> &sounds = ent ? mapsounds : gamesounds;
    vector<soundslot> &sounds = gamesounds;
    if(!sounds.inrange(n)) 
    { 
        // FIXME conoutf(CON_WARN, "unregistered sound: %d", n); return -1; 
        printf("\n\nUNREGISTERED: unregistered sound: %d", n); return -1; 
    }
    soundslot &slot = sounds[n];

    if(loc && (maxsoundradius || radius > 0))
    {
        // cull sounds that are unlikely to be heard
        int rad = radius > 0 ? (maxsoundradius ? min(maxsoundradius, radius) : radius) : maxsoundradius;
        // if(camera1->o.dist(*loc) > 1.5f*rad)
        // FIXME if(camera1->o.dist(*loc) > 1.5f*rad)
        if (0)
        {
            bool inrange = channels.inrange(chanid) ;
            printf("\nINRANGE: %s    bad chanid=%d\n", (!inrange)?("false"):("true"), chanid) ;
            if(channels.inrange(chanid) && channels[chanid].inuse && channels[chanid].slot == &slot)
            {
                Mix_HaltChannel(chanid);
                freechannel(chanid);
            }
            return -1;    
        }
    }
    printf("\n\nHAHHHHHHHHHHHHHHHHH\n\n") ;

    if(chanid < 0)
    {
        if(slot.maxuses)
        {
            int uses = 0;
            loopv(channels) if(channels[i].inuse && channels[i].slot == &slot && ++uses >= slot.maxuses) return -1;
        }

        // avoid bursts of sounds with heavy packetloss and in sp
        static int soundsatonce = 0, lastsoundmillis = 0;
        if(millis == lastsoundmillis) soundsatonce++; else soundsatonce = 1;
        lastsoundmillis = millis;
        if(maxsoundsatonce && soundsatonce > maxsoundsatonce) return -1;
    }

    if(!slot.sample->chunk)
    {
        if(!slot.sample->name[0]) return -1;

        const char *exts[] = { ".mp3", ".wav", ".ogg" };
        string buf;
        loopi(sizeof(exts)/sizeof(exts[0]))
        {
            // formatstring(buf)("packages/sounds/%s%s", slot.sample->name, exts[i]);
            formatstring(buf)("%s%s", slot.sample->name, exts[i]);
            path(buf);
            slot.sample->chunk = loadwav(buf);
            if(slot.sample->chunk) break;
        }

        if(!slot.sample->chunk) 
        { 
            // FIXME conoutf(CON_ERROR, "failed to load sample: %s", buf); return -1; 
        }
    }
    printf("\n\nYESSSSSSSSSSSSSSS\n\n") ;

    if(channels.inrange(chanid))
    {
        soundchannel &chan = channels[chanid];
        if(chan.inuse && chan.slot == &slot) 
        {
            if(loc) chan.loc = *loc;
            else if(chan.hasloc()) chan.clearloc();
            return chanid;
        }
    }
    if(fade < 0) return -1;
           
    if(dbgsound) 
    {
        // FIXME - conoutf("sound: %s", slot.sample->name);
        printf("sound: %s", slot.sample->name);
    }
 

    chanid = -1;
    loopv(channels) if(!channels[i].inuse) { chanid = i; break; }
    if(chanid < 0 && channels.length() < maxchannels) chanid = channels.length();
    if(chanid < 0) loopv(channels) if(!channels[i].volume) { chanid = i; break; }
    if(chanid < 0) return -1;

    printf("\n\nNEW CHANNEL \n\n") ;

    SDL_LockAudio(); // must lock here to prevent freechannel/Mix_SetPanning race conditions
    if(channels.inrange(chanid) && channels[chanid].inuse)
    {
        Mix_HaltChannel(chanid);
        freechannel(chanid);
    }
    soundchannel &chan = newchannel(chanid, &slot, loc, /*ent,*/ radius);
    updatechannel(chan);
    int playing = -1;
    if(fade) 
    {
        Mix_Volume(chanid, chan.volume);
        playing = expire >= 0 ? Mix_FadeInChannelTimed(chanid, slot.sample->chunk, loops, fade, expire) : Mix_FadeInChannel(chanid, slot.sample->chunk, loops, fade);
    }
    else playing = expire >= 0 ? Mix_PlayChannelTimed(chanid, slot.sample->chunk, loops, expire) : Mix_PlayChannel(chanid, slot.sample->chunk, loops);
    if(playing >= 0) syncchannel(chan); 
    else freechannel(chanid);
    SDL_UnlockAudio();
    return playing;
}



int justplay(int id)
{
    static char hello[] = "../data/cranberry-radio_edit.mp3" ;
    startmusic(hello, hello) ;
    return 0 ;
}


void stopsounds()
{
    loopv(channels) if(channels[i].inuse)
    {
        Mix_HaltChannel(i);
        freechannel(i);
    }
}

bool stopsound(int n, int chanid, int fade)
{
    if(!channels.inrange(chanid) || !channels[chanid].inuse || !gamesounds.inrange(n) || channels[chanid].slot != &gamesounds[n]) return false;
    if(dbgsound) 
    {
        // FIXME - conoutf("stopsound: %s", channels[chanid].slot->sample->name);
    }
    if(!fade || !Mix_FadeOutChannel(chanid, fade))
    {
        Mix_HaltChannel(chanid);
        freechannel(chanid);
    }
    return true;
}

//int playsoundname(const char *s, const vec *loc, int vol, int loops, int fade, int chanid, int radius, int expire) 
int playsoundname(const char *s, vec *loc, int vol, int loops, int fade, int chanid, int radius, int expire) 
{ 
    if(!vol) vol = 100;
    int id = findsound(s, vol, gamesounds);
    if(id < 0) id = addsound(s, vol, 0, gamesounds);
    return playsound(loc, id, /*NULL,*/ loops, fade, chanid, radius, expire);
}

// void sound(int *n) { playsound(*n); }
// COMMAND(sound, "i");
void soundoff() 
{
    stopmusic() ;
    stopsounds() ;
    nosound = true ;
}
void soundon()
{
    nosound = false ;
}
void resetsound()
{
    const SDL_version *v = Mix_Linked_Version();
    if(SDL_VERSIONNUM(v->major, v->minor, v->patch) <= SDL_VERSIONNUM(1, 2, 8))
    {
        // FIXME - conoutf(CON_ERROR, "Sound reset not available in-game due to SDL_mixer-1.2.8 bug. Please restart for changes to take effect.");
        return;
    }
    // clearchanges(CHANGE_SOUND);
    if(!nosound) 
    {
        enumerate(samples, soundsample, s, { Mix_FreeChunk(s.chunk); s.chunk = NULL; });
        if(music)
        {
            Mix_HaltMusic();
            Mix_FreeMusic(music);
        }
        if(musicstream) musicstream->seek(0, SEEK_SET);
        Mix_CloseAudio();
    }
    initsound();
    resetchannels();
    if(nosound)
    {
        DELETEA(musicfile);
        DELETEA(musicdonecmd);
        music = NULL;
        gamesounds.setsize(0);
        mapsounds.setsize(0);
        samples.clear();
        return;
    }
    if(music && loadmusic(musicfile))
    {
        Mix_PlayMusic(music, musicdonecmd ? 0 : -1);
        Mix_PlayMusic(music, musicdonecmd ? 0 : -1);
        Mix_VolumeMusic((musicvol*MAXVOL)/255);
    }
    else
    {
        DELETEA(musicfile);
        DELETEA(musicdonecmd);
    }
}

// COMMAND(resetsound, "");

#ifdef WIN32

#include <wchar.h>

#else

#include <unistd.h>

#ifdef _POSIX_SHARED_MEMORY_OBJECTS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <wchar.h>
#endif

#endif

#if defined(WIN32) || defined(_POSIX_SHARED_MEMORY_OBJECTS)
struct MumbleInfo
{
    int version, timestamp;
    vec pos, front, top;
    wchar_t name[256];
};
#endif

#ifdef WIN32
static HANDLE mumblelink = NULL;
static MumbleInfo *mumbleinfo = NULL;
#define VALID_MUMBLELINK (mumblelink && mumbleinfo)
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
static int mumblelink = -1;
static MumbleInfo *mumbleinfo = (MumbleInfo *)-1; 
#define VALID_MUMBLELINK (mumblelink >= 0 && mumbleinfo != (MumbleInfo *)-1)
#endif

#ifdef VALID_MUMBLELINK
//VARFP(mumble, 0, 1, 1, { if(mumble) initmumble(); else closemumble(); });
#else
//VARFP(mumble, 0, 0, 1, { if(mumble) initmumble(); else closemumble(); });
#endif

/*
void initmumble()
{
    if(!mumble) return;
#ifdef VALID_MUMBLELINK
    if(VALID_MUMBLELINK) return;

    #ifdef WIN32
        mumblelink = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "MumbleLink");
        if(mumblelink)
        {
            mumbleinfo = (MumbleInfo *)MapViewOfFile(mumblelink, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MumbleInfo));
            if(mumbleinfo) wcsncpy(mumbleinfo->name, L"Sauerbraten", 256);
        }
    #elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
        defformatstring(shmname)("/MumbleLink.%d", getuid());
        mumblelink = shm_open(shmname, O_RDWR, 0);
        if(mumblelink >= 0)
        {
            mumbleinfo = (MumbleInfo *)mmap(NULL, sizeof(MumbleInfo), PROT_READ|PROT_WRITE, MAP_SHARED, mumblelink, 0);
            if(mumbleinfo != (MumbleInfo *)-1) wcsncpy(mumbleinfo->name, L"Sauerbraten", 256);
        }
    #endif
    if(!VALID_MUMBLELINK) closemumble();
#else
    conoutf(CON_ERROR, "Mumble positional audio is not available on this platform.");
#endif
}
*/

void closemumble()
{
#ifdef WIN32
    if(mumbleinfo) { UnmapViewOfFile(mumbleinfo); mumbleinfo = NULL; }
    if(mumblelink) { CloseHandle(mumblelink); mumblelink = NULL; }
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
    if(mumbleinfo != (MumbleInfo *)-1) { munmap(mumbleinfo, sizeof(MumbleInfo)); mumbleinfo = (MumbleInfo *)-1; } 
    if(mumblelink >= 0) { close(mumblelink); mumblelink = -1; }
#endif
}

static inline vec mumblevec(const vec &v, bool pos = false)
{
    // change from X left, Z up, Y forward to X right, Y up, Z forward
    // 8 cube units = 1 meter
    vec m(-v.x, v.z, v.y);
    if(pos) m.div(8);
    return m;
}

/*
void updatemumble()
{
#ifdef VALID_MUMBLELINK
    if(!VALID_MUMBLELINK) return;

    static int timestamp = 0;

    mumbleinfo->version = 1;
    mumbleinfo->timestamp = ++timestamp;

    mumbleinfo->pos = mumblevec(player->o, true);
    mumbleinfo->front = mumblevec(vec(RAD*player->yaw, RAD*player->pitch));
    mumbleinfo->top = mumblevec(vec(RAD*player->yaw, RAD*(player->pitch+90)));
#endif
}
*/

