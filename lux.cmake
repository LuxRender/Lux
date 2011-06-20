###########################################################################
#   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 #
#                                                                         #
#   This file is part of Lux.                                             #
#                                                                         #
#   Lux is free software; you can redistribute it and/or modify           #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 3 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   Lux is distributed in the hope that it will be useful,                #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
#                                                                         #
#   Lux website: http://www.luxrender.net                                 #
###########################################################################

FIND_PACKAGE(BISON REQUIRED)
IF (NOT BISON_FOUND)
	MESSAGE(FATAL_ERROR "bison not found - aborting")
ENDIF (NOT BISON_FOUND)

FIND_PACKAGE(FLEX REQUIRED)
IF (NOT FLEX_FOUND)
	MESSAGE(FATAL_ERROR "flex not found - aborting")
ENDIF (NOT FLEX_FOUND)

# Create custom command for bison/yacc
BISON_TARGET(LuxParser ${CMAKE_SOURCE_DIR}/core/luxparse.y ${CMAKE_BINARY_DIR}/luxparse.cpp)
IF(APPLE AND !APPLE_64)
	EXECUTE_PROCESS(COMMAND mv ${CMAKE_SOURCE_DIR}/luxparse.cpp.h ${CMAKE_BINARY_DIR}/luxparse.hpp)
ENDIF(APPLE AND !APPLE_64)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/core/luxparse.cpp GENERATED)

# Create custom command for flex/lex
FLEX_TARGET(LuxLexer ${CMAKE_SOURCE_DIR}/core/luxlex.l ${CMAKE_BINARY_DIR}/luxlex.cpp)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/luxlex.cpp GENERATED)
ADD_FLEX_BISON_DEPENDENCY(LuxLexer LuxParser)

SET(lux_SRCS_CORE
	${CMAKE_BINARY_DIR}/luxparse.cpp
	${CMAKE_BINARY_DIR}/luxlex.cpp
	core/api.cpp
	core/camera.cpp
	core/cameraresponse.cpp
	core/color.cpp
	core/stats.cpp
	core/context.cpp
	core/contribution.cpp
	core/dynload.cpp
	core/epsilon.cpp
	core/exrio.cpp
	core/filedata.cpp
	core/film.cpp
	core/igiio.cpp
	core/imagereader.cpp
	core/light.cpp
	core/material.cpp
	core/mc.cpp
	core/motionsystem.cpp
	core/osfunc.cpp
	core/paramset.cpp
	core/photonmap.cpp
	core/pngio.cpp
	core/primitive.cpp
	core/renderfarm.cpp
	core/renderinghints.cpp
	core/sampling.cpp
	core/scene.cpp
	core/shape.cpp
	core/spd.cpp
	core/spectrum.cpp
	core/spectrumwavelengths.cpp
	core/texture.cpp
	core/tgaio.cpp
	core/timer.cpp
	core/transport.cpp
	core/util.cpp
	core/volume.cpp
)
SOURCE_GROUP("Source Files\\Core" FILES ${lux_SRCS_CORE})

SET(lux_SRCS_CORE_GEOMETRY
	core/geometry/bbox.cpp
	core/geometry/matrix4x4.cpp
	core/geometry/quaternion.cpp
	core/geometry/raydifferential.cpp
	core/geometry/transform.cpp
)
SOURCE_GROUP("Source Files\\Core\\Geometry" FILES ${lux_SRCS_CORE_GEOMETRY})

SET(lux_SRCS_CORE_QUERYABLE
	core/queryable/queryable.cpp
	core/queryable/queryableattribute.cpp
	core/queryable/queryableregistry.cpp
)
SOURCE_GROUP("Source Files\\Core\\Queryable" FILES ${lux_SRCS_CORE_QUERYABLE})

SET(lux_SRCS_CORE_REFLECTION
	core/reflection/bxdf.cpp
	core/reflection/fresnel.cpp
	core/reflection/microfacetdistribution.cpp
)
SOURCE_GROUP("Source Files\\Core\\Reflection" FILES ${lux_SRCS_CORE_REFLECTION})

SET(lux_SRCS_CORE_REFLECTION_BXDF
	core/reflection/bxdf/asperity.cpp
	core/reflection/bxdf/brdftobtdf.cpp
	core/reflection/bxdf/cooktorrance.cpp
	core/reflection/bxdf/fresnelblend.cpp
	core/reflection/bxdf/lafortune.cpp
	core/reflection/bxdf/lambertian.cpp
	core/reflection/bxdf/microfacet.cpp
	core/reflection/bxdf/nulltransmission.cpp
	core/reflection/bxdf/orennayar.cpp
	core/reflection/bxdf/schlickbrdf.cpp
	core/reflection/bxdf/schlickscatter.cpp
	core/reflection/bxdf/schlicktranslucentbtdf.cpp
	core/reflection/bxdf/specularreflection.cpp
	core/reflection/bxdf/speculartransmission.cpp
)
SOURCE_GROUP("Source Files\\Core\\Reflection\\BXDF" FILES ${lux_SRCS_CORE_REFLECTION_BXDF})

SET(lux_SRCS_CORE_REFLECTION_FRESNEL
	core/reflection/fresnel/fresnelcauchy.cpp
	core/reflection/fresnel/fresnelconductor.cpp
	core/reflection/fresnel/fresneldielectric.cpp
	core/reflection/fresnel/fresnelgeneral.cpp
	core/reflection/fresnel/fresnelnoop.cpp
	core/reflection/fresnel/fresnelslick.cpp
)
SOURCE_GROUP("Source Files\\Core\\Reflection\\Fresnel" FILES ${lux_SRCS_CORE_REFLECTION_FRESNEL})


SET(lux_SRCS_CORE_REFLECTION_MICROFACETDISTRIBUTION
	core/reflection/microfacetdistribution/anisotropic.cpp
	core/reflection/microfacetdistribution/beckmann.cpp
	core/reflection/microfacetdistribution/blinn.cpp
	core/reflection/microfacetdistribution/schlickdistribution.cpp
	core/reflection/microfacetdistribution/wardisotropic.cpp
)
SOURCE_GROUP("Source Files\\Core\\Reflection\\Microfacetdistribution" FILES ${lux_SRCS_CORE_REFLECTION_MICROFACETDISTRIBUTION})

SET(lux_SRCS_ACCELERATORS
	accelerators/bruteforce.cpp
	accelerators/bvhaccel.cpp
	accelerators/qbvhaccel.cpp
	accelerators/tabreckdtree.cpp
	accelerators/unsafekdtree.cpp
)
SOURCE_GROUP("Source Files\\Accelerators" FILES ${lux_SRCS_ACCELERATORS})

SET(lux_SRCS_CAMERAS
	cameras/environment.cpp
	cameras/perspective.cpp
	cameras/orthographic.cpp
	cameras/realistic.cpp
)
SOURCE_GROUP("Source Files\\Cameras" FILES ${lux_SRCS_CAMERAS})

SET(lux_SRCS_FILM
	film/fleximage.cpp
)
SOURCE_GROUP("Source Files\\Films" FILES ${lux_SRCS_FILM})

SET(lux_SRCS_FILTERS
	filters/box.cpp
	filters/gaussian.cpp
	filters/mitchell.cpp
	filters/sinc.cpp
	filters/triangle.cpp
)
SOURCE_GROUP("Source Files\\Filters" FILES ${lux_SRCS_FILTERS})

SET(lux_SRCS_INTEGRATORS
	integrators/bidirectional.cpp
	integrators/directlighting.cpp
	integrators/distributedpath.cpp
	integrators/emission.cpp
	integrators/exphotonmap.cpp
	integrators/igi.cpp
	integrators/multi.cpp
	integrators/path.cpp
	integrators/single.cpp
	integrators/sppm.cpp
)
SOURCE_GROUP("Source Files\\Integrators" FILES ${lux_SRCS_INTEGRATORS})

SET(lux_SRCS_LIGHTS
	lights/area.cpp
	lights/distant.cpp
	lights/infinite.cpp
	lights/infinitesample.cpp
	lights/pointlight.cpp
	lights/projection.cpp
	lights/spot.cpp
	lights/sky.cpp
	lights/sun.cpp
)
SOURCE_GROUP("Source Files\\Lights" FILES ${lux_SRCS_LIGHTS})

SET(lux_SRCS_LIGHTS_SPHERICALFUNCTION
	lights/sphericalfunction/photometricdata_ies.cpp
	lights/sphericalfunction/sphericalfunction.cpp
	lights/sphericalfunction/sphericalfunction_ies.cpp
)
SOURCE_GROUP("Source Files\\Lights\\Sphericalfunction" FILES ${lux_SRCS_LIGHTS_SPHERICALFUNCTION})

SET(lux_SRCS_MATERIALS
	materials/carpaint.cpp
	materials/glass.cpp
	materials/glass2.cpp
	materials/glossy.cpp
	materials/glossy2.cpp
	materials/glossytranslucent.cpp
	materials/matte.cpp
	materials/mattetranslucent.cpp
	materials/metal.cpp
	materials/mirror.cpp
	materials/mixmaterial.cpp
	materials/null.cpp
	materials/roughglass.cpp
	materials/scattermaterial.cpp
	materials/shinymetal.cpp
	materials/velvet.cpp
)
SOURCE_GROUP("Source Files\\Materials" FILES ${lux_SRCS_MATERIALS})

SET(lux_SRCS_PIXELSAMPLERS
	pixelsamplers/hilbertpx.cpp
	pixelsamplers/linear.cpp
	pixelsamplers/lowdiscrepancypx.cpp
	pixelsamplers/tilepx.cpp
	pixelsamplers/vegas.cpp
)
SOURCE_GROUP("Source Files\\PixelSamplers" FILES ${lux_SRCS_PIXELSAMPLERS})

SET(lux_SRCS_RENDERERS
	renderers/hybridrenderer.cpp
	renderers/hybridsamplerrenderer.cpp
	renderers/samplerrenderer.cpp
	renderers/sppmrenderer.cpp
)
SOURCE_GROUP("Source Files\\Renderers" FILES ${lux_SRCS_RENDERERS})

SET(lux_SRCS_RENDERERS_SPPM
	renderers/sppm/hashgrid.cpp
	renderers/sppm/hitpoints.cpp
	renderers/sppm/hybridhashgrid.cpp
	renderers/sppm/lookupaccel.cpp
	renderers/sppm/photonsampler.cpp
)
SOURCE_GROUP("Source Files\\Renderers\\SPPM" FILES ${lux_SRCS_RENDERERS_SPPM})

SET(lux_SRCS_SAMPLERS
	samplers/erpt.cpp
	samplers/lowdiscrepancy.cpp
	samplers/metrosampler.cpp
	samplers/random.cpp
)
SOURCE_GROUP("Source Files\\Samplers" FILES ${lux_SRCS_SAMPLERS})

SET(lux_SRCS_SHAPES
	shapes/cone.cpp
	shapes/cylinder.cpp
	shapes/disk.cpp
	shapes/heightfield.cpp
	shapes/hyperboloid.cpp
	shapes/lenscomponent.cpp
	shapes/loopsubdiv.cpp
	shapes/mesh.cpp
	shapes/meshbarytriangle.cpp
	shapes/meshmicrodisplacementtriangle.cpp
	shapes/meshquadrilateral.cpp
	shapes/meshwaldtriangle.cpp
	shapes/nurbs.cpp
	shapes/paraboloid.cpp
	shapes/plymesh.cpp
	shapes/plymesh/rply.c
	shapes/sphere.cpp
	shapes/stlmesh.cpp
	shapes/torus.cpp
)
SOURCE_GROUP("Source Files\\Shapes" FILES ${lux_SRCS_SHAPES})

SET(lux_SRCS_SPDS
	spds/blackbodyspd.cpp
	spds/equalspd.cpp
	spds/frequencyspd.cpp
	spds/gaussianspd.cpp
	spds/irregular.cpp
	spds/regular.cpp
	spds/rgbillum.cpp
	spds/rgbrefl.cpp
)
SOURCE_GROUP("Source Files\\SPDS" FILES ${lux_SRCS_SPDS})

SET(lux_SRCS_TEXTURES

	# Blender
	textures/blender_base.cpp
	textures/blender_blend.cpp
	textures/blender_clouds.cpp
	textures/blender_distortednoise.cpp
	textures/blender_magic.cpp
	textures/blender_marble.cpp
	textures/blender_musgrave.cpp
	textures/blender_noise.cpp
	textures/blender_noiselib.cpp
	textures/blender_stucci.cpp
	textures/blender_texlib.cpp
	textures/blender_voronoi.cpp
	textures/blender_wood.cpp

	# Uniform
	textures/blackbody.cpp
	textures/constant.cpp
	textures/equalenergy.cpp
	textures/frequencytexture.cpp
	textures/gaussiantexture.cpp
	textures/irregulardata.cpp
	textures/lampspectrum.cpp
	textures/regulardata.cpp
	textures/tabulateddata.cpp

	# Fresnel
	textures/cauchytexture.cpp
	textures/sellmeiertexture.cpp
	textures/tabulatedfresnel.cpp

	# General
	textures/band.cpp
	textures/bilerp.cpp
	textures/brick.cpp
	textures/checkerboard.cpp
	textures/dots.cpp
	textures/fbm.cpp
	textures/harlequin.cpp
	textures/imagemap.cpp
	textures/marble.cpp
	textures/mix.cpp
	textures/multimix.cpp
	textures/scale.cpp
	textures/uv.cpp
	textures/uvmask.cpp
	textures/windy.cpp
	textures/wrinkled.cpp
)
SOURCE_GROUP("Source Files\\Textures" FILES ${lux_SRCS_TEXTURES})

SET(lux_SRCS_TONEMAPS
	tonemaps/contrast.cpp
	tonemaps/lineartonemap.cpp
	tonemaps/maxwhite.cpp
	tonemaps/nonlinear.cpp
	tonemaps/reinhard.cpp
)
SOURCE_GROUP("Source Files\\Tonemaps" FILES ${lux_SRCS_TONEMAPS})

SET(lux_SRCS_VOLUMES
	volumes/clearvolume.cpp
	volumes/cloud.cpp
	volumes/exponential.cpp
	volumes/homogeneous.cpp
	volumes/volumegrid.cpp
)
SOURCE_GROUP("Source Files\\Volumes" FILES ${lux_SRCS_VOLUMES})

SET(lux_SRCS_SERVER
	server/renderserver.cpp
)
SOURCE_GROUP("Source Files\\Server" FILES ${lux_SRCS_SERVER})


SET(lux_lib_src
	${lux_SRCS}
	${lux_SRCS_CORE}
	${lux_SRCS_CORE_GEOMETRY}
	${lux_SRCS_CORE_QUERYABLE}
	${lux_SRCS_CORE_REFLECTION}
	${lux_SRCS_CORE_REFLECTION_BXDF}
	${lux_SRCS_CORE_REFLECTION_FRESNEL}
	${lux_SRCS_CORE_REFLECTION_MICROFACETDISTRIBUTION}
	${lux_SRCS_ACCELERATORS}
	${lux_SRCS_CAMERAS}
	${lux_SRCS_FILM}
	${lux_SRCS_FILTERS}
	${lux_SRCS_INTEGRATORS}
	${lux_SRCS_LIGHTS}
	${lux_SRCS_LIGHTS_SPHERICALFUNCTION}
	${lux_SRCS_MATERIALS}
	${lux_SRCS_PIXELSAMPLERS}
	${lux_SRCS_RENDERERS}
	${lux_SRCS_RENDERERS_HYBRIDSPPM}
	${lux_SRCS_SAMPLERS}
	${lux_SRCS_SHAPES}
	${lux_SRCS_SPDS}
	${lux_SRCS_TEXTURES}
	${lux_SRCS_TONEMAPS}
	${lux_SRCS_VOLUMES}
	${lux_SRCS_SERVER}
)

ADD_LIBRARY(luxStatic ${lux_lib_src})

SET_TARGET_PROPERTIES(luxStatic PROPERTIES COMPILE_FLAGS -DLUX_INTERNAL)

TARGET_LINK_LIBRARIES(luxStatic ${SYS_LIBRARIES} ${LuxRays_LIBRARY} ${OPENCL_LIBRARY} ${OPENGL_LIBRARY} ${Boost_LIBRARIES} ${OpenEXR_LIBRARIES} ${FreeImage_LIBRARIES} ${PNG_LIBRARY})

INCLUDE_DIRECTORIES(${OpenEXR_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LUXRAYS_INCLUDE_DIR})
