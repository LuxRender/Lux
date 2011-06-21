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

SET( lux_SRCS_QTGUI
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
SOURCE_GROUP("Source Files\\QTGUI" FILES ${lux_SRCS_QTGUI})

SET( lux_MOC_QTGUI
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
SOURCE_GROUP("Moc\\QTGUI" FILES ${lux_MOC_QTGUI})


SET( lux_UIS_QTGUI
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
SOURCE_GROUP("UIS\\QTGUI" FILES ${lux_UIS_QTGUI})

SET( lux_RCS_QTGUI
	qtgui/icons.qrc
	qtgui/splash.qrc
	qtgui/images.qrc
	)
SOURCE_GROUP("RCS\\QTGUI" FILES ${lux_RCS_QTGUI})

INCLUDE(${QT_USE_FILE})

QT4_ADD_RESOURCES( LUXQTGUI_RC_SRCS ${lux_RCS_QTGUI})
QT4_WRAP_UI( LUXQTGUI_UI_HDRS ${lux_UIS_QTGUI} )
QT4_WRAP_CPP( LUXQTGUI_MOC_SRCS ${lux_MOC_QTGUI} )

ADD_EXECUTABLE(luxrender ${GUI_TYPE} ${lux_SRCS_QTGUI} ${LUXQTGUI_MOC_SRCS} ${LUXQTGUI_RC_SRCS} ${LUXQTGUI_UI_HDRS})

TARGET_LINK_LIBRARIES(luxrender luxStatic)
INCLUDE_DIRECTORIES(${lux_INCLUDE_DIR})

TARGET_LINK_LIBRARIES(luxrender ${Boost_LIBRARIES} ${QT_LIBRARIES})
