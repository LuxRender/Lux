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

ADD_EXECUTABLE(luxcomp tools/luxcomp.cpp)
IF(APPLE)
	IF(OSX_OPTION_DYNAMIC_BUILD)
		TARGET_LINK_LIBRARIES(luxcomp ${OSX_SHARED_CORELIB} ${CMAKE_THREAD_LIBS_INIT} )
	ELSE(OSX_OPTION_DYNAMIC_BUILD)
		TARGET_LINK_LIBRARIES(luxcomp -Wl,-undefined -Wl,dynamic_lookup -all_load luxStatic -noall_load ${CMAKE_THREAD_LIBS_INIT} ${LUXRAYS_LIBRARY} ${OPENCL_LIBRARY} ${OPENGL_LIBRARY} ${FREEIMAGE_LIBRARIES} ${SYS_LIBRARIES} ${Boost_LIBRARIES} )
	ENDIF(OSX_OPTION_DYNAMIC_BUILD)
ELSE(APPLE)
	TARGET_LINK_LIBRARIES(luxcomp -Wl,--whole-archive luxStatic -Wl,--no-whole-archive ${CMAKE_THREAD_LIBS_INIT} ${LUXRAYS_LIBRARY} ${OPENCL_LIBRARY} ${OPENGL_LIBRARY} ${FREEIMAGE_LIBRARIES} ${Boost_LIBRARIES}  ${SYS_LIBRARIES} ${OPENEXR_LIBRARIES} ${PNG_LIBRARY})
ENDIF(APPLE)
