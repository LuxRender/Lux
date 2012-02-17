
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


SET(FREEIMAGE_ROOT            "${DEPS_ROOT}/FreeImage3141/FreeImage")
SET(OPENEXR_ROOT              "${FREEIMAGE_ROOT}/Source/OpenEXR")
SET(PNG_ROOT                  "${FREEIMAGE_ROOT}/Source/LibPNG")
SET(ZLIB_ROOT                 "${FREEIMAGE_ROOT}/Source/Zlib")
ADD_DEFINITIONS(-DFREEIMAGE_LIB)


SET(BOOST_ROOT                "${DEPS_ROOT}/boost_1_43_0")
SET(BOOST_LIBRARYDIR          "${BOOST_ROOT}/stage/boost/lib")
SET(BOOST_python_LIBRARYDIR   "${BOOST_ROOT}/stage/python3/lib")


SET(OPENCL_ROOT               "$ENV{AMDAPPSDKROOT}")
SET(OPENCL_LIBRARYDIR         "${OPENCL_ROOT}/lib/x86_64")


SET(GLUT_ROOT                 "${DEPS_ROOT}/freeglut-2.6.0")
SET(GLUT_LIBRARYDIR           "${GLUT_ROOT}/VisualStudio2008Static/x64/Release")
ADD_DEFINITIONS(-DFREEGLUT_STATIC)

set(GLEW_ROOT                 "${DEPS_ROOT}/glew-1.5.5")
ADD_DEFINITIONS(-DGLEW_STATIC)

INCLUDE_DIRECTORIES ( SYSTEM "${DEPS_HOME}/include" ) # for unistd.h