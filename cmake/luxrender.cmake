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


FIND_PACKAGE(Qt4 4.6.0 COMPONENTS QtCore QtGui)
IF(QT4_FOUND)
	MESSAGE(STATUS "Qt library directory: " ${QT_LIBRARY_DIR} )
	MESSAGE( STATUS "Qt include directory: " ${QT_INCLUDE_DIR} )
	INCLUDE(${QT_USE_FILE})

	SET(LUXQTGUI_SRCS
		qtgui/main.cpp
		qtgui/histogramview.cpp
		qtgui/mainwindow.cpp
		qtgui/renderview.cpp
		qtgui/luxapp.cpp
		qtgui/aboutdialog.cpp
		qtgui/advancedinfowidget.cpp
		qtgui/lightgroupwidget.cpp
		qtgui/tonemapwidget.cpp
		qtgui/lenseffectswidget.cpp
		qtgui/colorspacewidget.cpp
		qtgui/gammawidget.cpp
		qtgui/noisereductionwidget.cpp
		qtgui/histogramwidget.cpp
		qtgui/panewidget.cpp
		qtgui/batchprocessdialog.cpp
		qtgui/openexroptionsdialog.cpp
		qtgui/guiutil.cpp
		)
	SOURCE_GROUP("Source Files\\Qt GUI" FILES ${LUXQTGUI_SRCS})

	SET( LUXQTGUI_MOC
		qtgui/histogramview.hxx
		qtgui/mainwindow.hxx
		qtgui/aboutdialog.hxx
		qtgui/advancedinfowidget.hxx
		qtgui/luxapp.hxx
		qtgui/renderview.hxx
		qtgui/lightgroupwidget.hxx
		qtgui/tonemapwidget.hxx
		qtgui/lenseffectswidget.hxx
		qtgui/colorspacewidget.hxx
		qtgui/gammawidget.hxx
		qtgui/noisereductionwidget.hxx
		qtgui/histogramwidget.hxx
		qtgui/panewidget.hxx
		qtgui/batchprocessdialog.hxx
		qtgui/openexroptionsdialog.hxx
		)
	SOURCE_GROUP("Header Files\\Qt GUI\\MOC" FILES ${LUXQTGUI_MOC})

	SET(LUXQTGUI_UIS
		qtgui/luxrender.ui
		qtgui/aboutdialog.ui
		qtgui/advancedinfo.ui
		qtgui/lightgroup.ui
		qtgui/tonemap.ui
		qtgui/lenseffects.ui
		qtgui/colorspace.ui
		qtgui/gamma.ui
		qtgui/noisereduction.ui
		qtgui/histogram.ui
		qtgui/pane.ui
		qtgui/batchprocessdialog.ui
		qtgui/openexroptionsdialog.ui
		)
	SOURCE_GROUP("UI Files" FILES ${LUXQTGUI_UIS})

	SET( LUXQTGUI_RCS
		qtgui/icons.qrc
		qtgui/splash.qrc
		qtgui/images.qrc
		)
	SOURCE_GROUP("Resource Files" FILES ${LUXQTGUI_RCS})

	QT4_ADD_RESOURCES( LUXQTGUI_RC_SRCS ${LUXQTGUI_RCS})
	QT4_WRAP_UI( LUXQTGUI_UI_HDRS ${LUXQTGUI_UIS} )
	QT4_WRAP_CPP( LUXQTGUI_MOC_SRCS ${LUXQTGUI_MOC} )

	#file (GLOB TRANSLATIONS_FILES qtgui/translations/*.ts)
	#qt4_create_translation(QM_FILES ${FILES_TO_TRANSLATE} ${TRANSLATIONS_FILES})

	#ADD_EXECUTABLE(luxrender ${GUI_TYPE} ${LUXQTGUI_SRCS} ${LUXQTGUI_MOC_SRCS} ${LUXQTGUI_RC_SRCS} ${LUXQTGUI_UI_HDRS} ${QM_FILES})
	ADD_EXECUTABLE(luxrender ${GUI_TYPE} ${LUXQTGUI_SRCS} ${LUXQTGUI_MOC_SRCS} ${LUXQTGUI_RC_SRCS} ${LUXQTGUI_UI_HDRS})
	IF(APPLE)
		IF( NOT CMAKE_VERSION VERSION_LESS 2.8.3) # only cmake >= 2.8.1 supports per target attributes
			SET_TARGET_PROPERTIES(luxrender PROPERTIES XCODE_ATTRIBUTE_GCC_VERSION 4.2) # QT will not play with xcode4 compiler default llvm-gcc-4.2 !
		ENDIF( NOT CMAKE_VERSION VERSION_LESS 2.8.3)
		INCLUDE_DIRECTORIES (SYSTEM /Developer/Headers/FlatCarbon /usr/local/include)
		FIND_LIBRARY(CARBON_LIBRARY Carbon)
		FIND_LIBRARY(QT_LIBRARY QtCore QtGui)
		FIND_LIBRARY(AGL_LIBRARY AGL )
		FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )

		MESSAGE(STATUS ${CARBON_LIBRARY})
		MARK_AS_ADVANCED (CARBON_LIBRARY)
		MARK_AS_ADVANCED (QT_LIBRARY)
		MARK_AS_ADVANCED (AGL_LIBRARY)
		MARK_AS_ADVANCED (APP_SERVICES_LIBRARY)
		SET(EXTRA_LIBS ${CARBON_LIBRARY} ${AGL_LIBRARY} ${APP_SERVICES_LIBRARY})
		IF(OSX_OPTION_DYNAMIC_BUILD)
			TARGET_LINK_LIBRARIES(luxrender ${OSX_SHARED_CORELIB} ${QT_LIBRARIES} ${EXTRA_LIBS} )
		ELSE(OSX_OPTION_DYNAMIC_BUILD)
			TARGET_LINK_LIBRARIES(luxrender -Wl,-undefined -Wl,dynamic_lookup ${LUX_LIBRARY} ${QT_LIBRARIES} ${LUX_LIBRARY_DEPENDS} ${EXTRA_LIBS} )
		ENDIF(OSX_OPTION_DYNAMIC_BUILD)
	ELSE(APPLE)
		TARGET_LINK_LIBRARIES(luxrender ${LUX_LIBRARY} ${QT_LIBRARIES} ${LUX_LIBRARY_DEPENDS})
	ENDIF(APPLE)
ELSE(QT4_FOUND)
	MESSAGE( STATUS "Warning : could not find Qt - not building Qt GUI")
ENDIF(QT4_FOUND)
