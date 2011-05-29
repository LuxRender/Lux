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

###########################################################################
#
# Configuration
#
# Use cmake "-DLUX_CUSTOM_CONFIG=YouFileCName" To define your personal settings
# where YouFileCName.cname must exist in one of the cmake include directories
# best to use cmake/SpecializedConfig/
#
# To not load defaults before loading custom values define
# -DLUX_NO_DEFAULT_CONFIG=true
#
# WARNING: These variables will be cashed like any other
#
###########################################################################

IF (NOT LUX_NO_DEFAULT_CONFIG)

	# Disable Boost automatic linking
	ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)

	IF (WIN32)

		MESSAGE(STATUS "Using default WIN32 Configuration settings")

		SET(BUILD_LUXMARK TRUE) # This will require QT

		SET(ENV{QTDIR} "c:/qt/")

		SET(FREEIMAGE_SEARCH_PATH     "${lux_SOURCE_DIR}/../FreeImage")
		SET(FreeImage_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/source")
		SET(FreeImage_LIB_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/release"
																		"${FREEIMAGE_SEARCH_PATH}/debug"
																		"${FREEIMAGE_SEARCH_PATH}/dist")

		SET(BOOST_SEARCH_PATH         "${lux_SOURCE_DIR}/../boost")
		SET(OPENCL_SEARCH_PATH        "${lux_SOURCE_DIR}/../opencl")
		SET(GLUT_SEARCH_PATH          "${lux_SOURCE_DIR}/../freeglut")

	ELSE(WIN32)


	ENDIF(WIN32)

ELSE(NOT LUX_NO_DEFAULT_CONFIG)

	MESSAGE(STATUS "LUX_NO_DEFAULT_CONFIG defined - not using default configuration values.")

ENDIF(NOT LUX_NO_DEFAULT_CONFIG)

#
# Overwrite defaults with Custom Settings
#
IF (LUX_CUSTOM_CONFIG)
	MESSAGE(STATUS "Using custom build config: ${LUX_CUSTOM_CONFIG}")
	INCLUDE(${LUX_CUSTOM_CONFIG})
ENDIF (LUX_CUSTOM_CONFIG)

