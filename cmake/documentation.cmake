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

FIND_PACKAGE(Doxygen)

IF (DOXYGEN_FOUND)
	MESSAGE( STATUS "Found Doxygen and generating documentation" )
	
	SET(DOXYGEN_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/doxygen.template)
	SET(DOXYGEN_INPUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf)
	SET(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc)
	SET(DOXYGEN_OUTPUT ${DOXYGEN_OUTPUT_DIR}/html/index.html)
	
	MESSAGE( STATUS "Doxygen output:" ${DOXYGEN_OUTPUT} )
	
	IF(DOXYGEN_DOT_FOUND)
		MESSAGE( STATUS "Found dot" )
		SET(DOXYGEN_DOT_CONF "HAVE_DOT = YES")
	ENDIF(DOXYGEN_DOT_FOUND)
	
	ADD_CUSTOM_COMMAND( 
	OUTPUT ${DOXYGEN_OUTPUT}
	#creating custom doxygen.conf
	COMMAND cp ${DOXYGEN_TEMPLATE} ${DOXYGEN_INPUT}
	COMMAND echo "INPUT = " ${CMAKE_CURRENT_SOURCE_DIR} >> ${DOXYGEN_INPUT}
	COMMAND echo "OUTPUT_DIRECTORY = " ${DOXYGEN_OUTPUT_DIR} >> ${DOXYGEN_INPUT}
	COMMAND echo ${DOXYGEN_DOT_CONF} >> ${DOXYGEN_INPUT}
	#launch doxygen
	COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
	DEPENDS ${DOXYGEN_TEMPLATE}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	
	ADD_CUSTOM_TARGET(apidoc ALL DEPENDS ${DOXYGEN_OUTPUT})
ENDIF (DOXYGEN_FOUND)
