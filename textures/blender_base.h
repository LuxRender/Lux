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
#include "texture.h"
#include "paramset.h"
#include "blender_texlib.h"

namespace lux {

class BlenderTexture3D : public Texture<float> {
public:
	// BlenderBlendTexture3D Public Methods

	virtual ~BlenderTexture3D() { }

	BlenderTexture3D(const Transform &tex2world, const ParamSet &tp,
		short type) : mapping(tex2world) {
		// Apply texture specified transformation option for 3D mapping
		mapping.Apply3DTextureMappingOptions(tp);
		tex1 = tp.GetFloatTexture("tex1", 0.f);
		tex2 = tp.GetFloatTexture("tex2", 1.f);
		tex.type = type;
		tex.bright = tp.FindOneFloat("bright", 1.0f),
		tex.contrast = tp.FindOneFloat("contrast", 1.0f),
		tex.rfac = 1.0f;
		tex.gfac = 1.0f;
		tex.bfac = 1.0f;
	}

	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		const Point P = mapping.Map(dg);
		const float t1 = tex1->Evaluate(sw, dg);
		const float t2 = tex2->Evaluate(sw, dg);

		return Lerp(GetF(P), t1, t2);
	}
	virtual float Y() const { return (tex1->Y() + tex2->Y()) * .5f; }
	virtual float Filter() const {
		return (tex1->Filter() + tex2->Filter()) * .5f;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const;
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
protected:
	float GetF(const Point &P) const {
		blender::TexResult texres;
		const int resultType = multitex(&tex, &P.x, &texres);

		if (resultType & TEX_RGB)
			texres.tin = min(0.35f * texres.tr + 0.45f * texres.tg +
				0.2f * texres.tb, 1.f); // values are already >0

		return texres.tin;
	}
	static short GetBlendType(const string &name);
	static short GetCloudType(const string &name);
	static short GetMarbleType(const string &name);
	static short GetMusgraveType(const string &name);
	static short GetStucciType(const string &name);
	static short GetVoronoiType(const string &name);
	static short GetWoodType(const string &name);
	static short GetNoiseType(const string &name);
	static short GetNoiseBasis(const string &name);
	static short GetNoiseShape(const string &name);
	// BlenderBlendTexture3D Private Data
	IdentityMapping3D mapping;
	boost::shared_ptr<Texture<float> > tex1, tex2;
	blender::Tex tex;
};

} // namespace lux
