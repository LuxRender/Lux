/***************************************************************************
 *   Copyright (C) 1998-2012 by authors (see AUTHORS.txt )                 *
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

#include "api.h"
#include "error.h"
#include "commandline.h"

#include <iostream>
#include <exception>
#include <fstream>
#include <unistd.h>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>

using namespace lux;
namespace po = boost::program_options;

boost::filesystem::path getDefaultWorkingDirectory()
{
#if defined(WIN32)
	boost::filesystem::path workingDirectory = boost::filesystem::temp_directory_path() / "luxrender";
#else
	boost::filesystem::path workingDirectory = boost::filesystem::temp_directory_path() / std::string("luxrender-").append(boost::lexical_cast<std::string>(geteuid()));
#endif
	return workingDirectory;
}

bool ProcessCommandLine(int argc, char **argv, clConfig& config, unsigned int features, std::streambuf* infoBuf, std::streambuf* warnBuf)
{
	std::ostream info(infoBuf);
	std::ostream warn(warnBuf);

	try {
		// Declare a group of generic options
		// that will be allowed only on the command line
		po::options_description optGeneric("Generic options");
		optGeneric.add_options()
				("version,v", "Print version string")
				("help,h",    "Produce help message")
				;

		// Declare a group of standalone and master node options
		// that will be allowed on both the command line
		// and in a configuration file
		po::options_description optStandalone("Standalone / Master node options");
		if (features & featureSet::RENDERER) {
			optStandalone.add_options()
					("resume,r",         "Resume from FLM")
					("overrideresume,R", po::value< std::string >(), "Resume from specified FLM")
					("output,o",         po::value< std::string >(), "Output base filename")
					("fixedseed,f",      "Disable random seed mode")
					("minepsilon,e",     po::value< float >()->default_value(-1.f), "Set minimum epsilon")
					("maxepsilon,E",     po::value< float >()->default_value(-1.f), "Set maximum epsilon")
					("list-file,L",      po::value< std::string >(), "Specify the queue list file")
					;

			if (!(features & featureSet::INTERACTIVE))
				optStandalone.add_options()
					("bindump,b",        "Dump binary RGB framebuffer to stdout when finished")
					;
		}

		// Declare a group of master node options
		// that will be allowed on both the command line
		// and in a configuration file
		po::options_description optMaster("Master node options");
		if (features & featureSet::MASTERNODE) {
			optMaster.add_options()
					("useserver,u",      po::value< std::vector< std::string > >()->composing(), "Specify the address of a slave node to use\n(May be used multiple times)")
					("serverinterval,i", po::value< unsigned int >()->default_value(config.pollInterval), "Specify the number of seconds between update requests to slave nodes")
					("resetserver",      po::value< std::vector< std::string > >()->composing(), "Specify the address of a slave node to reset\n(May be used multiple times)")
					;
		}

		// Declare a group of slave node options
		// that will be allowed on both the command line
		// and in a configuration file
		po::options_description optSlave("Slave node options");
		if (features & featureSet::SLAVENODE) {
			optSlave.add_options()
					("server,s",         "Run as a slave node")
					("serverport,p",     po::value < unsigned int >()->default_value(config.tcpPort), "Specify the tcp port to listen on")
					("serverwriteflm,W", "Write film to disk before transmitting")
					("cachedir,c",       po::value< std::string >()->default_value((getDefaultWorkingDirectory() / "cache").string()), "Specify the cache directory to use")
					;
		}

		// Declare a group of configuration options
		// that will be allowed on both the command line
		// and in a configuration file
		po::options_description optConfig("Configuration options");
		optConfig.add_options()
				("verbose,V",     "Increase output verbosity (show DEBUG messages)")
				("quiet,q",       "Reduce output verbosity (hide INFO messages)")
				("very-quiet,x",  "Reduce output verbosity even more (hide WARNING messages)")
				("configfile,C",  po::value< std::string >()->default_value("luxconsole.cfg"), "Specify the configuration file to use")
				("logconsole,l",  "Copy the log to the console")
				("debug,d",       "Enable debug mode")
				;

		if (features & featureSet::RENDERER)
			optConfig.add_options()
				("threads,t",     po::value< unsigned int >(), "Specify the number of threads to run in parallel")
				;

		if (features & (featureSet::MASTERNODE | featureSet::SLAVENODE))
			optConfig.add_options()
				("password,P",    po::value< std::string >()->default_value(""), "Specify the reset password")
				;

		// Declare a group of hidden options
		// that will be allowed on both the command line
		// and in a configuration file
		// but will not be shown to the user
		po::options_description optHidden("Hidden options");
		if (features & featureSet::RENDERER) {
			optHidden.add_options()
				("input-file", po::value< std::vector< std::string > >(), "Specify the input file")
				("test", "Debug test mode")
				;
		}

		po::options_description optCommandLine;
		po::options_description optConfigFile;
		po::options_description optVisible;
		po::positional_options_description optPositional;

		optCommandLine.add(optGeneric);
		optVisible.add(optGeneric);

		if (features & featureSet::RENDERER) {
			optCommandLine.add(optHidden).add(optStandalone);
			optConfigFile.add(optHidden).add(optStandalone);
			optVisible.add(optStandalone);
		}

		if (features & featureSet::MASTERNODE) {
			optCommandLine.add(optMaster);
			optConfigFile.add(optMaster);
			optVisible.add(optMaster);
		}

		if (features & featureSet::SLAVENODE) {
			optCommandLine.add(optSlave);
			optConfigFile.add(optSlave);
			optVisible.add(optSlave);
		}

		optCommandLine.add(optConfig);
		optConfigFile.add(optConfig);
		optVisible.add(optConfig);

		if (features & featureSet::RENDERER)
			optPositional.add("input-file", -1);

		po::variables_map vm;
		// Disable guessing of option names
		int cmdstyle = po::command_line_style::default_style & ~po::command_line_style::allow_guessing;
		store(po::command_line_parser(argc, argv).
			style(cmdstyle).options(optCommandLine).positional(optPositional).run(), vm);

		// Load config file
		std::ifstream ifs(vm["configfile"].as<std::string>().c_str());
		store(parse_config_file(ifs, optConfigFile), vm);
		notify(vm);
		config.vm = vm;

		// BEGIN Handling generic options
		if (vm.count("help")) {
			if (features & featureSet::INTERACTIVE)
				info << "Usage: luxrender [options] file\n" << optVisible;
			else
				info << "Usage: luxconsole [options] file\n" << optVisible;
			return false;
		}

		if (vm.count("version")) {
			info << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;
			return false;
		}

		if (!(features & featureSet::INTERACTIVE))
			if (!vm.count("input-file") && !vm.count("list-file") && !vm.count("server") && !vm.count("resetserver")) {
				warn << "Usage: luxconsole [options] file\n" << optVisible;
				return false;
			}
		// END Handling generic options

		// BEGIN Handling configuration options
		LOG(LUX_INFO,LUX_NOERROR) << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;

		if (vm.count("logconsole"))
			config.log2console = true;

		if (vm.count("verbose"))
		{
			config.verbosity = 1;
			luxErrorFilter(LUX_DEBUG);
		}

		if (vm.count("quiet"))
		{
			config.verbosity = 2;
			luxErrorFilter(LUX_WARNING);
		}

		if (vm.count("very-quiet"))
		{
			config.verbosity = 3;
			luxErrorFilter(LUX_ERROR);
		}

		if (vm.count("debug")) {
			LOG(LUX_INFO,LUX_NOERROR) << "Debug mode enabled";
			luxEnableDebugMode();
		}

		if (vm.count("threads"))
			config.threadCount = vm["threads"].as<unsigned int>();
		else
			config.threadCount = std::max<unsigned int>(1, boost::thread::hardware_concurrency());
		LOG(LUX_INFO,LUX_NOERROR) << "Threads: " << config.threadCount;

		config.password = vm["password"].as<std::string>();

		// BEGIN Handling standalone and standalone / master node options
		if (!vm.count("server")) {
			// Warn when ignoring command line options meant for slave nodes
			for (po::variables_map::const_iterator it = vm.begin(); it != vm.end(); it++)
				if (optSlave.find_nothrow(it->first, false) && !it->second.defaulted())
					LOG(LUX_WARNING,LUX_CONSISTENCY) << "Ignoring command line option: " << it->first;

			// BEGIN Handling master node options
			if (vm.count("resetserver")) {
				std::vector<std::string> slaveNodes = vm["resetserver"].as< std::vector<std::string> >();
				std::string password = config.password;
				for (std::vector<std::string>::iterator it = slaveNodes.begin(); it < slaveNodes.end(); it++)
					luxResetServer((*it).c_str(), password.c_str());
			}

			config.pollInterval = vm["serverinterval"].as<unsigned int>();
			luxSetIntAttribute("render_farm", "pollingInterval", config.pollInterval);

			if (vm.count("useserver"))
				config.slaveNodeList = vm["useserver"].as< std::vector<std::string> >();
			// END Handling master node options

			// BEGIN Handling standalone / master node options
			config.fixedSeed = vm.count("fixedseed") != 0;

			// Any call to Lux API must be done _after_ luxAddServer
			luxSetEpsilon(vm["minepsilon"].as<float>(), vm["maxepsilon"].as<float>());

			if (vm.count("resume"))
				luxOverrideResumeFLM("");

			if (vm.count("overrideresume")) {
				std::string resumefile = vm["overrideresume"].as<std::string>();

				boost::filesystem::path resumePath(resumefile);
				if (boost::filesystem::exists(resumePath))
					luxOverrideResumeFLM(resumefile.c_str());
				else
					LOG(LUX_WARNING,LUX_NOFILE) << "Could not find resume file '" << resumefile << "', using filename in scene";
			}

			if (vm.count("output")) {
				std::string outputFile = vm["output"].as<std::string>();

				boost::filesystem::path outputPath(outputFile);
				if (boost::filesystem::exists(outputPath.parent_path()))
					luxOverrideFilename(outputFile.c_str());
				else
					LOG(LUX_WARNING,LUX_NOFILE) << "Could not find output path '" << outputPath.parent_path() << "', using output path in scenefile";
			}

			if (vm.count("list-file")) {
				std::string queueFile = vm["list-file"].as<std::string>();

				if (queueFile != "-")
				{
					boost::filesystem::path queueFileComplete(boost::filesystem::system_complete(queueFile));
					if (!queueFileComplete.empty())
						config.queueFile = queueFileComplete.string();
					else
						LOG(LUX_ERROR,LUX_NOFILE) << "Could not find queue file '" << queueFile << "'";
				} else
					config.queueFile = "-";
			}

			if (vm.count("input-file")) {
				std::vector<std::string> inputFiles = vm["input-file"].as< std::vector<std::string> >();
				for (std::vector<std::string>::iterator it = inputFiles.begin(); it < inputFiles.end(); it++)
				{
					if (*it != "-")
					{
						boost::filesystem::path inputFileComplete(boost::filesystem::system_complete(*it));
						if (!inputFileComplete.empty() && boost::filesystem::exists(inputFileComplete))
							config.inputFiles.push_back(inputFileComplete.string());
						else
							LOG(LUX_ERROR,LUX_NOFILE) << "Could not find scene file '" << *it << "'";
					}
					else
						config.inputFiles.push_back("-");
				}
			}

			if (vm.count("bindump"))
				config.binDump = true;
		// END Handling standalone and standalone / master node options

		// BEGIN Handling slave node options
		} else {
			config.slave = true;

			// Warn when ignoring command line options meant for standalone / master nodes
			for (po::variables_map::const_iterator it = vm.begin(); it != vm.end(); it++)
				if ((optStandalone.find_nothrow(it->first, false) || optMaster.find_nothrow(it->first, false)) && !it->second.defaulted())
					LOG(LUX_WARNING,LUX_CONSISTENCY) << "Ignoring command line option: " << it->first;
			if (vm.count("input-file"))
				LOG(LUX_WARNING,LUX_CONSISTENCY) << "Ignoring input file";

			config.tcpPort = vm["serverport"].as<unsigned int>();
			config.writeFlmFile = vm.count("serverwriteflm") != 0;

			std::string cachedir = vm["cachedir"].as<std::string>();
			boost::filesystem::path cachePath(cachedir);
			try {
				if (!boost::filesystem::is_directory(cachePath))
					boost::filesystem::create_directories(cachePath);

				boost::filesystem::current_path(cachePath);
			} catch (std::exception &e) {
				LOG(LUX_ERROR,LUX_NOFILE) << "Unable to use cache directory '" << cachedir << "': " << e.what();
				return false;
			}
			LOG(LUX_INFO,LUX_NOERROR) << "Using cache directory '" << cachedir << "'";
		}
		// END Handling slave node options

		return true;
	} catch(const std::exception &e) {
		warn << "Command line argument parsing failed with error '" << e.what() << "', please use the --help option to view the allowed syntax.";
		return false;
	}
}
