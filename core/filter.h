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

#ifndef LUX_FILTER_H
#define LUX_FILTER_H
// filter.h*

namespace lux
{

class Filter {
public:
	// Filter Interface
	virtual ~Filter() { }
	Filter(float xw, float yw) : xWidth(xw), yWidth(yw),
		invXWidth(1.f / xw), invYWidth(1.f / yw) {}
	virtual float Evaluate(float x, float y) const = 0;
	// Filter Public Data
	const float xWidth, yWidth;
	const float invXWidth, invYWidth;
};

}//namespace lux

#endif // LUX_FILTER_H
