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

#include "sphericalfunction_ies.h"
#include "error.h"
#include "mcdistribution.h"

namespace lux {

IESSphericalFunction::IESSphericalFunction()
{
	initDummy();
}

IESSphericalFunction::IESSphericalFunction(const PhotometricDataIES& data,
	bool flipZ)
{
	if (data.m_PhotometricType != PhotometricDataIES::PHOTOMETRIC_TYPE_C) {
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "unsupported photometric type IES file, result may be wrong";
	}
	vector<double> vertAngles = data.m_VerticalAngles;
	vector<double> horizAngles = data.m_HorizontalAngles;
	vector<vector<double> > values = data.m_CandelaValues;

	// Add a begin/end vertical angle with 0 emission if necessary
	if (vertAngles[0] < 0.) {
		for (u_int i = 0; i < vertAngles.size(); ++i)
			vertAngles[i] = vertAngles[i] + 90.;
	}
	if (vertAngles[0] > 0.) {
		vertAngles.insert(vertAngles.begin(), max(0., vertAngles[0] - 1e-3));
		for (u_int i = 0; i < horizAngles.size(); ++i)
			values[i].insert(values[i].begin(), 0.);
		if (vertAngles[0] > 0.) {
			vertAngles.insert(vertAngles.begin(), 0.);
			for (u_int i = 0; i < horizAngles.size(); ++i)
				values[i].insert(values[i].begin(), 0.);
		}
	}
	if (vertAngles.back() < 180.) {
		vertAngles.push_back(min(180., vertAngles.back() + 1e-3));
		for (u_int i = 0; i < horizAngles.size(); ++i)
			values[i].push_back(0.);
		if (vertAngles.back() < 180.) {
			vertAngles.push_back(180.);
			for (u_int i = 0; i < horizAngles.size(); ++i)
				values[i].push_back(0.);
		}
	}
	// Generate missing horizontal angles
	if (horizAngles[0] == 0.) {
		if (horizAngles.size() == 1) {
			horizAngles.push_back(90.);
			vector<double> tmpVals = values[0];
			values.push_back(tmpVals);
		}
		if (horizAngles.back() == 90.) {
			for (int i = horizAngles.size() - 2; i >= 0; --i) {
				horizAngles.push_back(180. - horizAngles[i]);
				vector<double> tmpVals = values[i]; // copy before adding!
				values.push_back(tmpVals);
			}
		}
		if (horizAngles.back() == 180.) {
			for (int i = horizAngles.size() - 2; i >= 0; --i) {
				horizAngles.push_back(360. - horizAngles[i]);
				vector<double> tmpVals = values[i]; // copy before adding!
				values.push_back(tmpVals);
			}
		}
		if (horizAngles.back() != 360.) {
 			if ((360. - horizAngles.back()) !=
				(horizAngles.back() - horizAngles[horizAngles.size() - 2])) {
				LOG(LUX_ERROR, LUX_UNIMPLEMENT) <<
					"unsupported horizontal angles in IES file";
				initDummy();
				return;
			}
			horizAngles.push_back(360.);
			vector<double> tmpVals = values[0];
			values.push_back(tmpVals);
		}
	} else {
		LOG(LUX_ERROR, LUX_BADFILE) <<
			"invalid horizontal angles in IES file";
		initDummy();
		return;
	}

	// Initialize irregular functions
	float valueScale = data.m_CandelaMultiplier * 
		   data.BallastFactor * 
		   data.BallastLampPhotometricFactor;
	u_int nVFuncs = horizAngles.size();
	IrregularFunction1D** vFuncs = new IrregularFunction1D*[nVFuncs];
	u_int vFuncLength = vertAngles.size();
	float* vFuncX = new float[vFuncLength];
	float* vFuncY = new float[vFuncLength];
	float* uFuncX = new float[nVFuncs];
	float* uFuncY = new float[nVFuncs];
	for (u_int i = 0; i < nVFuncs; ++i) {
		for (u_int j = 0; j < vFuncLength; ++j) {
			vFuncX[j] = Clamp(Radians(vertAngles[j]) * INV_PI, 0.f, 1.f);
			vFuncY[j] = values[i][j] * valueScale;
		}

		vFuncs[i] = new IrregularFunction1D(vFuncX, vFuncY, vFuncLength);

		uFuncX[i] = Clamp(Radians(horizAngles[i] ) * INV_TWOPI, 0.f, 1.f);
		uFuncY[i] = i;
	}
	delete[] vFuncX;
	delete[] vFuncY;

	IrregularFunction1D* uFunc = new IrregularFunction1D(uFuncX, uFuncY, nVFuncs);
	delete[] uFuncX;
	delete[] uFuncY;

	// Resample the irregular functions
	u_int xRes = 512;
	u_int yRes = 256;
	TextureColor<float, 1>* img = new TextureColor<float, 1>[xRes * yRes];
	for (u_int y = 0; y < yRes; ++y) {
		const float t = (y + .5f) / yRes;
		for (u_int x = 0; x < xRes; ++x) {
			const float s = (x + .5f) / xRes;
			const float u = uFunc->Eval(s);
			const u_int u1 = Floor2UInt(u);
			const u_int u2 = min(nVFuncs - 1, u1 + 1);
			const float du = u - u1;
			const u_int tgtY = flipZ ? (yRes - 1) - y : y;
			img[x + tgtY * xRes] = Lerp(du, vFuncs[u1]->Eval(t),
				vFuncs[u2]->Eval(t));
		}
	}
	delete uFunc;
	for (u_int i = 0; i < nVFuncs; ++i)
		delete vFuncs[i];
	delete[] vFuncs;

	boost::shared_ptr<const MIPMap> ptr(new MIPMapFastImpl<TextureColor<float, 1> >(BILINEAR, xRes, yRes, &img[0]));
	SetMipMap(ptr);

	delete[] img;
}

void IESSphericalFunction::initDummy()
{
	TextureColor<float, 1> img[1] = {1.f};
	boost::shared_ptr<const MIPMap> ptr(new MIPMapFastImpl<TextureColor<float, 1> >(NEAREST, 1, 1, &img[0]));
	SetMipMap(ptr);
}

} //namespace lux
