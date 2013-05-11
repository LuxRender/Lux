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
#
# Try to find the FreeImage library and include path.
# Once done this will define
#
# FFTW_FOUND
# FFTW_INCLUDE_DIR
# FFTW_LIBRARIES
# 

# Lookup user provide path first
include(FindPkgMacros)
SET(FFTW_INC_SUFFIXES include/FFTW include Include Headers Dist Source api .libs)

FIND_PATH(FFTW_INCLUDE_DIR
	NAMES fftw3.h
	PATHS "${FFTW_ROOT}"
	PATH_SUFFIXES ${FFTW_INC_SUFFIXES}
	NO_DEFAULT_PATH
	DOC "The directory where FFTW.h resides")
FIND_PATH(FFTW_INCLUDE_DIR
	NAMES fftw3.h
	PATHS /usr/local /usr /sw /opt/local /opt/csw /opt
	PATH_SUFFIXES ${FFTW_INC_SUFFIXES}
	DOC "The directory where FFTW.h resides")

SET(FFTW_NAMES_REL fftw3 fftw)
SET(FFTW_LIB_SUFFIXES lib64 lib Lib lib/FFTW Libs Dist Release Debug)
SET(FFTW_LIB_SUFFIXES_REL)
SET(FFTW_LIB_SUFFIXES_DBG)
FOREACH(i ${FFTW_LIB_SUFFIXES})
	SET(FFTW_LIB_SUFFIXES_REL ${FFTW_LIB_SUFFIXES_REL}
		"${i}" "${i}/release" "${i}/relwithdebinfo" "${i}/minsizerel" "${i}/dist")
	SET(FFTW_LIB_SUFFIXES_DBG ${FFTW_LIB_SUFFIXES_DBG}
		"${i}" "${i}/debug" "${i}/dist")
ENDFOREACH(i)
SET(FFTW_NAMES_DBG)
FOREACH(i ${FFTW_NAMES_REL})
	SET(FFTW_NAMES_DBG ${FFTW_NAMES_DBG} "${i}d" "${i}D" "${i}_d" "${i}_D" "${i}_debug")
ENDFOREACH(i)
FIND_LIBRARY(FFTW_LIBRARY_REL
	NAMES ${FFTW_NAMES_REL}
	PATHS "${FFTW_ROOT}"
	PATH_SUFFIXES ${FFTW_LIB_SUFFIXES_REL}
	NO_DEFAULT_PATH
	DOC "The FFTW release library"
)
FIND_LIBRARY(FFTW_LIBRARY_REL
	NAMES ${FFTW_NAMES_REL}
	PATHS /usr/local /usr /sw /opt/local /opt/csw /opt
	PATH_SUFFIXES ${FFTW_LIB_SUFFIXES_REL}
	DOC "The FFTW release library"
)
FIND_LIBRARY(FFTW_LIBRARY_DBG
	NAMES ${FFTW_NAMES_DBG}
	PATHS "${FFTW_ROOT}"
	PATH_SUFFIXES ${FFTW_LIB_SUFFIXES_DBG}
	NO_DEFAULT_PATH
	DOC "The FFTW debug library"
)
FIND_LIBRARY(FFTW_LIBRARY_DBG
	NAMES ${FFTW_NAMES_DBG}
	PATHS /usr/local /usr /sw /opt/local /opt/csw /opt
	PATH_SUFFIXES ${FFTW_LIB_SUFFIXES_DBG}
	DOC "The FFTW debug library"
)
IF (FFTW_LIBRARY_REL AND FFTW_LIBRARY_DBG)
	SET(FFTW_LIBRARIES
		optimized ${FFTW_LIBRARY_REL}
		debug ${FFTW_LIBRARY_DBG})
ELSEIF (FFTW_LIBRARY_REL)
	SET(FFTW_LIBRARIES ${FFTW_LIBRARY_REL})
ELSEIF (FFTW_LIBRARY_DBG)
	SET(FFTW_LIBRARIES ${FFTW_LIBRARY_DBG})
ENDIF (FFTW_LIBRARY_REL AND FFTW_LIBRARY_DBG)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFTW  DEFAULT_MSG  FFTW_LIBRARIES FFTW_INCLUDE_DIR)

MARK_AS_ADVANCED(FFTW_LIBRARIES FFTW_INCLUDE_DIR)
