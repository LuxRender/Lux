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

// perspective.cpp*
#include "camera.h"

namespace lux
{

// PerspectiveCamera Declarations
class PerspectiveCamera : public ProjectiveCamera {
public:
	// PerspectiveCamera Public Methods
	PerspectiveCamera(const Transform &world2cam,
		const float Screen[4], float hither, float yon,
		float sopen, float sclose,
		float lensr, float focald, bool autofocus, float fov,
		Film *film);
	float GenerateRay(const Sample &sample, Ray *) const;
	bool Sample_W(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *We) const;
	bool Sample_W(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n, float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility, SWCSpectrum *We) const;
	float Pdf(const Point &p, const Normal &n, const Vector &wi) const;
	void GetSamplePosition(const Point &p, const Vector &wi, float *x, float *y) const;
	bool IsDelta() const
	{
		return LensRadius==0.0f;
	}
	void AutoFocus(Scene* scene);

	static Camera *CreateCamera(const Transform &world2cam, const ParamSet &params, Film *film);

private:
	Point pos;
	Normal normal;
	float fov;
	float posPdf;
	float screen[4];
	float R,xWidth,yHeight,xPixelWidth,yPixelHeight,Apixel;

	// Dade - field used for autofocus feature
	bool autoFocus;

	boost::shared_ptr<Shape> lens;
};

}//namespace lux
