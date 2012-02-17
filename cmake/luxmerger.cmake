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

SOURCE_GROUP("Source Files\\Tools" FILES tools/luxmerger.cpp)
ADD_EXECUTABLE(luxmerger tools/luxmerger.cpp)
IF(APPLE)
	add_dependencies(luxmerger luxShared) # explicitly say that the target depends on corelib build first
	TARGET_LINK_LIBRARIES(luxmerger ${OSX_SHARED_CORELIB} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
ELSE(APPLE)
	TARGET_LINK_LIBRARIES(luxmerger ${LUX_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LUX_LIBRARY_DEPENDS})
ENDIF(APPLE)
