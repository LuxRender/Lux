
###########################################################################
#
# Configuration
#
###########################################################################

MESSAGE(STATUS "Using LordCrc's Configuration settings")


SET ( DEPS_HOME               "E:/Dev/LuxRender/deps" )
IF(ARCH_X86_64)
	SET ( DEPS_BITS       "x64" )
ELSE(ARCH_X86_64)
	SET ( DEPS_BITS       "x86" )
ENDIF(ARCH_X86_64)

SET ( DEPS_ROOT               "${DEPS_HOME}/${DEPS_BITS}")

SET ( ENV{QTDIR}              "${DEPS_ROOT}/qt-everywhere-opensource-src-4.7.2")
SET ( ENV{LuxRays_HOME}       "${lux_SOURCE_DIR}/../luxrays")


SET(FREEIMAGE_SEARCH_PATH     "${DEPS_ROOT}/FreeImage3141/FreeImage")
SET(FreeImage_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/source")
SET(FreeImage_LIB_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/release"
                              "${FREEIMAGE_SEARCH_PATH}/debug"
                              "${FREEIMAGE_SEARCH_PATH}/dist")
ADD_DEFINITIONS(-DFREEIMAGE_LIB)
SET(FREEIMAGE_ROOT            "${FREEIMAGE_SEARCH_PATH}")

SET ( OPENEXR_ROOT            "${FREEIMAGE_SEARCH_PATH}/Source/OpenEXR")
SET ( OpenEXR_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/Source/OpenEXR")


SET ( PNG_ROOT                "${FREEIMAGE_SEARCH_PATH}/Source/LibPNG")
SET ( PNG_INC_SEARCH_PATH     "${FREEIMAGE_SEARCH_PATH}/Source/LibPNG")

SET ( ZLIB_ROOT                "${FREEIMAGE_SEARCH_PATH}/Source/Zlib")
SET ( ZLIB_INC_SEARCH_PATH     "${FREEIMAGE_SEARCH_PATH}/Source/Zlib")


set(BOOST_SEARCH_PATH         "${DEPS_ROOT}/boost_1_43_0")
set(BOOST_LIBRARYDIR          "${BOOST_SEARCH_PATH}/stage/boost/lib")
set(BOOST_python_LIBRARYDIR   "${BOOST_SEARCH_PATH}/stage/python3/lib")
SET(BOOST_ROOT                "${BOOST_SEARCH_PATH}")

set(OPENCL_SEARCH_PATH        "$ENV{AMDAPPSDKROOT}")
set(OPENCL_LIBRARYDIR         "${OPENCL_SEARCH_PATH}/lib/x86_64")
SET(OPENCL_ROOT               "${OPENCL_SEARCH_PATH}")


set(GLUT_SEARCH_PATH          "${DEPS_ROOT}/freeglut-2.6.0")
set(GLUT_LIBRARYDIR           "${GLUT_SEARCH_PATH}/VisualStudio2008Static/x64/Release")
ADD_DEFINITIONS(-DFREEGLUT_STATIC)

set(GLEW_SEARCH_PATH          "${DEPS_ROOT}/glew-1.5.5")
ADD_DEFINITIONS(-DGLEW_STATIC)

INCLUDE_DIRECTORIES ( SYSTEM "${DEPS_HOME}/include" ) # for unistd.h