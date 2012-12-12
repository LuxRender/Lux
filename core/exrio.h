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

#ifndef LUX_EXRIO_H
#define LUX_EXRIO_H
// exrio.h*

#include "lux.h"

namespace lux
{

// Write a generic EXR
void WriteOpenEXRImage(int channeltype, bool halftype, bool savezbuf, int compressiontype, const string &name, vector<RGBColor> &pixels,
        vector<float> &alpha, u_int xRes, u_int yRes,
        u_int totalXRes, u_int totalYRes,
        u_int xOffset, u_int yOffset, vector<float> &zbuf);

// Write a single channel float EXR
void WriteOpenEXRImage(const string &name, u_int xRes, u_int yRes, const float *map);

}//namespace lux

#endif // LUX_EXRIO_H

