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
IF (NOT BISON_NOT_AVAILABLE AND NOT FLEX_NOT_AVAILABLE)
	# Create custom command for bison/yacc
	BISON_TARGET(LuxParser ${CMAKE_SOURCE_DIR}/core/luxparse.y ${CMAKE_BINARY_DIR}/luxparse.cpp)
	IF(APPLE AND !APPLE_64)
		EXECUTE_PROCESS(COMMAND mv ${CMAKE_SOURCE_DIR}/luxparse.cpp.h ${CMAKE_BINARY_DIR}/luxparse.hpp)
	ENDIF(APPLE AND !APPLE_64)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/core/luxparse.cpp GENERATED)
	#SOURCE_GROUP("Parser Files" FILES core/luxparse.y)

	# Create custom command for flex/lex
	FLEX_TARGET(LuxLexer ${CMAKE_SOURCE_DIR}/core/luxlex.l ${CMAKE_BINARY_DIR}/luxlex.cpp)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/luxlex.cpp GENERATED)
	#SOURCE_GROUP("Parser Files" FILES core/luxlex.l)
	SET(lux_parser_src
		core/luxparse.y
		core/luxlex.l)
	SOURCE_GROUP("Parser Files" FILES ${lux_core_parser_src})

	ADD_FLEX_BISON_DEPENDENCY(LuxLexer LuxParser)
ENDIF (NOT BISON_NOT_AVAILABLE AND NOT FLEX_NOT_AVAILABLE)
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
	core/rendererstatistics.cpp
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

SET(lux_core_reflection_bsdf_src
	core/reflection/bsdf/layeredbsdf.cpp
	core/reflection/bsdf/mixbsdf.cpp
	core/reflection/bsdf/multibsdf.cpp
	core/reflection/bsdf/schlickbsdf.cpp
	core/reflection/bsdf/singlebsdf.cpp
	)
SOURCE_GROUP("Source Files\\Core\\Reflection\\BSDF" FILES ${lux_core_reflection_bsdf_src})

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
	${lux_core_reflection_bsdf_src}
	${lux_core_reflection_bxdf_src}
	${lux_core_reflection_fresnel_src}
	${lux_core_reflection_microfacetdistribution_src}
	)

#############################################################################

SET(lux_accelerators_src
	accelerators/bruteforce.cpp
	accelerators/bvhaccel.cpp
	accelerators/qbvhaccel.cpp
	accelerators/sqbvhaccel.cpp
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
	integrators/none.cpp
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
	materials/layeredmaterial.cpp
	materials/matte.cpp
	materials/mattetranslucent.cpp
	materials/metal.cpp
	materials/metal2.cpp
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
	renderers/sppm/parallelhashgrid.cpp
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

SET(lux_rendererstatistics_src
	renderers/statistics/samplerstatistics.cpp
	renderers/statistics/hybridsamplerstatistics.cpp
	renderers/statistics/sppmstatistics.cpp
	)
SOURCE_GROUP("Source Files\\Renderers\\Statistics" FILES ${lux_rendererstatistics_src})

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
	shapes/mikktspace/mikktspace.c
	shapes/mikktspace/weldmesh.c
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
	textures/fresnelcolor.cpp
	textures/sellmeiertexture.cpp
	textures/tabulatedfresnel.cpp
	)
SOURCE_GROUP("Source Files\\Textures\\Fresnel" FILES ${lux_fresnel_textures_src})

SET(lux_textures_src
	textures/band.cpp
	textures/bilerp.cpp
	textures/brick.cpp
	textures/checkerboard.cpp
	textures/colordepth.cpp
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
	${lux_rendererstatistics_src}
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

SET(lux_cpp_api_src
	cpp_api/dllmain.cpp
	cpp_api/lux_api.cpp
	cpp_api/lux_wrapper_factories.cpp
	)
SOURCE_GROUP("Source Files\\C++ API" FILES ${lux_cpp_api_src})



#############################################################################
#############################################################################
#####################  HEADER FILES FOR static liblux.a  ####################
#############################################################################
#############################################################################

SET(lux_core_hdr
	core/api.h
	core/bsh.h
	core/camera.h
	core/cameraresponse.h
	core/color.h
	core/context.h
	core/contribution.h
	core/dynload.h
	core/epsilon.h
	core/error.h
	core/exrio.h
	core/fastmutex.h
	core/filedata.h
	core/film.h
	core/filter.h
	core/igiio.h
	core/imagereader.h
	core/kdtree.h
	core/light.h
	core/lux.h
	core/material.h
	core/mc.h
	core/mcdistribution.h
	core/memory.h
	core/mipmap.h
	core/motionsystem.h
	core/octree.h
	core/osfunc.h
	core/paramset.h
	core/photonmap.h
	core/pngio.h
	core/primitive.h
	core/randomgen.h
	core/renderer.h
	core/rendererstatistics.h
	core/renderfarm.h
	core/renderinghints.h
	core/sampling.h
	core/scene.h
	core/shape.h
	core/spd.h
	core/spectrum.h
	core/spectrumwavelengths.h
	core/streamio.h
	core/texture.h
	core/texturecolor.h
	core/tgaio.h
	core/timer.h
	core/tonemap.h
	core/transport.h
	core/version.h
	core/volume.h
	server/renderserver.h
	)
SOURCE_GROUP("Header Files\\Core" FILES ${lux_core_hdr})
SET(lux_core_external_hdr
	core/external/cimg.h
	core/external/greycstoration.h
	)
SOURCE_GROUP("Header Files\\Core\\External" FILES ${lux_core_external_hdr})
SET(lux_core_geometry_hdr
	core/geometry/bbox.h
	core/geometry/matrix3x3.h
	core/geometry/matrix4x4-sse.h
	core/geometry/matrix4x4.h
	core/geometry/normal.h
	core/geometry/point.h
	core/geometry/quaternion.h
	core/geometry/ray.h
	core/geometry/raydifferential.h
	core/geometry/transform-sse.inl
	core/geometry/transform.h
	core/geometry/transform.inl
	core/geometry/vector.h
	core/geometry/vector_normal.h
	)
SOURCE_GROUP("Header Files\\Core\\Geometry" FILES ${lux_core_geometry_hdr})
SET(lux_core_queryable_hdr
	core/queryable/queryable.h
	core/queryable/queryableattribute.h
	core/queryable/queryableregistry.h
	)
SOURCE_GROUP("Header Files\\Core\\Queryable" FILES ${lux_core_queryable_hdr})
SET(lux_core_reflection_hdr
	core/reflection/bxdf.h
	core/reflection/fresnel.h
	core/reflection/microfacetdistribution.h
	)
SOURCE_GROUP("Header Files\\Core\\Reflection" FILES ${lux_core_reflection_hdr})
SET(lux_core_reflection_bsdf_hdr
	core/reflection/bsdf/layeredbsdf.h
	core/reflection/bsdf/mixbsdf.h
	core/reflection/bsdf/multibsdf.h
	core/reflection/bsdf/schlickbsdf.h
	core/reflection/bsdf/singlebsdf.h
	)
SOURCE_GROUP("Header Files\\Core\\Reflection\\BSDF" FILES ${lux_core_reflection_bsdf_hdr})
SET(lux_core_reflection_bxdf_hdr
	core/reflection/bxdf/asperity.h
	core/reflection/bxdf/brdftobtdf.h
	core/reflection/bxdf/cooktorrance.h
	core/reflection/bxdf/fresnelblend.h
	core/reflection/bxdf/lafortune.h
	core/reflection/bxdf/lambertian.h
	core/reflection/bxdf/microfacet.h
	core/reflection/bxdf/nulltransmission.h
	core/reflection/bxdf/orennayar.h
	core/reflection/bxdf/schlickbrdf.h
	core/reflection/bxdf/schlickscatter.h
	core/reflection/bxdf/schlicktranslucentbtdf.h
	core/reflection/bxdf/specularreflection.h
	core/reflection/bxdf/speculartransmission.h
	)
SOURCE_GROUP("Header Files\\Core\\Reflection\\BxDF" FILES ${lux_core_reflection_bxdf_hdr})
SET(lux_core_reflection_fresnel_hdr
	core/reflection/fresnel/fresnelcauchy.h
	core/reflection/fresnel/fresnelconductor.h
	core/reflection/fresnel/fresneldielectric.h
	core/reflection/fresnel/fresnelgeneral.h
	core/reflection/fresnel/fresnelnoop.h
	core/reflection/fresnel/fresnelslick.h
	)
SOURCE_GROUP("Header Files\\Core\\Reflection\\Fresnel" FILES ${lux_core_reflection_fresnel_hdr})
SET(lux_core_reflection_microfacetdistribution_hdr
	core/reflection/microfacetdistribution/anisotropic.h
	core/reflection/microfacetdistribution/beckmann.h
	core/reflection/microfacetdistribution/blinn.h
	core/reflection/microfacetdistribution/schlickdistribution.h
	core/reflection/microfacetdistribution/wardisotropic.h
	)
SOURCE_GROUP("Header Files\\Core\\Reflection\\Microfacet Distribution" FILES ${lux_core_reflection_microfacetdistribution_hdr})
SET(lux_accelerators_hdr
	accelerators/bruteforce.h
	accelerators/bvhaccel.h
	accelerators/qbvhaccel.h
	accelerators/tabreckdtreeaccel.h
	accelerators/unsafekdtreeaccel.h
	)
SOURCE_GROUP("Header Files\\Accelerators" FILES ${lux_accelerators_hdr})
SET(lux_cameras_hdr
	cameras/environment.h
	cameras/orthographic.h
	cameras/perspective.h
	cameras/realistic.h
	)
SOURCE_GROUP("Header Files\\Cameras" FILES ${lux_cameras_hdr})
SET(lux_cpp_api_hdr
	cpp_api/export_defs.h
	cpp_api/lux_api.h
	cpp_api/lux_instance.h
	cpp_api/lux_paramset.h
	)
SOURCE_GROUP("Header Files\\C++ API" FILES ${lux_cpp_api_hdr})
SET(lux_film_hdr
	film/fleximage.h
	)
SOURCE_GROUP("Header Files\\Film" FILES ${lux_film_hdr})
SET(lux_film_data_hdr
	film/data/cameraresponsefunctions.h
	)
SOURCE_GROUP("Header Files\\Film\\Data" FILES ${lux_film_data_hdr})
SET(lux_filters_hdr
	filters/box.h
	filters/gaussian.h
	filters/mitchell.h
	filters/sinc.h
	filters/triangle.h
	)
SOURCE_GROUP("Header Files\\Filters" FILES ${lux_filters_hdr})
SET(lux_integrators_hdr
	integrators/bidirectional.h
	integrators/directlighting.h
	integrators/distributedpath.h
	integrators/emission.h
	integrators/exphotonmap.h
	integrators/igi.h
	integrators/multi.h
	integrators/path.h
	integrators/single.h
	integrators/sppm.h
	)
SOURCE_GROUP("Header Files\\Integrators" FILES ${lux_integrators_hdr})
SET(lux_lights_hdr
	lights/distant.h
	lights/infinite.h
	lights/infinitesample.h
	lights/pointlight.h
	lights/projection.h
	lights/sky.h
	lights/spot.h
	lights/sun.h
	)
SOURCE_GROUP("Header Files\\Lights" FILES ${lux_lights_hdr})
SET(lux_lights_data_hdr
	lights/data/lamp_spect.h
	lights/data/skychroma_spect.h
	lights/data/sun_spect.h
	)
SOURCE_GROUP("Header Files\\Lights\\Data" FILES ${lux_lights_data_hdr})
SET(lux_lights_sphericalfunction_hdr
	lights/sphericalfunction/photometricdata_ies.h
	lights/sphericalfunction/sphericalfunction.h
	lights/sphericalfunction/sphericalfunction_ies.h
	)
SOURCE_GROUP("Header Files\\Lights\\Spherical Functions" FILES ${lux_lights_sphericalfunction_hdr})
SET(lux_materials_hdr
	materials/carpaint.h
	materials/glass.h
	materials/glass2.h
	materials/glossy.h
	materials/glossy2.h
	materials/glossytranslucent.h
	materials/layeredmaterial.h
	materials/matte.h
	materials/mattetranslucent.h
	materials/metal.h
	materials/metal2.h
	materials/mirror.h
	materials/mixmaterial.h
	materials/null.h
	materials/roughglass.h
	materials/scattermaterial.h
	materials/shinymetal.h
	materials/velvet.h
	)
SOURCE_GROUP("Header Files\\Materials" FILES ${lux_materials_hdr})
SET(lux_pixelsamplers_hdr
	pixelsamplers/hilbertpx.h
	pixelsamplers/linear.h
	pixelsamplers/lowdiscrepancypx.h
	pixelsamplers/tilepx.h
	pixelsamplers/vegas.h
	)
SOURCE_GROUP("Header Files\\Pixel Samplers" FILES ${lux_pixelsamplers_hdr})
SET(lux_renderers_hdr
	renderers/hybridrenderer.h
	renderers/hybridsamplerrenderer.h
	renderers/samplerrenderer.h
	renderers/sppmrenderer.h
	)
SOURCE_GROUP("Header Files\\Renderers" FILES ${lux_renderers_hdr})
SET(lux_rendererstatistics_hdr
	renderers/statistics/samplerstatistics.h
	renderers/statistics/hybridsamplerstatistics.h
	renderers/statistics/sppmstatistics.h
	)
SOURCE_GROUP("Header Files\\Renderers\\Statistics" FILES ${lux_rendererstatistics_hdr})
SET(lux_renderers_sppm_hdr
	renderers/sppm/hitpoints.h
	renderers/sppm/lookupaccel.h
	renderers/sppm/photonsampler.h
	)
SOURCE_GROUP("Header Files\\Renderers\\SPPM" FILES ${lux_renderers_sppm_hdr})
SET(lux_samplers_hdr
	samplers/erpt.h
	samplers/lowdiscrepancy.h
	samplers/metrosampler.h
	samplers/random.h
	)
SOURCE_GROUP("Header Files\\Samplers" FILES ${lux_samplers_hdr})
SET(lux_shapes_hdr
	shapes/cone.h
	shapes/cylinder.h
	shapes/disk.h
	shapes/heightfield.h
	shapes/hyperboloid.h
	shapes/lenscomponent.h
	shapes/loopsubdiv.h
	shapes/mesh.h
	shapes/mikktspace/mikktspace.h
	shapes/mikktspace/weldmesh.h
	shapes/nurbs.h
	shapes/paraboloid.h
	shapes/plymesh.h
	shapes/plymesh/rply.h
	shapes/sphere.h
	shapes/stlmesh.h
	shapes/torus.h
	)
SOURCE_GROUP("Header Files\\Shapes" FILES ${lux_shapes_hdr})
SET(lux_spds_hdr
	spds/blackbodyspd.h
	spds/equalspd.h
	spds/frequencyspd.h
	spds/gaussianspd.h
	spds/irregular.h
	spds/regular.h
	spds/rgbillum.h
	spds/rgbrefl.h
	)
SOURCE_GROUP("Header Files\\SPDs" FILES ${lux_spds_hdr})
SET(lux_spds_data_hdr
	spds/data/rgbD65_32.h
	spds/data/rgbE_32.h
	spds/data/xyzbasis.h
	)
SOURCE_GROUP("Header Files\\SPDs\\Data" FILES ${lux_spds_data_hdr})
SET(lux_textures_hdr
	textures/band.h
	textures/bilerp.h
	textures/brick.h
	textures/checkerboard.h
	textures/colordepth.h
	textures/dots.h
	textures/fbm.h
	textures/harlequin.h
	textures/imagemap.h
	textures/marble.h
	textures/mix.h
	textures/multimix.h
	textures/scale.h
	textures/uv.h
	textures/uvmask.h
	textures/windy.h
	textures/wrinkled.h
	)
SOURCE_GROUP("Header Files\\Textures" FILES ${lux_textures_hdr})
SET(lux_textures_blender_hdr
	textures/blender_base.h
	textures/blender_blend.h
	textures/blender_clouds.h
	textures/blender_distortednoise.h
	textures/blender_magic.h
	textures/blender_marble.h
	textures/blender_musgrave.h
	textures/blender_noise.h
	textures/blender_noiselib.h
	textures/blender_stucci.h
	textures/blender_texlib.h
	textures/blender_voronoi.h
	textures/blender_wood.h
	)
SOURCE_GROUP("Header Files\\Textures\\Blender" FILES ${lux_textures_blender_hdr})
SET(lux_textures_uniform_hdr
	textures/blackbody.h
	textures/constant.h
	textures/equalenergy.h
	textures/frequencytexture.h
	textures/gaussiantexture.h
	textures/irregulardata.h
	textures/lampspectrum.h
	textures/regulardata.h
	textures/tabulateddata.h
	)
SOURCE_GROUP("Header Files\\Textures\\Uniform" FILES ${lux_textures_uniform_hdr})
SET(lux_textures_fresnel_hdr
	textures/cauchytexture.h
	textures/fresnelcolor.h
	textures/sellmeiertexture.h
	textures/tabulatedfresnel.h
	)
SOURCE_GROUP("Header Files\\Textures\\Fresnel" FILES ${lux_textures_fresnel_hdr})
SET(lux_tonemaps_hdr
	tonemaps/contrast.h
	tonemaps/lineartonemap.h
	tonemaps/maxwhite.h
	tonemaps/nonlinear.h
	tonemaps/reinhard.h
	)
SOURCE_GROUP("Header Files\\Tonemaps" FILES ${lux_tonemaps_hdr})
SET(lux_volumes_hdr
	volumes/clearvolume.h
	volumes/cloud.h
	volumes/exponential.h
	volumes/homogeneous.h
	volumes/volumegrid.h
	)
SOURCE_GROUP("Header Files\\Volumes" FILES ${lux_volumes_hdr})

SET(lux_lib_hdr
	${lux_core_hdr}
	${lux_core_external_hdr}
	${lux_core_geometry_hdr}
	${lux_core_queryable_hdr}
	${lux_core_reflection_hdr}
	${lux_core_reflection_bsdf_hdr}
	${lux_core_reflection_bxdf_hdr}
	${lux_core_reflection_fresnel_hdr}
	${lux_core_reflection_microfacetdistribution_hdr}
	${lux_accelerators_hdr}
	${lux_cameras_hdr}
	${lux_cpp_api_hdr}
	${lux_film_hdr}
	${lux_film_data_hdr}
	${lux_filters_hdr}
	${lux_integrators_hdr}
	${lux_lights_hdr}
	${lux_lights_data_hdr}
	${lux_lights_sphericalfunction_hdr}
	${lux_materials_hdr}
	${lux_pixelsamplers_hdr}
	${lux_renderers_hdr}
	${lux_renderers_sppm_hdr}
	${lux_rendererstatistics_hdr}
	${lux_sampelrs_hdr}
	${lux_shapes_hdr}
	${lux_spds_hdr}
	${lux_spds_data_hdr}
	${lux_textures_hdr}
	${lux_textures_blender_hdr}
	${lux_textures_uniform_hdr}
	${lux_textures_fresnel_hdr}
	${lux_tonemaps_hdr}
	${lux_volumes_hdr}
	)


#############################################################################

INCLUDE_DIRECTORIES(SYSTEM
	${CMAKE_SOURCE_DIR}/core/external
	)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core
	${CMAKE_SOURCE_DIR}/core/queryable
	${CMAKE_SOURCE_DIR}/core/reflection
	${CMAKE_SOURCE_DIR}/core/reflection/bsdf
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
#IF(NOT APPLE)
#	ADD_LIBRARY(luxStatic STATIC ${lux_lib_src} ${lux_lib_hdr} ${lux_parser_src})
#	#TARGET_LINK_LIBRARIES(luxStatic ${FREEIMAGE_LIBRARIES} ${Boost_LIBRARIES} )
#	SET_TARGET_PROPERTIES(luxStatic PROPERTIES OUTPUT_NAME lux)
#ENDIF(NOT APPLE)

#############################################################################
# Here we build the shared core library liblux.so
#############################################################################
IF(APPLE)
	ADD_LIBRARY(luxShared SHARED ${lux_cpp_api_src} ${lux_lib_src} ${lux_lib_hdr} ${lux_parser_src})
	TARGET_LINK_LIBRARIES(luxShared ${LUX_LIBRARY_DEPENDS})
	SET_TARGET_PROPERTIES(luxShared PROPERTIES OUTPUT_NAME lux)
	SET_TARGET_PROPERTIES(luxShared PROPERTIES DEFINE_SYMBOL LUX_INTERNAL) # for controlling visibility

	if(${CMAKE_GENERATOR} MATCHES "Xcode")
		SET_TARGET_PROPERTIES(luxShared PROPERTIES XCODE_ATTRIBUTE_LD_DYLIB_INSTALL_NAME @loader_path/liblux.dylib)
	else()
		ADD_CUSTOM_COMMAND(
			TARGET luxShared POST_BUILD
			COMMAND install_name_tool -id @loader_path/liblux.dylib ${CMAKE_BUILD_TYPE}/liblux.dylib)
	endif()
ELSEIF(MSVC)
	ADD_LIBRARY(luxShared SHARED ${lux_lib_src} ${lux_lib_hdr} ${lux_parser_src})
	TARGET_LINK_LIBRARIES(luxShared ${LUX_LIBRARY} ${LUX_LIBRARY_DEPENDS})
	# Make CMake output both libs with the same name
	SET_TARGET_PROPERTIES(luxShared PROPERTIES OUTPUT_NAME lux)
	SET_TARGET_PROPERTIES(luxShared PROPERTIES DEFINE_SYMBOL LUX_INTERNAL)
ELSE(APPLE)
	ADD_LIBRARY(luxShared SHARED ${lux_cpp_api_src} ${lux_lib_src} ${lux_lib_hdr} ${lux_parser_src})
	TARGET_LINK_LIBRARIES(luxShared ${LUX_LIBRARY_DEPENDS})
	SET_TARGET_PROPERTIES(luxShared PROPERTIES OUTPUT_NAME lux)
	SET_TARGET_PROPERTIES(luxShared PROPERTIES DEFINE_SYMBOL LUX_INTERNAL) # for controlling visibility
ENDIF(APPLE)


#ADD_CUSTOM_TARGET(luxStatic SOURCES ${lux_lib_hdr})
