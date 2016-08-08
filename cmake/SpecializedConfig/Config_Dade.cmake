
###########################################################################
#
# Configuration
#
###########################################################################

#cmake -DLUX_CUSTOM_CONFIG=cmake/SpecializedConfig/Config_Dade.cmake -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.4m.so -DPYTHON_INCLUDE_DIR=/usr/include/python3.4m -DPYTHON_INCLUDE_DIR2=/usr/include/python3.4m .

MESSAGE(STATUS "Using Dade Configuration settings")

SET(LuxRays_HOME			"../luxrays")

SET(OPENCL_SEARCH_PATH		"$ENV{AMDAPPSDKROOT}")
SET(OPENCL_INCLUDE_DIR		"${OPENCL_SEARCH_PATH}/include")
#SET(OPENCL_LIBRARYDIR		"${OPENCL_SEARCH_PATH}/lib/x86_64")
SET(OPENIMAGEIO_ROOT_DIR	"/home/david/projects/luxrender-dev/oiio/dist/linux64")
#SET(OPENEXR_ROOT			"/usr/local")
SET(EMBREE_SEARCH_PATH		"/home/david/projects/luxrender-dev/embree-dade")

SET(BOOST_SEARCH_PATH		"/home/david/projects/luxrender-dev/boost_1_56_0-bin")
SET(BOOST_LIBRARYDIR		"${BOOST_SEARCH_PATH}")
SET(BOOST_python_LIBRARYDIR	"${BOOST_SEARCH_PATH}")
SET(BOOST_ROOT				"${BOOST_SEARCH_PATH}")

#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")
