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

#ifndef LUX_SLGRENDERER_H
#define LUX_SLGRENDERER_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include "hybridrenderer.h"
#include "mipmap.h"

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"
#include "slg.h"
#include "luxrays/utils/sdl/scene.h"

extern void DebugHandler(const char *msg);
extern void SDLDebugHandler(const char *msg);
extern void SLGDebugHandler(const char *msg);

namespace lux
{

class SLGRenderer;
class SLGHostDescription;

//------------------------------------------------------------------------------
// SLGDeviceDescription
//------------------------------------------------------------------------------

class SLGDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	u_int GetAvailableUnitsCount() const { return 1; }
	u_int GetUsedUnitsCount() const { return 1; }
	void SetUsedUnitsCount(const u_int units) { }

	friend class SLGRenderer;
	friend class SLGHostDescription;

protected:
	SLGDeviceDescription(SLGHostDescription *h, const string &n) : host(h), name(n) { }

	SLGHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// SLGHostDescription
//------------------------------------------------------------------------------

class SLGHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class SLGRenderer;
	friend class SLGDeviceDescription;

private:
	SLGHostDescription(SLGRenderer *r, const string &n);
	~SLGHostDescription();

	void AddDevice(SLGDeviceDescription *devDesc);

	SLGRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// SLGRenderer
//------------------------------------------------------------------------------

class SLGRenderer : public Renderer {
public:
	SLGRenderer(const luxrays::Properties &overwriteConfig);
	~SLGRenderer();

	RendererType GetType() const { return SLG_TYPE; }

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class SLGDeviceDescription;
	friend class SLGHostDescription;
	friend class SLGStatistics;

	static Renderer *CreateRenderer(const ParamSet &params);

private:
	class SLGTexMapInstanceInfo {
	public:
		SLGTexMapInstanceInfo() : name(""),
		uScale(1.f), vScale(1.f), uDelta(0.f), vDelta(0.f) { }
		
		string name;
		float uScale, vScale, uDelta, vDelta;
	};

	class SLGBumpMapInstanceInfo : public SLGTexMapInstanceInfo {
	public:
		SLGBumpMapInstanceInfo() : scale(1.f) { }

		float scale;
	};

	class SLGNormalMapInstanceInfo : public SLGTexMapInstanceInfo {
	};

	class SLGMaterialInfo {
	public:
		SLGMaterialInfo() : matName("mat_default"), color0(1.f), color1(1.f) { }

		string matName;
		luxrays::Spectrum color0, color1;

		SLGTexMapInstanceInfo texMap;
		SLGBumpMapInstanceInfo bumpMap;
		SLGNormalMapInstanceInfo normalMap;
	};

	void DefineSLGDefaultTexMap(luxrays::sdl::Scene *slgScene);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMap *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<unsigned char, 1> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<unsigned char, 3> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<unsigned char, 4> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<unsigned short, 1> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<unsigned short, 3> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<unsigned short, 4> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<float, 1> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<float, 3> > *mipMap, const float gamma);
	string GetSLGTexName(luxrays::sdl::Scene *slgScene, const MIPMapFastImpl<TextureColor<float, 4> > *mipMap, const float gamma);
	bool GetSLGBumpMapInfo(luxrays::sdl::Scene *slgScene, SLGMaterialInfo *matInfo,
		const Texture<float> *bump);
	bool GetSLGMaterialTexInfo(luxrays::sdl::Scene *slgScene, SLGMaterialInfo *matInfo,
		const Texture<SWCSpectrum> *tex0);
	bool GetSLGMaterialTexInfo(luxrays::sdl::Scene *slgScene, SLGMaterialInfo *matInfo,
		const Texture<SWCSpectrum> *tex0, const Texture<SWCSpectrum> *tex1);
	bool GetSLGMaterialInfo(luxrays::sdl::Scene *slgScene, const Primitive *prim, SLGMaterialInfo *matInfo);

	void ConvertEnvLights(luxrays::sdl::Scene *slgScene);
	vector<luxrays::ExtTriangleMesh *> DefinePrimitive(luxrays::sdl::Scene *slgScene, const Primitive *prim);
	void ConvertGeometry(luxrays::sdl::Scene *slgScene);
	luxrays::sdl::Scene *CreateSLGScene(const luxrays::Properties &slgConfigProps);
	luxrays::Properties CreateSLGConfig();

	mutable boost::mutex classWideMutex;

	RendererState state;
	vector<RendererHostDescription *> hosts;

	luxrays::Properties overwriteConfig;
	Scene *scene;
	vector<Normal *> alloctedMeshNormals;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_SLGRENDERER_H
