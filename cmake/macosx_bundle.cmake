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

# Gather the date in finder-style
execute_process(COMMAND date "+%m/%d/%Y/%H:%M" OUTPUT_VARIABLE BUNDLING_TIME OUTPUT_STRIP_TRAILING_WHITESPACE)

add_dependencies(luxrender luxShared luxrender luxconsole luxmerger luxcomp luxvr) # assure we can pack the bundle
	ADD_CUSTOM_COMMAND(
		TARGET luxrender POST_BUILD
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxrender.app ${CMAKE_BUILD_TYPE}/LuxRender.app # this assures bundle name is right and case sensitive operations following do not fail
		COMMAND SetFile -d ${BUNDLING_TIME} -m ${BUNDLING_TIME} ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app # set the creation/modification date to bundling time
		COMMAND rm -rf ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/Resources
		COMMAND rm -rf ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/SmallluxGPU
		COMMAND mkdir ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxrender.icns ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxscene.icns ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxfilm.icns ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/icons/luxqueue.icns ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/Resources
		COMMAND cp ${OSX_BUNDLE_COMPONENTS_ROOT}/plists/1.5/Info.plist ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxconsole ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxconsole
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxcomp ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxcomp
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxmerger ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxmerger
		COMMAND mv ${CMAKE_BUILD_TYPE}/luxvr ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/luxvr
		COMMAND cp ${OSX_DEPENDENCY_ROOT}/lib/embree2/libembree.2.4.0.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/libembree.2.4.0.dylib
		COMMAND cp -f ${OSX_DEPENDENCY_ROOT}/lib/LuxRays/libluxcore.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
		COMMAND cp -f ${OSX_DEPENDENCY_ROOT}/lib/LuxRays/libluxcore.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS
		COMMAND cp ${CMAKE_BUILD_TYPE}/libembree.2.4.0.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/libembree.2.4.0.dylib
		COMMAND cp ${CMAKE_BUILD_TYPE}/liblux.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/MacOS/liblux.dylib
		COMMAND mkdir ${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/SmallluxGPU
		COMMAND cp ${OSX_DEPENDENCY_ROOT}/bin/slg4 ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/SmallluxGPU/slg4
		COMMAND install_name_tool -change @loader_path/libembree.2.4.0.dylib @loader_path/../MacOS/libembree.2.4.0.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/SmallluxGPU/slg4 # change dyld path
		COMMAND install_name_tool -change @loader_path/libluxcore.dylib @loader_path/../MacOS/libluxcore.dylib ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/LuxRender.app/Contents/SmallluxGPU/slg4 # change dyld path
		)
