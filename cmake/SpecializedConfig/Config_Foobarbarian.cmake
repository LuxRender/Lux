
###########################################################################
#
# Configuration
#
###########################################################################

MESSAGE ( STATUS "Using Foobarbarians Configuration settings" )


SET ( DEPS_HOME                "d:/ExtProjects/luxtest" )

IF ( ARCH_X86_64 )
  SET ( DEPS_BITS              "x64" )
ELSE ( ARCH_X86_64 )
  SET ( DEPS_BITS              "x86" )
ENDIF ( ARCH_X86_64 )

SET ( DEPS_ROOT                "${DEPS_HOME}" )

SET ( ENV{QTDIR}               "d:/qt/4.7.3" )
SET ( LuxRays_HOME             "${DEPS_HOME}/../luxrays" )

SET( FREEIMAGE_SEARCH_PATH     "${DEPS_ROOT}/../FreeImage" )
SET( FreeImage_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/source" )
SET( FreeImage_LIB_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/release"
                               "${FREEIMAGE_SEARCH_PATH}/debug"
                               "${FREEIMAGE_SEARCH_PATH}/dist" )
ADD_DEFINITIONS ( -DFREEIMAGE_LIB )
SET ( FREEIMAGE_ROOT           "${FREEIMAGE_SEARCH_PATH}" )

SET ( OPENEXR_ROOT             "${FREEIMAGE_SEARCH_PATH}/Source/OpenEXR" )
SET ( OpenEXR_INC_SEARCH_PATH  "${FREEIMAGE_SEARCH_PATH}/Source/OpenEXR" )


SET ( PNG_ROOT                 "${FREEIMAGE_SEARCH_PATH}/Source/LibPNG" )
SET ( PNG_INC_SEARCH_PATH      "${FREEIMAGE_SEARCH_PATH}/Source/LibPNG" )

SET ( ZLIB_ROOT                "${FREEIMAGE_SEARCH_PATH}/Source/Zlib" )
SET ( ZLIB_INC_SEARCH_PATH     "${FREEIMAGE_SEARCH_PATH}/Source/Zlib" )


SET ( BOOST_SEARCH_PATH        "${DEPS_ROOT}/../boost" )
SET ( BOOST_LIBRARYDIR         "${BOOST_SEARCH_PATH}/stage/lib" )
SET ( BOOST_python_LIBRARYDIR  "${BOOST_SEARCH_PATH}/stage/lib" )
SET ( BOOST_ROOT               "${BOOST_SEARCH_PATH}" )

SET ( PYTHON_CUSTOM            TRUE )
SET ( PYTHON_HOME              "d:/python32" )
SET ( PYTHON_LIBRARIES         "${PYTHON_HOME}/libs/python32.lib" )
SET ( PYTHON_INCLUDE_PATH      "${PYTHON_HOME}/include" )


SET ( OPENCL_SEARCH_PATH       "${DEPS_ROOT}/../opencl" )
SET ( OPENCL_LIBRARYDIR        "${OPENCL_SEARCH_PATH}/CL" )
SET ( OPENCL_ROOT              "${OPENCL_SEARCH_PATH}" )


SET ( GLUT_SEARCH_PAT          "${DEPS_ROOT}/../freeglut" )
SET ( GLUT_LIBRARYDIR          "${GLUT_SEARCH_PATH}/lib" )
ADD_DEFINITIONS ( -DFREEGLUT_STATIC )

SET ( GLEW_SEARCH_PATH         "${DEPS_ROOT}/../glew" )
ADD_DEFINITIONS ( -DGLEW_STATIC )

INCLUDE_DIRECTORIES ( SYSTEM "${DEPS_HOME}/include" ) # for unistd.h
