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

#ifndef LUX_EXRIO_H
#define LUX_EXRIO_H
// exrio.h*

#include "lux.h"
#include "color.h"

namespace lux
{

void WriteRGBAImage(const string &name,
	float *pixels, float *alpha, int XRes, int YRes,
	int totalXRes, int totalYRes, int xOffset, int yOffset);
void WriteRGBAImageFloat(const string &name,
	vector<Color> &pixels, vector<float> &alpha, int XRes, int YRes,
	int totalXRes, int totalYRes, int xOffset, int yOffset);

}

#endif // LUX_EXRIO_H

