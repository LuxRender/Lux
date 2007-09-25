ARCH = $(shell uname)
LEX=flex
YACC=bison -d -v -t
LEXLIB = -lfl
DLLLIB = -ldl
ifeq ($(ARCH),Darwin)
  DLLLIB =
endif
ifeq ($(ARCH),OpenBSD)
  DLLLIB =
endif

EXRINCLUDE=-I/usr/include/OpenEXR
EXRLIBDIR=-L/usr/lib -L/usr/lib64
#EXRLIBS=$(EXRLIBDIR) -Bstatic -lIex -lIlmImf -lImath -lHalf -Bdynamic -lz
EXRLIBS=$(EXRLIBDIR) -lIex -lIlmImf -lImath -lHalf -Bdynamic -lz 
ifeq ($(ARCH),Linux)
  EXRLIBS += -lpthread
endif


CC=gcc
CXX=g++
#CC=icc
#CXX=icc
LD=$(CXX) $(OPT)
OPT=-O1 -g
# OPT=-O2 -msse -mfpmath=sse
INCLUDE=-I. -Icore $(EXRINCLUDE)
WARN=-Wall
CWD=$(shell pwd)
CXXFLAGS=$(OPT) $(INCLUDE) $(WARN) -fPIC
CCFLAGS=$(CXXFLAGS)
LIBS=$(LEXLIB) $(DLLLIB) $(EXRLIBDIR) $(EXRLIBS) -lm

SHARED_LDFLAGS = -shared
LRT_LDFLAGS=-rdynamic $(OPT)
#PBRTPRELINK=-Wl,--export-dynamic -Wl,-whole-archive
#PBRTPOSTLINK=-Wl,-no-whole-archive

ifeq ($(ARCH), Darwin)
  OS_VERSION = $(shell uname -r)
  SHARED_LDFLAGS = -flat_namespace -undefined suppress -bundle -noprebind
  LRT_LDFLAGS=$(OPT) # -L/sw/lib
  INCLUDE += -I/sw/include
  WARN += -Wno-long-double
endif

#ACCELERATORS = grid kdtree
#CAMERAS      = environment orthographic perspective
CORE         = api camera color dynload exrio film geometry light material mc \
               paramset parser primitive reflection sampling scene shape \
               texture timer transform transport util volume luxparse luxlex \
               cone cylinder disk heightfield hyperboloid loopsubdiv nurbs \
               paraboloid sphere trianglemesh \
               bestcandidate lowdiscrepancy random stratified \
               grid kdtree \
               environment orthographic perspective \
               image \
               box gaussian mitchell sinc triangle \
               directlighting emission irradiancecache \
               path photonmap single whitted igi debug exphotonmap bidirectional
#FILM         = image
#FILTERS      = box gaussian mitchell sinc triangle
#INTEGRATORS  = directlighting emission irradiancecache \
#               path photonmap single whitted igi debug exphotonmap
LIGHTS       = area distant goniometric infinite point projection spot infinitesample
MATERIALS    = bluepaint brushedmetal clay felt \
               glass matte mirror plastic primer \
               shinymetal skin substrate translucent uber
#SAMPLERS     = bestcandidate lowdiscrepancy random stratified
#SHAPES       = cone cylinder disk heightfield hyperboloid loopsubdiv nurbs \
#               paraboloid sphere trianglemesh
TEXTURES     = bilerp checkerboard constant dots fbm imagemap marble mix \
               scale uv windy wrinkled
TONEMAPS     = contrast highcontrast maxwhite nonlinear
VOLUMES      = exponential homogeneous volumegrid

RENDERER     = lux



RENDERER_OBJS     := $(addprefix objs/, $(RENDERER:=.o) )
CORE_OBJS         := $(addprefix objs/, $(CORE:=.o) )
CORE_LIB          := core/liblux.a

SHAPES_DSOS       := $(addprefix bin/, $(SHAPES:=.so))
MATERIALS_DSOS    := $(addprefix bin/, $(MATERIALS:=.so))
LIGHTS_DSOS       := $(addprefix bin/, $(LIGHTS:=.so))
INTEGRATORS_DSOS  := $(addprefix bin/, $(INTEGRATORS:=.so))
VOLUMES_DSOS      := $(addprefix bin/, $(VOLUMES:=.so))
TEXTURES_DSOS     := $(addprefix bin/, $(TEXTURES:=.so))
ACCELERATORS_DSOS := $(addprefix bin/, $(ACCELERATORS:=.so))
CAMERAS_DSOS      := $(addprefix bin/, $(CAMERAS:=.so))
FILTERS_DSOS      := $(addprefix bin/, $(FILTERS:=.so))
FILM_DSOS         := $(addprefix bin/, $(FILM:=.so))
TONEMAPS_DSOS     := $(addprefix bin/, $(TONEMAPS:=.so))
SAMPLERS_DSOS     := $(addprefix bin/, $(SAMPLERS:=.so))

SHAPES_OBJS       := $(addprefix objs/, $(SHAPES:=.o))
MATERIALS_OBJS    := $(addprefix objs/, $(MATERIALS:=.o))
LIGHTS_OBJS       := $(addprefix objs/, $(LIGHTS:=.o))
INTEGRATORS_OBJS  := $(addprefix objs/, $(INTEGRATORS:=.o))
VOLUMES_OBJS      := $(addprefix objs/, $(VOLUMES:=.o))
TEXTURES_OBJS     := $(addprefix objs/, $(TEXTURES:=.o))
ACCELERATORS_OBJS := $(addprefix objs/, $(ACCELERATORS:=.o))
CAMERAS_OBJS      := $(addprefix objs/, $(CAMERAS:=.o))
FILTERS_OBJS      := $(addprefix objs/, $(FILTERS:=.o))
FILM_OBJS         := $(addprefix objs/, $(FILM:=.o))
TONEMAPS_OBJS     := $(addprefix objs/, $(TONEMAPS:=.o))
SAMPLERS_OBJS     := $(addprefix objs/, $(SAMPLERS:=.o))

RENDERER_BINARY = bin/lux

CORE_HEADERFILES = api.h camera.h color.h dynload.h film.h geometry.h \
                  kdtree.h light.h lux.h material.h mc.h mipmap.h octree.h \
                  paramset.h primitive.h reflection.h sampling.h scene.h \
                  shape.h texture.h timer.h tonemap.h transform.h transport.h \
                  volume.h 

CORE_HEADERS := $(addprefix core/, $(CORE_HEADERFILES) )

.SECONDARY: $(SHAPES_OBJS) $(MATERIALS_OBJS) $(LIGHTS_OBJS) $(INTEGRATORS_OBJS) \
            $(VOLUMES_OBJS) $(ACCELERATORS_OBJS) $(CAMERAS_OBJS) $(FILTERS_OBJS) \
            $(FILM_OBJS) $(TONEMAPS_OBJS) $(SAMPLERS_OBJS) $(TEXTURES_OBJS)

.PHONY: tools exrcheck

default: $(CORE_LIB) $(RENDERER_BINARY) $(VOLUMES_DSOS) $(MATERIALS_DSOS) $(LIGHTS_DSOS) $(TONEMAPS_DSOS) $(TEXTURES_DSOS) #tools

tools: $(CORE_LIB)
	(cd tools && $(MAKE))

$(CORE_LIB): $(CORE_OBJS)
	@echo "Building the core rendering library (liblux.a)"
	@ar rcs $(CORE_LIB) $(CORE_OBJS) 

bin/%.so: objs/%.o 
	@$(LD) $(SHARED_LDFLAGS) $^ -o $@

objs/%.o: renderer/%.cpp $(CORE_HEADERS)
	@echo "Building the rendering binary (lux)"
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: core/%.cpp $(CORE_HEADERS)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: core/%.c $(CORE_HEADERS)
	@echo "Compiling $<"
	@$(CC) $(CCFLAGS) -o $@ -c $<

objs/%.o: shapes/%.cpp $(CORE_HEADERS)
	@echo "Building Shape Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: integrators/%.cpp $(CORE_HEADERS)
	@echo "Building Integrator Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: volumes/%.cpp $(CORE_HEADERS)
	@echo "Building Volume Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: textures/%.cpp $(CORE_HEADERS)
	@echo "Building Texture Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: materials/%.cpp $(CORE_HEADERS)
	@echo "Building Material Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: lights/%.cpp $(CORE_HEADERS)
	@echo "Building Light Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: accelerators/%.cpp $(CORE_HEADERS)
	@echo "Building Accelerator Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: cameras/%.cpp $(CORE_HEADERS)
	@echo "Building Camera Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: filters/%.cpp $(CORE_HEADERS)
	@echo "Building Filter Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: tonemaps/%.cpp $(CORE_HEADERS)
	@echo "Building Tone Map Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: film/%.cpp $(CORE_HEADERS)
	@echo "Building Film Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

objs/%.o: samplers/%.cpp $(CORE_HEADERS)
	@echo "Building Sampler Plugin \"$*\""
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

core/luxlex.cpp: core/luxlex.l
	@echo "Lex'ing luxlex.l"
	@$(LEX) -o$@ core/luxlex.l

core/luxparse.h core/luxparse.cpp: core/luxparse.y
	@echo "YACC'ing luxparse.y"
	@$(YACC) -o $@ core/luxparse.y
	@if [ -e core/luxparse.cpp.h ]; then /bin/mv core/luxparse.cpp.h core/luxparse.h; fi
	@if [ -e core/luxparse.hpp ]; then /bin/mv core/luxparse.hpp core/luxparse.h; fi

$(RENDERER_BINARY): $(RENDERER_OBJS) $(CORE_LIB)
	@echo "Linking $@"
	@$(CXX) $(LRT_LDFLAGS) -o $@ $(RENDERER_OBJS) $(LUXPRELINK) $(CORE_OBJS) $(LUXPOSTLINK) $(LIBS)

clean:
	rm -f */*.o */*.so */*.a bin/lux core/luxlex.[ch]* core/luxparse.[ch]*
	(cd tools && $(MAKE) clean)

objs/exrio.o: exrcheck

exrcheck:
	@echo -n Checking for EXR installation... 
	@echo $(LIBS)
	@$(CXX) $(CXXFLAGS) -o exrcheck exrcheck.cpp $(LIBS) || \
		(cat exrinstall.txt; exit 1)
