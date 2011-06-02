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

#
# From: http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/cmake/FindGLEW.cmake?r=96 
#
# Try to find GLUT library and include path.
# Once done this will define
#
# GLUT_FOUND
# GLUT_INCLUDE_PATH
# GLUT_LIBRARY
# 

IF (WIN32)
	FIND_PATH( GLUT_INCLUDE_PATH GL/glut.h
		${GLUT_ROOT}/include
		$ENV{PROGRAMFILES}/GLUT/include
		${LuxRays_SOURCE_DIR}/../freeglut/include
		${LuxRays_SOURCE_DIR}/../glut/include
		DOC "The directory where GL/glew.h resides")
	FIND_LIBRARY( GLUT_LIBRARY
		NAMES glut GLUT glut32 glut32s freeglut freeglut_static
		PATHS
		${GLUT_LIBRARYDIR}
		${GLUT_ROOT}/lib
		$ENV{PROGRAMFILES}/GLUT/lib
		${LuxRays_SOURCE_DIR}/../freeglut/lib
		${LuxRays_SOURCE_DIR}/../glut/lib
		DOC "The GLUT library")
ELSE (WIN32)
	FIND_PATH( GLUT_INCLUDE_PATH GL/glut.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		DOC "The directory where GL/glut.h resides")
	FIND_LIBRARY( GLUT_LIBRARY
		NAMES glut GLUT freeglut freeglut32 freeglut_static
		PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		DOC "The GLUT library")
ENDIF (WIN32)

IF (GLUT_INCLUDE_PATH)
	SET( GLUT_FOUND 1 CACHE STRING "Set to 1 if GLUT is found, 0 otherwise")
ELSE (GLUT_INCLUDE_PATH)
	SET( GLUT_FOUND 0 CACHE STRING "Set to 1 if GLUT is found, 0 otherwise")
ENDIF (GLUT_INCLUDE_PATH)

MARK_AS_ADVANCED( GLUT_FOUND )
