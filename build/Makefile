################################################################################
################################################################################
################################################################################
#
#   File:     Recalc's Makefile
#
#
#
#
################################################################################
################################################################################
################################################################################

# default is likely linux. 
ifeq ($(PLATFORM),)
	PLATFORM=linux
endif

################################################################################

ifeq ($(HELLO),1)
    $(warning HELLO MY FRIEND)
    $(quit)
endif

################################################################################
################################################################################
#### LINUX PLATFORM
################################################################################
ifeq ($(PLATFORM),linux)
    
#########
#PROGRAMS             
#########

    CXX=g++
    CXX_FLAGS= -msse -msse2 -msse3
    ifeq ($(DEBUG),1)
        DEBUG_FLAGS=-g -DDEBUG
    else
        OPT_FLAGS=-O3
    endif
###################
#CONTROL PARAMETERS       
###################

    OUT_FLAG=-o
    OBJ_FLAG=-c
    OBJ_EXT=o

#####################   LOCATIONS 
    INCLUDES=\
        -I/usr/include/SDL \
        -I/usr/include/nvidia-current/ \
        -I/usr/include/freetype2 \
        -I../include

#        -I/usr/include/GL 

    LIB_LOCATIONS=\
        -L../bin

    LIBS=\
        -lrt \
        -lm \
        -lz \
        -lGL \
        -lGLU \
        -lSDL \
        -lSDL_image \
        -lSDL_mixer \
        -lfreetype \
        -lNewton

endif
################################################################################
################################################################################
#### WINDOWS PLATFORM
################################################################################
ifeq ($(PLATFORM),windows)
    $(warning NO NO NO NO NO NO NO)
#####################           PROGRAMS             ###########################
    CXX=g++
#    CXX_FLAGS= -mwindows
    ifeq ($(DEBUG),1)
        DEBUG_FLAGS=-g
    else
        OPT_FLAGS=-O3
    endif
#####################       CONTROL PARAMETERS       ###########################

    OUT_FLAG=-o
    OBJ_FLAG=-c
    OBJ_EXT=obj

#####################            LOCATIONS           ###########################
    INCLUDES=\
        -I/usr/include/SDL \
        -I/usr/include/GL \
        -I../include \
        -I../include/GL \
        -I../include/SDL

    LIB_LOCATIONS=\
        -L../bin \
        -L/usr/local/lib
#        -L/usr/lib/nvidia-current
#        -L/c/WINDOWS/system32/ \
#        -L../bin \
#        -Lc:/Windows/system32 \
#        -Lc:/MinGW/bin

#-lmingw32  -lSDL -mwindows
#-lSDL_image -lglu32 -lopengl32

    LIBS=\
        -lmingw32 \
        -lSDLmain \
        -lSDL \
        -mwindows \
        -lglew32 \
        -lglew32mx \
        -lopengl32 \
        -lglu32 \
        -lSDL_image \
        -lSDL_mixer \
        -llibfreetype-6 \
        -lm \
        -lzlib1 \
        -lnewton




#        -lSDLmain \

endif
################################################################################

################################################################################
################################################################################
################################################################################

OBJS=\
    console.$(OBJ_EXT)  \
    geometry.$(OBJ_EXT) \
    input.$(OBJ_EXT)    \
    math.$(OBJ_EXT)     \
    menu.$(OBJ_EXT)     \
    physics.$(OBJ_EXT)  \
    platform.$(OBJ_EXT) \
    recalc.$(OBJ_EXT)   \
    render.$(OBJ_EXT)   \
    test.$(OBJ_EXT)     \
    text.$(OBJ_EXT)     \
    sound.$(OBJ_EXT)    \
    zip.$(OBJ_EXT)      \
    stream.$(OBJ_EXT) 

#HEADERS=\
#    recalc.h \
#    console.h \

#console.$(OBJ_EXT)



################################################################################
#####################         THE TARGETS            ###########################
################################################################################


################################################################################
all: recalc

# normal: 
# CXX_FLAGS=""

################################################################################
clean:
	echo removing all compiled files... 
	@rm $(OBJS) ../bin/recalc 
	echo done. 

################################################################################
again: clean all
#	include clean
#	include all

################################################################################
#####################         THE EXECUTABLE         ###########################
################################################################################
recalc: ../src/recalc.h ../src/main.cpp $(OBJS)
	$(info )
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(OBJS) ../src/main.cpp $(OUT_FLAG) ../bin/recalc \
    $(LIB_LOCATIONS) $(LIBS) 
    
	$(info )


################################################################################
#####################          THE OBJECTS           ###########################
################################################################################

################################################################################
input.$(OBJ_EXT): ../src/input.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/input.cpp $(OUT_FLAG) input.$(OBJ_EXT)
	$(info )

################################################################################
math.$(OBJ_EXT): ../src/math.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIBS) ../src/math.cpp $(OUT_FLAG) math.$(OBJ_EXT)
	$(info )

################################################################################
render.$(OBJ_EXT): ../src/render.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/render.cpp $(OUT_FLAG) render.$(OBJ_EXT)
	$(info )

################################################################################
sound.$(OBJ_EXT): ../src/sound.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/sound.cpp $(OUT_FLAG) sound.$(OBJ_EXT)
	$(info )

################################################################################
geometry.$(OBJ_EXT): ../src/geometry.cpp ../src/geometry.h
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/geometry.cpp $(OUT_FLAG) geometry.$(OBJ_EXT)
	$(info )

################################################################################
platform.$(OBJ_EXT): ../src/platform.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/platform.cpp $(OUT_FLAG) platform.$(OBJ_EXT)
	$(info )

################################################################################
physics.$(OBJ_EXT): ../src/physics.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/physics.cpp $(OUT_FLAG) physics.$(OBJ_EXT)
	$(info )

################################################################################
recalc.$(OBJ_EXT): ../src/recalc.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
	$(LIB_LOCATIONS) $(LIBS) ../src/recalc.cpp $(OUT_FLAG) recalc.$(OBJ_EXT)
	$(info )

################################################################################
test.$(OBJ_EXT): ../src/test.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/test.cpp $(OUT_FLAG) test.$(OBJ_EXT)
	$(info )

################################################################################
text.$(OBJ_EXT): ../src/text.cpp ../src/text.h
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/text.cpp $(OUT_FLAG) text.$(OBJ_EXT)
	$(info )


################################################################################
console.$(OBJ_EXT): ../src/console.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/console.cpp $(OUT_FLAG) console.$(OBJ_EXT)
	$(info )


################################################################################
menu.$(OBJ_EXT): ../src/menu.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/menu.cpp $(OUT_FLAG) menu.$(OBJ_EXT)
	$(info )

################################################################################
stream.$(OBJ_EXT): ../src/stream.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/stream.cpp $(OUT_FLAG) stream.$(OBJ_EXT)
	$(info )

################################################################################
zip.$(OBJ_EXT): ../src/zip.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/zip.cpp $(OUT_FLAG) zip.$(OBJ_EXT)
	$(info )

    
################################################################################
#####################             FIXME              ###########################
################################################################################
# : 
# a proper build system will include ways to: 
# 
#       - compile any combination of modules as a library
# 
#       - provide the ability to observe the preprocessor output created 
#         when object files are built. 
# 
#       - provide assembly listings if desired
# 
#       - provide options to include debug symbols or not into the program
# 
# 
# 
# 

















