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
#  PNG_FOUND - system has PNG
#  PNG_INCLUDE_DIR - the PNG include directories
#  PNG_LIBRARIES - link these to use PNG

INCLUDE ( FindPkgMacros )
FINDPKG_BEGIN ( PNG )

GETENV_PATH ( PNG_HOME )
SET ( PNG_PREFIX_PATH ${PNG_HOME} ${ENV_PNG_HOME} )
CREATE_SEARCH_PATHS ( PNG )

CLEAR_IF_CHANGED ( PNG_PREFIX_PATH
	PNG_LIBRARY_REL
	PNG_LIBRARY_DBG
	PNG_INCLUDE_DIR
)

MESSAGE ( STATUS "PNG_INC_SEARCH_PATH" ${PNG_INC_SEARCH_PATH} )

FIND_PATH ( PNG_INCLUDE_DIR NAMES png.h HINTS ${PNG_INC_SEARCH_PATH} )
MESSAGE ( STATUS "PNG_INCLUDE_DIR " ${PNG_INCLUDE_DIR} )

SET ( PNG_LIBRARY_NAMES png libpng png15 libpng15 png14 libpng14 png12 libpng12  )

GET_DEBUG_NAMES ( PNG_LIBRARY_NAMES )

FIND_LIBRARY ( PNG_LIBRARY_REL NAMES ${PNG_LIBRARY_NAMES} HINTS ${PNG_LIB_SEARCH_PATH} PATH_SUFFIXES "" release relwithdebinfo minsizerel dist )
FIND_LIBRARY ( PNG_LIBRARY_DBG NAMES ${PNG_LIBRARY_NAMES_DBG} HINTS ${PNG_LIB_SEARCH_PATH} PATH_SUFFIXES "" debug dist )
MAKE_LIBRARY_SET ( PNG_LIBRARY )

MESSAGE ( STATUS "PNG_LIBRARIES NAMES " ${PNG_LIBRARIES} )

IF ( PNG_INCLUDE_DIR AND PNG_LIBRARIES )
	SET ( PNG_FOUND TRUE )
ELSE ( PNG_INCLUDE_DIR AND PNG_LIBRARIES )
	SET( PNG_FOUND FALSE )
ENDIF ( PNG_INCLUDE_DIR AND PNG_LIBRARIES )

MARK_AS_ADVANCED ( PNG_INCLUDE_DIR PNG_LIBRARIES )

FINDPKG_FINISH ( PNG )
