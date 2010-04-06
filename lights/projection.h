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
#include "shape.h"
#include "mipmap.h"

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
	SWCSpectrum Projection(const TsPack *tspack, const Vector &w) const;
	virtual float Power(const Scene *) const {
		return Lbase->Y() * gain * 
			2.f * M_PI * (1.f - cosTotalWidth) *
			projectionMap->LookupFloat(CHANNEL_WMEAN, .5f, .5f, .5f);
	}
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const;
	virtual float Pdf(const TsPack *tspack, const Point &p, const Normal &n,
		const Point &po, const Normal &ns) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *Le) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		VisibilityTester *visibility, SWCSpectrum *Le) const;
	virtual SWCSpectrum Le(const TsPack *tspack, const Scene *scene,
		const Ray &r, const Normal &n, BSDF **bsdf, float *pdf,
		float *pdfDirect) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// ProjectionLight Private Data
	MIPMap *projectionMap;
	Point lightPos;
	boost::shared_ptr<Texture<SWCSpectrum> > Lbase;
	DifferentialGeometry dummydg;
	float gain;
	Transform lightProjection;
	float hither, yon;
	float screenX0, screenX1, screenY0, screenY1, area;
	float cosTotalWidth;
};

}//namespace lux
