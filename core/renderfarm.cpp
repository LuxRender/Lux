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

#include "lux.h"
#include "api.h"
#include "error.h"
#include "paramset.h"
#include "renderfarm.h"

#include <boost/iostreams/filtering_stream.hpp>
//#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

using namespace boost::iostreams;
using namespace lux;
using asio::ip::tcp;

bool RenderFarm::connect(const string &serverName) {
	serverList.push_back(std::string(serverName));

	return true;
}

void RenderFarm::flush() {
	//flush network buffer
	for (vector<string>::iterator server = serverList.begin(); server
			!= serverList.end(); ++server) {
		try
		{
			tcp::iostream stream((*server).c_str(), "18018");
			stream<<netBuffer.str()<<std::endl;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	}
}

void RenderFarm::updateFilm(MultiImageFilm *film) {
	for (vector<string>::iterator server = serverList.begin(); server
			!= serverList.end(); ++server) {
		try
		{
			//std::cout << "getting film from "<<*server<<std::endl;
			{
				std::stringstream ss;
				ss<<"Getting film from from '"<<(*server)<<"'";
				luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());
			}
			
			tcp::iostream stream((*server).c_str(), "18018");
			//std::cout << "connected"<<std::endl;
			
			stream<<"luxGetFilm"<<std::endl;
			
			//std::cout<<"getfilm"<<std::endl;
			
			filtering_stream<input> in;
			in.push(zlib_decompressor());
			in.push(stream);
			
			//std::cout<<"in"<<std::endl;
			
			boost::archive::text_iarchive ia(in);
			MultiImageFilm m(320,200);
			
			//std::cout<<"before ia>>m"<<std::endl;
			ia>>m;
			
			//std::cout<<"ok, i got the film! merging...";
			{
				std::stringstream ss;
				ss<<"Recieved film from '"<<(*server)<<"'";
				luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());
			}
			
			film->merge(m);
			//std::cout<<"merged!"<<std::endl;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	}
}

void RenderFarm::send(const std::string &command) {
	//std::cout<<"sending "<<command<<std::endl;
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
	//	try
	//	{
			//std::cout<<(*server)<<std::endl;
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl;
	//	}
	//	catch (std::exception& e)
	//	{
	//		luxError(LUX_SYSTEM,LUX_ERROR,e.what());
	//	}
	//}
}

void RenderFarm::send(const std::string &command, const std::string &name,
		const ParamSet &params) {
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
		try
		{
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl<<name<<' ';
			boost::archive::text_oarchive oa(netBuffer);
			oa<<params;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	//}
}

void RenderFarm::send(const std::string &command, const std::string &name) {
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
		try
		{
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl<<name<<std::endl;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	//}
}

void RenderFarm::send(const std::string &command, float x, float y, float z) {
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
		try
		{
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl<<x<<' '<<y<<' '<<z<<std::endl;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	//}
}

void RenderFarm::send(const std::string &command, float a, float x, float y,
		float z) {
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
		try
		{
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl<<a<<' '<<x<<' '<<y<<' '<<z<<std::endl;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	//}
}

void RenderFarm::send(const std::string &command, float ex, float ey, float ez,
		float lx, float ly, float lz, float ux, float uy, float uz) {
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
		try
		{
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl<<ex<<' '<<ey<<' '<<ez<<' '<<lx<<' '<<ly<<' '<<lz<<' '<<ux<<' '<<uy<<' '<<uz<<std::endl;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	//}
}

void RenderFarm::send(const std::string &command, float tr[16]) {
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
		try
		{
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl;//<<x<<' '<<y<<' '<<z<<' ';
			for(int i=0;i<16;i++)
			netBuffer<<tr[i]<<' ';
			netBuffer<<std::endl;
		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	//}
}

void RenderFarm::send(const std::string &command, const string &name,
		const string &type, const string &texname, const ParamSet &params) {
	//Send command to the render servers
	//for (vector<string>::iterator server = serverList.begin(); server
	//		!= serverList.end(); ++server) {
		try
		{
			//tcp::iostream stream((*server).c_str(), "18018");
			netBuffer<<command<<std::endl<<name<<' '<<type<<' '<<texname<<' ';
			boost::archive::text_oarchive oa(netBuffer);
			oa<<params;

			//send the file
			std::string file="";
			file=params.FindOneString(std::string("filename"),file);
			if(file.size())
			{
				std::string s;
				std::ifstream in(file.c_str(),std::ios::out|std::ios::binary);
				while(getline(in,s))
				netBuffer<<s<<"\n";
				netBuffer<<"LUX_END_FILE\n";
			}

		}
		catch (std::exception& e) {luxError(LUX_SYSTEM,LUX_ERROR,e.what());}
	//}
}
