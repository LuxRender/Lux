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

#ifndef LUXAPP_H
#define LUXAPP_H

#include <QtGui/QApplication>

#include "mainwindow.hxx"

class LuxGuiApp : public QApplication
{
	Q_OBJECT

public:
	MainWindow *mainwin;

	LuxGuiApp(int argc, char **argv);
	~LuxGuiApp();
	
	void init(void);
	void InfoDialogBox(const std::string &msg, const std::string &caption);
protected:
	bool event(QEvent *);	
private:
	int m_argc;
	char **m_argv;
	int m_threads;
	bool m_useServer, m_copyLog2Console;
	bool ProcessCommandLine (void);
#if defined(__APPLE__)
	QString m_inputFile;
	void loadFile(const QString &fileName);
#endif

};

#endif // LUXAPP_H
