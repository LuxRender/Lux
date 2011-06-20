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

SET( pylux_SRCS
	python/binding.cpp
	)
SOURCE_GROUP("Source Files\\" FILES ${pylux_SRCS})

ADD_LIBRARY(pylux SHARED ${pylux_SRCS})

# Output .pyd files for direct blender plugin usage
SET_TARGET_PROPERTIES(pylux
	PROPERTIES
	SUFFIX ".pyd"
	)

INCLUDE_DIRECTORIES(${lux_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})

SET_TARGET_PROPERTIES(pylux PROPERTIES COMPILE_FLAGS -DBOOST_PYTHON_STATIC_LIB)

TARGET_LINK_LIBRARIES(pylux luxStatic ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
