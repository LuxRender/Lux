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

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
using std::ifstream;
#include <sstream>
using std::stringstream;

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QTextStream>

#include "api.h"
#include "error.h"

#include "mainwindow.hxx"
#include "luxapp.hxx"

#if defined(WIN32) && !defined(__CYGWIN__)
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

namespace po = boost::program_options;

LuxGuiApp::LuxGuiApp(int &argc, char **argv) : QApplication(argc, argv), m_argc(argc)
{
	m_argv = argv;
	mainwin = NULL;
}

LuxGuiApp::~LuxGuiApp()
{
	if (mainwin != NULL)
		delete mainwin;
}

void LuxGuiApp::init(void) {
	// Dade - initialize rand() number generator
	srand(time(NULL));

	// Set numeric format to standard to avoid errors when parsing files
	setlocale(LC_ALL, "C");
	
	luxInit();

	if (ProcessCommandLine()) {

// AttachConsole is XP only, restrict to SSE2+
#if defined(WIN32) && !defined(__CYGWIN__) && (_M_IX86_FP >= 2)
		// attach to parent process' console if it exists, otherwise ignore
		if (m_copyLog2Console) {
			if (AttachConsole(ATTACH_PARENT_PROCESS)) {
				AttachStderr();
				std::cerr << "\nRedirecting log to console...\n";
			}
		}
#endif

		mainwin = new MainWindow(0,m_copyLog2Console);
		mainwin->show();
#if defined(__APPLE__)
		mainwin->raise();
		mainwin->activateWindow();
#endif
		mainwin->SetRenderThreads(m_threads);
		if (!m_inputFile.isEmpty())
			mainwin->renderScenefile(m_inputFile);

		// Add files to the render queue
    if (renderQueueList.count()) {
      QString renderQueueEntry;
      foreach( renderQueueEntry, renderQueueList ) {
        mainwin->addFileToRenderQueue(renderQueueEntry);
      }
      mainwin->RenderNextFileInQueue();
    }
	} else {
	}	
}

#if defined(__APPLE__) // Doubleclick or dragging .lxs in OSX Finder to LuxRender
bool LuxGuiApp::event(QEvent *event)
{
	switch (event->type()) {
        case QEvent::FileOpen:
			if (m_inputFile.isEmpty()){
				mainwin->loadFile(static_cast<QFileOpenEvent *>(event)->file());
				return true;
			}
        default:
            break;
    }
	return QApplication::event(event);
}
#endif

void LuxGuiApp::InfoDialogBox(const string &msg, const string &caption = "LuxRender") {
	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText(msg.c_str());
	msgBox.exec();
}

bool LuxGuiApp::ProcessCommandLine(void)
{
	try {
		const int line_length = 150;

		// allowed only on command line
		po::options_description generic("Generic options", line_length);
		generic.add_options()
			("version,v", "Print version string")
			("help,h", "Produce help message")
			("debug,d", "Enable debug mode")
			("fixedseed,f", "Disable random seed mode")
			("minepsilon,e", po::value< float >(), "Set minimum epsilon")
			("maxepsilon,E", po::value< float >(), "Set maximum epsilon")
			("verbose,V", "Increase output verbosity (show DEBUG messages)")
			("quiet,q", "Reduce output verbosity (hide INFO messages)")
			("very-quiet,x", "Reduce output verbosity even more (hide WARNING messages)")
			("configfile,C", po::value< std::string >(), "Specify the configuration file to use")
      ("list-file,L", po::value< string >(), "A file that contains a list of files to be rendered in the Queue")
		;

		// Declare a group of options that will be
		// allowed both on command line and in
		// config file
		po::options_description config("Configuration", line_length);
		config.add_options()
			("threads,t", po::value < int >(), "Specify the number of threads that Lux will run in parallel.")
			("useserver,u", po::value< vector<string> >()->composing(), "Specify the adress of a rendering server to use.")
			("serverinterval,i", po::value < int >(), "Specify the number of seconds between requests to rendering servers.")
			("logconsole,l", "Copy the output of the log tab to the console.")
		;

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden("Hidden options", line_length);
		hidden.add_options()
			("input-file", po::value < vector < string > >(), "input file")
		;

		po::options_description cmdline_options(line_length);
		cmdline_options.add(generic).add(config).add(hidden);

		po::options_description config_file_options(line_length);
		config_file_options.add(config).add(hidden);

		po::options_description visible("Allowed options", line_length);
		visible.add(generic).add(config);

		po::positional_options_description p;

		p.add("input-file", -1);

		po::variables_map vm;
		
		store(po::command_line_parser(m_argc, m_argv).
			options(cmdline_options).positional(p).run(), vm);

		std::string configFile("luxconsole.cfg");
		if (vm.count("configfile"))
			configFile = vm["configfile"].as<std::string>();
		std::ifstream ifs(configFile.c_str());
		store(parse_config_file(ifs, config_file_options), vm);
		po::notify(vm);

		if(vm.count("help")) {
			stringstream ss;
			ss << "Usage: luxrender [options] file..." << endl;
			visible.print(ss);
			ss << endl;
			InfoDialogBox(ss.str());
			return false;
		}

		if(vm.count("version")) {
			stringstream ss;
			ss << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__ << endl;
			InfoDialogBox(ss.str());			
			return false;
		}

		if(vm.count("logconsole"))
			m_copyLog2Console = true;
		else
			m_copyLog2Console = false;

		if(vm.count("threads")) {
			m_threads = vm["threads"].as < int >();
		} else {
			// Dade - check for the hardware concurrency available
			m_threads = boost::thread::hardware_concurrency();
			if (m_threads == 0)
				m_threads = 1;
		}

		if(vm.count("debug")) {
			LOG( LUX_INFO,LUX_NOERROR)<< "Debug mode enabled";
			luxEnableDebugMode();
		}

		if (vm.count("verbose")) {
			luxErrorFilter(LUX_DEBUG);
		}

		if (vm.count("quiet")) {
			luxErrorFilter(LUX_WARNING);
		}

		if (vm.count("very-quiet")) {
			luxErrorFilter(LUX_ERROR);
		}

		if (vm.count("fixedseed"))
			luxDisableRandomMode();

		int serverInterval;
		if(vm.count("serverinterval")) {
			serverInterval = vm["serverinterval"].as<int>();
			luxSetNetworkServerUpdateInterval(serverInterval);
		} else {
			serverInterval = luxGetNetworkServerUpdateInterval();
		}

		if(vm.count("useserver")) {
			stringstream ss;

			vector<string> names = vm["useserver"].as<vector<string> >();

			for(vector<string>::iterator i = names.begin(); i < names.end(); i++) {
				LOG(LUX_INFO,LUX_NOERROR) << "Connecting to server '" <<(*i) << "'";

				//TODO jromang : try to connect to the server, and get version number. display message to see if it was successfull
				luxAddServer((*i).c_str());
			}

			m_useServer = true;

			LOG( LUX_INFO,LUX_NOERROR) << "Server requests interval:  " << serverInterval << " secs";
		} else {
			m_useServer = false;
		}

		if(vm.count("input-file")) {
			const vector<string> &v = vm["input-file"].as<vector<string> >();
			if(v.size() > 1) {
				LOG( LUX_SEVERE,LUX_SYSTEM)<< "More than one file passed on command line : rendering the first one.";
			}

			m_inputFile = QString(v[0].c_str());
		} else {
			m_inputFile.clear();
		}

		// Any call to Lux API must be done _after_ luxAddServer
		if (vm.count("minepsilon")) {
			const float mine = vm["minepsilon"].as<float>();
			if (vm.count("maxepsilon")) {
				const float maxe = vm["maxepsilon"].as<float>();
				luxSetEpsilon(mine, maxe);
			} else
				luxSetEpsilon(mine, -1.f);
		} else {
			if (vm.count("maxepsilon")) {
				const float maxe = vm["maxepsilon"].as<float>();
				luxSetEpsilon(-1.f, maxe);
			} else
				luxSetEpsilon(-1.f, -1.f);
		}

		// Read file names for the Reander Queue
		if (vm.count("list-file")) {
			LOG( LUX_INFO,LUX_NOERROR) << "Reading file list from: " << vm["list-file"].as<string>().c_str();
			QFile listFile(vm["list-file"].as<string>().c_str());
			QString renderQueueEntry;
			if ( listFile.open(QIODevice::ReadOnly) ) {
        QTextStream lfStream(&listFile);
				while(!lfStream.atEnd()) {
					renderQueueEntry = lfStream.readLine();
					if (!renderQueueEntry.isNull()) {
            renderQueueList << renderQueueEntry;
					}
				};
			}
		}

		return true;
	} catch(std::exception &e) {
		cout << e.what() << endl;
		return false;
	}
}
