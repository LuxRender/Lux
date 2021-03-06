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

#ifndef CAMERARESPONSE_H
#define CAMERARESPONSE_H
// cameraresponse.h*
// Original code by Daniel90
#include "lux.h"

namespace lux {

class CameraResponse {
public:
	CameraResponse(const string &film);
	void Map(RGBColor &rgb) const;

	string filmName;
	bool validFile;
private:
	float ApplyCrf(float point, const vector<float> &from, const vector<float> &to) const;
	bool loadPreset();
	bool loadFile();

	bool color;
	vector<float> RedI; // image irradiance (on the image plane)
	vector<float> RedB; // measured intensity
	vector<float> GreenI; // image irradiance (on the image plane)
	vector<float> GreenB; // measured intensity
	vector<float> BlueI; // image irradiance (on the image plane)
	vector<float> BlueB; // measured intensity
};

}

#endif
