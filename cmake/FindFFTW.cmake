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

# Try to find FFTW library and include path.

SET(FFTW_INC_SEARCHPATH
	/usr/include
	/usr/include/fftw
	/usr/local/include
	/usr/local/include/fftw
	/share/apps/fftw-install/include)

SET(FFTW_LIB_SEARCHPATH
	/usr/lib
	/usr/lib/fftw
	/usr/local/lib
	/usr/local/lib/fftw
	/share/apps/fftw-install/lib)

FIND_PATH(FFTW_INCLUDE_DIR fftw3.h ${FFTW_INC_SEARCHPATH})
FIND_LIBRARY(FFTW_LIBRARIES fftw3 ${FFTW_LIB_SEARCHPATH})

MARK_AS_ADVANCED(FFTW_INCLUDE_DIR FFTW_LIBRARIES)
