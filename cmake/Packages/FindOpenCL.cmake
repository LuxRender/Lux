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

# Try to find OpenCL library and include path.
# Once done this will define
#
# OPENCL_FOUND
# OPENCL_INCLUDE_PATH
# OPENCL_LIBRARY
# 

IF (WIN32)
	FIND_PATH( OPENCL_INCLUDE_PATH CL/cl.h
		${OPENCL_ROOT}/include
		${OPENCL_ROOT}
		${lux_SOURCE_DIR}/../opencl
		DOC "The directory where CL/cl.h resides")
	FIND_LIBRARY( OPENCL_LIBRARY
		NAMES OpenCL.lib
		PATHS
		${OPENCL_ROOT}
		${OPENCL_LIBRARYDIR}
		${OPENCL_ROOT}/lib/
		${lux_SOURCE_DIR}/../opencl/
		DOC "The OpenCL library")
ELSE (WIN32)
	FIND_PATH( OPENCL_INCLUDE_PATH CL/cl.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		/usr/src/opencl-sdk/include
		${OPENCL_ROOT}
		${OPENCL_ROOT}/include
		DOC "The directory where CL/cl.h resides")
	FIND_LIBRARY( OPENCL_LIBRARY
		NAMES opencl OpenCL
		PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		/usr/src/opencl-sdk/lib/x86_64
		/usr/src/opencl-sdk/lib/x86
		${OPENCL_ROOT}
		${OPENCL_LIBRARYDIR}
		DOC "The OpenCL library")
ENDIF (WIN32)

IF (OPENCL_INCLUDE_PATH)
	SET( OPENCL_FOUND 1 CACHE STRING "Set to 1 if OpenCL is found, 0 otherwise")
ELSE (OPENCL_INCLUDE_PATH)
	SET( OPENCL_FOUND 0 CACHE STRING "Set to 1 if OpenCL is found, 0 otherwise")
ENDIF (OPENCL_INCLUDE_PATH)

MARK_AS_ADVANCED( OPENCL_FOUND )
