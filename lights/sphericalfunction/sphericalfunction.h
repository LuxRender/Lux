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

#ifndef LUX_SPHERICALFUNCTION_H
#define LUX_SPHERICALFUNCTION_H

#include "lux.h"
#include "color.h"
#include "mipmap.h"

namespace lux {

/**
 * A simple interface for functions on a sphere.
 */
class SphericalFunction {
public:
	virtual ~SphericalFunction() {};

	/**
	 * Evaluates this function for the given direction.
	 * 
	 * @param w A normalized direction.
	 *
	 * @return The function value for the given direction.
	 */
	SWCSpectrum f(const TsPack *tspack, const Vector& w) const {
		return f(tspack, SphericalPhi(w), SphericalTheta(w));
	}

	/**
	 * Evaluates this function for the given direction.
	 * 
	 * @param phi   The angle in the xy plane.
	 * @param theta The angle from the z axis.
	 *
	 * @return The function value for the given direction.
	 */
	virtual SWCSpectrum f(const TsPack *tspack, float phi, float theta) const = 0;
};

/**
 * A basic spherical functions that is 1 everywhere.
 */
class NoopSphericalFunction : public SphericalFunction {
public:
	SWCSpectrum f(const TsPack *tspack, float phi, float theta) const {
		return SWCSpectrum(1.f);
	}
};

/**
 * A spherical functions that obtains its function values from a mipmap.
 */
class MipMapSphericalFunction : public SphericalFunction {
public:
	MipMapSphericalFunction();
	MipMapSphericalFunction(boost::shared_ptr<const MIPMap> &aMipMap,
		bool flipZ);

	void SetMipMap(boost::shared_ptr<const MIPMap> &aMipMap) {
		boost::shared_ptr<const MIPMap> mm(aMipMap);
		mipMap = mm;
	}

	using SphericalFunction::f;
	SWCSpectrum f(const TsPack *tspack, float phi, float theta) const;
private:
	boost::shared_ptr<const MIPMap> mipMap;
};

/**
 * A spherical functions that composes multiple spherical functions
 * by multiplying their results.
 */
class CompositeSphericalFunction : public SphericalFunction {
public:
	CompositeSphericalFunction() { }

	void Add(boost::shared_ptr<const SphericalFunction> &aFunc) {
		funcs.push_back(aFunc);
	}

	using SphericalFunction::f;
	SWCSpectrum f(const TsPack *tspack, float phi, float theta) const {
		SWCSpectrum ret(1.f);
		for (u_int i = 0; i < funcs.size(); ++i)
			ret *= funcs[i]->f(tspack, phi, theta);
		return ret;
	}
private:
	vector<boost::shared_ptr<const SphericalFunction> > funcs;
};

/**
 * A spherical functions that allows efficient sampling.
 */
class SampleableSphericalFunction : public SphericalFunction {
public:
	SampleableSphericalFunction(boost::shared_ptr<const SphericalFunction> &aFunc, 
		u_int xRes = 512, u_int yRes = 256);
	~SampleableSphericalFunction();

	using SphericalFunction::f;
	SWCSpectrum f(const TsPack *tspack, float phi, float theta) const;

	/**
	 * Samples this spherical function.
	 *
	 * @param u1  The first random value.
	 * @param u2  The second random value.
	 * @param w   The address to store the sampled direction in.
	 * @param pdf The address to store the pdf (w.r.t. solid angle) of the 
	 *            sample direction in.
	 *
	 * @return The function value of the sampled direction.
	 */
	SWCSpectrum Sample_f(const TsPack *tspack, float u1, float u2,
		Vector *w, float *pdf) const;

	/**
	 * Computes the pdf for sampling the given direction.
	 *
	 * @param w The direction.
	 *
	 * @return The pdf (w.r.t. solid angle) for the direction.
	 */
	float Pdf(const Vector& w) const;

	/**
	 * Returns the average function value over the entire sphere.
	 *
	 * @return The average function value.
	 */
	float Average_f() const;
private:
	Distribution2D* uvDistrib;
	boost::shared_ptr<const SphericalFunction> func;
};

/**
 * Creates a spherical function from the given parameters.
 *
 * @param ps The regular parameters.
 * @param tp The texture parameters.
 *
 * @return A spherical function or NULL.
 */
SphericalFunction *CreateSphericalFunction(const ParamSet &ps);

} // namespace lux

#endif //LUX_SPHERICALFUNCTION_H
