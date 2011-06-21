
###########################################################################
#
# Configuration
#
###########################################################################

MESSAGE(STATUS "Using LordCrc's Configuration settings")


SET ( DEPS_HOME               "E:/Dev/Lux2010/deps_2010" )
SET ( DEPS_BITS               "x64" )
SET ( DEPS_ROOT               "${DEPS_HOME}/${DEPS_BITS}")

SET ( ENV{QTDIR}              "${DEPS_ROOT}/qt-everywhere-opensource-src-4.7.2")
SET ( ENV{LuxRays_HOME}       "${lux_SOURCE_DIR}/../luxrays")


set(FREEIMAGE_SEARCH_PATH     "${DEPS_ROOT}/FreeImage3150/FreeImage")
set(FreeImage_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/source")
set(FreeImage_LIB_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/release"
                              "${FREEIMAGE_SEARCH_PATH}/debug"
                              "${FREEIMAGE_SEARCH_PATH}/dist")
ADD_DEFINITIONS(-DFREEIMAGE_LIB)

SET ( ENV{OpenEXR_HOME}       "${FREEIMAGE_SEARCH_PATH}/Source/OpenEXR")
SET ( OpenEXR_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/Source/OpenEXR")


SET ( ENV{PNG_HOME}           "${FREEIMAGE_SEARCH_PATH}/Source/LibPNG")
SET ( PNG_INC_SEARCH_PATH     "${FREEIMAGE_SEARCH_PATH}/Source/LibPNG")

set(BOOST_SEARCH_PATH         "${DEPS_ROOT}/boost_1_43_0")
set(BOOST_LIBRARYDIR          "${BOOST_SEARCH_PATH}/stage/boost/lib")

set(OPENCL_SEARCH_PATH        "$ENV{AMDAPPSDKROOT}")
set(OPENCL_LIBRARYDIR         "${OPENCL_SEARCH_PATH}/lib/x86_64")

INCLUDE_DIRECTORIES ( SYSTEM "${FREEIMAGE_SEARCH_PATH}/Source/Zlib" )
INCLUDE_DIRECTORIES ( SYSTEM "${DEPS_HOME}/include" ) # for unistd.h