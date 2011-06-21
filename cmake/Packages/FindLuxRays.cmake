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

# Try to find Luxrays
# Once done, this will define
#
#  LuxRays_FOUND - system has LuxRays
#  LuxRays_INCLUDE_DIR - the LuxRays include directories
#  LuxRays_LIBRARIES - link these to use LuxRays

INCLUDE ( FindPkgMacros )
FINDPKG_BEGIN ( LuxRays )

GETENV_PATH ( LuxRays_HOME )
SET ( LuxRays_PREFIX_PATH ../luxrays ${LuxRays_HOME} ${ENV_LuxRays_HOME} )
CREATE_SEARCH_PATHS ( LuxRays )

CLEAR_IF_CHANGED ( LuxRays_PREFIX_PATH
	LuxRays_LIBRARY_REL
	LuxRays_LIBRARY_DBG
	LuxRays_INCLUDE_DIR
)

MESSAGE ( STATUS "LuxRays_INC_SEARCH_PATH" ${LuxRays_INC_SEARCH_PATH} )

FIND_PATH ( LuxRays_INCLUDE_DIR NAMES luxrays/luxrays.h HINTS ${LuxRays_INC_SEARCH_PATH} )
MESSAGE ( STATUS "LuxRays_INCLUDE_DIR " ${LuxRays_INCLUDE_DIR} )

set( LuxRays_LIBRARY_NAMES luxrays )

get_debug_names( LuxRays_LIBRARY_NAMES )

FIND_LIBRARY ( LuxRays_LIBRARY_REL NAMES ${LuxRays_LIBRARY_NAMES} HINTS ${LuxRays_LIB_SEARCH_PATH} PATH_SUFFIXES "" release relwithdebinfo minsizerel dist )
FIND_LIBRARY ( LuxRays_LIBRARY_DBG NAMES ${LuxRays_LIBRARY_NAMES_DBG} HINTS ${LuxRays_LIB_SEARCH_PATH} PATH_SUFFIXES "" debug dist )
MAKE_LIBRARY_SET ( LuxRays_LIBRARY )

MESSAGE ( STATUS "LuxRays_LIBRARIES NAMES " ${LuxRays_LIBRARIES} )

IF ( LuxRays_INCLUDE_DIR AND LuxRays_LIBRARIES )
	SET ( LuxRays_FOUND TRUE )
ELSE ( LuxRays_INCLUDE_DIR AND LuxRays_LIBRARIES )
	SET( LuxRays_FOUND FALSE )
ENDIF ( LuxRays_INCLUDE_DIR AND LuxRays_LIBRARIES )

MARK_AS_ADVANCED ( LuxRays_INCLUDE_DIR LuxRays_LIBRARIES )

FINDPKG_FINISH ( LuxRays )
