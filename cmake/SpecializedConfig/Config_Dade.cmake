
###########################################################################
#
# Configuration
#
###########################################################################

#cmake -DLUX_CUSTOM_CONFIG=cmake/SpecializedConfig/Config_Dade.cmake .

MESSAGE ( STATUS "Using Dade Configuration settings" )

SET ( LuxRays_HOME             "${DEPS_HOME}/../luxrays" )

SET ( OPENCL_SEARCH_PATH       "/home/david/src/AMD-APP-SDK-v2.6-RC3-lnx64" )
SET ( OPENCL_LIBRARYDIR        "${OPENCL_SEARCH_PATH}/include" )
SET ( OPENCL_ROOT              "${OPENCL_SEARCH_PATH}" )

SET ( BOOST_SEARCH_PATH        "/home/david/projects/luxrender-dev/boost_1_43_0" )
SET ( BOOST_LIBRARYDIR         "${BOOST_SEARCH_PATH}" )
SET ( BOOST_python_LIBRARYDIR  "${BOOST_SEARCH_PATH}" )
SET ( BOOST_ROOT               "${BOOST_SEARCH_PATH}" )

set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")
