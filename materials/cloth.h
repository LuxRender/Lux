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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/


/*
 * This code is adapted from Mitsuba renderer by Wenzel Jakob (http://www.mitsuba-renderer.org/)
 * by permission.
 *
 * This file implements the Irawan & Marschner BRDF,
 * a realistic model for rendering woven materials.
 * This spatially-varying reflectance model uses an explicit description
 * of the underlying weave pattern to create fine-scale texture and
 * realistic reflections across a wide range of different weave types.
 * To use the model, you must provide a special weave pattern
 * file---for an example of what these look like, see the
 * examples scenes available on the Mitsuba website.
 *
 * For reference, it is described in detail in the PhD thesis of
 * Piti Irawan ("The Appearance of Woven Cloth").
 *
 * The code in Mitsuba is a modified port of a previous Java implementation
 * by Piti.
 *
 */

// cloth.cpp*
#include "lux.h"
#include "material.h"
#include "color.h"
#include "irawan.h"

namespace lux
{

// Cloth Class Declarations

class Cloth : public Material {
public:
	// Cloth Public Methods
	Cloth(boost::shared_ptr<Texture<SWCSpectrum> > &warp_kd,
		  boost::shared_ptr<Texture<SWCSpectrum> > &warp_ks,
		  boost::shared_ptr<Texture<SWCSpectrum> > &weft_kd,
		  boost::shared_ptr<Texture<SWCSpectrum> > &weft_ks,
		  boost::shared_ptr<WeavePattern> &pattern,
		  const ParamSet &mp);
	virtual ~Cloth() { }

	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	static Material * CreateMaterial(const Transform &xform, const ParamSet &mp);
private:
	// Cloth Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > warp_Kd, warp_Ks, weft_Kd, weft_Ks;
	boost::shared_ptr<WeavePattern> Pattern;
	float specularNormalization;
};

}//namespace lux
