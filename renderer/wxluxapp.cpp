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

// For compilers that don't support precompilation, include "wx/wx.h"
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#	include <wx/wx.h>
#endif

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

#include "api.h"

#include "wxluxapp.h"
#include "wxluxgui.h"

namespace po = boost::program_options;

IMPLEMENT_APP(LuxGuiApp)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool LuxGuiApp::OnInit() {
	// Dade - initialize rand() number generator
	srand(time(NULL));

	// Set numeric format to standard to avoid errors when parsing files
	setlocale(LC_ALL, "C");

	luxInit();

	if(ProcessCommandLine()) {
		m_guiFrame = new LuxGui((wxFrame*)NULL, m_openglEnabled, m_copyLog2Console);
		m_guiFrame->Show(true);
		SetTopWindow(m_guiFrame);
		m_guiFrame->SetRenderThreads(m_threads);
		if(!m_inputFile.IsEmpty())
			m_guiFrame->RenderScenefile(m_inputFile);
		return true;
	} else {
		return false;
	}
}

void InfoDialogBox(const string &msg, const string &caption = "LuxRender") {
	wxMessageBox(wxString(msg.c_str(), wxConvUTF8), 
		wxString(caption.c_str(), wxConvUTF8), 
		wxOK | wxICON_INFORMATION);
}


bool LuxGuiApp::ProcessCommandLine() {
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
	#if wxUSE_UNICODE == 1
		store(po::wcommand_line_parser(wxApp::argc, wxApp::argv).
			options(cmdline_options).positional(p).run(), vm);
	#else // ANSI
		store(po::command_line_parser(wxApp::argc, wxApp::argv).
			options(cmdline_options).positional(p).run(), vm);
	#endif // Unicode/ANSI

		ifstream ifs("luxrender.cfg");
		store(parse_config_file(ifs, config_file_options), vm);
		notify(vm);

		if(vm.count("help")) {
			stringstream ss;
			ss << "Usage: luxrender [options] file..." << endl;
			visible.print(ss);
			ss << endl;
			InfoDialogBox(ss.str());			
			return false;
		}

		if (vm.count("verbosity"))
			luxErrorFilter(vm["verbosity"].as<int>());

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
			stringstream ss;

			vector<string> names = vm["useserver"].as<vector<string> >();

			for(vector<string>::iterator i = names.begin(); i < names.end(); i++) {
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
		} else {
			#ifdef LUX_USE_OPENGL
				#if wxUSE_GLCANVAS == 1
					m_openglEnabled = true;
				#else
					m_openglEnabled = false;
					luxError(LUX_NOERROR, LUX_INFO, "GUI: wxWidgets without suppport for OpenGL canvas - will not be used.");
				#endif // wxUSE_GLCANVAS
			#else
				m_openglEnabled = false;
				luxError(LUX_NOERROR, LUX_INFO, "GUI: OpenGL support was not compiled in - will not be used.");
			#endif // LUX_USE_OPENGL
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

		if(vm.count("input-file")) {
			const vector<string> &v = vm["input-file"].as<vector<string> >();
			if(v.size() > 1) {
				luxError(LUX_SYSTEM, LUX_SEVERE, "More than one file passed on command line : rendering the first one.");
			}

			m_inputFile = wxString(v[0].c_str(), wxConvUTF8);
		} else {
			m_inputFile.Clear();
		}

		return true;
	} catch(std::exception &e) {
		cout << e.what() << endl;
		return false;
	}
}
