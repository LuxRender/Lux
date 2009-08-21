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

#include "tonemap.h"
#include "paramset.h"

namespace lux
{

// ReinhardOp Declarations
class ReinhardOp : public ToneMap {
public:
	ReinhardOp(float prS, float poS, float b);
	virtual ~ReinhardOp() { }
	virtual void Map(vector<Color> &xyz, int xRes, int yRes, float maxDisplayY) const;
	static ToneMap *CreateToneMap(const ParamSet &ps);
	
private:
	float pre_scale;
	float post_scale;
	float burn;
};

}//namespace lux

