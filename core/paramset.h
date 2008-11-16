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

#ifndef LUX_PARAMSET_H
#define LUX_PARAMSET_H
// paramset.h*
#include "lux.h"
#include "geometry.h"
#include "texture.h"
#include "color.h"



#include <map>
using std::map;
#if (_MSC_VER >= 1400) // NOBOOK
#include <stdio.h>     // NOBOOK
#define snprintf _snprintf // NOBOOK
#endif // NOBOOK
// ParamSet Macros
#define ADD_PARAM_TYPE(T, vec) \
	(vec).push_back(new ParamSetItem<T>(name, (T *)data, nItems))
#define LOOKUP_PTR(vec) \
	for (u_int i = 0; i < (vec).size(); ++i) \
		if ((vec)[i]->name == name) { \
			*nItems = (vec)[i]->nItems; \
			(vec)[i]->lookedUp = true; \
			return (vec)[i]->data; \
		} \
	return NULL
#define LOOKUP_ONE(vec) \
	for (u_int i = 0; i < (vec).size(); ++i) { \
		if ((vec)[i]->name == name && \
			(vec)[i]->nItems == 1) { \
			(vec)[i]->lookedUp = true; \
			return *((vec)[i]->data); \
}		} \
	return d

namespace lux
{

// ParamSet Declarations
class  ParamSet {
	friend class boost::serialization::access;
	
public:
	// ParamSet Public Methods
	ParamSet() { }
	ParamSet &operator=(const ParamSet &p2);
	ParamSet(const ParamSet &p2);
	ParamSet(int n, const char * pluginName,char* tokens[], char* params[]);
	
	void AddFloat(const string &, const float *, int nItems = 1);
	void AddInt(const string &,
	            const int *,
	            int nItems = 1);
	void AddBool(const string &,
	             const bool *,
	             int nItems = 1);
	void AddPoint(const string &,
	              const Point *,
	              int nItems = 1);
	void AddVector(const string &,
	               const Vector *,
	               int nItems = 1);
	void AddNormal(const string &,
	               const Normal *,
	               int nItems = 1);
	void AddRGBColor(const string &,
	                 const RGBColor *,
	                 int nItems = 1);
	void AddString(const string &,
	              const string *,
	              int nItems = 1);
	void AddTexture(const string &,
	                const string &);
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
	Point FindOnePoint(const string &, const Point &d) const;
	Vector FindOneVector(const string &, const Vector &d) const;
	Normal FindOneNormal(const string &, const Normal &d) const;
	RGBColor FindOneRGBColor(const string &,
	                         const RGBColor &d) const;
	string FindOneString(const string &, const string &d) const;
	string FindTexture(const string &) const;
	const float *FindFloat(const string &, int *nItems) const;
	const int *FindInt(const string &, int *nItems) const;
	const bool *FindBool(const string &, int *nItems) const;
	const Point *FindPoint(const string &, int *nItems) const;
	const Vector *FindVector(const string &, int *nItems) const;
	const Normal *FindNormal(const string &, int *nItems) const;
	const RGBColor *FindRGBColor(const string &,
	                             int *nItems) const;
	const string *FindString(const string &,
	                         int *nItems) const;
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
	
/*	enum StringValue
	{
	evNotDefined,
	evStringValue1,
	evStringValue2,
	evStringValue3,
	evEnd,
	evNumVals // list terminator
	};

	//typedef std::map<std::string, StringValue> StringMap;
	//typedef StringMap::value_type StringMapValue;

	static const std::map<std::string, StringValue>::value_type stringMapEntries[] =
	{
		std::map<std::string, StringValue>::value_type( "First Value", evStringValue1 ),
		std::map<std::string, StringValue>::value_type( "Second Value", evStringValue2 ),
		std::map<std::string, StringValue>::value_type( "Third Value", evStringValue3 ),
		std::map<std::string, StringValue>::value_type( "end", evEnd ),
	};

	static const std::map<std::string, StringValue> s_mapStringValues( &stringMapEntries[evStringValue1], &stringMapEntries[evNumVals] ); */
};
template <class T> struct ParamSetItem {
	// ParamSetItem Public Methods
	
	ParamSetItem<T> *Clone() const {
		return new ParamSetItem<T>(name, data, nItems);
	}
	ParamSetItem() { data=0; }
	ParamSetItem(const string &name, const T *val, int nItems = 1);
	~ParamSetItem() {

		if(data!=0)
	delete[] data;

	}
	
	template<class Archive>
				void save(Archive & ar, const unsigned int version) const
				{
					ar & name;
					ar & nItems;
								
					for(int i=0;i<nItems;i++)
							ar & data[i];
								
					ar & lookedUp;
				}
		
		template<class Archive>
					void load(Archive & ar, const unsigned int version)
					{
			
						ar & name;
						ar & nItems;
						if(data!=0) delete[] data;
						data=new T[nItems];
						for(int i=0;i<nItems;i++)
								ar & data[i];
									
						ar & lookedUp;
					}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	
	// ParamSetItem Data
	string name;
	int nItems;
	T *data;
	bool lookedUp;


};
// ParamSetItem Methods
/*
template<>
template<class Archive>
		void ParamSetItem<int>::serialize(Archive & ar, const unsigned int version)
		{
			ar & name;
			ar & nItems;
			//ar & data;
			ar & lookedUp;
		}*/

template <class T>
inline ParamSetItem<T>::ParamSetItem(const string &n,
							  const T *v,
							  int ni) {
	name = n;
	nItems = ni;
	data = new T[nItems];
	for (int i = 0; i < nItems; ++i)
		data[i] = v[i];
	lookedUp = false;
}

/*
#ifdef LUX_USE_SSE
template<>
inline ParamSetItem<Point>::ParamSetItem(const string &n,
							  const Point *v,
							  int ni) {
	name = n;
	nItems = ni;
	data = new Point[nItems];
	for (int i = 0; i < nItems; ++i)
	{
		data[i].x = v->x;
		data[i].y = v->y;
		data[i].z = v->z;
		//std::cout<<v->x <<','<<v->y<<','<<v->z<<std::endl;
	}
	lookedUp = false;
}
#endif
*/

// TextureParams Declarations
class  TextureParams {
public:
	// TextureParams Public Methods
	TextureParams(const ParamSet &geomp, const ParamSet &matp,
			map<string, boost::shared_ptr<Texture<float> > > &ft,
			map<string, boost::shared_ptr<Texture<SWCSpectrum> > > &st)
		: geomParams(geomp),
		  materialParams(matp),
		  floatTextures(ft),
		  SWCSpectrumTextures(st) {
	}
	boost::shared_ptr<Texture<SWCSpectrum> > GetSWCSpectrumTexture(const string &name,
	                                                         const RGBColor &def) const;
	boost::shared_ptr<Texture<float> > GetFloatTexture(const string &name) const;
	boost::shared_ptr<Texture<float> > GetFloatTexture(const string &name,
	                                                   float def) const;
	float FindFloat(const string &n, float d) const {
		return geomParams.FindOneFloat(n,
			materialParams.FindOneFloat(n, d));
	}

	const float *FindFloats(const string &n, int *nItems) const {
		return geomParams.FindFloat(n, nItems);
	}

	string FindString(const string &n) const {
		return geomParams.FindOneString(n, materialParams.FindOneString(n, ""));
	}
	int FindInt(const string &n, int d) const {
		return geomParams.FindOneInt(n, materialParams.FindOneInt(n, d));
	}
	bool FindBool(const string &n, bool d) const {
		return geomParams.FindOneBool(n, materialParams.FindOneBool(n, d));
	}
	Point FindPoint(const string &n, const Point &d) const {
		return geomParams.FindOnePoint(n, materialParams.FindOnePoint(n, d));
	}
	Vector FindVector(const string &n, const Vector &d) const {
		return geomParams.FindOneVector(n, materialParams.FindOneVector(n, d));
	}
	Normal FindNormal(const string &n, const Normal &d) const {
		return geomParams.FindOneNormal(n, materialParams.FindOneNormal(n, d));
	}
	RGBColor FindRGBColor(const string &n, const RGBColor &d) const {
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
