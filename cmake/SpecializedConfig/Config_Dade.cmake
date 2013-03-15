
###########################################################################
#
# Configuration
#
###########################################################################

#cmake -DLUX_CUSTOM_CONFIG=cmake/SpecializedConfig/Config_Dade.cmake .

MESSAGE(STATUS "Using Dade Configuration settings")

SET(LuxRays_HOME		"../luxrays")

SET(OPENCL_SEARCH_PATH		"$ENV{ATISTREAMSDKROOT}")
SET(OPENCL_INCLUDE_DIR		"${OPENCL_SEARCH_PATH}/include")
#SET(OPENCL_LIBRARYDIR		"${OPENCL_SEARCH_PATH}/lib/x86_64")

#SET(BOOST_SEARCH_PATH		"/home/david/projects/luxrender-dev/boost_1_43_0")
#SET(BOOST_LIBRARYDIR		"${BOOST_SEARCH_PATH}")
#SET(BOOST_python_LIBRARYDIR	"${BOOST_SEARCH_PATH}")
#SET(BOOST_ROOT			"${BOOST_SEARCH_PATH}")

#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")
