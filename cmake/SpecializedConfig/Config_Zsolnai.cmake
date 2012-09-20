
###########################################################################
#
# Configuration
#
###########################################################################

MESSAGE(STATUS "Using Karoly Zsolnai's Configuration settings")


SET ( DEPS_HOME               "d:/lux/deps" )
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


SET(BOOST_ROOT                "${DEPS_ROOT}/boost_1_47_0")
SET(BOOST_LIBRARYDIR          "${BOOST_ROOT}/stage/boost/lib")
SET(BOOST_python_LIBRARYDIR   "${BOOST_ROOT}/stage/python3/lib")


#SET(OPENCL_ROOT               "$ENV{AMDAPPSDKROOT}")
#SET(OPENCL_LIBRARYDIR         "${OPENCL_ROOT}/lib/x86_64")
set(OPENCL_SEARCH_PATH        "c:/CUDA/v4.1")
set(OPENCL_LIBRARYDIR         "${OPENCL_SEARCH_PATH}/lib/Win32") 


SET(GLUT_ROOT                 "${DEPS_ROOT}/freeglut-2.6.0")
#SET(GLUT_LIBRARYDIR           "${GLUT_ROOT}/VisualStudio2008Static/x64/Release")
SET(GLUT_LIBRARYDIR           "${GLUT_ROOT}/VisualStudio2008Static/Win32/Release")
ADD_DEFINITIONS(-DFREEGLUT_STATIC)

set(GLEW_SEARCH_PATH          "${DEPS_ROOT}/glew-1.5.5")
ADD_DEFINITIONS(-DGLEW_STATIC)

#INCLUDE_DIRECTORIES ( SYSTEM "${DEPS_HOME}/include" ) # for unistd.h
INCLUDE_DIRECTORIES ( SYSTEM "d:/lux/windows/include" ) # for unistd.h
ADD_DEFINITIONS(-DNOMINMAX)
