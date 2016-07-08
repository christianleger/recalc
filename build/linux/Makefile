################################################################################
################################################################################
################################################################################
#
#   File:     Recalc's Makefile
#
#
#   Todo: haha stop using make for the love of baby Stephen Hawking. Yes, 
#   in just a few days of work python would make a superior build system 
#   that: 
#        - specifies dependencies 
#        - checks which dependencies are more recent than build targets
#        - collects all the paths, tools and commands required. 
#        - executes all the required commands for the desired build target. 
#
################################################################################
################################################################################
################################################################################

SRC=../code

# default is likely linux. 
ifeq ($(PLATFORM),)
	PLATFORM=linux
endif

ifeq ($(OS),Windows_NT)
$(info Windows platform detected. )
PLATFORM=windows
EXE=.exe
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
    CXX_FLAGS= -msse -msse2 -msse3 -fpermissive
    ifeq ($(DEBUG),1)
        # SWITCH THIS ON To see preprocessor output!!!! :-D
        #DEBUG_FLAGS=-g -DDEBUG -E  USE THIS TO 
        DEBUG_FLAGS=-g -DDEBUG
        OPT_FLAGS=-O0
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
    INCLUDES= \
        -I/usr/include/SDL \
        -I/usr/include/nvidia-current/ \
        -I/usr/include/freetype2 \
        -I$(SRC)/include \
        -I$(SRC)/include/GL 

    LIB_LOCATIONS= -L../bin

    LIBS= -lrt \
        -lm \
        -lz \
        -lGL \
        -lGLU \
        -lSDL \
        -lSDL_image \
        -lSDL_mixer \
        -lfreetype \
#        -lNewton   maybe one day we'll use this; not for now. 

endif
################################################################################
################################################################################
#### WINDOWS PLATFORM
################################################################################
ifeq ($(PLATFORM),windows)
    #$(warning NO NO NO NO NO NO NO)
#####################           PROGRAMS             ###########################
#    CXX=/c/MinGW/bin/g++
    CXX=g++
#    CXX_FLAGS= -mwindows
    CXX_FLAGS= -msse -msse2 -msse3 -fpermissive
    ifeq ($(DEBUG),1)
        DEBUG_FLAGS=-g
        OPT_FLAGS=-O0
    else
        OPT_FLAGS=-O3
    endif
#####################       CONTROL PARAMETERS       ###########################

    OUT_FLAG=-o
    OBJ_FLAG=-c
    OBJ_EXT=obj

#####################            LOCATIONS           ###########################
    INCLUDES= -I/usr/include/SDL \
        -I/usr/include/GL \
        -I$(SRC)/include \
        -I$(SRC)/include/GL \
        -I$(SRC)/include/SDL

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
		-static-libgcc \
		-static-libstdc++ \
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
        -lzlib1
#		\
#        -lnewton




#        -lSDLmain \

endif
################################################################################

################################################################################
################################################################################
################################################################################

OBJS =   console.$(OBJ_EXT)  \
    script.$(OBJ_EXT) \
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
    shader.$(OBJ_EXT)   \
    stream.$(OBJ_EXT)   \
    texture.$(OBJ_EXT)   \
    world.$(OBJ_EXT)      \
    zip.$(OBJ_EXT)

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
	@rm $(OBJS) ../bin/recalc$(EXE)
	echo done. 

################################################################################
again: clean all
#	include clean
#	include all

################################################################################
#####################         THE EXECUTABLE         ###########################
################################################################################
recalc: $(SRC)/src/recalc.h $(SRC)/src/main.cpp $(OBJS)
	\
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) \
    $(INCLUDES) \
    $(OBJS) $(SRC)/src/main.cpp \
    $(OUT_FLAG) ../bin/recalc \
    $(LIB_LOCATIONS) \
    $(LIBS) 
    
#	$(info )


################################################################################
#####################          THE OBJECTS           ###########################
################################################################################

################################################################################
input.$(OBJ_EXT): $(SRC)/src/input.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/input.cpp $(OUT_FLAG) input.$(OBJ_EXT)
	$(info )

################################################################################
math.$(OBJ_EXT): $(SRC)/src/math.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIBS) $(SRC)/src/math.cpp $(OUT_FLAG) math.$(OBJ_EXT)
	$(info )

################################################################################
render.$(OBJ_EXT): $(SRC)/src/render.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/render.cpp $(OUT_FLAG) render.$(OBJ_EXT)
	$(info )

################################################################################
sound.$(OBJ_EXT): $(SRC)/src/sound.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/sound.cpp $(OUT_FLAG) sound.$(OBJ_EXT)
	$(info )

################################################################################
geometry.$(OBJ_EXT): $(SRC)/src/geometry.cpp $(SRC)/src/geometry.h
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/geometry.cpp $(OUT_FLAG) geometry.$(OBJ_EXT)
	$(info )

################################################################################
platform.$(OBJ_EXT): $(SRC)/src/platform.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/platform.cpp $(OUT_FLAG) platform.$(OBJ_EXT)
	$(info )

################################################################################
physics.$(OBJ_EXT): $(SRC)/src/physics.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/physics.cpp $(OUT_FLAG) physics.$(OBJ_EXT)
	$(info )

################################################################################
recalc.$(OBJ_EXT): $(SRC)/src/recalc.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
	$(LIB_LOCATIONS) $(LIBS) $(SRC)/src/recalc.cpp $(OUT_FLAG) recalc.$(OBJ_EXT)
	$(info )

################################################################################
test.$(OBJ_EXT): $(SRC)/src/test.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/test.cpp $(OUT_FLAG) test.$(OBJ_EXT)
	$(info )

################################################################################
text.$(OBJ_EXT): $(SRC)/src/text.cpp $(SRC)/src/text.h
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/text.cpp $(OUT_FLAG) text.$(OBJ_EXT)
	$(info )

################################################################################
console.$(OBJ_EXT): $(SRC)/src/console.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/console.cpp $(OUT_FLAG) console.$(OBJ_EXT)
	$(info )

################################################################################
script.$(OBJ_EXT): $(SRC)/src/script.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/script.cpp $(OUT_FLAG) script.$(OBJ_EXT)
	$(info )

################################################################################
menu.$(OBJ_EXT): $(SRC)/src/menu.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/menu.cpp $(OUT_FLAG) menu.$(OBJ_EXT)
	$(info )

################################################################################
shader.$(OBJ_EXT): $(SRC)/src/shader.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/shader.cpp $(OUT_FLAG) shader.$(OBJ_EXT)
	$(info )

################################################################################
stream.$(OBJ_EXT): $(SRC)/src/stream.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/stream.cpp $(OUT_FLAG) stream.$(OBJ_EXT)
	$(info )

################################################################################
texture.$(OBJ_EXT): $(SRC)/src/texture.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/texture.cpp $(OUT_FLAG) texture.$(OBJ_EXT)
	$(info )

################################################################################
world.$(OBJ_EXT): $(SRC)/src/world.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/world.cpp $(OUT_FLAG) world.$(OBJ_EXT)
	$(info )

################################################################################
zip.$(OBJ_EXT): $(SRC)/src/zip.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) $(SRC)/src/zip.cpp $(OUT_FLAG) zip.$(OBJ_EXT)
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

















