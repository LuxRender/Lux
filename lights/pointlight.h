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

// point.cpp*
#include "lux.h"
#include "light.h"
#include "shape.h"
#include "scene.h"
#include "texture.h"
#include "sphericalfunction.h"

namespace lux
{

// PointLight Declarations
class PointLight : public Light {
public:
	// PointLight Public Methods
	PointLight(const Transform &light2world, 
		const boost::shared_ptr< Texture<SWCSpectrum> > &L, float gain,
		SampleableSphericalFunction *ssf);
	virtual ~PointLight();
	virtual bool IsDeltaLight() const { return true; }
	virtual bool IsEnvironmental() const { return false; }
	virtual float Power(const Scene *) const;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Point &P, float u1, float u2, float u3,
		Vector *wo, float *pdf, VisibilityTester *visibility) const;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	virtual float Pdf(const TsPack *tspack, const Point &, const Vector &) const;
	virtual float Pdf(const TsPack *tspack, const Point &p, const Normal &n,
		const Point &po, const Normal &ns) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
		VisibilityTester *visibility, SWCSpectrum *Le) const;
	virtual SWCSpectrum Le(const TsPack *tspack, const Scene *scene, const Ray &r,
		const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	/**
	 * Return the emmitted radiance in the given diretion.
	 *
	 * @param tspack The thread specific pack.
	 * @param w      The normalized direction in LOCAL coordinates.
	 * 
	 * @return The emmitted radiance.
	 */
	SWCSpectrum L(const TsPack *tspack, const Vector& w) const;
	// PointLight Private Data
	Point lightPos;
	bool flipZ;
	boost::shared_ptr< Texture<SWCSpectrum> > Lbase;
	DifferentialGeometry dummydg;
	float gain;
	SampleableSphericalFunction *func;
};

}//namespace lux

