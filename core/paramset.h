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

#ifndef LUX_PARAMSET_H
#define LUX_PARAMSET_H
// paramset.h*
#include "lux.h"

#include <boost/serialization/split_member.hpp>

#include <map>
using std::map;
#if (_MSC_VER >= 1400) // NOBOOK
#include <stdio.h>     // NOBOOK
#define snprintf _snprintf // NOBOOK
#endif // NOBOOK

namespace lux
{

// ParamSet Declarations
template <class T> struct ParamSetItem {
	// ParamSetItem Public Methods
	
	ParamSetItem<T> *Clone() const {
		return new ParamSetItem<T>(name, data, nItems);
	}
	ParamSetItem() { data=0; }
	// The const_cast forces a copy of the string data
	ParamSetItem(const string &n, const T *v, u_int ni = 1) :
		name(const_cast<string &>(n)), nItems(ni), lookedUp(false) {
		data = new T[nItems];
		for (u_int i = 0; i < nItems; ++i)
			data[i] = v[i];
	}
	~ParamSetItem();
	
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const {
		ar & name;
		ar & nItems;

		for (u_int i = 0; i < nItems; ++i)
			ar & data[i];

		ar & lookedUp;
	}

	template<class Archive>
	void load(Archive & ar, const unsigned int version) {
		ar & name;
		ar & nItems;
		if(data!=0)
			delete[] data;
		data=new T[nItems];
		for (u_int i = 0; i < nItems; ++i)
			ar & data[i];

		ar & lookedUp;
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()
	
	// ParamSetItem Data
	string name;
	u_int nItems;
	T *data;
	mutable bool lookedUp;
};
class  ParamSet {
	friend class boost::serialization::access;
	
public:
	// ParamSet Public Methods
	ParamSet() { }
	ParamSet &operator=(const ParamSet &p2);
	ParamSet(const ParamSet &p2);
	ParamSet(u_int n, const char *pluginName, const char * const tokens[], const char * const params[]);
	
	void Add(ParamSet& params);
	void AddFloat(const string &, const float *, u_int nItems = 1);
	void AddInt(const string &, const int *, u_int nItems = 1);
	void AddBool(const string &, const bool *, u_int nItems = 1);
	void AddPoint(const string &, const Point *, u_int nItems = 1);
	void AddVector(const string &, const Vector *, u_int nItems = 1);
	void AddNormal(const string &, const Normal *, u_int nItems = 1);
	void AddRGBColor(const string &, const RGBColor *, u_int nItems = 1);
	void AddString(const string &, const string *, u_int nItems = 1);
	void AddTexture(const string &, const string &);
	bool EraseInt(const string &);
	bool EraseBool(const string &);
	bool EraseFloat(const string &);
	bool ErasePoint(const string &);
	bool EraseVector(const string &);
	bool EraseNormal(const string &);
	bool EraseRGBColor(const string &);
	bool EraseString(const string &);
	bool EraseTexture(const string &);
	float FindOneFloat(const string &, float d) const;
	int FindOneInt(const string &, int d) const;
	bool FindOneBool(const string &, bool d) const;
	const Point &FindOnePoint(const string &, const Point &d) const;
	const Vector &FindOneVector(const string &, const Vector &d) const;
	const Normal &FindOneNormal(const string &, const Normal &d) const;
	const RGBColor &FindOneRGBColor(const string &, const RGBColor &d) const;
	const string &FindOneString(const string &, const string &d) const;
	const string &FindTexture(const string &) const;
	const float *FindFloat(const string &, u_int *nItems) const;
	const int *FindInt(const string &, u_int *nItems) const;
	const bool *FindBool(const string &, u_int *nItems) const;
	const Point *FindPoint(const string &, u_int *nItems) const;
	const Vector *FindVector(const string &, u_int *nItems) const;
	const Normal *FindNormal(const string &, u_int *nItems) const;
	const RGBColor *FindRGBColor(const string &,
	                             u_int *nItems) const;
	const string *FindString(const string &,
	                         u_int *nItems) const;
	void ReportUnused() const;
	~ParamSet() {
		Clear();
	}
	void Clear();
	string ToString() const;

private:
	// ParamSet Data
	vector<ParamSetItem<int> *> ints;
	vector<ParamSetItem<bool> *> bools;
	vector<ParamSetItem<float> *> floats;
	vector<ParamSetItem<Point> *> points;
	vector<ParamSetItem<Vector> *> vectors;
	vector<ParamSetItem<Normal> *> normals;
	vector<ParamSetItem<RGBColor> *> spectra;
	vector<ParamSetItem<string> *> strings;
	vector<ParamSetItem<string> *> textures;
	
	template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & ints;
			ar & bools;
			ar & floats;
			ar & points;
			ar & vectors;
			ar & normals;
			ar & spectra;
			ar & strings;
			ar & textures;
		}
	
};

// TextureParams Declarations
class  TextureParams {
public:
	// TextureParams Public Methods
	TextureParams(const ParamSet &geomp, const ParamSet &matp,
		map<string, boost::shared_ptr<Texture<float> > > &ft,
		map<string, boost::shared_ptr<Texture<SWCSpectrum> > > &st) :
		geomParams(geomp), materialParams(matp), floatTextures(ft),
		SWCSpectrumTextures(st) { }
	boost::shared_ptr<Texture<SWCSpectrum> > GetSWCSpectrumTexture(const string &name,
		const RGBColor &def) const;
	boost::shared_ptr<Texture<float> > GetFloatTexture(const string &name) const;
	boost::shared_ptr<Texture<float> > GetFloatTexture(const string &name,
		float def) const;
	float FindFloat(const string &n, float d) const {
		return geomParams.FindOneFloat(n,
			materialParams.FindOneFloat(n, d));
	}

	const float *FindFloats(const string &n, u_int *nItems) const {
		return geomParams.FindFloat(n, nItems);
	}

	const string &FindString(const string &n) const {
		return geomParams.FindOneString(n, materialParams.FindOneString(n, ""));
	}
	int FindInt(const string &n, int d) const {
		return geomParams.FindOneInt(n, materialParams.FindOneInt(n, d));
	}
	bool FindBool(const string &n, bool d) const {
		return geomParams.FindOneBool(n, materialParams.FindOneBool(n, d));
	}
	const Point &FindPoint(const string &n, const Point &d) const {
		return geomParams.FindOnePoint(n, materialParams.FindOnePoint(n, d));
	}
	const Vector &FindVector(const string &n, const Vector &d) const {
		return geomParams.FindOneVector(n, materialParams.FindOneVector(n, d));
	}
	const Normal &FindNormal(const string &n, const Normal &d) const {
		return geomParams.FindOneNormal(n, materialParams.FindOneNormal(n, d));
	}
	const RGBColor &FindRGBColor(const string &n, const RGBColor &d) const {
		return geomParams.FindOneRGBColor(n, materialParams.FindOneRGBColor(n, d));
	}
	void ReportUnused() const {
		//geomParams.ReportUnused(); // note - radiance - incompatible with mix material. (must recursiveley pass)
		materialParams.ReportUnused();
	}
	const ParamSet &GetGeomParams() const { return geomParams; }
	const ParamSet &GetMaterialParams() const { return materialParams; }
private:
	// TextureParams Private Data
	const ParamSet &geomParams, &materialParams;
	map<string, boost::shared_ptr<Texture<float> > >    &floatTextures;
	map<string, boost::shared_ptr<Texture<SWCSpectrum> > > &SWCSpectrumTextures;
};

}//namespace lux

#endif // LUX_PARAMSET_H
