
###########################################################################
#
# Configuration
#
###########################################################################

#cmake -DLUX_CUSTOM_CONFIG=cmake/SpecializedConfig/Config_Aldo.cmake .

MESSAGE(STATUS "Using Aldo Configuration settings")

SET(LuxRays_HOME			"${DEPS_HOME}/../luxrays")

#SET(OPENCL_SEARCH_PATH		"$ENV{ATISTREAMSDKROOT}")
SET(OPENCL_INCLUDE_DIR		"/usr/include/nvidia-current")
SET(OPENCL_LIBRARYDIR		"/usr/lib/nvidia-current")

#SET(BOOST_SEARCH_PATH			"/home/david/projects/luxrender-dev/boost_1_43_0")
#SET(BOOST_LIBRARYDIR			"${BOOST_SEARCH_PATH}")
#SET(BOOST_python_LIBRARYDIR	"${BOOST_SEARCH_PATH}")
#SET(BOOST_ROOT					"${BOOST_SEARCH_PATH}")

#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")
