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

# - Try to find OpenEXR
# Once done, this will define
#
#  OpenEXR_FOUND - system has OpenEXR
#  OpenEXR_INCLUDE_DIR - the OpenEXR include directories
#  OpenEXR_LIBRARIES - link these to use OpenEXR

INCLUDE(FindPkgMacros)
FINDPKG_BEGIN(OpenEXR)

GETENV_PATH(OpenEXR_HOME)
SET(OpenEXR_PREFIX_PATH ${OpenEXR_HOME} ${ENV_OpenEXR_HOME})
CREATE_SEARCH_PATHS(OpenEXR)

CLEAR_IF_CHANGED(OpenEXR_PREFIX_PATH
	OpenEXR_LIBRARY_FWK
	OpenEXR_LIBRARY_REL
	OpenEXR_LIBRARY_DBG
	OpenEXR_INCLUDE_DIR
)

# apple?
USE_PKGCONFIG(OpenEXR_PKGC openexr)
FINDPKG_FRAMEWORK(OpenEXR)

MESSAGE(STATUS "OpenEXR_INC_SEARCH_PATH" ${OpenEXR_INC_SEARCH_PATH})

FIND_PATH(OpenEXR_BASE_INCLUDE_DIR NAMES IlmBaseConfig.h HINTS ${OpenEXR_INC_SEARCH_PATH} ${OpenEXR_PKGC_INCLUDE_DIRS})
SET(OpenEXR_INCLUDE_DIR ${OpenEXR_BASE_INCLUDE_DIR} "${OpenEXR_BASE_INCLUDE_DIR}/Iex" "${OpenEXR_BASE_INCLUDE_DIR}/IlmImf" "${OpenEXR_BASE_INCLUDE_DIR}/IlmThread"  "${OpenEXR_BASE_INCLUDE_DIR}/Imath")

#FIND_PATH(OpenEXR_IEX_INCLUDE_DIR NAMES Iex.h HINTS ${OpenEXR_INC_SEARCH_PATH} ${OpenEXR_PKGC_INCLUDE_DIRS} "${OpenEXR_INC_SEARCH_PATH}/Iex" "${OpenEXR_PKGC_INCLUDE_DIRS}/Iex")
#FIND_PATH(OpenEXR_ILMIMF_INCLUDE_DIR NAMES ImfHeader.h HINTS ${OpenEXR_INC_SEARCH_PATH} ${OpenEXR_PKGC_INCLUDE_DIRS} "${OpenEXR_INC_SEARCH_PATH}/IlmImf" "${OpenEXR_PKGC_INCLUDE_DIRS}/IlmImf")
#FIND_PATH(OpenEXR_ILMTHREAD_INCLUDE_DIR NAMES IlmThread.h HINTS ${OpenEXR_INC_SEARCH_PATH} ${OpenEXR_PKGC_INCLUDE_DIRS} "${OpenEXR_INC_SEARCH_PATH}/IlmThread" "${OpenEXR_PKGC_INCLUDE_DIRS}/IlmThread")
#FIND_PATH(OpenEXR_IMATH_INCLUDE_DIR NAMES ImathMath.h HINTS ${OpenEXR_INC_SEARCH_PATH} ${OpenEXR_PKGC_INCLUDE_DIRS} "${OpenEXR_INC_SEARCH_PATH}/Imath" "${OpenEXR_PKGC_INCLUDE_DIRS}/Imath")
#FIND_PATH(OpenEXR_BASE_INCLUDE_DIR NAMES IlmBaseConfig.h HINTS ${OpenEXR_INC_SEARCH_PATH} ${OpenEXR_PKGC_INCLUDE_DIRS})

#MESSAGE(STATUS "OpenEXR_IEX_INCLUDE_DIR NAMES " ${OpenEXR_IEX_INCLUDE_DIR})
#MESSAGE(STATUS "OpenEXR_ILMIMF_INCLUDE_DIR NAMES " ${OpenEXR_ILMIMF_INCLUDE_DIR})
#MESSAGE(STATUS "OpenEXR_ILMTHREAD_INCLUDE_DIR NAMES " ${OpenEXR_ILMTHREAD_INCLUDE_DIR})
#MESSAGE(STATUS "OpenEXR_IMATH_INCLUDE_DIR NAMES " ${OpenEXR_IMATH_INCLUDE_DIR})
#MESSAGE(STATUS "OpenEXR_BASE_INCLUDE_DIR NAMES " ${OpenEXR_BASE_INCLUDE_DIR})

MESSAGE(STATUS "OpenEXR_INCLUDE_DIR " ${OpenEXR_INCLUDE_DIR})

#SET (OpenEXR_INCLUDE_DIR ${OpenEXR_IEX_INCLUDE_DIR} ${OpenEXR_ILMIMF_INCLUDE_DIR} ${OpenEXR_ILMTHREAD_INCLUDE_DIR} ${OpenEXR_IMATH_INCLUDE_DIR} ${OpenEXR_BASE_INCLUDE_DIR})

set( OpenEXR_IEX_LIBRARY_NAMES Iex )
set( OpenEXR_ILMIMF_LIBRARY_NAMES IlmImf )
set( OpenEXR_HALF_LIBRARY_NAMES Half )
set( OpenEXR_IMATH_LIBRARY_NAMES Imath )
set( OpenEXR_ILMTHREAD_LIBRARY_NAMES IlmThread )

get_debug_names( OpenEXR_IEX_LIBRARY_NAMES )
get_debug_names( OpenEXR_ILMIMF_LIBRARY_NAMES )
get_debug_names( OpenEXR_HALF_LIBRARY_NAMES )
get_debug_names( OpenEXR_IMATH_LIBRARY_NAMES )
get_debug_names( OpenEXR_ILMTHREAD_LIBRARY_NAMES )

FIND_LIBRARY(OpenEXR_IEX_LIBRARY_REL NAMES ${OpenEXR_IEX_LIBRARY_NAMES} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel dist)
FIND_LIBRARY(OpenEXR_IEX_LIBRARY_DBG NAMES ${OpenEXR_IEX_LIBRARY_NAMES_DBG} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug dist)
MAKE_LIBRARY_SET(OpenEXR_IEX_LIBRARY)

FIND_LIBRARY(OpenEXR_ILMIMF_LIBRARY_REL NAMES ${OpenEXR_ILMIMF_LIBRARY_NAMES} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel dist)
FIND_LIBRARY(OpenEXR_ILMIMF_LIBRARY_DBG NAMES ${OpenEXR_ILMIMF_LIBRARY_NAMES_DBG} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug dist)
MAKE_LIBRARY_SET(OpenEXR_ILMIMF_LIBRARY)

FIND_LIBRARY(OpenEXR_HALF_LIBRARY_REL NAMES ${OpenEXR_HALF_LIBRARY_NAMES} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel dist)
FIND_LIBRARY(OpenEXR_HALF_LIBRARY_DBG NAMES ${OpenEXR_HALF_LIBRARY_NAMES_DBG} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug dist)
MAKE_LIBRARY_SET(OpenEXR_HALF_LIBRARY)

FIND_LIBRARY(OpenEXR_IMATH_LIBRARY_REL NAMES ${OpenEXR_IMATH_LIBRARY_NAMES} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel dist)
FIND_LIBRARY(OpenEXR_IMATH_LIBRARY_DBG NAMES ${OpenEXR_IMATH_LIBRARY_NAMES_DBG} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug dist)
MAKE_LIBRARY_SET(OpenEXR_IMATH_LIBRARY)

FIND_LIBRARY(OpenEXR_ILMTHREAD_LIBRARY_REL NAMES ${OpenEXR_ILMTHREAD_LIBRARY_NAMES} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel dist)
FIND_LIBRARY(OpenEXR_ILMTHREAD_LIBRARY_DBG NAMES ${OpenEXR_ILMTHREAD_LIBRARY_NAMES_DBG} HINTS ${OpenEXR_LIB_SEARCH_PATH} ${OpenEXR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug dist)
MAKE_LIBRARY_SET(OpenEXR_ILMTHREAD_LIBRARY)

SET(OpenEXR_LIBRARIES_REL ${OpenEXR_IEX_LIBRARY_REL} ${OpenEXR_ILMIMF_LIBRARY_REL} ${OpenEXR_HALF_LIBRARY_REL} ${OpenEXR_IMATH_LIBRARY_REL} ${OpenEXR_ILMTHREAD_LIBRARY_REL})
SET(OpenEXR_LIBRARIES_DBG ${OpenEXR_IEX_LIBRARY_DBG} ${OpenEXR_ILMIMF_LIBRARY_DBG} ${OpenEXR_HALF_LIBRARY_DBG} ${OpenEXR_IMATH_LIBRARY_DBG} ${OpenEXR_ILMTHREAD_LIBRARY_DBG})

# Combine sub libs
SET(OpenEXR_LIBRARIES)
foreach(i ${OpenEXR_LIBRARIES_REL})
	set(OpenEXR_LIBRARIES ${OpenEXR_LIBRARIES} optimized ${i})
endforeach(i)
foreach(i ${OpenEXR_LIBRARIES_DBG})
	set(OpenEXR_LIBRARIES ${OpenEXR_LIBRARIES} debug ${i})
endforeach(i)

MESSAGE(STATUS "OpenEXR_LIBRARIES NAMES " ${OpenEXR_LIBRARIES})

IF (OpenEXR_INCLUDE_PATH AND OpenEXR_LIBRARIES)
		SET(OpenEXR_FOUND TRUE)
ELSE (OpenEXR_INCLUDE_PATH AND OpenEXR_LIBRARIES)
		SET(OpenEXR_FOUND FALSE)
ENDIF (OpenEXR_INCLUDE_PATH AND OpenEXR_LIBRARIES)

MARK_AS_ADVANCED(OpenEXR_INCLUDE_DIR OpenEXR_LIBRARIES)

FINDPKG_FINISH(OpenEXR)


