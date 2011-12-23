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


################################################################################
ifeq ($(PLATFORM),linux)
    
#########
#PROGRAMS             
#########

    CXX=g++
    CXX_FLAGS=
    ifeq ($(DEBUG),1)
        DEBUG_FLAGS=-g
    else
        OPT_FLAGS=-O3
    endif
###################
#CONTROL PARAMETERS       
###################

    OUT_FLAG=-o
    OBJ_FLAG=-c
    OBJ_EXT=o

#####################            LOCATIONS           ###########################
    INCLUDES=\
        -I/usr/include/SDL \
        -I/usr/include/nvidia-current/ \
        -I/usr/include/freetype2
#        -I/usr/include/GL 

#	LIB_LOCATIONS=
		
    LIBS=\
        -lrt \
        -lm \
        -lGL \
        -lGLU \
        -lSDL \
        -lSDL_image \
        -lSDL_mixer \
        -lfreetype

endif
################################################################################
ifeq ($(PLATFORM),windows)
    
#####################           PROGRAMS             ###########################
    CXX=g++
    CXX_FLAGS= -mwindows
    ifeq ($(DEBUG),1)
        DEBUG_FLAGS=-g
    else
        OPT_FLAGS=-O3
    endif
#####################       CONTROL PARAMETERS       ###########################

    OUT_FLAG=-o
    OBJ_FLAG=-c
    OBJ_EXT=o

#####################            LOCATIONS           ###########################
    INCLUDES=\
        -I/usr/include/SDL \
        -I/usr/include/GL 
		
	LIB_LOCATIONS=\
        -L/usr/lib/nvidia-current
#        -L../bin \
#        -Lc:/Windows/system32 \
#        -Lc:/MinGW/bin
		

    LIBS=\
        -lm \
        -lopengl32 \
        -lglu32 \
        -lmingw32 \
        -lSDLmain \
        -lSDL \
        -lSDL_image \
        -lSDL_mixer

endif
################################################################################

#suggestion: rename '../src/' to $(SRC_DIR)



################################################################################
################################################################################
################################################################################

OBJS=\
    recalc.$(OBJ_EXT) \
    render.$(OBJ_EXT) \
    menu.$(OBJ_EXT) \
    input.$(OBJ_EXT) \
    math.$(OBJ_EXT) \
    geometry.$(OBJ_EXT) \
    platform.$(OBJ_EXT) \
    physics.$(OBJ_EXT) \
    text.$(OBJ_EXT) \
    console.$(OBJ_EXT)

#HEADERS=\
#    recalc.h \
#    console.h \

#console.$(OBJ_EXT)



################################################################################
#####################         THE TARGETS            ###########################
################################################################################


################################################################################
all: recalc


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
recalc: ../src/main.cpp $(OBJS)
	$(info )
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) $(LIB_LOCATIONS) $(LIBS) \
    $(OBJS) ../src/main.cpp $(OUT_FLAG) ../bin/recalc
	$(info )


################################################################################
#####################          THE OBJECTS           ###########################
################################################################################
################################################################################
recalc.$(OBJ_EXT): ../src/recalc.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
	$(LIB_LOCATIONS) $(LIBS) ../src/recalc.cpp $(OUT_FLAG) recalc.$(OBJ_EXT)
	$(info )

################################################################################
render.$(OBJ_EXT): ../src/render.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/render.cpp $(OUT_FLAG) render.$(OBJ_EXT)
	$(info )

################################################################################
math.$(OBJ_EXT): ../src/math.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIBS) ../src/math.cpp $(OUT_FLAG) math.$(OBJ_EXT)
	$(info )

################################################################################
input.$(OBJ_EXT): ../src/input.cpp
	$(info )
	$(CXX) $(CXX_FLAGS) $(OBJ_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) $(INCLUDES) \
    $(LIB_LOCATIONS) $(LIBS) ../src/input.cpp $(OUT_FLAG) input.$(OBJ_EXT)
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
text.$(OBJ_EXT): ../src/text.cpp
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
#####################             FIXME              ###########################
################################################################################
# : 
# a proper build system will include ways to: 
# 
#       - compile any combination of modules as a library
# 
#       - provide the ability to observe the preprocessor output created 
#         when this program is built
# 
#       - provide assembly listings if desired
# 
#       - provide options to include debug symbols or not into the program
# 
# 
# 
# 

















