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
#include <iostream>
#include <fstream>
#include <sstream>

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>

#include "lux.h"
#include "api.h"
#include "error.h"
#include "osfunc.h"
#include "epsilon.h"

#include "mainwindow.hxx"
#include "luxapp.hxx"

using namespace lux;
namespace po = boost::program_options;

LuxGuiApp::LuxGuiApp(int argc, char **argv) : QApplication(argc, argv)
{
	m_argc = argc;
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
	m_openglEnabled = true;

	if (ProcessCommandLine()) {
		mainwin = new MainWindow(0,m_openglEnabled,m_copyLog2Console);
		mainwin->show();
		mainwin->SetRenderThreads(m_threads);
		if (!m_inputFile.isEmpty())
			mainwin->renderScenefile(m_inputFile);
	} else {
	}	
}

void LuxGuiApp::InfoDialogBox(const std::string &msg, const std::string &caption = "LuxRender") {
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
			("verbosity,V", po::value< int >(), "Log output verbosity")
		;

		// Declare a group of options that will be
		// allowed both on command line and in
		// config file
		po::options_description config("Configuration", line_length);
		config.add_options()
			("threads,t", po::value < int >(), "Specify the number of threads that Lux will run in parallel.")
			("useserver,u", po::value< std::vector<std::string> >()->composing(), "Specify the adress of a rendering server to use.")
			("serverinterval,i", po::value < int >(), "Specify the number of seconds between requests to rendering servers.")
			("logconsole,l", "Copy the output of the log tab to the console.")
		;

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden("Hidden options", line_length);
		hidden.add_options()
			("input-file", po::value < vector < string > >(), "input file")
		;

		#ifdef LUX_USE_OPENGL
			generic.add_options()
				("noopengl", "Disable OpenGL to display the image")
			;
		#else
			hidden.add_options()
				("noopengl", "Disable OpenGL to display the image")
			;
		#endif // LUX_USE_OPENGL

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

		std::ifstream ifs("luxrender.cfg");
		store(parse_config_file(ifs, config_file_options), vm);
		po::notify(vm);

		if(vm.count("help")) {
			//std::cout << "Usage: luxrender [options] file..." << std::endl;
			//std::cout << visible << std::endl;
			std::stringstream ss;
			ss << "Usage: luxrender [options] file..." << std::endl;
			visible.print(ss);
			ss << std::endl;
			InfoDialogBox(ss.str());
			return false;
		}

		if (vm.count("verbosity"))
			luxErrorFilter(vm["verbosity"].as<int>());

		if(vm.count("version")) {
			//std::cout << "Lux version " << LUX_VERSION_STRING << " of " << __DATE__ << " at " << __TIME__ << std::endl;
			std::stringstream ss;
			ss << "Lux version " << LUX_VERSION_STRING << " of " << __DATE__ << " at " << __TIME__ << std::endl;
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
			m_threads = osHardwareConcurrency();
			if (m_threads == 0)
				m_threads = 1;
		}

		if(vm.count("debug")) {
			luxError(LUX_NOERROR, LUX_INFO, "Debug mode enabled");
			luxEnableDebugMode();
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
			std::stringstream ss;

			std::vector<std::string> names = vm["useserver"].as<std::vector<std::string> >();

			for(std::vector<std::string>::iterator i = names.begin(); i < names.end(); i++) {
				ss.str("");
				ss << "Connecting to server '" <<(*i) << "'";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

				//TODO jromang : try to connect to the server, and get version number. display message to see if it was successfull
				luxAddServer((*i).c_str());
			}

			m_useServer = true;

			ss.str("");
			ss << "Server requests interval:  " << serverInterval << " secs";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
		} else {
			m_useServer = false;
		}

		if(vm.count("noopengl")) {
			m_openglEnabled = false;
			luxError(LUX_SYSTEM, LUX_INFO, "OpenGL support will not be used.");
		} else {
			m_openglEnabled = true;
		}

		if(vm.count("input-file")) {
			const std::vector < std::string > &v = vm["input-file"].as < vector < string > >();
			if(v.size() > 1) {
				luxError(LUX_SYSTEM, LUX_SEVERE, "More than one file passed on command line : rendering the first one.");
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
				luxSetEpsilon(mine, DEFAULT_EPSILON_MAX);
		} else {
			if (vm.count("maxepsilon")) {
				const float maxe = vm["maxepsilon"].as<float>();
				luxSetEpsilon(DEFAULT_EPSILON_MIN, maxe);
			} else
				luxSetEpsilon(DEFAULT_EPSILON_MIN, DEFAULT_EPSILON_MAX);
		}

		return true;
	} catch(std::exception &e) {
		std::cout << e.what() << std::endl;
		return false;
	}
}
