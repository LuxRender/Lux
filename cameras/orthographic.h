/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// orthographic.cpp*
#include "camera.h"

namespace lux
{

// OrthographicCamera Declarations
class OrthoCamera : public ProjectiveCamera {
public:
	// OrthoCamera Public Methods
	OrthoCamera(const MotionSystem &world2cam,
		const float Screen[4],
		float hither, float yon, float sopen, float sclose, int sdist, 
		float lensr, float focald, bool autofocus, Film *film);
	virtual ~OrthoCamera() { }

	virtual void SampleMotion(float time);

	virtual bool SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, SWCSpectrum *We) const;
	virtual bool SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, const Point &p, const Normal &n,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		float *pdfDirect, SWCSpectrum *We) const;
	virtual bool GetSamplePosition(const Point &p, const Vector &wi,
		float distance, float *x, float *y) const;
	virtual void ClampRay(Ray &ray) const;
	virtual bool IsDelta() const { return false; }
	virtual bool IsLensBased() const { return false; }
	virtual BBox Bounds() const;
	virtual void AutoFocus(const Scene &scene);

	virtual OrthoCamera* Clone() const { return new OrthoCamera(*this); }

	static Camera *CreateCamera(const MotionSystem &world2cam,
		const ParamSet &params, Film *film);

private:
	// Dade - field used for autofocus feature
	bool autoFocus;

	float screenDx,screenDy, posPdf;
	Normal normal;
};

}//namespace lux
