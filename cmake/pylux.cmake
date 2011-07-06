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

# SET(CMAKE_USE_PYTHON_VERSION 3.2)

IF(APPLE)
	IF(OSX_OPTION_PYLUX)
		# use Blender python libs for static compiling !
		SET(PYTHON_LIBRARIES ${CMAKE_SOURCE_DIR}/../macos/lib/BF_pythonlibs/py32_uni_intel/libbf_python_ext.a ${CMAKE_SOURCE_DIR}/../macos/lib/BF_pythonlibs/py32_uni_intel/libbf_python.a)
		SET(PYTHON_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/../macos/include/Python3.2)
		SET(PYTHONLIBS_FOUND ON)
	ELSE(OSX_OPTION_PYLUX)
		# compile pylux for genral purpose against Python framework
		FIND_LIBRARY(PYTHON_LIBRARY Python )
		FIND_PATH(PYTHON_INCLUDE_PATH python.h )
		MARK_AS_ADVANCED (PYTHON_LIBRARY)
		SET(PYTHONLIBS_FOUND on)
		SET(PYTHON_LIBRARIES ${PYTHON_LIBRARY})
	ENDIF(OSX_OPTION_PYLUX)
ELSE(APPLE)
	IF(PYTHON_CUSTOM)
		IF (NOT PYTHON_LIBRARIES)
			MESSAGE(FATAL_ERROR " PYTHON_CUSTOM set but PYTHON_LIBRARIES NOT set.")
		ENDIF (NOT PYTHON_LIBRARIES)
		IF (NOT PYTHON_INCLUDE_PATH)
			MESSAGE(FATAL_ERROR " PYTHON_CUSTOM set but PYTHON_INCLUDE_PATH NOT set.")
		ENDIF (NOT PYTHON_INCLUDE_PATH)
	ELSE(PYTHON_CUSTOM)
		FIND_PACKAGE(PythonLibs)
	ENDIF(PYTHON_CUSTOM)
ENDIF(APPLE)

IF(PYTHONLIBS_FOUND OR PYTHON_CUSTOM)
	MESSAGE(STATUS "Python library directory: " ${PYTHON_LIBRARIES} )
	MESSAGE(STATUS "Python include directory: " ${PYTHON_INCLUDE_PATH} )

	INCLUDE_DIRECTORIES(SYSTEM ${PYTHON_INCLUDE_PATH})

	SOURCE_GROUP("Source Files\\Python" FILES python/binding.cpp)
	SOURCE_GROUP("Header Files\\Python" FILES
		python/binding.h
		python/pycontext.h
		python/pydoc.h
		python/pydoc_context.h
		python/pydoc_renderserver.h
		python/pydynload.h
		python/pyfleximage.h
		python/pyrenderserver.h
		)

	ADD_LIBRARY(pylux MODULE python/binding.cpp)
	IF(APPLE)
		IF( NOT CMAKE_VERSION VERSION_LESS 2.8.3) # only cmake >= 2.8.3 supports per target attributes
			SET_TARGET_PROPERTIES(pylux PROPERTIES XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING NO) # exclude pylux from strip symbols
		ENDIF( NOT CMAKE_VERSION VERSION_LESS 2.8.3)
		TARGET_LINK_LIBRARIES(pylux -Wl,-undefined -Wl,dynamic_lookup ${LUX_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LUX_LIBRARY_DEPENDS} ${EXTRA_LIBS} ${PYTHON_LIBRARIES} ${Boost_python_LIBRARIES})
		ADD_CUSTOM_COMMAND(
			TARGET pylux POST_BUILD
			COMMAND mv release/libpylux.so release/pylux.so
		)
		ADD_CUSTOM_COMMAND(
			TARGET pylux POST_BUILD
			COMMAND cp ${CMAKE_SOURCE_DIR}/python/pyluxconsole.py release/pyluxconsole.py
		)
	ELSE(APPLE)
		TARGET_LINK_LIBRARIES(pylux ${LUX_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LUX_LIBRARY_DEPENDS} ${PYTHON_LIBRARIES} ${Boost_python_LIBRARIES})
		IF(CYGWIN)
			ADD_CUSTOM_COMMAND(
				TARGET pylux POST_BUILD
				COMMAND mv cygpylux.dll pylux.dll
			)
		ELSE(CYGWIN)
			ADD_CUSTOM_COMMAND(
				TARGET pylux POST_BUILD
				COMMAND mv libpylux.so pylux.so
			)
		ENDIF(CYGWIN)
		ADD_CUSTOM_COMMAND(
			TARGET pylux POST_BUILD
			COMMAND cp ${CMAKE_SOURCE_DIR}/python/pyluxconsole.py pyluxconsole.py
		)
	ENDIF(APPLE)
ELSE(PYTHONLIBS_FOUND OR PYTHON_CUSTOM)
	MESSAGE( STATUS "Warning : could not find Python libraries - not building python module")
ENDIF(PYTHONLIBS_FOUND OR PYTHON_CUSTOM)
