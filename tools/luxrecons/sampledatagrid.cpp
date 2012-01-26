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

#include "lux.h"
#include "error.h"
#include "sampledatagrid.h"

using namespace lux;

SampleDataGrid::SampleDataGrid(SampleData *data) : sampleData(data) {
	//assert (sampleData->count > 0);

	// Manually add the first sample
	const float *imageXY = sampleData->GetImageXY(0);
	xPixelEnd = xPixelStart = (int)imageXY[0];
	yPixelEnd = yPixelStart = (int)imageXY[1];

	// Check all sample to calculate the image plane extension
	for (size_t i = 1; i < sampleData->count; ++i) {
		const float *imageXY = sampleData->GetImageXY(i);
		const int x = (int)imageXY[0];
		const int y = (int)imageXY[1];

		xPixelStart = min(xPixelStart, x);
		xPixelEnd = max(xPixelEnd, x);
		yPixelStart = min(yPixelStart, y);
		yPixelEnd = max(yPixelEnd, y);
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "SampleDataGrid xPixelStart: " << xPixelStart;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SampleDataGrid xPixelEnd: " << xPixelEnd;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SampleDataGrid yPixelStart: " << yPixelStart;
	LOG(LUX_DEBUG, LUX_NOERROR) << "SampleDataGrid yPixelEnd: " << yPixelEnd;

	// Resize the grid
	xResolution = xPixelEnd - xPixelStart + 1;
	yResolution = yPixelEnd - yPixelStart + 1;
	sampleList.resize(xResolution);
	for (size_t x = 0; x < xResolution; ++x)
		sampleList[x].resize(yResolution);

	// Add all samples
	for (size_t i = 0; i < sampleData->count; ++i) {
		const float *imageXY = sampleData->GetImageXY(i);
		const int x = ((int)imageXY[0]) - xPixelStart;
		const int y = ((int)imageXY[1]) - yPixelStart;
		sampleList[x][y].push_back(i);
	}
}

SampleDataGrid::~SampleDataGrid() {
}
