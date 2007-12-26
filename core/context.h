/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_CONTEXT_H
#define LUX_CONTEXT_H

#include "api.h"
#include "paramset.h"
#include "color.h"
#include "scene.h"
#include "film.h"
#include "dynload.h"
#include "volume.h"
#include "../film/multiimage.h"
#include "error.h"
#include "renderfarm.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <boost/date_time/posix_time/posix_time.hpp>
using std::map;
#if (_MSC_VER >= 1400) // NOBOOK
#include <stdio.h>     // NOBOOK
#define snprintf _snprintf // NOBOOK
#endif // NOBOOK
#include "../renderer/include/asio.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

//TODO - jromang : convert to enum
#define STATE_UNINITIALIZED  0
#define STATE_OPTIONS_BLOCK  1
#define STATE_WORLD_BLOCK    2

namespace lux
{

class Context
{
	public:
 		

		Context(std::string n="Lux default context") : name(n)
		{
			currentApiState = STATE_OPTIONS_BLOCK;
			renderOptions = new RenderOptions;
			graphicsState = GraphicsState();
			luxCurrentScene=NULL;
		}

		//static bool checkMode(unsigned char modeMask, std::string callerName, int errorCode); //!< Check the graphics state mode in the active context

		//GState graphicsState;
		
		//TODO jromang - const & reference
		static Context* getActive() { return activeContext; }
		static void setActive(Context *c) { activeContext=c; }
		
		
		// API Function Declarations
		void identity();
		void translate(float dx, float dy, float dz);
		void rotate(float angle, float ax, float ay, float az);
		void scale(float sx, float sy, float sz);
		void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz);
		void concatTransform(float transform[16]);
		void transform(float transform[16]);
		void coordinateSystem(const string &);
		void coordSysTransform(const string &);
		void pixelFilter(const string &name, const ParamSet &params);
		void film(const string &type, const ParamSet &params);
		void sampler(const string &name, const ParamSet &params);
		void accelerator(const string &name, const ParamSet &params);
		void surfaceIntegrator(const string &name, const ParamSet &params);
		void volumeIntegrator(const string &name, const ParamSet &params);
		void camera(const string &, const ParamSet &cameraParams);
		void worldBegin();
		void attributeBegin();
		void attributeEnd();
		void transformBegin();
		void transformEnd();
		void texture(const string &name, const string &type, const string &texname, const ParamSet &params);
		void material(const string &name, const ParamSet &params);
		void lightSource(const string &name, const ParamSet &params);
		void areaLightSource(const string &name, const ParamSet &params);
		void portalShape(const string &name, const ParamSet &params);
		void shape(const string &name, const ParamSet &params);
		void reverseOrientation();
		void volume(const string &name, const ParamSet &params);
		void objectBegin(const string &name);
		void objectEnd();
		void objectInstance(const string &name);
		void worldEnd();

		//TODO - jromang replace by a destructor or remove
		void cleanup();
		
		
		//CORE engine control
		//user interactive thread functions		
		void start();
		void pause();
		void exit();

		//controlling number of threads
		int addThread();
		void removeThread();

		//framebuffer access
		void updateFramebuffer();
		unsigned char* framebuffer();
		int displayInterval();
		int filmXres();
		int filmYres();

		//film access (networking)
		void getFilm(std::basic_ostream<char> &stream);
		void updateFilmFromNetwork();

		//statistics
		double statistics(char *statName);
		void addServer(const string &name);
		

		
		
	private:
		static Context *activeContext;
		string name;
		Scene *luxCurrentScene;
		
		// API Local Classes
		struct RenderOptions {
			// RenderOptions Public Methods
			RenderOptions()
			{
						// RenderOptions Constructor Implementation
						FilterName = "mitchell";
						FilmName = "multiimage";
						SamplerName = "random";
						AcceleratorName = "kdtree";
						SurfIntegratorName = "path";
						VolIntegratorName = "emission";
						CameraName = "perspective";
						currentInstance = NULL;
					}
			Scene *MakeScene() const;
			// RenderOptions Public Data
			string FilterName;
			ParamSet FilterParams;
			string FilmName;
			ParamSet FilmParams;
			string SamplerName;
			ParamSet SamplerParams;
			string AcceleratorName;
			ParamSet AcceleratorParams;
			string SurfIntegratorName, VolIntegratorName;
			ParamSet SurfIntegratorParams, VolIntegratorParams;
			string CameraName;
			ParamSet CameraParams;
			Transform WorldToCamera;
			bool gotSearchPath;
			mutable vector<Light *> lights;
			mutable vector<Primitive* > primitives;
			mutable vector<VolumeRegion *> volumeRegions;
			map<string, vector<Primitive* > > instances;
			vector<Primitive* > *currentInstance;
		};
		
		struct GraphicsState {
			// Graphics State Methods
			GraphicsState(){
				// GraphicsState Constructor Implementation
				material = "matte";
				reverseOrientation = false;
			}
			// Graphics State
			map<string, boost::shared_ptr<Texture<float> > > floatTextures;
			map<string, boost::shared_ptr<Texture<Spectrum> > > spectrumTextures;
			ParamSet materialParams;
			string material;
			ParamSet areaLightParams;
			string areaLight;
			string currentLight;
			Light* currentLightPtr;
			bool reverseOrientation;
		};
		
		int currentApiState;
		Transform curTransform;
		map<string, Transform> namedCoordinateSystems;
		RenderOptions *renderOptions;
		GraphicsState graphicsState;
		vector<GraphicsState> pushedGraphicsStates;
		vector<Transform> pushedTransforms;
		RenderFarm renderFarm;


};


}

#endif //LUX_CONTEXT_H
