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
	ADD_CUSTOM_TARGET(DYNAMIC_BUILD DEPENDS luxShared luxrender luxconsole luxmerger luxcomp pylux )
	ADD_CUSTOM_COMMAND(
		TARGET DYNAMIC_BUILD POST_BUILD
		COMMAND rm -rf Release/luxrender.app/Contents/Resources
		COMMAND mkdir Release/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxrender.icns Release/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxscene.icns Release/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxfilm.icns Release/luxrender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/plists/09/Info.plist Release/luxrender.app/Contents
		COMMAND mv Release/luxrender.app Release/LuxRender.app
#		COMMAND macdeployqt Release/LuxRender.app ### uncomment for bundling Qt frameworks ###
		COMMAND mv Release/luxconsole ${CMAKE_BINARY_DIR}/Release/LuxRender.app/Contents/MacOS/luxconsole
		COMMAND mv Release/luxcomp ${CMAKE_BINARY_DIR}/Release/LuxRender.app/Contents/MacOS/luxcomp
		COMMAND mv Release/luxmerger ${CMAKE_BINARY_DIR}/Release/LuxRender.app/Contents/MacOS/luxmerger
		COMMAND install_name_tool -id @loader_path/liblux.dylib Release/liblux.dylib
		COMMAND cp Release/liblux.dylib ${CMAKE_BINARY_DIR}/Release/LuxRender.app/Contents/MacOS/liblux.dylib
		)
