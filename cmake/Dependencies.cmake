###########################################################################
#   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  #
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
##########################      Find LuxRays       ##########################
#############################################################################
#############################################################################

IF(APPLE)
	FIND_PATH(LUXRAYS_INCLUDE_DIRS NAMES luxrays/luxrays.h PATHS ${OSX_DEPENDENCY_ROOT}/include/LuxRays)
	FIND_LIBRARY(LUXRAYS_LIBRARY libluxrays.a ${OSX_DEPENDENCY_ROOT}/lib/LuxRays)
ELSE(APPLE)
	FIND_PATH(LUXRAYS_INCLUDE_DIRS NAMES luxrays/luxrays.h PATHS ${LuxRays_HOME}/include ../luxrays/include  )
	FIND_LIBRARY(LUXRAYS_LIBRARY luxrays PATHS ${LuxRays_HOME}/lib ../luxrays/lib PATH_SUFFIXES "" release relwithdebinfo minsizerel dist )
ENDIF(APPLE)

IF (LUXRAYS_INCLUDE_DIRS AND LUXRAYS_LIBRARY)
	MESSAGE(STATUS "LuxRays include directory: " ${LUXRAYS_INCLUDE_DIRS})
	MESSAGE(STATUS "LuxRays library directory: " ${LUXRAYS_LIBRARY})
	INCLUDE_DIRECTORIES(SYSTEM ${LUXRAYS_INCLUDE_DIRS})
ELSE (LUXRAYS_INCLUDE_DIRS AND LUXRAYS_LIBRARY)
	MESSAGE(FATAL_ERROR "LuxRays not found.")
ENDIF (LUXRAYS_INCLUDE_DIRS AND LUXRAYS_LIBRARY)


#############################################################################
#############################################################################
########################        Find LuxCore         ########################
#############################################################################
#############################################################################

IF(APPLE)
	FIND_PATH(LUXCORE_INCLUDE_DIRS NAMES luxcore/luxcore.h PATHS ${OSX_DEPENDENCY_ROOT}/include/LuxRays)
	FIND_LIBRARY(LUXCORE_LIBRARY libluxcore.a ${OSX_DEPENDENCY_ROOT}/lib/LuxRays)
	FIND_PATH(SLG_INCLUDE_DIRS NAMES slg/slg.h PATHS ${OSX_DEPENDENCY_ROOT}/include/LuxRays)
	FIND_LIBRARY(SLG_LIBRARY libsmallluxgpu.a ${OSX_DEPENDENCY_ROOT}/lib/LuxRays)
ELSE(APPLE)
	FIND_PATH(LUXCORE_INCLUDE_DIRS NAMES luxcore/luxcore.h PATHS ${LuxRays_HOME}/include ../luxrays/include)
	FIND_LIBRARY(LUXCORE_LIBRARY luxcore PATHS ${LuxRays_HOME}/lib ../luxrays/lib PATH_SUFFIXES "" release relwithdebinfo minsizerel dist)
	FIND_PATH(SLG_INCLUDE_DIRS NAMES slg/slg.h PATHS ${LuxRays_HOME}/include ../luxrays/include)
	FIND_LIBRARY(SLG_LIBRARY smallluxgpu PATHS ${LuxRays_HOME}/lib ../luxrays/lib PATH_SUFFIXES "" release relwithdebinfo minsizerel dist )
ENDIF(APPLE)

IF (SLG_INCLUDE_DIRS AND SLG_LIBRARY)
	MESSAGE(STATUS "SLG include directory: " ${SLG_INCLUDE_DIRS})
	MESSAGE(STATUS "SLG library directory: " ${SLG_LIBRARY})
	INCLUDE_DIRECTORIES(SYSTEM ${SLG_INCLUDE_DIRS})
ELSE (SLG_INCLUDE_DIRS AND SLG_LIBRARY)
	MESSAGE(FATAL_ERROR "SLG Library not found.")
ENDIF (SLG_INCLUDE_DIRS AND SLG_LIBRARY)

IF (LUXCORE_INCLUDE_DIRS AND LUXCORE_LIBRARY)
	MESSAGE(STATUS "LuxCore include directory: " ${LUXCORE_INCLUDE_DIRS})
	MESSAGE(STATUS "LuxCore library directory: " ${LUXCORE_LIBRARY})
	INCLUDE_DIRECTORIES(SYSTEM ${LUXCORE_INCLUDE_DIRS})
ELSE (LUXCORE_INCLUDE_DIRS AND LUXCORE_LIBRARY)
	MESSAGE(FATAL_ERROR "LuxCore Library not found.")
ENDIF (LUXCORE_INCLUDE_DIRS AND LUXCORE_LIBRARY)

#############################################################################
#############################################################################
###########################      Find OpenCL       ##########################
#############################################################################
#############################################################################

IF(LUXRAYS_DISABLE_OPENCL)
	SET(OPENCL_LIBRARIES "")
ELSE(LUXRAYS_DISABLE_OPENCL)
	IF(NOT OPENCL_ROOT)
		SET(OPENCL_ROOT /usr/src/opencl-sdk)
	ENDIF(NOT OPENCL_ROOT)
	FIND_PACKAGE(OpenCL)

	IF (OPENCL_FOUND)
		MESSAGE(STATUS "OpenCL include directory: " ${OPENCL_INCLUDE_DIR})
		MESSAGE(STATUS "OpenCL library: " ${OPENCL_LIBRARIES})
		INCLUDE_DIRECTORIES(SYSTEM ${OPENCL_INCLUDE_DIR})
	ELSE (OPENCL_FOUND)
		MESSAGE(FATAL_ERROR "OpenCL not found, try to compile with LUXRAYS_DISABLE_OPENCL=ON")
	ENDIF (OPENCL_FOUND)
ENDIF(LUXRAYS_DISABLE_OPENCL)


#############################################################################
#############################################################################
###########################      Find OpenGL       ##########################
#############################################################################
#############################################################################

FIND_PACKAGE(OpenGL)
IF (OPENGL_FOUND)
	MESSAGE(STATUS "OpenGL include directory: " "${OPENGL_INCLUDE_DIRS}")
	MESSAGE(STATUS "OpenGL library: " "${OPENGL_LIBRARY}")
	INCLUDE_DIRECTORIES(SYSTEM ${OPENGL_INCLUDE_DIRS})
ELSE (OPENGL_FOUND)
	MESSAGE(FATAL_ERROR "OpenGL not found.")
ENDIF (OPENGL_FOUND)


#############################################################################
#############################################################################
########################      Find Intel Embree       #######################
#############################################################################
#############################################################################

SET(EMBREE_ROOT "${EMBREE_SEARCH_PATH}")
FIND_PACKAGE(Embree)

IF (EMBREE_FOUND)
	INCLUDE_DIRECTORIES(SYSTEM ${EMBREE_INCLUDE_PATH})
ELSE (EMBREE_FOUND)
	MESSAGE(FATAL_ERROR "Intel Embree not found.")
endif (EMBREE_FOUND)


#############################################################################
#############################################################################
###########################      Find OpenMP       ##########################
#############################################################################
#############################################################################

IF(NOT APPLE)
	FIND_PACKAGE(OpenMP)
	IF (OPENMP_FOUND)
		MESSAGE(STATUS "OpenMP found - compiling with")
	    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	ELSE(OPENMP_FOUND)
		MESSAGE(WARNING "OpenMP not found - compiling without")
	endif(OPENMP_FOUND)
endif()

#############################################################################
#############################################################################
###########################      Find BISON       ###########################
#############################################################################
#############################################################################

IF (NOT BISON_NOT_AVAILABLE)
	FIND_PACKAGE(BISON REQUIRED)
	IF (NOT BISON_FOUND)
		MESSAGE(FATAL_ERROR "bison not found - aborting")
	ENDIF (NOT BISON_FOUND)
ENDIF (NOT BISON_NOT_AVAILABLE)


#############################################################################
#############################################################################
###########################      Find FLEX        ###########################
#############################################################################
#############################################################################

IF (NOT FLEX_NOT_AVAILABLE)
	FIND_PACKAGE(FLEX REQUIRED)
	IF (NOT FLEX_FOUND)
		MESSAGE(FATAL_ERROR "flex not found - aborting")
	ENDIF (NOT FLEX_FOUND)
ENDIF (NOT FLEX_NOT_AVAILABLE)


#############################################################################
#############################################################################
########################### BOOST LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

IF(APPLE)
	SET(BOOST_ROOT ${OSX_DEPENDENCY_ROOT})
ENDIF(APPLE)
SET(Boost_MINIMUM_VERSION "1.44.0")
SET(Boost_ADDITIONAL_VERSIONS "1.47.0" "1.46.1" "1.46.0" "1.46" "1.45.0" "1.45" "1.44.0" "1.44" )
SET(Boost_COMPONENTS thread program_options filesystem serialization iostreams regex system)
IF(WIN32)
	SET(Boost_COMPONENTS ${Boost_COMPONENTS} zlib)
	SET(Boost_USE_STATIC_LIBS ON)
	SET(Boost_USE_MULTITHREADED ON)
	SET(Boost_USE_STATIC_RUNTIME OFF)
	SET(BOOST_ROOT "${BOOST_SEARCH_PATH}")
ENDIF(WIN32)

IF(MSVC AND BOOST_python_LIBRARYDIR)
	SET(_boost_libdir "${BOOST_LIBRARYDIR}")
	SET(BOOST_LIBRARYDIR "${BOOST_python_LIBRARYDIR}")
ENDIF(MSVC AND BOOST_python_LIBRARYDIR)

FIND_PACKAGE(Boost ${Boost_MINIMUM_VERSION} COMPONENTS python REQUIRED)

IF(MSVC AND BOOST_python_LIBRARYDIR)
	SET(BOOST_LIBRARYDIR "${_boost_libdir}")
	SET(_boost_libdir)
ENDIF(MSVC AND BOOST_python_LIBRARYDIR)

SET(Boost_python_FOUND ${Boost_FOUND})
SET(Boost_python_LIBRARIES ${Boost_LIBRARIES})
SET(Boost_FOUND)
SET(Boost_LIBRARIES)

FIND_PACKAGE(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${Boost_COMPONENTS} REQUIRED)

IF(APPLE)
	# the searchmacro always gives confusing "NOTFOUND" on macos repo
	SET(Boost_DIR ${OSX_DEPENDENCY_ROOT} CACHE STRING "x" FORCE)
ENDIF(APPLE)

IF(Boost_FOUND)
	MESSAGE(STATUS "Boost library directory: " ${Boost_LIBRARY_DIRS})
	MESSAGE(STATUS "Boost include directory: " ${Boost_INCLUDE_DIRS})

	# Don't use old boost versions interfaces
	ADD_DEFINITIONS(-DBOOST_FILESYSTEM_NO_DEPRECATED)
	# Retain compatibility with Boost 1.44 and 1.45
	ADD_DEFINITIONS(-DBOOST_FILESYSTEM_VERSION=3)
	INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})
	if (Boost_USE_STATIC_LIBS)
		ADD_DEFINITIONS(-DBOOST_STATIC_LIB)
		ADD_DEFINITIONS(-DBOOST_PYTHON_STATIC_LIB)
	endif()
ELSE(Boost_FOUND)
	MESSAGE(FATAL_ERROR "Could not find Boost")
ENDIF(Boost_FOUND)


#############################################################################
#############################################################################
###########################   FREEIMAGE LIBRARIES    ########################
#############################################################################
#############################################################################

IF(LUX_USE_FREEIMAGE)
	FIND_PACKAGE(FreeImage REQUIRED)

	IF(FREEIMAGE_FOUND)
		MESSAGE(STATUS "FreeImage include directory: " ${FREEIMAGE_INCLUDE_DIR})
		MESSAGE(STATUS "FreeImage library: " ${FREEIMAGE_LIBRARIES})
		INCLUDE_DIRECTORIES(SYSTEM ${FREEIMAGE_INCLUDE_DIR})
	ELSE(FREEIMAGE_FOUND)
		MESSAGE(FATAL_ERROR "Could not find FreeImage")
	ENDIF(FREEIMAGE_FOUND)
ENDIF(LUX_USE_FREEIMAGE)

#############################################################################
#############################################################################
##########################   OPENIMAGEIO LIBRARIES    #######################
#############################################################################
#############################################################################

find_package(OpenImageIO REQUIRED)
include_directories(SYSTEM ${OPENIMAGEIO_INCLUDE_DIR})

if(NOT APPLE)
    find_package(TIFF REQUIRED)
    include_directories(SYSTEM ${TIFF_INCLUDE_DIR})
    find_package(JPEG REQUIRED)
    include_directories(SYSTEM ${JPEG_INCLUDE_DIR})
endif()

IF(OPENIMAGEIO_FOUND)
	MESSAGE(STATUS "OpenImageIO include directory: " ${OPENIMAGEIO_INCLUDE_DIR})
	MESSAGE(STATUS "OpenImageIO library: " ${OPENIMAGEIO_LIBRARIES})
	INCLUDE_DIRECTORIES(SYSTEM ${OPENIMAGEIO_INCLUDE_DIR})
ELSE(OPENIMAGEIO_FOUND)
	MESSAGE(FATAL_ERROR "Could not find OpenImageIO")
ENDIF(OPENIMAGEIO_FOUND)


#############################################################################
#############################################################################
######################### OPENEXR LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

# !!!!freeimage needs headers from or matched with freeimage !!!!
FIND_PACKAGE(OpenEXR)
IF (OPENEXR_INCLUDE_DIRS)
	MESSAGE(STATUS "OpenEXR include directory: " ${OPENEXR_INCLUDE_DIRS})
	INCLUDE_DIRECTORIES(SYSTEM ${OPENEXR_INCLUDE_DIRS})
ELSE(OPENEXR_INCLUDE_DIRS)
	MESSAGE(FATAL_ERROR "OpenEXR headers not found.")
ENDIF(OPENEXR_INCLUDE_DIRS)


#############################################################################
#############################################################################
########################### PNG   LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

# !!!!freeimage needs headers from or matched with freeimage !!!!
IF(NOT APPLE)
	FIND_PACKAGE(PNG)
	IF(PNG_INCLUDE_DIRS)
		MESSAGE(STATUS "PNG include directory: " ${PNG_INCLUDE_DIRS})
		INCLUDE_DIRECTORIES(SYSTEM ${PNG_INCLUDE_DIRS})
	ELSE(PNG_INCLUDE_DIRS)
		MESSAGE(STATUS "Warning : could not find PNG headers - building without png support")
	ENDIF(PNG_INCLUDE_DIRS)
ENDIF(NOT APPLE)

#############################################################################
#############################################################################
###########################   ADDITIONAL LIBRARIES   ########################
#############################################################################
#############################################################################

IF(LUX_USE_FREEIMAGE)
	# The OpenEXR library might be accessible from the FreeImage library
	# Otherwise add it to the FreeImage library (required by exrio)
	TRY_COMPILE(FREEIMAGE_PROVIDES_OPENEXR ${CMAKE_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/cmake/FindFreeImage.cxx
		CMAKE_FLAGS
		"-DINCLUDE_DIRECTORIES:STRING=${OPENEXR_INCLUDE_DIRS}"
		"-DLINK_LIBRARIES:STRING=${FREEIMAGE_LIBRARIES}"
		COMPILE_DEFINITIONS -D__TEST_OPENEXR__)

	# The PNG library might be accessible from the FreeImage library
	# Otherwise add it to the FreeImage library (required by pngio)
	TRY_COMPILE(FREEIMAGE_PROVIDES_PNG ${CMAKE_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/cmake/FindFreeImage.cxx
		CMAKE_FLAGS
		"-DINCLUDE_DIRECTORIES:STRING=${PNG_INCLUDE_DIRS}"
		"-DLINK_LIBRARIES:STRING=${FREEIMAGE_LIBRARIES}"
		COMPILE_DEFINITIONS -D__TEST_PNG__)

ENDIF(LUX_USE_FREEIMAGE)

	IF(NOT FREEIMAGE_PROVIDES_OPENEXR)
		IF(OPENEXR_LIBRARIES)
			MESSAGE(STATUS "OpenEXR library: " ${OPENEXR_LIBRARIES})
			SET(FREEIMAGE_LIBRARIES ${FREEIMAGE_LIBRARIES} ${OPENEXR_LIBRARIES})
		ELSE(OPENEXR_LIBRARIES)
			MESSAGE(FATAL_ERROR "Unable to find OpenEXR library")
		ENDIF(OPENEXR_LIBRARIES)
	ENDIF(NOT FREEIMAGE_PROVIDES_OPENEXR)

	IF (PNG_INCLUDE_DIRS AND NOT FREEIMAGE_PROVIDES_PNG)
		IF(PNG_LIBRARIES)
			MESSAGE(STATUS "PNG library: " ${PNG_LIBRARIES})
			SET(FREEIMAGE_LIBRARIES ${FREEIMAGE_LIBRARIES} ${PNG_LIBRARIES})
		ELSE(PNG_LIBRARIES)
			MESSAGE(FATAL_ERROR "Unable to find PNG library")
		ENDIF(PNG_LIBRARIES)
	ENDIF(PNG_INCLUDE_DIRS AND NOT FREEIMAGE_PROVIDES_PNG)

#############################################################################
#############################################################################
########################### FFTW  LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

IF(APPLE)
	SET(FFTW_INCLUDE_DIR ${OSX_DEPENDENCY_ROOT}/include/fftw3)
	SET(FFTW_LIBRARIES ${OSX_DEPENDENCY_ROOT}/lib/libfftw3.a)
	INCLUDE_DIRECTORIES(SYSTEM ${FFTW_INCLUDE_DIR})
ELSE(APPLE)
	FIND_PACKAGE(FFTW REQUIRED)
	MESSAGE(STATUS "FFTW include directory: " ${FFTW_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(SYSTEM ${FFTW_INCLUDE_DIR})
	MESSAGE(STATUS "FFTW library: " ${FFTW_LIBRARIES})
ENDIF(APPLE)

#############################################################################
#############################################################################
############################ THREADING LIBRARIES ############################
#############################################################################
#############################################################################

FIND_PACKAGE(Threads REQUIRED)

if (LUXRAYS_DISABLE_OPENCL)
	ADD_DEFINITIONS("-DLUXRAYS_DISABLE_OPENCL")
endif()
