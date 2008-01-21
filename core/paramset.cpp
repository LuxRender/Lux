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

// paramset.cpp*
#include "paramset.h"
#include "error.h"
#include <sstream>
#include <string>

using namespace lux;

// ParamSet Methods
ParamSet::ParamSet(const ParamSet &p2) {
	*this = p2;
}
/* Here we create a paramset with the argument list provided by the C API.
 * We have to 'guess' the parameter type according the the parameter's name
 */
ParamSet::ParamSet(int n, const char * pluginName, char* tokens[], char* params[])
{
	//TODO - jromang : implement this using a std::map or string hashing
	std::string p(pluginName);
	
	for(int i=0;i<n;i++)
	{
		std::string s(tokens[i]);
		//float parameters
		if(s=="xwidth") { std::cout<<*(float*)params[i]<<std::endl; AddFloat(s,(float*)(params[i])); }
		if(s=="ywidth") AddFloat(s,(float*)(params[i]));
		if(s=="aplha") AddFloat(s,(float*)(params[i]));
		if(s=="B") AddFloat(s,(float*)(params[i]));
		if(s=="C") AddFloat(s,(float*)(params[i]));
		if(s=="tau") AddFloat(s,(float*)(params[i]));
		if(s=="hither") AddFloat(s,(float*)(params[i]));
		if(s=="yon") AddFloat(s,(float*)(params[i]));
		if(s=="shutteropen") AddFloat(s,(float*)(params[i]));
		if(s=="shutterclose") AddFloat(s,(float*)(params[i]));
		if(s=="lensradius") AddFloat(s,(float*)(params[i]));
		if(s=="focaldistance") AddFloat(s,(float*)(params[i]));
		if(s=="frameaspectratio") AddFloat(s,(float*)(params[i]));
		if(s=="screenwindow") AddFloat(s,(float*)(params[i]),4);
		if(s=="fov") AddFloat(s,(float*)(params[i]));
		if(s=="cropwindow") AddFloat(s,(float*)(params[i]),4);
		if(s=="radius") AddFloat(s,(float*)(params[i]));
		if(s=="height") AddFloat(s,(float*)(params[i]));
		if(s=="phimax") AddFloat(s,(float*)(params[i]));
		if(s=="zmin") AddFloat(s,(float*)(params[i]));
		if(s=="zmax") AddFloat(s,(float*)(params[i]));
		if(s=="innerradius") AddFloat(s,(float*)(params[i]));
		if(s=="uknots") AddFloat(s,(float*)(params[i]),FindOneInt("nu", i)+FindOneInt("uorder", i));
		if(s=="vknots") AddFloat(s,(float*)(params[i]),FindOneInt("nv", i)+FindOneInt("vorder", i));
		if(s=="u0") AddFloat(s,(float*)(params[i]));
		if(s=="v0") AddFloat(s,(float*)(params[i]));
		if(s=="u1") AddFloat(s,(float*)(params[i]));
		if(s=="v1") AddFloat(s,(float*)(params[i]));
		if(s=="Pw") AddFloat(s,(float*)(params[i]),4*FindOneInt("nu", i)*FindOneInt("nv", i));
		if(s=="st") AddFloat(s,(float*)(params[i]),2*FindOneInt("nvertices", i));  // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if(s=="uv") AddFloat(s,(float*)(params[i]),2*FindOneInt("nvertices", i));  // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if(s=="emptybonus") AddInt(s,(int*)(params[i]));
		if(s=="uscale") AddFloat(s,(float*)(params[i]));
		if(s=="vscale") AddFloat(s,(float*)(params[i]));
		if(s=="udelta") AddFloat(s,(float*)(params[i]));
		if(s=="vdelta") AddFloat(s,(float*)(params[i]));
		if(s=="v00") AddFloat(s,(float*)(params[i]));
		if(s=="v01") AddFloat(s,(float*)(params[i]));
		if(s=="v10") AddFloat(s,(float*)(params[i]));
		if(s=="v11") AddFloat(s,(float*)(params[i]));
		if(s=="maxanisotropy") AddFloat(s,(float*)(params[i]));
		if(s=="roughness" && p=="fbm") AddTexture(s,std::string(params[i]));
		if(s=="roughness" && p=="wrinkled") AddTexture(s,std::string(params[i]));
		if(s=="roughness" && p=="marble") AddTexture(s,std::string(params[i]));
		if(s=="variation") AddFloat(s,(float*)(params[i]));
		if(s=="g") AddFloat(s,(float*)(params[i]));
		if(s=="a") AddFloat(s,(float*)(params[i]));
		if(s=="b") AddFloat(s,(float*)(params[i]));
		if(s=="density") AddFloat(s,(float*)(params[i]),FindOneInt("nx", i)*FindOneInt("ny", i)*FindOneInt("ny", i));
		if(s=="coneangle") AddFloat(s,(float*)(params[i]));
		if(s=="conedeltaangle") AddFloat(s,(float*)(params[i]));
		if(s=="maxdist") AddFloat(s,(float*)(params[i]));
		if(s=="maxerror") AddFloat(s,(float*)(params[i]));
		if(s=="stepsize") AddFloat(s,(float*)(params[i]));
		
		//int parameters
		if(s=="pixelsamples") AddInt(s,(int*)(params[i]));
		if(s=="xsamples") AddInt(s,(int*)(params[i]));
		if(s=="ysamples") AddInt(s,(int*)(params[i]));
		if(s=="xresolution") AddInt(s,(int*)(params[i]));
		if(s=="yresolution") AddInt(s,(int*)(params[i]));
		if(s=="writefrequency") AddInt(s,(int*)(params[i]));
		if(s=="premultiplyalpha") AddInt(s,(int*)(params[i]));
		if(s=="nu") AddInt(s,(int*)(params[i]));
		if(s=="nv") AddInt(s,(int*)(params[i]));
		if(s=="uorder") AddInt(s,(int*)(params[i]));
		if(s=="vorder") AddInt(s,(int*)(params[i]));
		if(s=="nlevels") AddInt(s,(int*)(params[i]));
		if(s=="indices") AddInt(s,(int*)(params[i]),FindOneInt("nvertices", i));  // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if(s=="intersectcost") AddInt(s,(int*)(params[i]));
		if(s=="traversalcost") AddInt(s,(int*)(params[i]));
		if(s=="maxprims") AddInt(s,(int*)(params[i]));
		if(s=="maxdepth") AddInt(s,(int*)(params[i]));
		if(s=="dimension") AddInt(s,(int*)(params[i]));
		if(s=="octaves") AddInt(s,(int*)(params[i]));
		if(s=="nx") AddInt(s,(int*)(params[i]));
		if(s=="ny") AddInt(s,(int*)(params[i]));
		if(s=="nz") AddInt(s,(int*)(params[i]));
		if(s=="nsamples") AddInt(s,(int*)(params[i]));
		if(s=="causticphotons") AddInt(s,(int*)(params[i]));
		if(s=="indirectphotons") AddInt(s,(int*)(params[i]));
		if(s=="directphotons") AddInt(s,(int*)(params[i]));
		if(s=="nused") AddInt(s,(int*)(params[i]));
		if(s=="finalgathersamples") AddInt(s,(int*)(params[i]));
		if(s=="maxspeculardepth") AddInt(s,(int*)(params[i]));
		if(s=="maxindirectdepth") AddInt(s,(int*)(params[i]));
		
		
		//bool parameters
		if(s=="jitter") AddBool(s,(bool*)(params[i]));
		if(s=="refineimmediately") AddBool(s,(bool*)(params[i]));
		if(s=="trilinear") AddBool(s,(bool*)(params[i]));
		if(s=="finalgather") AddBool(s,(bool*)(params[i]));
		if(s=="directwithphotons") AddBool(s,(bool*)(params[i]));
		
		//string parameters
		if(s=="filename") { string *str=new string(params[i]); std::cout<<*str<<std::endl; AddString(s,str); delete str; }
		if(s=="mapping") AddString(s,(string*)(params[i]));
		if(s=="wrap") AddString(s,(string*)(params[i]));
		if(s=="aamode") AddString(s,(string*)(params[i]));
		if(s=="mapname") AddString(s,(string*)(params[i]));
		if(s=="strategy") AddString(s,(string*)(params[i]));
		
		//point parameters
		if(s=="p1") AddPoint(s,(Point*)(params[i]));
		if(s=="p2") AddPoint(s,(Point*)(params[i]));
		if(s=="P" && p=="nurbs") AddPoint(s,(Point*)(params[i]),FindOneInt("nu", i)*FindOneInt("nv", i));
		if(s=="P" && p=="trianglemesh") AddPoint(s,(Point*)(params[i]),FindOneInt("nvertices", i)); // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if(s=="p0") AddPoint(s,(Point*)(params[i]));
		//if(s=="from") { Point *p=new Point(((float*)(params[i]))[0],((float*)(params[i]))[1],((float*)(params[i]))[2]); AddPoint(s,(Point*)(params[i])); delete p; }
		//if(s=="to") { Point *p=new Point(((float*)(params[i]))[0],((float*)(params[i]))[1],((float*)(params[i]))[2]); AddPoint(s,(Point*)(params[i])); delete p; }
		if(s=="from") { Point *p=new Point((float*)params[i]); AddPoint(s,p); delete p; }
		if(s=="to") { Point *p=new Point((float*)params[i]); AddPoint(s,p); delete p; }
				
		
		//normal paraneters
		if(s=="N") AddNormal(s,(Normal*)(params[i]),FindOneInt("nvertices", i)); // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		
		//vector parameters
		if(s=="S") AddVector(s,(Vector*)(params[i]),FindOneInt("nvertices", i)); // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if(s=="v1") AddVector(s,(Vector*)(params[i]));
		if(s=="v2") AddVector(s,(Vector*)(params[i]));
		if(s=="updir") AddVector(s,(Vector*)(params[i]));
		
		//texture parameters
		if(s=="bumpmap") AddTexture(s,std::string(params[i]));
		if(s=="Kd") AddTexture(s,std::string(params[i]));
		if(s=="sigma") AddTexture(s,std::string(params[i]));
		if(s=="Ks") AddTexture(s,std::string(params[i]));
		if(s=="roughness" && p=="plastic") AddTexture(s,std::string(params[i]));
		if(s=="roughness" && p=="translucent") AddTexture(s,std::string(params[i]));
		if(s=="roughness" && p=="shinymetal") AddTexture(s,std::string(params[i]));
		if(s=="roughness" && p=="substrate") AddTexture(s,std::string(params[i]));
		if(s=="roughness" && p=="uber") AddTexture(s,std::string(params[i]));
		if(s=="reflect") AddTexture(s,std::string(params[i]));
		if(s=="transmit") AddTexture(s,std::string(params[i]));
		if(s=="Kr") AddTexture(s,std::string(params[i]));
		if(s=="Kt") AddTexture(s,std::string(params[i]));
		if(s=="index") AddTexture(s,std::string(params[i]));
		if(s=="uroughness") AddTexture(s,std::string(params[i]));
		if(s=="vroughness") AddTexture(s,std::string(params[i]));
		if(s=="opacity") AddTexture(s,std::string(params[i]));
		if(s=="value") AddTexture(s,std::string(params[i]));
		if(s=="tex1") AddTexture(s,std::string(params[i]));
		if(s=="tex2") AddTexture(s,std::string(params[i]));
		if(s=="amount") AddTexture(s,std::string(params[i]));
		if(s=="inside") AddTexture(s,std::string(params[i]));
		if(s=="outside") AddTexture(s,std::string(params[i]));
		
		//color (spectrum) parameters
		if(s=="sigma_a") AddSpectrum(s, new Spectrum((float*)params[i]));
		if(s=="sigma_s") AddSpectrum(s, new Spectrum((float*)params[i]));
		if(s=="Le") AddSpectrum(s, new Spectrum((float*)params[i]));
		if(s=="L") AddSpectrum(s, new Spectrum((float*)params[i]));
		if(s=="I") AddSpectrum(s, new Spectrum((float*)params[i]));
		if(s=="tex1") AddSpectrum(s, new Spectrum((float*)params[i]));
		if(s=="tex2") AddSpectrum(s, new Spectrum((float*)params[i]));
		
		//unknown parameter
		/*
		else
		{
			std::stringstream ss;
			ss<<"Unknown parameter '"<<p<<":"<<s<<"', Ignoring.";
			luxError(LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
		}*/
	}
}

ParamSet &ParamSet::operator=(const ParamSet &p2) {
	if (&p2 != this) {
		Clear();
		u_int i;
		for (i = 0; i < p2.ints.size(); ++i)
			ints.push_back(p2.ints[i]->Clone());
		for (i = 0; i < p2.bools.size(); ++i)
			bools.push_back(p2.bools[i]->Clone());
		for (i = 0; i < p2.floats.size(); ++i)
			floats.push_back(p2.floats[i]->Clone());
		for (i = 0; i < p2.points.size(); ++i)
			points.push_back(p2.points[i]->Clone());
		for (i = 0; i < p2.vectors.size(); ++i)
			vectors.push_back(p2.vectors[i]->Clone());
		for (i = 0; i < p2.normals.size(); ++i)
			normals.push_back(p2.normals[i]->Clone());
		for (i = 0; i < p2.spectra.size(); ++i)
			spectra.push_back(p2.spectra[i]->Clone());
		for (i = 0; i < p2.strings.size(); ++i)
			strings.push_back(p2.strings[i]->Clone());
		for (i = 0; i < p2.textures.size(); ++i)
			textures.push_back(p2.textures[i]->Clone());
	}
	return *this;
}
void ParamSet::AddFloat(const string &name,
			            const float *data,
						int nItems) {
	EraseFloat(name);
	floats.push_back(new ParamSetItem<float>(name,
											 data,
											 nItems));
}
void ParamSet::AddInt(const string &name, const int *data, int nItems) {
	EraseInt(name);
	ADD_PARAM_TYPE(int, ints);
}
void ParamSet::AddBool(const string &name, const bool *data, int nItems) {
	EraseInt(name);
	ADD_PARAM_TYPE(bool, bools);
}
void ParamSet::AddPoint(const string &name, const Point *data, int nItems) {
	ErasePoint(name);
	ADD_PARAM_TYPE(Point, points);
}
void ParamSet::AddVector(const string &name, const Vector *data, int nItems) {
	EraseVector(name);
	ADD_PARAM_TYPE(Vector, vectors);
}
void ParamSet::AddNormal(const string &name, const Normal *data, int nItems) {
	EraseNormal(name);
	ADD_PARAM_TYPE(Normal, normals);
}
void ParamSet::AddSpectrum(const string &name, const Spectrum *data, int nItems) {
	EraseSpectrum(name);
	ADD_PARAM_TYPE(Spectrum, spectra);
}
void ParamSet::AddString(const string &name, const string *data, int nItems) {
	EraseString(name);
	ADD_PARAM_TYPE(string, strings);
}
void ParamSet::AddTexture(const string &name, const string &value) {
	EraseTexture(name);
	textures.push_back(new ParamSetItem<string>(name, (string *)&value, 1));
}
bool ParamSet::EraseInt(const string &n) {
	for (u_int i = 0; i < ints.size(); ++i)
		if (ints[i]->name == n) {
			delete ints[i];
			ints.erase(ints.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::EraseBool(const string &n) {
	for (u_int i = 0; i < bools.size(); ++i)
		if (bools[i]->name == n) {
			delete bools[i];
			bools.erase(bools.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::EraseFloat(const string &n) {
	for (u_int i = 0; i < floats.size(); ++i)
		if (floats[i]->name == n) {
			delete floats[i];
			floats.erase(floats.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::ErasePoint(const string &n) {
	for (u_int i = 0; i < points.size(); ++i)
		if (points[i]->name == n) {
			delete points[i];
			points.erase(points.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::EraseVector(const string &n) {
	for (u_int i = 0; i < vectors.size(); ++i)
		if (vectors[i]->name == n) {
			delete vectors[i];
			vectors.erase(vectors.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::EraseNormal(const string &n) {
	for (u_int i = 0; i < normals.size(); ++i)
		if (normals[i]->name == n) {
			delete normals[i];
			normals.erase(normals.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::EraseSpectrum(const string &n) {
	for (u_int i = 0; i < spectra.size(); ++i)
		if (spectra[i]->name == n) {
			delete spectra[i];
			spectra.erase(spectra.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::EraseString(const string &n) {
	for (u_int i = 0; i < strings.size(); ++i)
		if (strings[i]->name == n) {
			delete strings[i];
			strings.erase(strings.begin() + i);
			return true;
		}
	return false;
}
bool ParamSet::EraseTexture(const string &n) {
	for (u_int i = 0; i < textures.size(); ++i)
		if (textures[i]->name == n) {
			delete textures[i];
			textures.erase(textures.begin() + i);
			return true;
		}
	return false;
}
float ParamSet::FindOneFloat(const string &name,
                             float d) const {
	for (u_int i = 0; i < floats.size(); ++i)
		if (floats[i]->name == name &&
			floats[i]->nItems == 1) {
			floats[i]->lookedUp = true;
			return *(floats[i]->data);
		}
	return d;
}
const float *ParamSet::FindFloat(const string &name,
		int *nItems) const {
	for (u_int i = 0; i < floats.size(); ++i)
		if (floats[i]->name == name) {
			*nItems = floats[i]->nItems;
			floats[i]->lookedUp = true;
			return floats[i]->data;
		}
	return NULL;
}
const int *ParamSet::FindInt(const string &name, int *nItems) const {
	LOOKUP_PTR(ints);
}
const bool *ParamSet::FindBool(const string &name, int *nItems) const {
	LOOKUP_PTR(bools);
}
int ParamSet::FindOneInt(const string &name, int d) const {
	LOOKUP_ONE(ints);
}
bool ParamSet::FindOneBool(const string &name, bool d) const {
	LOOKUP_ONE(bools);
}
const Point *ParamSet::FindPoint(const string &name, int *nItems) const {
	LOOKUP_PTR(points);
}
Point ParamSet::FindOnePoint(const string &name, const Point &d) const {
	LOOKUP_ONE(points);
}
const Vector *ParamSet::FindVector(const string &name, int *nItems) const {
	LOOKUP_PTR(vectors);
}
Vector ParamSet::FindOneVector(const string &name, const Vector &d) const {
	LOOKUP_ONE(vectors);
}
const Normal *ParamSet::FindNormal(const string &name, int *nItems) const {
	LOOKUP_PTR(normals);
}
Normal ParamSet::FindOneNormal(const string &name, const Normal &d) const {
	LOOKUP_ONE(normals);
}
const Spectrum *ParamSet::FindSpectrum(const string &name, int *nItems) const {
	LOOKUP_PTR(spectra);
}
Spectrum ParamSet::FindOneSpectrum(const string &name, const Spectrum &d) const {
	LOOKUP_ONE(spectra);
}
const string *ParamSet::FindString(const string &name, int *nItems) const {
	LOOKUP_PTR(strings);
}
string ParamSet::FindOneString(const string &name, const string &d) const {
	LOOKUP_ONE(strings);
}
string ParamSet::FindTexture(const string &name) const {
	string d = "";
	LOOKUP_ONE(textures);
}
void ParamSet::ReportUnused() const {
#define CHECK_UNUSED(v) \
	for (i = 0; i < (v).size(); ++i) \
		if (!(v)[i]->lookedUp) \
		{ \
			std::stringstream ss; \
			ss<<"Parameter '"<<(v)[i]->name<<"' not used"; \
			luxError(LUX_NOERROR,LUX_WARNING,ss.str().c_str()); \
		}
	u_int i;
	CHECK_UNUSED(ints);       CHECK_UNUSED(bools);
	CHECK_UNUSED(floats);   CHECK_UNUSED(points);
	CHECK_UNUSED(vectors); CHECK_UNUSED(normals);
	CHECK_UNUSED(spectra); CHECK_UNUSED(strings);
	CHECK_UNUSED(textures);
}
void ParamSet::Clear() {
#define DEL_PARAMS(name) \
	for (u_int i = 0; i < (name).size(); ++i) \
		delete (name)[i]; \
	(name).erase((name).begin(), (name).end())
	DEL_PARAMS(ints);    DEL_PARAMS(bools);
	DEL_PARAMS(floats);  DEL_PARAMS(points);
	DEL_PARAMS(vectors); DEL_PARAMS(normals);
	DEL_PARAMS(spectra); DEL_PARAMS(strings);
	DEL_PARAMS(textures);
#undef DEL_PARAMS
}
string ParamSet::ToString() const {
	string ret;
	u_int i;
	int j;
	string typeString;
	const int bufLen = 48*1024*1024;
	static char *buf = new char[bufLen];
	char *bufEnd = buf + bufLen;
	for (i = 0; i < ints.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<int> *item = ints[i];
		typeString = "integer ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "%d ", item->data[j]);
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < bools.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<bool> *item = bools[i];
		typeString = "bool ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "\"%s\" ", item->data[j] ? "true" : "false");
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < floats.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<float> *item = floats[i];
		typeString = "float ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "%.8g ", item->data[j]);
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < points.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<Point> *item = points[i];
		typeString = "point ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "%.8g %.8g %.8g ", item->data[j].x,
				item->data[j].y, item->data[j].z);
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < vectors.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<Vector> *item = vectors[i];
		typeString = "vector ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "%.8g %.8g %.8g ", item->data[j].x,
				item->data[j].y, item->data[j].z);
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < normals.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<Normal> *item = normals[i];
		typeString = "normal ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "%.8g %.8g %.8g ", item->data[j].x,
				item->data[j].y, item->data[j].z);
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < strings.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<string> *item = strings[i];
		typeString = "string ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "\"%s\" ", item->data[j].c_str());
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < textures.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<string> *item = textures[i];
		typeString = "texture ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "\"%s\" ", item->data[j].c_str());
		ret += buf;
		ret += string("] ");
	}
	for (i = 0; i < spectra.size(); ++i) {
		char *bufp = buf;
		*bufp = '\0';
		ParamSetItem<Spectrum> *item = spectra[i];
		typeString = "color ";
		// Print _ParamSetItem_ declaration, determine how many to print
		int nPrint = item->nItems;
		ret += string("\"");
		ret += typeString;
		ret += item->name;
		ret += string("\"");
		ret += string(" [");
		for (j = 0; j < nPrint; ++j)
			bufp += snprintf(bufp, bufEnd - bufp, "%.8g %.8g %.8g ", item->data[j].c[0],
				item->data[j].c[1], item->data[j].c[2]);
		ret += buf;
		ret += string("] ");
	}
	return ret;
}
// TextureParams Method Definitions
boost::shared_ptr<Texture<Spectrum> >
	TextureParams::GetSpectrumTexture(const string &n,
             const Spectrum &def) const {
	string name = geomParams.FindTexture(n);
	if (name == "") name = materialParams.FindTexture(n);
	if (name != "") {
		if (spectrumTextures.find(name) !=
		       spectrumTextures.end())
			return spectrumTextures[name];
		else
		{
			//Error("Couldn't find spectrum"
			//      "texture named \"%s\"", n.c_str());
			std::stringstream ss;
			ss<<"Couldn't find spectrum texture named '"<<n<<"'";
			luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		}
	}
	Spectrum val = geomParams.FindOneSpectrum(n,
		materialParams.FindOneSpectrum(n, def));
	boost::shared_ptr<Texture<Spectrum> > o (new ConstantTexture<Spectrum>(val));
	return o;
}
boost::shared_ptr<Texture<float> > TextureParams::GetFloatTexture(const string &n,
		float def) const {
	string name = geomParams.FindTexture(n);
	if (name == "") name = materialParams.FindTexture(n);
	if (name != "") {
		if (floatTextures.find(name) != floatTextures.end())
			return floatTextures[name];
		else
		{
					//Error("Couldn't find float texture named \"%s\"", n.c_str());
					std::stringstream ss;
					ss<<"Couldn't find float texture named '"<<n<<"'";
					luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		}
	}
	float val = geomParams.FindOneFloat(n,
		materialParams.FindOneFloat(n, def));
	boost::shared_ptr<Texture<float> > o (new ConstantTexture<float>(val));
	return o;
}
