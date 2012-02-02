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
		const int x = Floor2Int(imageXY[0] + .5f);
		const int y = Floor2Int(imageXY[1] + .5f);

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
		const int x = Floor2Int(imageXY[0] + .5f) - xPixelStart;
		const int y = Floor2Int(imageXY[1] + .5f) - yPixelStart;
		sampleList[x][y].push_back(i);
	}

	// Sanity check
	/*for (size_t y = 0; y < yResolution; ++y) {
		for (size_t x = 0; x < xResolution; ++x) {
			for (size_t i = 0; i < sampleList[x][y].size(); ++i) {
				const float *imageXY = data->GetImageXY(sampleList[x][y][i]);

				if ((fabsf(imageXY[0] - x  - xPixelStart) > .5f) || (fabsf(imageXY[1] - y - yPixelStart) > .5f))
					LOG(LUX_DEBUG, LUX_NOERROR) << "[" << imageXY[0] << ", " << imageXY[1] << "] "
							"=> [" << x << ", " << y << "] "
							"Delta(" << fabsf(imageXY[0] - x - xPixelStart) << ", " << fabsf(imageXY[1] - y - yPixelStart) << ")";
			}
		}
	}*/

	avgSamplesPerPixel = sampleData->count / (float)(xResolution * yResolution);
	LOG(LUX_DEBUG, LUX_NOERROR) << "SampleDataGrid avg. samples per pixel: " << avgSamplesPerPixel;
}

SampleDataGrid::~SampleDataGrid() {
}
