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

// projection.cpp*
#include "lux.h"
#include "light.h"
#include "mipmap.h"
#include "texture.h"

namespace lux
{

// ProjectionLight Declarations
class ProjectionLight : public Light {
public:
	// ProjectionLight Public Methods
	ProjectionLight(const Transform &light2world, 
		const boost::shared_ptr< Texture<SWCSpectrum> > &L, float gain,
		const string &texname, float fov);
	virtual ~ProjectionLight();
	virtual bool IsDeltaLight() const { return true; }
	virtual bool IsEnvironmental() const { return false; }
	virtual float Power(const Scene *) const {
		return Lbase->Y() * gain * 
			2.f * M_PI * (1.f - cosTotalWidth) *
			projectionMap->LookupFloat(CHANNEL_WMEAN, .5f, .5f, .5f);
	}
	virtual float Pdf(const Point &p, const Point &po,
		const Normal &ns) const;
	virtual bool Sample_L(MemoryArena *arena, const Scene *scene,
		const Sample *sample, float u1, float u2, float u3, BSDF **bsdf,
		float *pdf, SWCSpectrum *Le) const;
	virtual bool Sample_L(MemoryArena *arena, const Scene *scene,
		const Sample *sample, const Point &p, float u1, float u2,
		float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *Le) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// ProjectionLight Private Data
	MIPMap *projectionMap;
	Point lightPos;
	boost::shared_ptr<Texture<SWCSpectrum> > Lbase;
	float gain;
	Transform lightProjection;
	float hither, yon;
	float screenX0, screenX1, screenY0, screenY1, area;
	float cosTotalWidth;
};

}//namespace lux
