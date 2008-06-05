/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#	include "wx/wx.h"
#endif

#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

#include "lux.h"
#include "api.h"
#include "error.h"

#include "wxluxapp.h"
#include "wxluxgui.h"

using namespace lux;
namespace po = boost::program_options;

IMPLEMENT_APP(LuxGuiApp)

// This is executed upon startup, like 'main()' in non-wxWidgets programs.
bool LuxGuiApp::OnInit() {
	// Set numeric format to standard to avoid errors when parsing files
	setlocale(LC_NUMERIC, "C");

	luxInit();

	m_guiFrame = new LuxGui((wxFrame*)NULL);
	m_guiFrame->Show(true);
	SetTopWindow(m_guiFrame);

	return ProcessCommandLine();
}

bool LuxGuiApp::ProcessCommandLine() {
	int threads;
	bool useServer, openglEnabled;

	// allowed only on command line
	po::options_description generic("Generic options");
	generic.add_options()
	  ("version,v", "Print version string")
	  ("help", "Produce help message")
	  ("debug,d", "Enable debug mode")
	;

	// Declare a group of options that will be
	// allowed both on command line and in
	// config file
	po::options_description config("Configuration");
	config.add_options()
	  ("threads,t", po::value < int >(), "Specify the number of threads that Lux will run in parallel.")
	  ("useserver,u", po::value< std::vector<std::string> >()->composing(), "Specify the adress of a rendering server to use.")
	  ("serverinterval,i", po::value < int >(), "Specify the number of seconds between requests to rendering servers.")
	;

	// Hidden options, will be allowed both on command line and
	// in config file, but will not be shown to the user.
	po::options_description hidden("Hidden options");
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

	po::options_description cmdline_options;
	cmdline_options.add(generic).add(config).add(hidden);

	po::options_description config_file_options;
	config_file_options.add(config).add(hidden);

	po::options_description visible("Allowed options");
	visible.add(generic).add(config);

	po::positional_options_description p;

	p.add("input-file", -1);

	po::variables_map vm;
	store(po::wcommand_line_parser(wxApp::argc, wxApp::argv).
	  options(cmdline_options).positional(p).run(), vm);

	std::ifstream ifs("luxrender.cfg");
	store(parse_config_file(ifs, config_file_options), vm);
	notify(vm);

	if(vm.count("help")) {
		std::cout << "Usage: luxrender [options] file..." << std::endl;
		std::cout << visible << std::endl;
		return false;
	}

	if(vm.count("version")) {
		std::cout << "Lux version " << LUX_VERSION_STRING << " of " << __DATE__ << " at " << __TIME__ << std::endl;
		return false;
	}

	if(vm.count("threads")) {
		threads = vm["threads"].as < int >();
	}
	else {
		threads = 1;;
	}

	if(vm.count("debug")) {
		luxError(LUX_NOERROR, LUX_INFO, "Debug mode enabled");
		luxEnableDebugMode();
	}

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

		useServer = true;

		ss.str("");
		ss << "Server requests interval:  " << serverInterval << " secs";
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	}

	if(vm.count("noopengl")) {
		openglEnabled = false;
	} else {
		#ifdef LUX_USE_OPENGL
			openglEnabled = true;
		#else
			openglEnabled = false;
			luxError(LUX_NOERROR, LUX_INFO, "GUI: OpenGL support was not compiled in - will not be used.");
		#endif // LUX_USE_OPENGL
	}

	if(vm.count("input-file")) {
		const std::vector < std::string > &v = vm["input-file"].as < vector < string > >();
		if(v.size() > 1) {
			luxError(LUX_SYSTEM, LUX_SEVERE, "More than one file passed on command line : rendering the first one.");
		}

		m_guiFrame->RenderScenefile(wxString(v[0].c_str(), wxConvUTF8));
		m_guiFrame->SetRenderThreads(threads);
	}

	return true;
}
