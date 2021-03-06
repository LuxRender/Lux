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

# SET(CMAKE_USE_PYTHON_VERSION 3.2)

IF(APPLE)
	IF(OSX_OPTION_PYLUX)
		# use Blender python libs for static compiling !
		SET(PYTHON_LIBRARIES ${OSX_DEPENDENCY_ROOT}/lib/BF_pythonlibs/py34_intel64/libbf_python_ext.a ${OSX_DEPENDENCY_ROOT}/lib/BF_pythonlibs/py34_intel64/libbf_python.a)
		SET(PYTHON_INCLUDE_PATH ${OSX_DEPENDENCY_ROOT}/include/Python3.4m)
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
		SET_TARGET_PROPERTIES(pylux PROPERTIES XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING NO) # exclude pylux from strip, not possible with external symbols !
		SET_TARGET_PROPERTIES(pylux PROPERTIES XCODE_ATTRIBUTE_LLVM_LTO NO) # exclude pylux from LTO, breaks compiling with xcode 4.4 and benefit is neglectible
		add_dependencies(pylux luxShared) # explicitly say that the target depends on corelib build first
		TARGET_LINK_LIBRARIES(pylux -Wl,-undefined -Wl,dynamic_lookup ${OSX_SHARED_CORELIB} ${LUXRAYS_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${PYTHON_LIBRARIES} ${Boost_python_LIBRARIES} ${Boost_LIBRARIES})
		SET_TARGET_PROPERTIES(pylux PROPERTIES XCODE_ATTRIBUTE_EXECUTABLE_PREFIX "") # just not set prefix instead of renaming later
		SET_TARGET_PROPERTIES(pylux PROPERTIES PREFIX "") # just not set prefix instead of renaming later
		ADD_CUSTOM_COMMAND(
			TARGET pylux POST_BUILD
			COMMAND cp ${CMAKE_SOURCE_DIR}/python/pyluxconsole.py ${CMAKE_BUILD_TYPE}/pyluxconsole.py
			)
	ELSE(APPLE)

		TARGET_LINK_LIBRARIES(pylux ${LUX_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LUX_LIBRARY_DEPENDS} ${Boost_python_LIBRARIES})
		
		IF(MSVC)
			# Output .pyd files for direct blender plugin usage
			SET_TARGET_PROPERTIES(pylux PROPERTIES SUFFIX ".pyd")
		ELSE(MSVC)
			SET_TARGET_PROPERTIES(pylux PROPERTIES PREFIX "")
		ENDIF(MSVC)

		IF (NOT WIN32 OR CYGWIN)
			ADD_CUSTOM_COMMAND(
				TARGET pylux POST_BUILD
				COMMAND cp ${CMAKE_SOURCE_DIR}/python/pyluxconsole.py pyluxconsole.py
			)
		ENDIF (NOT WIN32 OR CYGWIN)

	ENDIF(APPLE)
ELSE(PYTHONLIBS_FOUND OR PYTHON_CUSTOM)
	MESSAGE( STATUS "Warning: could not find Python libraries - not building python module")
ENDIF(PYTHONLIBS_FOUND OR PYTHON_CUSTOM)
