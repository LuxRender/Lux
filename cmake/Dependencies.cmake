###########################################################################
#   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 #
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
	FIND_PATH(LUXRAYS_INCLUDE_DIRS NAMES luxrays/luxrays.h PATHS ../macos/include/LuxRays)
	FIND_LIBRARY(LUXRAYS_LIBRARY libluxrays.a ../macos/lib/LuxRays)
ELSE(APPLE)
	FIND_PATH(LUXRAYS_INCLUDE_DIRS NAMES luxrays/luxrays.h PATHS ../luxrays/include)
	FIND_LIBRARY(LUXRAYS_LIBRARY luxrays ../luxrays/lib)
ENDIF(APPLE)

MESSAGE(STATUS "LuxRays include directory: " ${LUXRAYS_INCLUDE_DIRS})
MESSAGE(STATUS "LuxRays library directory: " ${LUXRAYS_LIBRARY})


#############################################################################
#############################################################################
###########################      Find OpenCL       ##########################
#############################################################################
#############################################################################

if(LUXRAYS_DISABLE_OPENCL)
    set(OCL_LIBRARY "")
else(LUXRAYS_DISABLE_OPENCL)
    FIND_PATH(OPENCL_INCLUDE_DIRS NAMES CL/cl.hpp OpenCL/cl.hpp PATHS /usr/src/opencl-sdk/include /usr/local/cuda/include)
    FIND_LIBRARY(OPENCL_LIBRARY OpenCL /usr/src/opencl-sdk/lib/x86_64)

    MESSAGE(STATUS "OpenCL include directory: " ${OPENCL_INCLUDE_DIRS})
    MESSAGE(STATUS "OpenCL library directory: " ${OPENCL_LIBRARY})
endif(LUXRAYS_DISABLE_OPENCL)


#############################################################################
#############################################################################
###########################      Find OpenGL       ##########################
#############################################################################
#############################################################################

FIND_PACKAGE(OpenGL)

message(STATUS "OpenGL include directory: " "${OPENGL_INCLUDE_DIRS}")
message(STATUS "OpenGL library: " "${OPENGL_LIBRARY}")


#############################################################################
#############################################################################
###########################      Find BISON       ###########################
#############################################################################
#############################################################################

FIND_PACKAGE(BISON REQUIRED)
IF (NOT BISON_FOUND)
	MESSAGE(FATAL_ERROR "bison not found - aborting")
ENDIF (NOT BISON_FOUND)


#############################################################################
#############################################################################
###########################      Find FLEX        ###########################
#############################################################################
#############################################################################

FIND_PACKAGE(FLEX REQUIRED)
IF (NOT FLEX_FOUND)
	MESSAGE(FATAL_ERROR "flex not found - aborting")
ENDIF (NOT FLEX_FOUND)



#############################################################################
#############################################################################
########################### BOOST LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

IF(APPLE)
	SET(BOOST_ROOT ../macos)
ENDIF(APPLE)

FIND_PACKAGE(Boost 1.42 COMPONENTS python REQUIRED)

SET(Boost_python_FOUND ${Boost_FOUND})
SET(Boost_python_LIBRARIES ${Boost_LIBRARIES})
SET(Boost_FOUND)
SET(Boost_LIBRARIES)

FIND_PACKAGE(Boost 1.42 COMPONENTS thread program_options filesystem serialization iostreams regex system REQUIRED)

IF(Boost_FOUND)
	MESSAGE(STATUS "Boost library directory: " ${Boost_LIBRARY_DIRS})
	MESSAGE(STATUS "Boost include directory: " ${Boost_INCLUDE_DIRS})
ELSE(Boost_FOUND)
	MESSAGE(FATAL_ERROR "Could not find Boost")
ENDIF(Boost_FOUND)


#############################################################################
#############################################################################
######################### OPENEXR LIBRARIES SETUP ###########################
#############################################################################
#############################################################################

# !!!!freeimage needs headers from or matched with freeimage !!!!
IF(APPLE)
	FIND_PATH(OPENEXR_INCLUDE_DIRS
		OpenEXRConfig.h
		PATHS
		../macos/include/OpenEXR
		NO_DEFAULT_PATH
	)
ELSE(APPLE)
	FIND_PATH(OPENEXR_INCLUDE_DIRS
		ImfXdr.h
		PATHS
		/usr/local/include/OpenEXR
		/usr/include/OpenEXR
		/sw/include/OpenEXR
		/opt/local/include/OpenEXR
		/opt/csw/include/OpenEXR
		/opt/include/OpenEXR
	)
	SET(OPENEXR_LIBRARIES Half IlmImf Iex Imath)
ENDIF(APPLE)

#############################################################################
#############################################################################
########################### PNG   LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
# - Find the native PNG includes and library
#
# This module defines
#  PNG_INCLUDE_DIR, where to find png.h, etc.
#  PNG_LIBRARIES, the libraries to link against to use PNG.
#  PNG_DEFINITIONS - You should ADD_DEFINITONS(${PNG_DEFINITIONS}) before compiling code that includes png library files.
#  PNG_FOUND, If false, do not try to use PNG.
# also defined, but not for general use are
#  PNG_LIBRARY, where to find the PNG library.
# None of the above will be defined unles zlib can be found.
# PNG depends on Zlib
IF(APPLE)
	FIND_PATH(PNG_INCLUDE_DIR
		pngconf.h
		PATHS
		../macos//include
		NO_DEFAULT_PATH
	)
ELSE(APPLE)
	FIND_PACKAGE(PNG)
	IF(NOT PNG_FOUND)
		MESSAGE(STATUS "Warning : could not find PNG - building without png support")
	ENDIF(NOT PNG_FOUND)
ENDIF(APPLE)



#############################################################################
#############################################################################
###########################   FREEIMAGE LIBRARIES    ########################
#############################################################################
#############################################################################

IF(APPLE)
	FIND_PATH(FREEIMAGE_INCLUDE_DIRS
		freeimage.h
		PATHS
	../macos//include
	NO_DEFAULT_PATH
	)
	FIND_LIBRARY(FREEIMAGE_LIBRARIES
		libfreeimage.a
		PATHS
	../macos//lib
	NO_DEFAULT_PATH
	)
ELSE(APPLE)
	FIND_PACKAGE(FreeImage REQUIRED)

	IF(FREEIMAGE_FOUND)
		MESSAGE(STATUS "FreeImage library directory: " ${FREEIMAGE_LIBRARIES})
		MESSAGE(STATUS "FreeImage include directory: " ${FREEIMAGE_INCLUDE_PATH})
	ELSE(FREEIMAGE_FOUND)
		MESSAGE(FATAL_ERROR "Could not find FreeImage")
	ENDIF(FREEIMAGE_FOUND)
ENDIF(APPLE)

#############################################################################
#############################################################################
############################ THREADING LIBRARIES ############################
#############################################################################
#############################################################################
IF(APPLE)
	FIND_PATH(THREADS_INCLUDE_DIRS
		pthread.h
		PATHS
		/usr/include/pthread
	)
	SET(THREADS_LIBRARIES pthread)
ELSE(APPLE)
	FIND_PACKAGE(Threads REQUIRED)
ENDIF(APPLE)
