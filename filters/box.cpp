/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// box.cpp*
#include "sampling.h"
#include "paramset.h"
// Box Filter Declarations
class BoxFilter : public Filter {
public:
	BoxFilter(float xw, float yw) : Filter(xw, yw) { }
	float Evaluate(float x, float y) const;
};
// Box Filter Method Definitions
float BoxFilter::Evaluate(float x, float y) const {
	return 1.;
}
extern "C" DLLEXPORT Filter *CreateFilter(const ParamSet &ps) {
	float xw = ps.FindOneFloat("xwidth", .5f);
	float yw = ps.FindOneFloat("ywidth", .5f);
	return new BoxFilter(xw, yw);
}
