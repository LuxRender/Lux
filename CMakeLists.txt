	${CMAKE_SOURCE_DIR}/core/reflection/fresnel
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

MESSAGE(STATUS "CMAKE VERSION DETECTED " ${CMAKE_VERSION})

###########################################################################
#
# check and configure cmake
#
###########################################################################
# Fresh start
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
CMAKE_POLICY(VERSION 2.8)

PROJECT(lux)
SET(VERSION 0.9)

# Dont overconfigure
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limit configs" FORCE)

###########################################################################
#
# Include necessary submodules
#
###########################################################################

SET(CMAKE_MODULE_PATH
	"${lux_SOURCE_DIR}"
	"${lux_SOURCE_DIR}/cmake"
	"${lux_SOURCE_DIR}/cmake/Utils"
	"${lux_SOURCE_DIR}/cmake/Packages"
	"${lux_SOURCE_DIR}/cmake/SpecializedConfig"
)

INCLUDE(PlatformSpecific)
INCLUDE(Configuration)

# Install CMake modules
#add_subdirectory(CMake)

#SET(lux_INCLUDE_DIR "${lux_SOURCE_DIR}/include")
#include_directories("${lux_INCLUDE_DIR}")
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core
	${CMAKE_SOURCE_DIR}/core/external
	${CMAKE_SOURCE_DIR}/core/queryable
	${CMAKE_SOURCE_DIR}/core/reflection
	${CMAKE_SOURCE_DIR}/core/reflection/bxdf
	${CMAKE_SOURCE_DIR}/core/reflection/microfacetdistribution
	${CMAKE_SOURCE_DIR}/spds
	${CMAKE_SOURCE_DIR}/lights/sphericalfunction
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	)

# When using single configuration generators like make
# cmake does need to know which of the possible configurations
# to generate
IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

MESSAGE(STATUS "Building mode: " ${CMAKE_BUILD_TYPE})

# Find dependencies
INCLUDE(Dependencies)

IF (NOT OPENGL_FOUND)
	MESSAGE(STATUS "--> Could not locate required OpenGL files")
ENDIF(NOT OPENGL_FOUND)

IF (NOT Boost_FOUND)
	MESSAGE(STATUS "--> Could not locate required Boost files - Please check '${BOOST_SEARCH_PATH}'")
ENDIF(NOT Boost_FOUND)

IF (NOT FreeImage_FOUND)
	MESSAGE(STATUS "--> Could not locate required FreeImage files - Please check '${FREEIMAGE_SEARCH_PATH}'")
ENDIF(NOT FreeImage_FOUND)

IF (NOT OPENCL_FOUND)
	MESSAGE(STATUS "--> Could not locate required OpenCL files - Please check '${OPENCL_SEARCH_PATH}'")
ENDIF(NOT OPENCL_FOUND)

IF (NOT PYTHONLIBS_FOUND)
	MESSAGE(STATUS "--> Could not locate required Python files")
ENDIF(NOT PYTHONLIBS_FOUND)

IF (NOT OpenEXR_FOUND)
	MESSAGE(STATUS "--> Could not locate required OpenEXR files - Please check '${OpenEXR_SEARCH_PATH}'")
ENDIF(NOT OpenEXR_FOUND)

#Generate the config.h file
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

ADD_DEFINITIONS(-DLUX_USE_CONFIG_H)
ADD_DEFINITIONS(-DBOOST_FILESYSTEM_VERSION=2)
ADD_DEFINITIONS(-DLUX_DLL)



###########################################################################
#
# Executables and Libraries
#
###########################################################################


INCLUDE(lux)
INCLUDE(luxrender)
INCLUDE(luxconsole)
INCLUDE(luxcomp)
INCLUDE(luxmerger)
INCLUDE(pylux)

###########################################################################
#
# For non win32 we'll have to copy everything to a single dir
#
###########################################################################
#INCLUDE(AssembleBinDirs)
