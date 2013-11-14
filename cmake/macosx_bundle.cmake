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


#############################################################################
#############################################################################
##########  APPLE CUSTOM GUI_TYPE MACOSX_BUNDLE AND BUILD TARGETS ###########
#############################################################################
#############################################################################

	SET(GUI_TYPE MACOSX_BUNDLE)
	# SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${OPENSCENEGRAPH_MAJOR_VERSION}.${OPENSCENEGRAPH_MINOR_VERSION}.${OPENSCENEGRAPH_PATCH_VERSION}")
	# Short Version is the "marketing version". It is the version
	# the user sees in an information panel.
	SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")
	# Bundle version is the version the OS looks at.
	SET(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}")
	SET(MACOSX_BUNDLE_GUI_IDENTIFIER "org.luxrender.luxrender" )
	SET(MACOSX_BUNDLE_BUNDLE_NAME "Luxrender" )
	SET(MACOSX_BUNDLE_ICON_FILE "luxrender.icns")
	# SET(MACOSX_BUNDLE_COPYRIGHT "")
	# SET(MACOSX_BUNDLE_INFO_STRING "Info string, localized?")
	if(OSX_OPTION_PYLUX)
		ADD_CUSTOM_TARGET(DYNAMIC_BUILD DEPENDS luxShared luxrender luxconsole luxmerger luxcomp luxvr pylux )
	else()
		ADD_CUSTOM_TARGET(DYNAMIC_BUILD DEPENDS luxShared luxrender luxconsole luxmerger luxcomp luxvr)
	endif()
	ADD_CUSTOM_COMMAND(
		TARGET DYNAMIC_BUILD POST_BUILD
		COMMAND rm -rf ${CMAKE_BUILD_TYPE}/luxrender.app/Contents/Resources
		COMMAND rm -rf ${CMAKE_BUILD_TYPE}/SmallluxGPU
		COMMAND mkdir ${CMAKE_BUILD_TYPE}/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxrender.icns ${CMAKE_BUILD_TYPE}/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxscene.icns ${CMAKE_BUILD_TYPE}/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxfilm.icns ${CMAKE_BUILD_TYPE}/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxqueue.icns ${CMAKE_BUILD_TYPE}/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/plists/1.3/Info.plist ${CMAKE_BUILD_TYPE}/luxrender.app/Contents
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxrender.app ${CMAKE_BUILD_TYPE}/LuxRender.app
#		COMMAND macdeployqt ${CMAKE_BUILD_TYPE}/LuxRender.app ### uncomment for bundling Qt frameworks ###
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxconsole ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxconsole
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxcomp ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxcomp
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxmerger ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxmerger
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxvr ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxvr
		COMMAND cp ${CMAKE_BUILD_TYPE}/liblux.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/liblux.dylib
		COMMAND mkdir ${CMAKE_BUILD_TYPE}/SmallluxGPU
		COMMAND cp ${OSX_DEPENDENCY_ROOT}/bin/slg4 ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/SmallluxGPU/slg4
		)
