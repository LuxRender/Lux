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

#############################################################################
#############################################################################
#########################      CUSTOM COMMAND     ###########################
#############################################################################
#############################################################################

# Create custom command for bison/yacc
BISON_TARGET(LuxParser ${CMAKE_SOURCE_DIR}/core/luxparse.y ${CMAKE_BINARY_DIR}/luxparse.cpp)
if(APPLE AND !APPLE_64)
	EXECUTE_PROCESS(COMMAND mv ${CMAKE_SOURCE_DIR}/luxparse.cpp.h ${CMAKE_BINARY_DIR}/luxparse.hpp)
ENDIF(APPLE AND !APPLE_64)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/core/luxparse.cpp GENERATED)

# Create custom command for flex/lex
FLEX_TARGET(LuxLexer ${CMAKE_SOURCE_DIR}/core/luxlex.l ${CMAKE_BINARY_DIR}/luxlex.cpp)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/luxlex.cpp GENERATED)
ADD_FLEX_BISON_DEPENDENCY(LuxLexer LuxParser)

#############################################################################
#############################################################################
#####################  SOURCE FILES FOR static liblux.a  ####################
#############################################################################
#############################################################################

SET(lux_core_generated_src
	${CMAKE_BINARY_DIR}/luxparse.cpp
	${CMAKE_BINARY_DIR}/luxlex.cpp
	)
SOURCE_GROUP("Source Files\\Core\\Generated" FILES ${lux_core_generated_src})

SET(lux_core_src
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
	server/renderserver.cpp
	)
SOURCE_GROUP("Source Files\\Core" FILES ${lux_core_src})

SET(lux_core_geometry_src
	core/geometry/bbox.cpp
	core/geometry/matrix4x4.cpp
	core/geometry/quaternion.cpp
	core/geometry/raydifferential.cpp
	core/geometry/transform.cpp
	)
SOURCE_GROUP("Source Files\\Core\\Geometry" FILES ${lux_core_geometry_src})

SET(lux_core_queryable_src
	core/queryable/queryable.cpp
	core/queryable/queryableattribute.cpp
	core/queryable/queryableregistry.cpp
	)
SOURCE_GROUP("Source Files\\Core\\Queryable" FILES ${lux_core_queryable_src})

SET(lux_core_reflection_src
	core/reflection/bxdf.cpp
	core/reflection/fresnel.cpp
	core/reflection/microfacetdistribution.cpp
	)
SOURCE_GROUP("Source Files\\Core\\Reflection" FILES ${lux_core_reflection_src})

SET(lux_core_reflection_bxdf_src
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
SOURCE_GROUP("Source Files\\Core\\Reflection\\BxDF" FILES ${lux_core_reflection_bxdf_src})

SET(lux_core_reflection_fresnel_src
	core/reflection/fresnel/fresnelcauchy.cpp
	core/reflection/fresnel/fresnelconductor.cpp
	core/reflection/fresnel/fresneldielectric.cpp
	core/reflection/fresnel/fresnelgeneral.cpp
	core/reflection/fresnel/fresnelnoop.cpp
	core/reflection/fresnel/fresnelslick.cpp
	)
SOURCE_GROUP("Source Files\\Core\\Reflection\\Fresnel" FILES ${lux_core_reflection_fresnel_src})

SET(lux_core_reflection_microfacetdistribution_src
	core/reflection/microfacetdistribution/anisotropic.cpp
	core/reflection/microfacetdistribution/beckmann.cpp
	core/reflection/microfacetdistribution/blinn.cpp
	core/reflection/microfacetdistribution/schlickdistribution.cpp
	core/reflection/microfacetdistribution/wardisotropic.cpp
	)
SOURCE_GROUP("Source Files\\Core\\Reflection\\Microfacet Distribution" FILES ${lux_core_reflection_microfacetdistribution_src})

SET(lux_core_all_src
	${lux_core_generated_src}
	${lux_core_src}
	${lux_core_geometry_src}
	${lux_core_queryable_src}
	${lux_core_reflection_src}
	${lux_core_reflection_bxdf_src}
	${lux_core_reflection_fresnel_src}
	${lux_core_reflection_microfacetdistribution_src}
	)

#############################################################################

SET(lux_accelerators_src
	accelerators/bruteforce.cpp
	accelerators/bvhaccel.cpp
	accelerators/qbvhaccel.cpp
	accelerators/tabreckdtree.cpp
	accelerators/unsafekdtree.cpp
	)
SOURCE_GROUP("Source Files\\Accelerators" FILES ${lux_accelerators_src})

SET(lux_cameras_src
	cameras/environment.cpp
	cameras/perspective.cpp
	cameras/orthographic.cpp
	cameras/realistic.cpp
	)
SOURCE_GROUP("Source Files\\Cameras" FILES ${lux_cameras_src})

SET(lux_films_src
	film/fleximage.cpp
	)
SOURCE_GROUP("Source Files\\Films" FILES ${lux_films_src})

SET(lux_filters_src
	filters/box.cpp
	filters/gaussian.cpp
	filters/mitchell.cpp
	filters/sinc.cpp
	filters/triangle.cpp
	)
SOURCE_GROUP("Source Files\\Filters" FILES ${lux_filters_src})

SET(lux_integrators_src
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
SOURCE_GROUP("Source Files\\Integrators" FILES ${lux_integrators_src})

SET(lux_lights_src
	lights/area.cpp
	lights/distant.cpp
	lights/infinite.cpp
	lights/infinitesample.cpp
	lights/pointlight.cpp
	lights/projection.cpp
	lights/sphericalfunction/photometricdata_ies.cpp
	lights/sphericalfunction/sphericalfunction.cpp
	lights/sphericalfunction/sphericalfunction_ies.cpp
	lights/spot.cpp
	lights/sky.cpp
	lights/sun.cpp
	)
SOURCE_GROUP("Source Files\\Lights" FILES ${lux_lights_src})

SET(lux_lights_sphericalfunction_src
	lights/sphericalfunction/photometricdata_ies.cpp
	lights/sphericalfunction/sphericalfunction.cpp
	lights/sphericalfunction/sphericalfunction_ies.cpp
	)
SOURCE_GROUP("Source Files\\Lights\\Spherical Functions" FILES ${lux_lights_sphericalfunction_src})

SET(lux_materials_src
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
SOURCE_GROUP("Source Files\\Materials" FILES ${lux_materials_src})

SET(lux_pixelsamplers_src
	pixelsamplers/hilbertpx.cpp
	pixelsamplers/linear.cpp
	pixelsamplers/lowdiscrepancypx.cpp
	pixelsamplers/tilepx.cpp
	pixelsamplers/vegas.cpp
	)
SOURCE_GROUP("Source Files\\Pixel Samplers" FILES ${lux_pixelsamplers_src})

SET(lux_samplers_src
	samplers/erpt.cpp
	samplers/lowdiscrepancy.cpp
	samplers/metrosampler.cpp
	samplers/random.cpp
	)
SOURCE_GROUP("Source Files\\Samplers" FILES ${lux_samplers_src})

SET(lux_renderers_src
	renderers/samplerrenderer.cpp
	renderers/sppmrenderer.cpp
	renderers/sppm/photonsampler.cpp
	renderers/sppm/lookupaccel.cpp
	renderers/sppm/hashgrid.cpp
	renderers/sppm/hitpoints.cpp
	renderers/sppm/hybridhashgrid.cpp
	renderers/sppm/kdtree.cpp
	)
IF(NOT LUXRAYS_DISABLE_OPENCL)
SET(lux_renderers_src
	${lux_renderers_src}
	renderers/hybridrenderer.cpp
	renderers/hybridsamplerrenderer.cpp
	)
endif(NOT LUXRAYS_DISABLE_OPENCL)
SOURCE_GROUP("Source Files\\Renderers" FILES ${lux_renderers_src})

SET(lux_shapes_src
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
SOURCE_GROUP("Source Files\\Shapes" FILES ${lux_shapes_src})

SET(lux_spds_src
	spds/blackbodyspd.cpp
	spds/equalspd.cpp
	spds/frequencyspd.cpp
	spds/gaussianspd.cpp
	spds/irregular.cpp
	spds/regular.cpp
	spds/rgbillum.cpp
	spds/rgbrefl.cpp
	)
SOURCE_GROUP("Source Files\\SPDs" FILES ${lux_spds_src})

SET(lux_blender_textures_src
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
	)
SOURCE_GROUP("Source Files\\Textures\\Blender" FILES ${lux_blender_textures_src})

SET(lux_uniform_textures_src
	textures/blackbody.cpp
	textures/constant.cpp
	textures/equalenergy.cpp
	textures/frequencytexture.cpp
	textures/gaussiantexture.cpp
	textures/irregulardata.cpp
	textures/lampspectrum.cpp
	textures/regulardata.cpp
	textures/tabulateddata.cpp
	)
SOURCE_GROUP("Source Files\\Textures\\Uniform" FILES ${lux_uniform_textures_src})

SET(lux_fresnel_textures_src
	textures/cauchytexture.cpp
	textures/sellmeiertexture.cpp
	textures/tabulatedfresnel.cpp
	)
SOURCE_GROUP("Source Files\\Textures\\Fresnel" FILES ${lux_fresnel_textures_src})

SET(lux_textures_src
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
SOURCE_GROUP("Source Files\\Textures" FILES ${lux_textures_src})

SET(lux_textures_all_src
	${lux_uniform_textures_src}
	${lux_blender_textures_src}
	${lux_fresnel_textures_src}
	${lux_textures_src}
	)

SET(lux_tonemaps_src
	tonemaps/contrast.cpp
	tonemaps/lineartonemap.cpp
	tonemaps/maxwhite.cpp
	tonemaps/nonlinear.cpp
	tonemaps/reinhard.cpp
	)
SOURCE_GROUP("Source Files\\Tonemaps" FILES ${lux_tonemaps_src})

SET(lux_volumes_src
	volumes/clearvolume.cpp
	volumes/cloud.cpp
	volumes/exponential.cpp
	volumes/homogeneous.cpp
	volumes/volumegrid.cpp
	)
SOURCE_GROUP("Source Files\\Volumes" FILES ${lux_volumes_src})

SET(lux_modules_src
	${lux_accelerators_src}
	${lux_cameras_src}
	${lux_films_src}
	${lux_filters_src}
	${lux_integrators_src}
	${lux_lights_src}
	${lux_lights_sphericalfunction_src}
	${lux_materials_src}
	${lux_pixelsamplers_src}
	${lux_renderers_src}
	${lux_samplers_src}
	${lux_shapes_src}
	${lux_spds_src}
	${lux_textures_all_src}
	${lux_tonemaps_src}
	${lux_volumes_src}
	)

SET(lux_lib_src
	${lux_core_all_src}
	${lux_modules_src}
	)

#############################################################################

INCLUDE_DIRECTORIES(SYSTEM
	${CMAKE_SOURCE_DIR}/core/external
	${PNG_INCLUDE_DIR}
	${OPENEXR_INCLUDE_DIRS}
	)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core
	${CMAKE_SOURCE_DIR}/core/queryable
	${CMAKE_SOURCE_DIR}/core/reflection
	${CMAKE_SOURCE_DIR}/core/reflection/bxdf
	${CMAKE_SOURCE_DIR}/core/reflection/fresnel
	${CMAKE_SOURCE_DIR}/core/reflection/microfacetdistribution
	${CMAKE_SOURCE_DIR}/spds
	${CMAKE_SOURCE_DIR}/lights/sphericalfunction
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	)

#############################################################################
# Here we build the static core library liblux.a
#############################################################################
ADD_LIBRARY(luxStatic STATIC ${lux_lib_src} )
IF( NOT CMAKE_VERSION VERSION_LESS 2.8.3 AND OSX_OPTION_CLANG) # only cmake >= 2.8.1 supports per target attributes
	SET_TARGET_PROPERTIES(luxStatic PROPERTIES XCODE_ATTRIBUTE_GCC_VERSION com.apple.compilers.llvm.clang.1_0) # for testing new CLANG2.0, will be ignored for other OS
	SET_TARGET_PROPERTIES(luxStatic PROPERTIES XCODE_ATTRIBUTE_LLVM_LTO NO ) # disabled due breaks bw compatibility
ENDIF()
#TARGET_LINK_LIBRARIES(luxStatic ${FREEIMAGE_LIBRARIES} ${Boost_LIBRARIES} )

#############################################################################
# Here we build the shared core library liblux.so
#############################################################################
ADD_LIBRARY(luxShared SHARED
	cpp_api/lux_api.cpp
	cpp_api/lux_wrapper_factories.cpp
)
IF(APPLE)
	IF( NOT CMAKE_VERSION VERSION_LESS 2.8.3 AND OSX_OPTION_CLANG) # only cmake >= 2.8.1 supports per target attributes
		SET_TARGET_PROPERTIES(luxShared PROPERTIES XCODE_ATTRIBUTE_GCC_VERSION com.apple.compilers.llvm.clang.1_0) # for testing new CLANG2.0, will be ignored for other OS
		SET_TARGET_PROPERTIES(luxShared PROPERTIES XCODE_ATTRIBUTE_LLVM_LTO NO ) # disabled due breaks bw compatibility
	ENDIF()
ENDIF(APPLE)
TARGET_LINK_LIBRARIES(luxShared ${LUX_LIBRARY} ${LUX_LIBRARY_DEPENDS})

# Make CMake output both libs with the same name
SET_TARGET_PROPERTIES(luxStatic luxShared PROPERTIES OUTPUT_NAME lux)
