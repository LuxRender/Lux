/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#include <ctime>
#include <iostream>

#include <QString>
#include <QVector>
#include <QtGui/QApplication>

#include "api.h"

#include "mainwindow.hxx"
#include "luxapp.hxx"
#include "guiutil.h"
#include "console/commandline.h"

#if defined(WIN32) && !defined(__CYGWIN__) && (_M_IX86_FP >= 2)
// for stderr redirection
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

void AttachStderr()
{
	int hCrt = _open_osfhandle((intptr_t)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);

	FILE *hf = _fdopen(hCrt, "w");
	*stderr = *hf;

	setvbuf(stderr, NULL, _IONBF, 0);
} 
#endif

LuxGuiApp::LuxGuiApp(int &argc, char **argv) : QApplication(argc, argv), mainwin(NULL)
{
	// Dade - initialize rand() number generator
	srand(time(NULL));

	// Set numeric format to standard to avoid errors when parsing files
	setlocale(LC_ALL, "C");
	
	luxInit();

	// Setup dialog box display method for ProcessCommandLine
	StrBufDialogBox infoDlg(QMessageBox::Information);
	StrBufDialogBox warnDlg(QMessageBox::Warning);

	config = new clConfig();
	if (ProcessCommandLine(argc, argv, *config, featureSet::RENDERER | featureSet::MASTERNODE | featureSet::INTERACTIVE, &infoDlg, &warnDlg))
		init(config);
}

LuxGuiApp::~LuxGuiApp()
{
	delete mainwin;
}

void LuxGuiApp::init(clConfig* config)
{
	// AttachConsole is XP only, restrict to SSE2+
#if defined(WIN32) && !defined(__CYGWIN__) && (_M_IX86_FP >= 2)
	// attach to parent process' console if it exists, otherwise ignore
	if (config->log2console && AttachConsole(ATTACH_PARENT_PROCESS)) {
		AttachStderr();
		std::cerr << "\nRedirecting log to console...\n";
	}
#endif

	mainwin = new MainWindow(0, config->log2console);
	mainwin->show();
#if defined(__APPLE__)
	mainwin->raise();
	mainwin->activateWindow();
#endif
	mainwin->SetRenderThreads(config->threadCount);
	mainwin->setVerbosity(config->verbosity);

	// Set server interval
	if (config->pollInterval > 0)
		mainwin->setServerUpdateInterval(config->pollInterval);

	// Add files on command line to the render queue
	for (std::vector<std::string>::const_iterator it = config->inputFiles.begin(); it != config->inputFiles.end(); it++)
		mainwin->addFileToRenderQueue(QString::fromStdString(*it));
	if (!config->inputFiles.empty() && config->queueFile.empty())
		mainwin->RenderNextFileInQueue();

	// Add files in queue file to the render queue
	if (!config->queueFile.empty())
		mainwin->openQueueFile(QString::fromStdString(config->queueFile));

	// Add slaves
	if (!config->slaveNodeList.empty())
	{
		QVector<QString> slaveNodes;
		for (std::vector<std::string>::const_iterator it = config->slaveNodeList.begin(); it != config->slaveNodeList.end(); it++)
			slaveNodes.push_back(QString::fromStdString(*it));
		mainwin->AddNetworkSlaves(slaveNodes);
	}
}

#if defined(__APPLE__) // Doubleclick or dragging .lxs in OSX Finder to LuxRender
bool LuxGuiApp::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::FileOpen:
		if (config->inputFiles.empty()) {
			mainwin->loadFile(static_cast<QFileOpenEvent *>(event)->file());
			return true;
		}
		break;
	default:
		break;
	}
	return QApplication::event(event);
}
#endif
