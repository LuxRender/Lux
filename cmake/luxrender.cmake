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


FIND_PACKAGE(Qt4 4.6.0 COMPONENTS QtCore QtGui QtMain)

IF(QT4_FOUND)
	MESSAGE(STATUS "Qt library directory: " ${QT_LIBRARY_DIR} )
	MESSAGE( STATUS "Qt include directory: " ${QT_INCLUDE_DIR} )
	INCLUDE(${QT_USE_FILE})

	SET(LUXQTGUI_SRCS
		qtgui/aboutdialog.cpp
		qtgui/advancedinfowidget.cpp
		qtgui/batchprocessdialog.cpp
		qtgui/colorspacewidget.cpp
		qtgui/gammawidget.cpp
		qtgui/guiutil.cpp
		qtgui/histogramview.cpp
		qtgui/histogramwidget.cpp
		qtgui/lenseffectswidget.cpp
		qtgui/lightgroupwidget.cpp
		qtgui/luxapp.cpp
		qtgui/main.cpp
		qtgui/mainwindow.cpp
		qtgui/noisereductionwidget.cpp
		qtgui/openexroptionsdialog.cpp
		qtgui/panewidget.cpp
		qtgui/queue.cpp
		qtgui/renderview.cpp
		qtgui/tonemapwidget.cpp
		console/commandline.cpp
		)
	SOURCE_GROUP("Source Files\\Qt GUI" FILES ${LUXQTGUI_SRCS})

	SET(LUXQTGUI_MOC
		qtgui/aboutdialog.hxx
		qtgui/advancedinfowidget.hxx
		qtgui/batchprocessdialog.hxx
		qtgui/colorspacewidget.hxx
		qtgui/gammawidget.hxx
		qtgui/histogramview.hxx
		qtgui/histogramwidget.hxx
		qtgui/lenseffectswidget.hxx
		qtgui/lightgroupwidget.hxx
		qtgui/luxapp.hxx
		qtgui/mainwindow.hxx
		qtgui/noisereductionwidget.hxx
		qtgui/openexroptionsdialog.hxx
		qtgui/panewidget.hxx
		qtgui/queue.hxx
		qtgui/renderview.hxx
		qtgui/tonemapwidget.hxx
		)
	SOURCE_GROUP("Header Files\\Qt GUI" FILES ${LUXQTGUI_MOC} qtgui/quiutil.h console/commandline.h)

	SET(LUXQTGUI_UIS
		qtgui/aboutdialog.ui
		qtgui/advancedinfo.ui
		qtgui/batchprocessdialog.ui
		qtgui/colorspace.ui
		qtgui/gamma.ui
		qtgui/histogram.ui
		qtgui/lenseffects.ui
		qtgui/lightgroup.ui
		qtgui/luxrender.ui
		qtgui/noisereduction.ui
		qtgui/openexroptionsdialog.ui
		qtgui/pane.ui
		qtgui/tonemap.ui
		)
	SOURCE_GROUP("UI Files\\Qt GUI" FILES ${LUXQTGUI_UIS})

	SET(LUXQTGUI_RCS
		qtgui/icons.qrc
		qtgui/splash.qrc
		qtgui/images.qrc
		)
	SOURCE_GROUP("Resource Files\\Qt GUI" FILES ${LUXQTGUI_RCS})

	QT4_ADD_RESOURCES( LUXQTGUI_RC_SRCS ${LUXQTGUI_RCS})
	QT4_WRAP_UI( LUXQTGUI_UI_HDRS ${LUXQTGUI_UIS} )
	QT4_WRAP_CPP( LUXQTGUI_MOC_SRCS ${LUXQTGUI_MOC} )

	#file (GLOB TRANSLATIONS_FILES qtgui/translations/*.ts)
	#qt4_create_translation(QM_FILES ${FILES_TO_TRANSLATE} ${TRANSLATIONS_FILES})

	#ADD_EXECUTABLE(luxrender ${GUI_TYPE} ${LUXQTGUI_SRCS} ${LUXQTGUI_MOC_SRCS} ${LUXQTGUI_RC_SRCS} ${LUXQTGUI_UI_HDRS} ${QM_FILES})
	ADD_EXECUTABLE(luxrender ${GUI_TYPE} ${LUXQTGUI_SRCS} ${LUXQTGUI_MOC_SRCS} ${LUXQTGUI_RC_SRCS} ${LUXQTGUI_UI_HDRS})

	IF(APPLE)
		IF( NOT OSX_OPTION_XCODE_4.1)
			SET_TARGET_PROPERTIES(luxrender PROPERTIES XCODE_ATTRIBUTE_GCC_VERSION 4.2) # QT will not play with xcode-4.0 compiler default llvm-gcc-4.2 !
		ENDIF( NOT OSX_OPTION_XCODE_4.1)
		SET_TARGET_PROPERTIES(luxrender PROPERTIES XCODE_ATTRIBUTE_LLVM_LTO NO ) # always disabled due Qt does not like it
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
		add_dependencies(luxrender luxShared) # explicitly say that the target depends on corelib build first
		TARGET_LINK_LIBRARIES(luxrender ${OSX_SHARED_CORELIB} ${QT_LIBRARIES} ${EXTRA_LIBS} ${Boost_LIBRARIES})
	ELSE(APPLE)
		MESSAGE(STATUS "Qt libs: ${QT_LIBRARIES}}")
		TARGET_LINK_LIBRARIES(luxrender ${LUX_LIBRARY} ${QT_LIBRARIES} ${LUX_LIBRARY_DEPENDS})
	ENDIF(APPLE)
ELSE(QT4_FOUND)
	MESSAGE( STATUS "Warning : could not find Qt - not building Qt GUI")
ENDIF(QT4_FOUND)
