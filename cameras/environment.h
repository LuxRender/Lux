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

// environment.cpp*
#include "camera.h"

namespace lux
{

// EnvironmentCamera Declarations
class EnvironmentCamera : public Camera {
public:
	// EnvironmentCamera Public Methods
	EnvironmentCamera(const Transform &world2cam, float hither,
		float yon, float sopen, float sclose, Film *film);
	float GenerateRay(const Sample &sample, Ray *) const;
	bool IsVisibleFromEyes(const Scene *scene, const Point &p, Sample *sample_gen, Ray *ray_gen);;
	float GetConnectingFactor(const Point &p, const Vector &wo, const Normal &n);
	void GetFlux2RadianceFactors(Film *film, float *factors, int xPixelCount, int yPixelCount);
	bool IsDelta() const
	{
		return true;
	}
	static Camera *CreateCamera(const ParamSet &params, const Transform &world2cam, Film *film);
private:
	bool GenerateSample(const Point &p, Sample *sample) const;
	// EnvironmentCamera Private Data
	Point rayOrigin;
};

}//namespace lux
