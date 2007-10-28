#CMake file for GNU/Linux static building
PROJECT(lux)

SET (CMAKE_EXE_LINK_DYNAMIC_C_FLAGS "-static")

#Find BISON
FIND_PROGRAM(BISON_EXECUTABLE bison)
IF (NOT BISON_EXECUTABLE)
MESSAGE(FATAL_ERROR "bison not found - aborting")
ENDIF (NOT BISON_EXECUTABLE)

#Find FLEX
FIND_PROGRAM(FLEX_EXECUTABLE flex)
IF (NOT FLEX_EXECUTABLE)
MESSAGE(FATAL_ERROR "flex not found - aborting")
ENDIF (NOT FLEX_EXECUTABLE)

# Find and setup FLTK library
#This FIND_PATH is a hack, FIND_PACKAGE doesn't find fltk otherwise
FIND_PATH(FLTK_INCLUDE_DIR
    Fl/Fl.H
    PATHS
    /usr/local/include/
    /usr/local/include/fltk-1.1
    /usr/include/
    /usr/include/fltk-1.1
    /opt/local/include/
    /opt/include/
)
IF( FLTK_INCLUDE_DIR )
	SET(FLTK_LIBRARIES fltk fltk_gl X11 Xext Xft Xinerama)
ELSE( FLTK_INCLUDE_DIR )
        MESSAGE(FATAL_ERROR "Could not find FLTK")
ENDIF( FLTK_INCLUDE_DIR )


# Find and setup Boost library
FIND_PACKAGE(Boost REQUIRED)
IF(Boost_FOUND)
	SET(Boost_LIBRARIES boost_thread boost_program_options boost_filesystem)
ELSE(Boost_FOUND)
	MESSAGE(FATAL_ERROR "Could not find Boost")
ENDIF(Boost_FOUND)


# Find and setup OpenEXR
SET(OPENEXR_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/platform/linux/include/OpenEXR)
SET(OPENEXR_LIBRARIES  IlmImf Half Iex Imath IlmThread z pthread )

#All dependencies OK !


#Compiler flags
ADD_DEFINITIONS(-O3 -Wall -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -Wall -msse -DLUX_USE_SSE -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-g -Wall -msse -DHAVE_PTHREAD_H)


# Create custom command for flex/lex (note the outputs)
ADD_CUSTOM_COMMAND(
SOURCE ${CMAKE_SOURCE_DIR}/core/luxlex.l
COMMAND ${FLEX_EXECUTABLE} 
ARGS -o${CMAKE_BINARY_DIR}/luxlex.cpp
      ${CMAKE_SOURCE_DIR}/core/luxlex.l
TARGET LuxLexer
OUTPUTS ${CMAKE_BINARY_DIR}/luxlex.cpp)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/luxlex.cpp GENERATED)


# Create custom command for bison/yacc (note the DEPENDS)
ADD_CUSTOM_COMMAND(
SOURCE ${CMAKE_SOURCE_DIR}/core/luxparse.y
COMMAND ${BISON_EXECUTABLE} 
ARGS -y ${CMAKE_SOURCE_DIR}/core/luxparse.y
     -o ${CMAKE_BINARY_DIR}/luxparse.cpp
     -d
TARGET LuxParser
OUTPUTS ${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxparse.hpp)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/core/luxparse.cpp GENERATED)

SET(lux_lib_src
		${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxlex.cpp
		core/api.cpp core/camera.cpp core/color.cpp core/dynload.cpp core/exrio.cpp core/film.cpp
		core/geometry.cpp core/igiio.cpp core/light.cpp core/material.cpp core/mc.cpp
        core/paramset.cpp core/parser.cpp core/primitive.cpp core/reflection.cpp core/sampling.cpp
        core/scene.cpp core/shape.cpp core/texture.cpp core/timer.cpp core/transform.cpp
        core/transport.cpp core/util.cpp core/volume.cpp
        shapes/cone.cpp shapes/cylinder.cpp shapes/disk.cpp shapes/heightfield.cpp
        shapes/hyperboloid.cpp shapes/loopsubdiv.cpp shapes/nurbs.cpp
        shapes/paraboloid.cpp shapes/sphere.cpp shapes/trianglemesh.cpp
        samplers/bestcandidate.cpp samplers/lowdiscrepancy.cpp
        samplers/random.cpp samplers/stratified.cpp
        accelerators/grid.cpp accelerators/kdtree.cpp
        cameras/environment.cpp cameras/orthographic.cpp cameras/perspective.cpp
        film/image.cpp film/multiimage.cpp
        filters/box.cpp filters/gaussian.cpp filters/mitchell.cpp filters/sinc.cpp filters/triangle.cpp
        integrators/directlighting.cpp integrators/emission.cpp integrators/irradiancecache.cpp
        integrators/path.cpp integrators/photonmap.cpp integrators/single.cpp
        integrators/whitted.cpp integrators/igi.cpp
        integrators/debug.cpp integrators/exphotonmap.cpp integrators/bidirectional.cpp integrationsamplers/metropolis.cpp
        lights/area.cpp lights/distant.cpp lights/goniometric.cpp lights/infinite.cpp
        lights/point.cpp lights/projection.cpp lights/spot.cpp lights/infinitesample.cpp
        lights/sun.cpp lights/sky.cpp lights/sun2.cpp
        materials/bluepaint.cpp materials/brushedmetal.cpp materials/clay.cpp materials/felt.cpp
        materials/glass.cpp materials/matte.cpp materials/mirror.cpp materials/plastic.cpp materials/primer.cpp
        materials/shinymetal.cpp materials/skin.cpp materials/substrate.cpp
        materials/translucent.cpp materials/uber.cpp
        textures/bilerp.cpp textures/checkerboard.cpp textures/constant.cpp textures/dots.cpp
        textures/fbm.cpp textures/imagemap.cpp textures/marble.cpp textures/mix.cpp
        textures/scale.cpp textures/uv.cpp textures/windy.cpp textures/wrinkled.cpp
        tonemaps/contrast.cpp tonemaps/highcontrast.cpp tonemaps/maxwhite.cpp tonemaps/nonlinear.cpp tonemaps/reinhard.cpp
        volumes/exponential.cpp volumes/homogeneous.cpp volumes/volumegrid.cpp
	)


INCLUDE_DIRECTORIES(${OPENEXR_INCLUDE_DIRS}
					${Boost_INCLUDE_DIRS}
					${CMAKE_SOURCE_DIR}/accelerators ${CMAKE_SOURCE_DIR}/cameras ${CMAKE_SOURCE_DIR}/core
					${CMAKE_SOURCE_DIR}/film ${CMAKE_SOURCE_DIR}/filters ${CMAKE_SOURCE_DIR}/integrators
					${CMAKE_SOURCE_DIR}/lights ${CMAKE_SOURCE_DIR}/materials ${CMAKE_SOURCE_DIR}/samplers
					${CMAKE_SOURCE_DIR}/shapes ${CMAKE_SOURCE_DIR}/textures ${CMAKE_SOURCE_DIR}/tonemaps
					${CMAKE_SOURCE_DIR}/volumes
					${CMAKE_SOURCE_DIR}/integrationsamplers
					${CMAKE_BINARY_DIR}
					)

LINK_DIRECTORIES(${LINK_DIRECTORIES} ${CMAKE_SOURCE_DIR}/platform/linux/x86 ${CMAKE_SOURCE_DIR}/platform/linux/x64 )
					
#Here we build the core library
ADD_LIBRARY(lux STATIC ${lux_lib_src} )
TARGET_LINK_LIBRARIES(lux ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES} png jpeg tiff)

#Here we build the console executable
ADD_EXECUTABLE(luxconsole renderer/luxconsole.cpp)
TARGET_LINK_LIBRARIES(luxconsole lux ${OPENEXR_LIBRARIES} ${FLTK_LIBRARIES} ${Boost_LIBRARIES} png jpeg tiff )

#Here we build the GUI executable
INCLUDE_DIRECTORIES(${FLTK_INCLUDE_DIR})
ADD_EXECUTABLE(luxrender renderer/luxgui.cpp)
TARGET_LINK_LIBRARIES(luxrender lux ${OPENEXR_LIBRARIES} ${FLTK_LIBRARIES} ${Boost_LIBRARIES})

