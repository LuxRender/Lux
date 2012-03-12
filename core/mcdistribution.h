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

#ifndef LUX_MCDISTRIBUTION_H
#define LUX_MCDISTRIBUTION_H

#include "mc.h"

#include <cstring>
using std::memcpy;

namespace lux {

/**
 * A utility class evaluating a regularly sampled 1D function.
 */
class Function1D {
public:
	/**
	 * Creates a 1D function from the given data.
	 * It is assumed that the given function is sampled regularly sampled in
	 * the interval [0,1] (ex. 0.1, 0.3, 0.5, 0.7, 0.9 for 5 samples).
	 *
	 * @param f The values of the function.
	 * @param n The number of samples.
	 */
	Function1D(float *f, int n) {
		func = new float[n];
		count = n;
		memcpy(func, f, n*sizeof(float));
	}
	~Function1D() {
		delete[] func;
	}

	/**
	 * Evaluates the function at the given position.
	 * 
	 * @param x The x value to evaluate the function at.
	 *
	 * @return The function value at the given position.
	 */
	float Eval(float x) const {
		float pos = Clamp(x, 0.f, 1.f) * count + .5f;
		int off1 = (int)pos;
		int off2 = min(count-1, off1 + 1);
		float d = pos - off1;
		return func[off1] * (1.f - d) * func[off2] * d;
	}

	// Function1D Data
	/*
	 * The function values.
	 */
	float *func;
	/*
	 * The number of function values.
	 */
	int count;
};

/**
 * A utility class for sampling from a regularly sampled 1D distribution.
 */
class Distribution1D {
public:
	/**
	 * Creates a 1D distribution for the given function.
	 * It is assumed that the given function is sampled regularly sampled in
	 * the interval [0,1] (ex. 0.1, 0.3, 0.5, 0.7, 0.9 for 5 samples).
	 *
	 * @param f The values of the function.
	 * @param n The number of samples.
	 */
	Distribution1D(const float *f, u_int n) {
		func = new float[n];
		cdf = new float[n + 1];
		count = n;
		memcpy(func, f, n * sizeof(float));
		// funcInt is the sum of all f elements divided by the number
		// of elements, ie the average value of f over [0;1)
		ComputeStep1dCDF(func, n, &funcInt, cdf);
		invFuncInt = 1.f / funcInt;
		invCount = 1.f / count;
	}
	~Distribution1D() {
		delete[] func;
		delete[] cdf;
	}

	/**
	 * Samples from this distribution.
	 *
	 * @param u   The random value used to sample.
	 * @param pdf The pointer to the float where the pdf of the sample
	 *            should be stored.
	 * @param off Optional parameter to get the offset of the value
	 *
	 * @return The x value of the sample (i.e. the x in f(x)).
	 */ 
	float SampleContinuous(float u, float *pdf, u_int *off = NULL) const {
		// Find surrounding CDF segments and _offset_
		if (u >= cdf[count]) {
			*pdf = func[count - 1] * invFuncInt;
			if (off)
				*off = count - 1;
			return 1.f;
		}
		if (u <= cdf[0]) {
			*pdf = func[0] * invFuncInt;
			if (off)
				*off = 0;
			return 0.f;
		}
		float *ptr = std::upper_bound(cdf, cdf + count + 1, u);
		u_int offset = ptr - cdf - 1;

		// Compute offset along CDF segment
		const float du = (u - cdf[offset]) /
			(cdf[offset + 1] - cdf[offset]);

		// Compute PDF for sampled offset
		*pdf = func[offset] * invFuncInt;

		// Save offset
		if (off)
			*off = offset;

		// Return $x \in [0,1)$ corresponding to sample
		return (offset + du) * invCount;
	}

	/**
	 * Samples from this distribution.
	 *
	 * @param u   The random value used to sample.
	 * @param pdf The pointer to the float where the pdf of the sample
	 *            should be stored.
	 * @param du  Optional parameter to get the remaining offset
	 *
	 * @return The index of the sampled interval.
	 */ 
	u_int SampleDiscrete(float u, float *pdf, float *du = NULL) const {
		// Find surrounding CDF segments and _offset_
		if (u >= cdf[count]) {
			if (du)
				*du = 1.f;
			*pdf = func[count - 1] * invFuncInt * invCount;
			return count - 1;
		}
		if (u <= cdf[0]) {
			if (du)
				*du = 0.f;
			*pdf = func[0] * invFuncInt * invCount;
			return 0;
		}
		float *ptr = std::upper_bound(cdf, cdf + count + 1, u);
		u_int offset = ptr - cdf - 1;

		// Compute offset along CDF segment
		if (du)
			*du = (u - cdf[offset]) /
				(cdf[offset + 1] - cdf[offset]);

		// Compute PDF for sampled offset
		*pdf = func[offset] * invFuncInt * invCount;
		return offset;
	}
	float Pdf(u_int offset) const { return func[offset] * invFuncInt; }
	float Pdf(float u) const { return Pdf(Offset(u)); }
	float Average() const { return funcInt; }
	u_int Offset(float u) const {
		return min(count - 1, Floor2UInt(u * count));
	}

private:
	// Distribution1D Private Data
	/*
	 * The function and its cdf.
	 */
	float *func, *cdf;
	/**
	 * The function integral (assuming it is regularly sampled with an interval of 1),
	 * the inverted function integral and the inverted count.
	 */
	float funcInt, invFuncInt, invCount;
	/*
	 * The number of function values. The number of cdf values is count+1.
	 */
	u_int count;
};

class Distribution2D {
public:
	// Distribution2D Public Methods
	Distribution2D(const float *data, u_int nu, u_int nv) {
		pConditionalV.reserve(nv);
		// Compute conditional sampling distribution for $\tilde{v}$
		for (u_int v = 0; v < nv; ++v)
			pConditionalV.push_back(new Distribution1D(data + v * nu, nu));
		// Compute marginal sampling distribution $p[\tilde{v}]$
		vector<float> marginalFunc;
		marginalFunc.reserve(nv);
		for (u_int v = 0; v < nv; ++v)
			marginalFunc.push_back(pConditionalV[v]->Average());
		pMarginal = new Distribution1D(&marginalFunc[0], nv);
	}
	~Distribution2D() {
		delete pMarginal;
		for (u_int i = 0; i < pConditionalV.size(); ++i)
			delete pConditionalV[i];
	}
	void SampleContinuous(float u0, float u1, float uv[2],
		float *pdf) const {
		float pdfs[2];
		u_int v;
		uv[1] = pMarginal->SampleContinuous(u1, &pdfs[1], &v);
		uv[0] = pConditionalV[v]->SampleContinuous(u0, &pdfs[0]);
		*pdf = pdfs[0] * pdfs[1];
	}
	void SampleDiscrete(float u0, float u1, u_int uv[2], float *pdf) const {
		float pdfs[2];
		uv[1] = pMarginal->SampleDiscrete(u1, &pdfs[1]);
		uv[0] = pConditionalV[uv[1]]->SampleDiscrete(u0, &pdfs[0]);
		*pdf = pdfs[0] * pdfs[1];
	}
	float Pdf(float u, float v) const {
		return pConditionalV[pMarginal->Offset(v)]->Pdf(u) *
			pMarginal->Pdf(v);
	}
	float Average() const { return pMarginal->Average(); }
private:
	// Distribution2D Private Data
	vector<Distribution1D *> pConditionalV;
	Distribution1D *pMarginal;
};

/**
 * A utility class for evaluating an irregularly sampled 1D function.
 */
class IrregularFunction1D {
public:
	/**
	 * Creates a 1D function from the given data.
	 * It is assumed that the given x values are ordered, starting with the
	 * smallest value. The function value is clamped at the edges. It is
	 * assumed there are no duplicate sample locations.
	 *
	 * @param aX   The sample locations of the function.
	 * @param aFx  The values of the function.
	 * @param aN   The number of samples.
	 */
	IrregularFunction1D(float *aX, float *aFx, int aN) {
		count = aN;
		xFunc = new float[count];
		yFunc = new float[count];
		memcpy(xFunc, aX, aN*sizeof(float));
		memcpy(yFunc, aFx, aN*sizeof(float));
	}

	~IrregularFunction1D() {
		delete[] xFunc;
		delete[] yFunc;
	}

	/**
	 * Evaluates the function at the given position.
	 * 
	 * @param x The x value to evaluate the function at.
	 *
	 * @return The function value at the given position.
	 */
	float Eval(float x) const {
		if (x <= xFunc[0])
			return yFunc[0];
		if (x >= xFunc[count - 1])
			return yFunc[count - 1];

		float *ptr = std::upper_bound(xFunc, xFunc + count, x);
		const u_int offset = static_cast<u_int>(ptr - xFunc - 1);

		float d = (x - xFunc[offset]) / (xFunc[offset + 1] - xFunc[offset]);

		return Lerp(d, yFunc[offset], yFunc[offset + 1]);
	}

	/**
	 * Returns the index of the given position.
	 * 
	 * @param x The x value to get the index of.
	 * @param d The address to store the offset from the index in.
	 *
	 * @return The index of the given position.
	 */
	int IndexOf(float x, float *d) const {
		if (x <= xFunc[0]) {
			*d = 0.f;
			return 0;
		}
		if (x >= xFunc[count - 1]) {
			*d = 0.f;
			return count - 1;
		}

		float *ptr = std::upper_bound(xFunc, xFunc + count, x);
		int offset = ptr - xFunc - 1;

		*d = (x - xFunc[offset]) / (xFunc[offset + 1] - xFunc[offset]);
		return offset;
	}

private:
	// IrregularFunction1D Data
	/*
	 * The sample locations and the function values.
	 */
	float *xFunc, *yFunc;
	/*
	 * The number of function values. The number of cdf values is count+1.
	 */
	int count;
};

/**
 * A utility class for sampling from a irregularly sampled 1D distribution.
 */
class IrregularDistribution1D {
public:
	/**
	 * Creates a 1D distribution for the given function.
	 * It is assumed that the given x values are ordered, starting with the
	 * smallest value.
	 *
	 * @param aX0 The start of the sample interval.
	 * @param aX1 The end of the sample interval.
	 * @param aX  The sample locations of the function.
	 * @param aFx The values of the function.
	 * @param aN  The number of samples.
	 */
	IrregularDistribution1D(float aX0, float aX1, float *aX, float *aFx, int aN) {
		count = aN;
		x0 = aX0;
		x1 = aX1;
		xFunc = new float[count];
		yFunc = new float[count];
		xCdf = new float[count+1];
		yCdf = new float[count+1];
		memcpy(xFunc, aX, count*sizeof(float));
		memcpy(yFunc, aFx, count*sizeof(float));
		
		// Compute integrals of step function
		xCdf[0] = aX0;
		for (int i = 1; i < count; ++i)
			xCdf[i] = ( xFunc[i-1] + xFunc[i] ) * .5f;
		xCdf[count] = aX1;
		yCdf[0] = 0.f;
		for (int i = 1; i < count+1; ++i) {
			yCdf[i] = yCdf[i-1] + max( 1e-3f, yFunc[i-1] ) * ( xCdf[i] - xCdf[i-1] );
		}
		funcInt = yCdf[count];
		// Transform step function integral into cdf
		for (int i = 1; i < count+1; ++i)
			yCdf[i] /= funcInt;

		invFuncInt = 1.f / funcInt;
		invCount = 1.f / count;
	}

	~IrregularDistribution1D() {
		delete[] xFunc;
		delete[] yFunc;
		delete[] xCdf;
		delete[] yCdf;
	}

	/**
	 * Samples from this distribution.
	 *
	 * @param u   The random value used to sample.
	 * @param pdf The pointer to the float where the pdf of the sample should 
	 *            be stored.
	 *
	 * @return The x value of the sample (i.e. the x in f(x)).
	 */ 
	float Sample(float u, float *pdf) const {
		// Find surrounding cdf segments
		if (u >= yCdf[count]) {
			*pdf = xFunc[count] * invFuncInt;
			return xCdf[count];
		}
		if (u <= yCdf[0]) {
			*pdf = xFunc[0] * invFuncInt;
			return xCdf[0];
		}
		float *ptr = std::upper_bound(yCdf, yCdf + count + 1, u);
		int offset = ptr - yCdf - 1;
		// Return offset along current cdf segment
		float du = (u - yCdf[offset]) / (yCdf[offset + 1] - yCdf[offset]);
		*pdf = xFunc[offset] * invFuncInt;
		return Lerp(du, xCdf[offset], xCdf[offset + 1]);
	}

	/**
	 * Evaluates the function at the given position.
	 * 
	 * @param x The x value to evaluate the function at.
	 *
	 * @return The function value at the given position.
	 */
	float Eval(float x) const {
		if (x <= xFunc[0])
			return yFunc[0];
		if (x >= xFunc[count - 1])
			return yFunc[count - 1];

		float *ptr = std::upper_bound(xFunc, xFunc + count, x);
		int offset = ptr - xFunc - 1;

		float d = (x - xFunc[offset]) / (xFunc[offset + 1] - xFunc[offset]);

		return Lerp(d, yFunc[offset], yFunc[offset + 1]);
	}

	/**
	 * Returns the index of the given position.
	 * 
	 * @param x The x value to get the index of.
	 * @param d The address to store the offset from the index in.
	 *
	 * @return The index of the given position.
	 */
	int IndexOf(float x, float *d) const {
		if (x <= xFunc[0]) {
			*d = 0.f;
			return 0;
		}
		else if (x >= xFunc[count - 1]) {
			*d = 0.f;
			return count - 1;
		}

		float *ptr = std::upper_bound(xFunc, xFunc + count, x);
		int offset = ptr - xFunc - 1;

		*d = (x - xFunc[offset]) / (xFunc[offset + 1] - xFunc[offset]);
		return offset;
	}

	// IrregularDistribution1D Data
	/**
	 * The function interval.
	 */
	float x0, x1;
	/*
	 * The sample locations and the function values.
	 */
	float *xFunc, *yFunc;
	/*
	 * The sample locations of the cdf and the cdf values.
	 */
	float *xCdf, *yCdf;
	/**
	 * The function integral (of the scaled function!),
	 * the inverted function integral and the inverted count.
	 */
	float funcInt, invFuncInt, invCount;
	/*
	 * The number of function values. The number of cdf values is count+1.
	 */
	int count;
};

} //namespace lux

#endif //LUX_MCDISTRIBUTION_H
