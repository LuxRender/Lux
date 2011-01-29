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

// paramset.cpp*
#include "paramset.h"
#include "error.h"
#include "context.h"
#include "textures/constant.h"
#include <sstream>
#include <string>

namespace lux {

// ParamSet Helper
bool LookupType(const char *token, ParamType *type, string &name)
{
	BOOST_ASSERT(token != NULL);
	*type = ParamType(0);
	const char *strp = token;
	while (*strp && isspace(*strp))
		++strp;
	if (!*strp) {
		LOG( LUX_ERROR,LUX_SYNTAX)
			<< "Parameter '" << token <<
			"' doesn't have a type declaration?!";
		name = string(token);
		return false;
	}
#define TRY_DECODING_TYPE(name, mask) \
	if (strncmp(name, strp, strlen(name)) == 0) { \
		*type = mask; strp += strlen(name); \
	}
	TRY_DECODING_TYPE("float", PARAM_TYPE_FLOAT)
	else TRY_DECODING_TYPE("integer", PARAM_TYPE_INT)
	else TRY_DECODING_TYPE("bool", PARAM_TYPE_BOOL)
	else TRY_DECODING_TYPE("point", PARAM_TYPE_POINT)
	else TRY_DECODING_TYPE("vector", PARAM_TYPE_VECTOR)
	else TRY_DECODING_TYPE("normal", PARAM_TYPE_NORMAL)
	else TRY_DECODING_TYPE("string", PARAM_TYPE_STRING)
	else TRY_DECODING_TYPE("texture", PARAM_TYPE_TEXTURE)
	else TRY_DECODING_TYPE("color", PARAM_TYPE_COLOR)
	else {
		LOG( LUX_ERROR,LUX_SYNTAX) << "Unable to decode type for token '" << token << "'";
		name = string(token);
		return false;
	}
	while (*strp && isspace(*strp))
		++strp;
	name = string(strp);
	return true;
}

// ParamSet Macros
template <class T> inline void DelParams(vector<ParamSetItem<T> *> &vec)
{
	for (u_int i = 0; i < vec.size(); ++i)
		delete vec[i];
	vec.clear();
}
template <class T> inline bool EraseParamType(vector<ParamSetItem<T> *> &vec,
	const string &name)
{
	for (u_int i = 0; i < vec.size(); ++i)
		if (vec[i]->name == name) {
			delete vec[i];
			vec.erase(vec.begin() + i);
			return true;
		}
	return false;
}
template <class T> inline void AddParamType(vector<ParamSetItem<T> *> &vec,
	const string &name, const T *data, u_int nItems)
{
	EraseParamType(vec, name);
	vec.push_back(new ParamSetItem<T>(name, data, nItems));
}
template <class T> inline const T *LookupPtr(const vector<ParamSetItem<T> *> &vec,
	const string &name, u_int *nItems)
{
	for (u_int i = 0; i < vec.size(); ++i)
		if (vec[i]->name == name) {
			*nItems = vec[i]->nItems;
			vec[i]->lookedUp = true;
			return vec[i]->data;
		}
	return NULL;
}
template <class T> inline const T &LookupOne(const vector<ParamSetItem<T> *> &vec,
	const string &name, const T &d)
{
	for (u_int i = 0; i < vec.size(); ++i)
		if (vec[i]->name == name && vec[i]->nItems == 1) {
			vec[i]->lookedUp = true;
			return *(vec[i]->data);
		}
	return d;
}
template <class T> inline void CheckUnused(const vector<ParamSetItem<T> *> &vec)
{
	for (u_int i = 0; i < vec.size(); ++i)
		if (!vec[i]->lookedUp) {
			LOG( LUX_WARNING,LUX_NOERROR) << "Parameter '" << vec[i]->name << "' not used";
		}
}
template <class T> inline void MarkAsUsed(const vector<ParamSetItem<T> *> &vec, const vector<ParamSetItem<T> *> &vecOther)
{
	for (u_int i = 0; i < vecOther.size(); ++i) {
		if (vecOther[i]->lookedUp) {
			u_int n;
			LookupPtr(vec, vecOther[i]->name, &n);
		}
	}
}

// ParamSet Methods
template <> ParamSetItem<int>::~ParamSetItem()
{
	delete[] data;
}

template <> ParamSetItem<bool>::~ParamSetItem()
{
	delete[] data;
}

template <> ParamSetItem<float>::~ParamSetItem()
{
	delete[] data;
}

template <> ParamSetItem<Point>::~ParamSetItem()
{
	delete[] data;
}

template <> ParamSetItem<Vector>::~ParamSetItem()
{
	delete[] data;
}

template <> ParamSetItem<Normal>::~ParamSetItem()
{
	delete[] data;
}

template <> ParamSetItem<RGBColor>::~ParamSetItem()
{
	delete[] data;
}

template <> ParamSetItem<string>::~ParamSetItem()
{
	delete[] data;
}

ParamSet::ParamSet(const ParamSet &p2) {
	*this = p2;
}
/* Here we create a paramset with the argument list provided by the C API.
 * We have to 'guess' the parameter type according the the parameter's name
 */
ParamSet::ParamSet(u_int n, const char * pluginName, const char * const tokens[], const char * const params[])
{
	//TODO - jromang : implement this using a std::map or string hashing

	// NOTE - radiance - THIS NEEDS TO BE UPDATED! :)

	std::string pn(pluginName);
	
	for(u_int i = 0; i < n; ++i)
	{
		ParamType type;
		std::string s;
		bool typed = LookupType(tokens[i], &type, s);
		if (typed) {
			switch (type) {
			case PARAM_TYPE_INT: {
				u_int np = 1;
				if (s == "indices")
					np = FindOneInt("ntris", 1);  // [add this special 'ntris' parameter when using the API]
				else if (s == "quadindices")
					np = FindOneInt("nquads", 1);  // [add this special 'nquads' parameter when using the API]
				else if (s == "triindices")
					np = FindOneInt("ntris", 1);  // [add this special 'ntris' parameter when using the API]
				AddInt(s, (int*)(params[i]), np);
				break;
			}
			case PARAM_TYPE_BOOL: {
				u_int np = 1;
				AddBool(s, (bool*)(params[i]), np);
				break;
			}
			case PARAM_TYPE_FLOAT: {
				u_int np = 1;
				if (s == "Pw")
					np = 4 * FindOneInt("nu", 1) *
						FindOneInt("nv", 1);
				else if (s == "Pz")
					np = FindOneInt("nu", 1) *
						FindOneInt("nv", 1);
				else if (s == "data" || s == "wavelengths")
					np = FindOneInt("nvalues", 1);  // [add this special 'nvalues' parameter when using the API]
				else if (s == "density")
					np = FindOneInt("nx", 1) *
						FindOneInt("ny", 1) *
						FindOneInt("nz", 1);
				else if (s == "screenwindow")
					np = 4;
				else if (s == "st" || s == "uv")
					np = 2 * FindOneInt("nvertices", 1); // [add this special 'nvertices' parameter when using the API]
				else if (s == "uknots")
					np = FindOneInt("nu", 1) +
						FindOneInt("uorder", 1);
				else if (s == "vknots")
					np = FindOneInt("nv", 1) +
						FindOneInt("vorder", 1);
				AddFloat(s, (float*)(params[i]), np);
				break;
			}
			case PARAM_TYPE_POINT: {
				u_int np = 1;
				if (s == "P") {
					if (pn == "nurbs")
						np = FindOneInt("nu", 1) *
							FindOneInt("nv", 1);
					else
						np = FindOneInt("nvertices", 1);  // [add this special 'nvertices' parameter when using the API]
				}
				AddPoint(s, (Point*)(params[i]), np);
				break;
			}
			case PARAM_TYPE_VECTOR: {
				u_int np = 1;
				AddVector(s, (Vector*)(params[i]), np);
				break;
			}
			case PARAM_TYPE_NORMAL: {
				u_int np = 1;
				if (s == "N")
					np = FindOneInt("nvertices", 1);  // [add this special 'nvertices' parameter when using the API]
				AddNormal(s, (Normal*)(params[i]), np);
				break;
			}
			case PARAM_TYPE_COLOR: {
				AddRGBColor(s, new RGBColor((float*)(params[i])));
				break;
			}
			case PARAM_TYPE_STRING: {
				AddString(s, new std::string((char*)(params[i])));
				break;
			}
			case PARAM_TYPE_TEXTURE: {
				AddTexture(s, std::string((char*)(params[i])));
				break;
			}
			default:
				break;
			}
			continue;
		}
		//float parameters
		if (s == "B")
			AddFloat(s,(float*)(params[i]));
		if (s == "C")
			AddFloat(s, (float*)(params[i]));
		if (s == "Pw")
			AddFloat(s, (float*)(params[i]),
				4 * FindOneInt("nu", i) * FindOneInt("nv", i));
		if (s == "Pz")
			AddFloat(s, (float*)(params[i]),
				FindOneInt("nu", i) * FindOneInt("nv", i));
		if (s == "a")
			AddFloat(s, (float*)(params[i]));
		if (s == "aconst")
			AddFloat(s, (float*)(params[i]));
		if (s == "alpha")
			AddFloat(s, (float*)(params[i]));
		if (s == "aperture")
			AddFloat(s, (float*)(params[i]));
		if (s == "aperture_diameter")
			AddFloat(s, (float*)(params[i]));
		if (s == "b")
			AddFloat(s, (float*)(params[i]));
		if (s == "baseflatness")
			AddFloat(s, (float*)(params[i]));
		if (s == "bconst")
			AddFloat(s, (float*)(params[i]));
		if (s == "brickdepth")
			AddFloat(s, (float*)(params[i]));
		if (s == "brickheight")
			AddFloat(s, (float*)(params[i]));
		if (s == "brickwidth")
			AddFloat(s, (float*)(params[i]));
		if (s == "bright")
			AddFloat(s, (float*)(params[i]));
		if (s == "bumpmapsampledistance")
			AddFloat(s, (float*)(params[i]));
		if (s == "burn")
			AddFloat(s, (float*)(params[i]));
		if (s == "cconst")
			AddFloat(s, (float*)(params[i]));
		if (s == "compo_override_alpha_value")
			AddFloat(s, (float*)(params[i]));
		if (s == "coneangle")
			AddFloat(s, (float*)(params[i]));
		if (s == "conedeltaangle")
			AddFloat(s, (float*)(params[i]));
		if (s == "contrast")
			AddFloat(s, (float*)(params[i]));
		if (s == "contrast_ywa")
			AddFloat(s, (float*)(params[i]));
		if (s == "cropwindow")
			AddFloat(s, (float*)(params[i]), 4);
/*		if (s == "data")
			AddFloat(s, (float*)(params[i]));*/ //FIXME - there's currently no way of getting the array length for regular and irregular spectrum data
		if (s == "dconst")
			AddFloat(s, (float*)(params[i]));
		if (s == "density") AddFloat(s, (float*)(params[i]), FindOneInt("nx", i) * FindOneInt("ny", i) * FindOneInt("nz", i));
		if (s == "diffusereflectreject_threshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "diffuserefractreject_threshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "distamount")
			AddFloat(s, (float*)(params[i]));
		if (s == "distancethreshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "dmoffset")
			AddFloat(s, (float*)(params[i]));
		if (s == "dmscale")
			AddFloat(s, (float*)(params[i]));
		if (s == "econst")
			AddFloat(s, (float*)(params[i]));
		if (s == "efficacy")
			AddFloat(s, (float*)(params[i]));
		if (s == "emptybonus")
			AddFloat(s, (float*)(params[i]));
		if (s == "end")
			AddFloat(s, (float*)(params[i]));
		if (s == "energy")
			AddFloat(s, (float*)(params[i]));
		if (s == "exposure")
			AddFloat(s, (float*)(params[i]));
		if (s == "eyerrthreshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "filmdiag")
			AddFloat(s, (float*)(params[i]));
		if (s == "filmdistance")
			AddFloat(s, (float*)(params[i]));
		if (s == "focaldistance")
			AddFloat(s, (float*)(params[i]));
		if (s == "fov")
			AddFloat(s, (float*)(params[i]));
		if (s == "frameaspectratio")
			AddFloat(s, (float*)(params[i]));
		if (s == "freq")
			AddFloat(s, (float*)(params[i]));
		if (s == "fstop")
			AddFloat(s, (float*)(params[i]));
		if (s == "fullsweepthreshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "g")
			AddFloat(s, (float*)(params[i]));
		if (s == "gain")
			AddFloat(s, (float*)(params[i]));
		if (s == "gamma")
			AddFloat(s, (float*)(params[i]));
		if (s == "gatherangle")
			AddFloat(s, (float*)(params[i]));
		if (s == "glossyreflectreject_threshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "glossyrefractreject_threshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "h")
			AddFloat(s, (float*)(params[i]));
		if (s == "height")
			AddFloat(s, (float*)(params[i]));
		if (s == "hither")
			AddFloat(s, (float*)(params[i]));
		if (s == "innerradius")
			AddFloat(s, (float*)(params[i]));
		if (s == "lacu")
			AddFloat(s, (float*)(params[i]));
		if (s == "largemutationprob")
			AddFloat(s, (float*)(params[i]));
		if (s == "lensradius")
			AddFloat(s, (float*)(params[i]));
		if (s == "lightrrthreshold")
			AddFloat(s, (float*)(params[i]));
		if (s == "linear_exposure")
			AddFloat(s, (float*)(params[i]));
		if (s == "linear_fstop")
			AddFloat(s, (float*)(params[i]));
		if (s == "linear_gamma")
			AddFloat(s, (float*)(params[i]));
		if (s == "linear_sensitivity")
			AddFloat(s, (float*)(params[i]));
		if (s == "majorradius")
			AddFloat(s, (float*)(params[i]));
		if (s == "maxY")
			AddFloat(s, (float*)(params[i]));
		if (s == "maxanisotropy")
			AddFloat(s, (float*)(params[i]));
		if (s == "maxphotondist")
			AddFloat(s, (float*)(params[i]));
		if (s == "micromutationprob")
			AddFloat(s, (float*)(params[i]));
		if (s == "mindist")
			AddFloat(s, (float*)(params[i]));
		if (s == "minkovsky_exp")
			AddFloat(s, (float*)(params[i]));
		if (s == "minorradius")
			AddFloat(s, (float*)(params[i]));
		if (s == "mortarsize")
			AddFloat(s, (float*)(params[i]));
		if (s == "mutationrange")
			AddFloat(s, (float*)(params[i]));
		if (s == "nabla")
			AddFloat(s, (float*)(params[i]));
		if (s == "noiseoffset")
			AddFloat(s, (float*)(params[i]));
		if (s == "noisescale")
			AddFloat(s, (float*)(params[i]));
		if (s == "noisesize")
			AddFloat(s, (float*)(params[i]));
		if (s == "octs")
			AddFloat(s, (float*)(params[i]));
		if (s == "offset")
			AddFloat(s, (float*)(params[i]));
		if (s == "omega")
			AddFloat(s, (float*)(params[i]));
		if (s == "outscale")
			AddFloat(s, (float*)(params[i]));
		if (s == "phimax")
			AddFloat(s, (float*)(params[i]));
		if (s == "postscale")
			AddFloat(s, (float*)(params[i]));
		if (s == "power" && pn == "area")
			AddFloat(s, (float*)(params[i]));
		if (s == "prescale")
			AddFloat(s, (float*)(params[i]));
		if (s == "radius")
			AddFloat(s, (float*)(params[i]));
		if (s == "reinhard_burn")
			AddFloat(s, (float*)(params[i]));
		if (s == "reinhard_postscale")
			AddFloat(s, (float*)(params[i]));
		if (s == "reinhard_prescale")
			AddFloat(s, (float*)(params[i]));
		if (s == "relsize")
			AddFloat(s, (float*)(params[i]));
		if (s == "roughness")
			AddFloat(s, (float*)(params[i]));
		if (s == "rrcontinueprob")
			AddFloat(s, (float*)(params[i]));
		if (s == "scale")
			AddFloat(s, (float*)(params[i]));
		if (s == "screenwindow")
			AddFloat(s, (float*)(params[i]), 4);
		if (s == "sensitivity")
			AddFloat(s, (float*)(params[i]));
		if (s == "sharpness")
			AddFloat(s, (float*)(params[i]));
		if (s == "shutterclose")
			AddFloat(s, (float*)(params[i]));
		if (s == "shutteropen")
			AddFloat(s, (float*)(params[i]));
		if (s == "spheresize")
			AddFloat(s, (float*)(params[i]));
		if (s == "st")
			AddFloat(s, (float*)(params[i]),
				2 * FindOneInt("nvertices", i));  // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if (s == "start")
			AddFloat(s, (float*)(params[i]));
		if (s == "stepsize")
			AddFloat(s, (float*)(params[i]));
		if (s == "tau")
			AddFloat(s, (float*)(params[i]));
		if (s == "temperature")
			AddFloat(s, (float*)(params[i]));
		if (s == "thetamax")
			AddFloat(s, (float*)(params[i]));
		if (s == "thetamin")
			AddFloat(s, (float*)(params[i]));
		if (s == "turbidity")
			AddFloat(s, (float*)(params[i]));
		if (s == "turbulance")
			AddFloat(s, (float*)(params[i]));
		if (s == "turbulence")
			AddFloat(s, (float*)(params[i]));
		if (s == "u0")
			AddFloat(s, (float*)(params[i]));
		if (s == "u1")
			AddFloat(s, (float*)(params[i]));
		if (s == "udelta")
			AddFloat(s, (float*)(params[i]));
		if (s == "uknots")
			AddFloat(s, (float*)(params[i]),
				FindOneInt("nu", i) + FindOneInt("uorder", i));
		if (s == "uscale")
			AddFloat(s, (float*)(params[i]));
		if (s == "uv")
			AddFloat(s, (float*)(params[i]),
				2 * FindOneInt("nvertices", i));  // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if (s == "v0")
			AddFloat(s, (float*)(params[i]));
		if (s == "v00")
			AddFloat(s, (float*)(params[i]));
		if (s == "v01")
			AddFloat(s, (float*)(params[i]));
		if (s == "v1")
			AddFloat(s, (float*)(params[i]));
		if (s == "v10")
			AddFloat(s, (float*)(params[i]));
		if (s == "v11")
			AddFloat(s, (float*)(params[i]));
		if (s == "value")
			AddFloat(s, (float*)(params[i]));
		if (s == "variability")
			AddFloat(s, (float*)(params[i]));
		if (s == "variation")
			AddFloat(s, (float*)(params[i]));
		if (s == "vdelta")
			AddFloat(s,(float*)(params[i]));
		if (s == "vknots")
			AddFloat(s, (float*)(params[i]),
				FindOneInt("nv", i) + FindOneInt("vorder", i));
		if (s == "vscale")
			AddFloat(s, (float*)(params[i]));
		if (s == "w1")
			AddFloat(s, (float*)(params[i]));
		if (s == "w2")
			AddFloat(s, (float*)(params[i]));
		if (s == "w3")
			AddFloat(s, (float*)(params[i]));
		if (s == "w4")
			AddFloat(s, (float*)(params[i]));
		if (s == "wavelength")
			AddFloat(s, (float*)(params[i]));
/*		if (s == "wavelengths")
			AddFloat(s, (float*)(params[i]));*/ //FIXME - there's currently no way of getting the array length for irregular spectrum data
		if (s == "width")
			AddFloat(s, (float*)(params[i]));
		if (s == "xwidth")
			AddFloat(s, (float*)(params[i]));
		if (s == "yon")
			AddFloat(s, (float*)(params[i]));
		if (s == "ywa")
			AddFloat(s, (float*)(params[i]));
		if (s == "ywidth")
			AddFloat(s, (float*)(params[i]));
		if (s == "zmax")
			AddFloat(s, (float*)(params[i]));
		if (s == "zmin")
			AddFloat(s, (float*)(params[i]));

		//int parameters
		if (s == "blades")
			AddInt(s, (int*)(params[i]));
		if (s == "causticphotons")
			AddInt(s, (int*)(params[i]));
		if (s == "chainlength")
			AddInt(s, (int*)(params[i]));
		if (s == "coltype")
			AddInt(s, (int*)(params[i]));
		if (s == "costsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "diffusereflectdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "diffusereflectsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "diffuserefractdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "diffuserefractsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "dimension")
			AddInt(s, (int*)(params[i]));
		if (s == "directphotons")
			AddInt(s, (int*)(params[i]));
		if (s == "directsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "discardmipmaps")
			AddInt(s, (int*)(params[i]));
		if (s == "displayinterval")
			AddInt(s, (int*)(params[i]));
		if (s == "eyedepth")
			AddInt(s, (int*)(params[i]));
		if (s == "finalgathersamples")
			AddInt(s, (int*)(params[i]));
		if (s == "glossyreflectdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "glossyreflectsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "glossyrefractdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "glossyrefractsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "haltspp")
			AddInt(s, (int*)(params[i]));
		if( s == "indices")
			AddInt(s, (int*)(params[i]),
				FindOneInt("nvertices", i));  // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		if (s == "indirectphotons")
			AddInt(s, (int*)(params[i]));
		if (s == "indirectsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "intersectcost")
			AddInt(s, (int*)(params[i]));
		if (s == "lightdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "maxconsecrejects")
			AddInt(s, (int*)(params[i]));
		if (s == "maxdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "maxphotondepth")
			AddInt(s, (int*)(params[i]));
		if (s == "maxprims")
			AddInt(s, (int*)(params[i]));
		if (s == "maxprimsperleaf")
			AddInt(s, (int*)(params[i]));
		if (s == "nlevels")
			AddInt(s, (int*)(params[i]));
		if (s == "nlights")
			AddInt(s, (int*)(params[i]));
		if (s == "noisedepth")
			AddInt(s, (int*)(params[i]));
		if (s == "nphotonused")
			AddInt(s, (int*)(params[i]));
		if (s == "nsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "nsets")
			AddInt(s, (int*)(params[i]));
		if (s == "nsubdivlevels")
			AddInt(s, (int*)(params[i]));
		if (s == "nu")
			AddInt(s, (int*)(params[i]));
		if (s == "nv")
			AddInt(s, (int*)(params[i]));
		if (s == "nx")
			AddInt(s, (int*)(params[i]));
		if (s == "ny")
			AddInt(s, (int*)(params[i]));
		if (s == "nz")
			AddInt(s, (int*)(params[i]));
		if (s == "octaves")
			AddInt(s, (int*)(params[i]));
		if (s == "pixelsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "power" && pn == "perspective")
			AddInt(s, (int*)(params[i]));
/*		if (s == "quadindices")
			AddInt(s, (int*)(params[i]));*/ //FIXME - there's no way of knowinf the number of elements
		if (s == "radiancephotons")
			AddInt(s, (int*)(params[i]));
		if (s == "reject_warmup")
			AddInt(s, (int*)(params[i]));
		if (s == "skipfactor")
			AddInt(s, (int*)(params[i]));
		if (s == "specularreflectdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "specularrefractdepth")
			AddInt(s, (int*)(params[i]));
		if (s == "spheres")
			AddInt(s, (int*)(params[i]));
		if (s == "traversalcost")
			AddInt(s, (int*)(params[i]));
		if (s == "treetype")
			AddInt(s, (int*)(params[i]));
/*		if( s == "triindices")
			AddInt(s, (int*)(params[i]));*/ //FIXME - there's no way of knowing the number of elements
		if (s == "uorder")
			AddInt(s, (int*)(params[i]));
		if (s == "vorder")
			AddInt(s, (int*)(params[i]));
		if (s == "writeinterval")
			AddInt(s, (int*)(params[i]));
		if (s == "xresolution")
			AddInt(s, (int*)(params[i]));
		if (s == "xsamples")
			AddInt(s, (int*)(params[i]));
		if (s == "yresolution")
			AddInt(s, (int*)(params[i]));
		if (s == "ysamples")
			AddInt(s, (int*)(params[i]));

		//bool parameters
		if (s == "architectural")
			AddBool(s, (bool*)(params[i]));
		if (s == "autofocus")
			AddBool(s, (bool*)(params[i]));
		if (s == "compo_override_alpha")
			AddBool(s, (bool*)(params[i]));
		if (s == "compo_use_key")
			AddBool(s, (bool*)(params[i]));
		if (s == "compo_visible_emission")
			AddBool(s, (bool*)(params[i]));
		if (s == "compo_visible_indirect_emission")
			AddBool(s, (bool*)(params[i]));
		if (s == "compo_visible_indirect_material")
			AddBool(s, (bool*)(params[i]));
		if (s == "compo_visible_material")
			AddBool(s, (bool*)(params[i]));
		if (s == "dbg_enabledirect")
			AddBool(s, (bool*)(params[i]));
		if (s == "dbg_enableindircaustic")
			AddBool(s, (bool*)(params[i]));
		if (s == "dbg_enableindirdiffuse")
			AddBool(s, (bool*)(params[i]));
		if (s == "dbg_enableindirspecular")
			AddBool(s, (bool*)(params[i]));
		if (s == "dbg_enableradiancemap")
			AddBool(s, (bool*)(params[i]));
		if (s == "debug")
			AddBool(s, (bool*)(params[i]));
		if (s == "diffusereflectreject")
			AddBool(s, (bool*)(params[i]));
		if (s == "diffuserefractreject")
			AddBool(s, (bool*)(params[i]));
		if (s == "directdiffuse")
			AddBool(s, (bool*)(params[i]));
		if (s == "directglossy")
			AddBool(s, (bool*)(params[i]));
		if (s == "directsampleall")
			AddBool(s, (bool*)(params[i]));
		if (s == "dmnormalsmooth")
			AddBool(s, (bool*)(params[i]));
		if (s == "dmsharpboundary")
			AddBool(s, (bool*)(params[i]));
		if (s == "finalgather")
			AddBool(s, (bool*)(params[i]));
		if (s == "flipxy")
			AddBool(s, (bool*)(params[i]));
		if (s == "flipz")
			AddBool(s, (bool*)(params[i]));
		if (s == "glossyreflectreject")
			AddBool(s, (bool*)(params[i]));
		if (s == "glossyrefractreject")
			AddBool(s, (bool*)(params[i]));
		if (s == "includeenvironment")
			AddBool(s, (bool*)(params[i]));
		if (s == "indirectdiffuse")
			AddBool(s, (bool*)(params[i]));
		if (s == "indirectglossy")
			AddBool(s, (bool*)(params[i]));
		if (s == "indirectsampleall")
			AddBool(s, (bool*)(params[i]));
		if (s == "premultiplyalpha")
			AddBool(s, (bool*)(params[i]));
		if (s == "refineimmediately")
			AddBool(s, (bool*)(params[i]));
		if (s == "restart_resume_flm")
			AddBool(s, (bool*)(params[i]));
		if (s == "smooth")
			AddBool(s, (bool*)(params[i]));
		if (s == "usevariance")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_exr")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_exr_ZBuf")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_exr_applyimaging")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_exr_gamutclamp")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_exr_halftype")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_png")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_png_16bit")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_png_ZBuf")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_png_gamutclamp")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_resume_flm")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_tga")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_tga_ZBuf")
			AddBool(s, (bool*)(params[i]));
		if (s == "write_tga_gamutclamp")
			AddBool(s, (bool*)(params[i]));
		
		//string parameters
		if (s == "aamode")
			AddString(s, new string(params[i]));
		if (s == "acceltype")
			AddString(s, new string(params[i]));
		if (s == "basesampler")
			AddString(s, new string(params[i]));
		if (s == "configfile")
			AddString(s, new string(params[i]));
		if (s == "displacementmap")
			AddString(s, new string(params[i]));
		if (s == "distmetric")
			AddString(s, new string(params[i]));
		if (s == "distribution")
			AddString(s, new string(params[i]));
		if (s == "endtransform")
			AddString(s, new string(params[i]));
		if (s == "filename")
			AddString(s, new string(params[i]));
		if (s == "filtertype")
			AddString(s, new string(params[i]));
		if (s == "iesname")
			AddString(s, new string(params[i]));
		if (s == "ldr_clamp_method")
			AddString(s, new string(params[i]));
		if (s == "mapname")
			AddString(s, new string(params[i]));
		if (s == "mapping")
			AddString(s, new string(params[i]));
		if (s == "name")
			AddString(s, new string(params[i]));
		if (s == "namedmaterial1")
			AddString(s, new string(params[i]));
		if (s == "namedmaterial2")
			AddString(s, new string(params[i]));
		if (s == "noisebasis")
			AddString(s, new string(params[i]));
		if (s == "noisebasis2")
			AddString(s, new string(params[i]));
		if (s == "noisetype")
			AddString(s, new string(params[i]));
		if (s == "photonmapsfile")
			AddString(s, new string(params[i]));
		if (s == "pixelsampler")
			AddString(s, new string(params[i]));
		if (s == "quadtype")
			AddString(s, new string(params[i]));
		if (s == "renderingmode")
			AddString(s, new string(params[i]));
		if (s == "rrstrategy")
			AddString(s, new string(params[i]));
		if (s == "scheme")
			AddString(s, new string(params[i]));
		if (s == "shutterdistribution")
			AddString(s, new string(params[i]));
		if (s == "specfile")
			AddString(s, new string(params[i]));
		if (s == "strategy")
			AddString(s, new string(params[i]));
		if (s == "subdivscheme")
			AddString(s, new string(params[i]));
		if (s == "tonemapkernel")
			AddString(s, new string(params[i]));
		if (s == "tritype")
			AddString(s, new string(params[i]));
		if (s == "type")
			AddString(s, new string(params[i]));
		if (s == "wrap")
			AddString(s, new string(params[i]));
		if (s == "write_exr_channels")
			AddString(s, new string(params[i]));
		if (s == "write_exr_compressiontype")
			AddString(s, new string(params[i]));
		if (s == "write_exr_zbuf_normalization")
			AddString(s, new string(params[i]));
		if (s == "write_png_channels")
			AddString(s, new string(params[i]));
		if (s == "write_pxr_zbuf_normalization")
			AddString(s, new string(params[i]));
		if (s == "write_tga_channels")
			AddString(s, new string(params[i]));
		if (s == "write_tga_zbuf_normalization")
			AddString(s, new string(params[i]));

		//point parameters
		if (s == "P") {
			if (pn == "nurbs")
				AddPoint(s, (Point*)(params[i]),
					FindOneInt("nu", i) * FindOneInt("nv", i));
			else
				AddPoint(s, (Point*)(params[i]),
					FindOneInt("nvertices", i)); // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		}
		if (s == "from")
			AddPoint(s, (Point*)(params[i]));
		if (s == "p0")
			AddPoint(s, (Point*)(params[i]));
		if (s == "p1")
			AddPoint(s, (Point*)(params[i]));
		if (s == "p2")
			AddPoint(s, (Point*)(params[i]));
		if (s == "to")
			AddPoint(s,(Point*)(params[i]));

		//normal parameters
		if (s == "N")
			AddNormal(s, (Normal*)(params[i]),
				FindOneInt("nvertices", i)); // jromang - p.926 find 'n' ? - [a ajouter dans le vecteur lors de l'appel de la fonction, avec un parametre 'nvertices supplementaire dans l API]
		
		//vector parameters
		if (s == "rotate")
			AddVector(s, (Vector*)(params[i]));
		if (s == "scale")
			AddVector(s, (Vector*)(params[i]));
		if (s == "sundir")
			AddVector(s, (Vector*)(params[i]));
		if (s == "translate")
			AddVector(s, (Vector*)(params[i]));
		if (s == "updir")
			AddVector(s, (Vector*)(params[i]));
		if (s == "v1")
			AddVector(s, (Vector*)(params[i]));
		if (s == "v2")
			AddVector(s, (Vector*)(params[i]));
		
		//texture parameters
		if (s == "Ka")
			AddTexture(s, std::string(params[i]));
		if (s == "Kd")
			AddTexture(s, std::string(params[i]));
		if (s == "Kr")
			AddTexture(s, std::string(params[i]));
		if (s == "Ks")
			AddTexture(s, std::string(params[i]));
		if (s == "Ks1")
			AddTexture(s, std::string(params[i]));
		if (s == "Ks2")
			AddTexture(s, std::string(params[i]));
		if (s == "Ks3")
			AddTexture(s, std::string(params[i]));
		if (s == "Kt")
			AddTexture(s, std::string(params[i]));
		if (s == "L")
			AddTexture(s, std::string(params[i]));
		if (s == "M1")
			AddTexture(s, std::string(params[i]));
		if (s == "M2")
			AddTexture(s, std::string(params[i]));
		if (s == "M3")
			AddTexture(s, std::string(params[i]));
		if (s == "R1")
			AddTexture(s, std::string(params[i]));
		if (s == "R2")
			AddTexture(s, std::string(params[i]));
		if (s == "R3")
			AddTexture(s, std::string(params[i]));
		if (s == "amount")
			AddTexture(s, std::string(params[i]));
		if (s == "bricktex")
			AddTexture(s, std::string(params[i]));
		if (s == "bumpmap")
			AddTexture(s, std::string(params[i]));
		if (s == "cauchyb")
			AddTexture(s, std::string(params[i]));
		if (s == "d")
			AddTexture(s, std::string(params[i]));
		if (s == "film")
			AddTexture(s, std::string(params[i]));
		if (s == "filmindex")
			AddTexture(s, std::string(params[i]));
		if (s == "index")
			AddTexture(s, std::string(params[i]));
		if (s == "inside")
			AddTexture(s, std::string(params[i]));
		if (s == "mortartex")
			AddTexture(s, std::string(params[i]));
		if (s == "outside")
			AddTexture(s, std::string(params[i]));
		if (s == "sigma")
			AddTexture(s, std::string(params[i]));
		if (s == "tex1")
			AddTexture(s, std::string(params[i]));
		if (s == "tex2")
			AddTexture(s, std::string(params[i]));
		if (s == "uroughness")
			AddTexture(s, std::string(params[i]));
		if (s == "vroughness")
			AddTexture(s, std::string(params[i]));
		
		//color (RGBColor) parameters
		if (s == "L")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "Le")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "compo_key_color")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "sigma_a")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "sigma_s")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "v00")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "v01")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "v10")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "v11")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		if (s == "value")
			AddRGBColor(s, new RGBColor((float*)params[i]));
		
		//unknown parameter
		/*
		else
		{
			LOG(LUX_ERROR,LUX_SYNTAX)<<"Unknown parameter '"<<p<<":"<<s<<"', Ignoring.";
		}*/
	}
}

ParamSet &ParamSet::operator=(const ParamSet &p2) {
	if (&p2 != this) {
		Clear();
		for (u_int i = 0; i < p2.ints.size(); ++i)
			ints.push_back(p2.ints[i]->Clone());
		for (u_int i = 0; i < p2.bools.size(); ++i)
			bools.push_back(p2.bools[i]->Clone());
		for (u_int i = 0; i < p2.floats.size(); ++i)
			floats.push_back(p2.floats[i]->Clone());
		for (u_int i = 0; i < p2.points.size(); ++i)
			points.push_back(p2.points[i]->Clone());
		for (u_int i = 0; i < p2.vectors.size(); ++i)
			vectors.push_back(p2.vectors[i]->Clone());
		for (u_int i = 0; i < p2.normals.size(); ++i)
			normals.push_back(p2.normals[i]->Clone());
		for (u_int i = 0; i < p2.spectra.size(); ++i)
			spectra.push_back(p2.spectra[i]->Clone());
		for (u_int i = 0; i < p2.strings.size(); ++i)
			strings.push_back(p2.strings[i]->Clone());
		for (u_int i = 0; i < p2.textures.size(); ++i)
			textures.push_back(p2.textures[i]->Clone());
	}
	return *this;
}

void ParamSet::Add(ParamSet &params) {
	for (u_int i = 0; i < params.ints.size(); ++i)
		AddInt(params.ints[i]->name, params.ints[i]->data, params.ints[i]->nItems);
	for (u_int i = 0; i < params.bools.size(); ++i)
		AddBool(params.bools[i]->name, params.bools[i]->data, params.bools[i]->nItems);
	for (u_int i = 0; i < params.floats.size(); ++i)
		AddFloat(params.floats[i]->name, params.floats[i]->data, params.floats[i]->nItems);
	for (u_int i = 0; i < params.points.size(); ++i)
		AddPoint(params.points[i]->name, params.points[i]->data, params.points[i]->nItems);
	for (u_int i = 0; i < params.vectors.size(); ++i)
		AddVector(params.vectors[i]->name, params.vectors[i]->data, params.vectors[i]->nItems);
	for (u_int i = 0; i < params.normals.size(); ++i)
		AddNormal(params.normals[i]->name, params.normals[i]->data, params.normals[i]->nItems);
	for (u_int i = 0; i < params.spectra.size(); ++i)
		AddRGBColor(params.spectra[i]->name, params.spectra[i]->data, params.spectra[i]->nItems);
	for (u_int i = 0; i < params.strings.size(); ++i)
		AddString(params.strings[i]->name, params.strings[i]->data, params.strings[i]->nItems);
	for (u_int i = 0; i < params.textures.size(); ++i)
		AddTexture(params.textures[i]->name, *(params.textures[i]->data));
}

void ParamSet::AddFloat(const string &name, const float *data, u_int nItems)
{
	AddParamType(floats, name, data, nItems);
}
void ParamSet::AddInt(const string &name, const int *data, u_int nItems)
{
	AddParamType(ints, name, data, nItems);
}
void ParamSet::AddBool(const string &name, const bool *data, u_int nItems)
{
	AddParamType(bools, name, data, nItems);
}
void ParamSet::AddPoint(const string &name, const Point *data, u_int nItems)
{
	AddParamType(points, name, data, nItems);
}
void ParamSet::AddVector(const string &name, const Vector *data, u_int nItems)
{
	AddParamType(vectors, name, data, nItems);
}
void ParamSet::AddNormal(const string &name, const Normal *data, u_int nItems)
{
	AddParamType(normals, name, data, nItems);
}
void ParamSet::AddRGBColor(const string &name, const RGBColor *data, u_int nItems)
{
	AddParamType(spectra, name, data, nItems);
}
void ParamSet::AddString(const string &name, const string *data, u_int nItems)
{
	AddParamType(strings, name, data, nItems);
}
void ParamSet::AddTexture(const string &name, const string &value)
{
	AddParamType(textures, name, &value, 1);
}
bool ParamSet::EraseInt(const string &n) {
	return EraseParamType(ints, n);
}
bool ParamSet::EraseBool(const string &n) {
	return EraseParamType(bools, n);
}
bool ParamSet::EraseFloat(const string &n) {
	return EraseParamType(floats, n);
}
bool ParamSet::ErasePoint(const string &n) {
	return EraseParamType(points, n);
}
bool ParamSet::EraseVector(const string &n) {
	return EraseParamType(vectors, n);
}
bool ParamSet::EraseNormal(const string &n) {
	return EraseParamType(normals, n);
}
bool ParamSet::EraseRGBColor(const string &n) {
	return EraseParamType(spectra, n);
}
bool ParamSet::EraseString(const string &n) {
	return EraseParamType(strings, n);
}
bool ParamSet::EraseTexture(const string &n) {
	return EraseParamType(textures, n);
}
float ParamSet::FindOneFloat(const string &name, float d) const
{
	return LookupOne(floats, name, d);
}
const float *ParamSet::FindFloat(const string &name, u_int *nItems) const
{
	return LookupPtr(floats, name, nItems);
}
const int *ParamSet::FindInt(const string &name, u_int *nItems) const
{
	return LookupPtr(ints, name, nItems);
}
const bool *ParamSet::FindBool(const string &name, u_int *nItems) const
{
	return LookupPtr(bools, name, nItems);
}
int ParamSet::FindOneInt(const string &name, int d) const
{
	return LookupOne(ints, name, d);
}
bool ParamSet::FindOneBool(const string &name, bool d) const
{
	return LookupOne(bools, name, d);
}
const Point *ParamSet::FindPoint(const string &name, u_int *nItems) const
{
	return LookupPtr(points, name, nItems);
}
const Point &ParamSet::FindOnePoint(const string &name, const Point &d) const
{
	return LookupOne(points, name, d);
}
const Vector *ParamSet::FindVector(const string &name, u_int *nItems) const
{
	return LookupPtr(vectors, name, nItems);
}
const Vector &ParamSet::FindOneVector(const string &name, const Vector &d) const
{
	return LookupOne(vectors, name, d);
}
const Normal *ParamSet::FindNormal(const string &name, u_int *nItems) const
{
	return LookupPtr(normals, name, nItems);
}
const Normal &ParamSet::FindOneNormal(const string &name, const Normal &d) const
{
	return LookupOne(normals, name, d);
}
const RGBColor *ParamSet::FindRGBColor(const string &name, u_int *nItems) const
{
	return LookupPtr(spectra, name, nItems);
}
const RGBColor &ParamSet::FindOneRGBColor(const string &name, const RGBColor &d) const
{
	return LookupOne(spectra, name, d);
}
const string *ParamSet::FindString(const string &name, u_int *nItems) const
{
	return LookupPtr(strings, name, nItems);
}
const string &ParamSet::FindOneString(const string &name, const string &d) const
{
	return LookupOne(strings, name, d);
}
const string &ParamSet::FindTexture(const string &name) const
{
	static const string empty("");
	return LookupOne(textures, name, empty);
}
void ParamSet::MarkUsed(const ParamSet &p2) const {
	// marks any used params in p2 as used in this
	MarkAsUsed(ints, p2.ints);
	MarkAsUsed(bools, p2.bools);
	MarkAsUsed(floats, p2.floats);
	MarkAsUsed(points, p2.points);
	MarkAsUsed(vectors, p2.vectors);
	MarkAsUsed(normals, p2.normals);
	MarkAsUsed(spectra, p2.spectra);
	MarkAsUsed(strings, p2.strings);
	MarkAsUsed(textures, p2.textures);
}
void ParamSet::ReportUnused() const {
	CheckUnused(ints);
	CheckUnused(bools);
	CheckUnused(floats);
	CheckUnused(points);
	CheckUnused(vectors);
	CheckUnused(normals);
	CheckUnused(spectra);
	CheckUnused(strings);
	CheckUnused(textures);
}
void ParamSet::Clear() {
	DelParams(ints);
	DelParams(bools);
	DelParams(floats);
	DelParams(points);
	DelParams(vectors);
	DelParams(normals);
	DelParams(spectra);
	DelParams(strings);
	DelParams(textures);
}
string ParamSet::ToString() const {
	std::stringstream ret("");
	for (u_int i = 0; i < ints.size(); ++i) {
		const ParamSetItem<int> *item = ints[i];
		ret << "\"integer " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j] << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < bools.size(); ++i) {
		const ParamSetItem<bool> *item = bools[i];
		ret << "\"bool " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << (item->data[j] ? "true" : "false") << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < floats.size(); ++i) {
		const ParamSetItem<float> *item = floats[i];
		ret << "\"float " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j] << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < points.size(); ++i) {
		const ParamSetItem<Point> *item = points[i];
		ret << "\"point " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j].x << " " <<
				item->data[j].y << " " <<
				item->data[j].z << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < vectors.size(); ++i) {
		const ParamSetItem<Vector> *item = vectors[i];
		ret << "\"vector " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j].x << " " <<
				item->data[j].y << " " <<
				item->data[j].z << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < normals.size(); ++i) {
		const ParamSetItem<Normal> *item = normals[i];
		ret << "\"normal " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j].x << " " <<
				item->data[j].y << " " <<
				item->data[j].z << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < strings.size(); ++i) {
		const ParamSetItem<string> *item = strings[i];
		ret << "\"string " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j] << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < textures.size(); ++i) {
		const ParamSetItem<string> *item = textures[i];
		ret << "\"texture " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j] << " ";
		ret << "] ";
	}
	for (u_int i = 0; i < spectra.size(); ++i) {
		const ParamSetItem<RGBColor> *item = spectra[i];
		ret << "\"color " << item->name << "\" [";
		for (u_int j = 0; j < item->nItems; ++j)
			ret << item->data[j].c[0] << " " <<
				item->data[j].c[1] << " " <<
				item->data[j].c[2] << " ";
		ret << "] ";
	}
	return ret.str();
}

boost::shared_ptr<Texture<SWCSpectrum> >
	ParamSet::GetSWCSpectrumTexture(const string &n,
	const RGBColor &def) const
{
	boost::shared_ptr<Texture<SWCSpectrum> > texture(
		Context::GetActive()->GetColorTexture(FindTexture(n)));
	if (texture)
		return texture;
	RGBColor val = FindOneRGBColor(n, def);
	return boost::shared_ptr<Texture<SWCSpectrum> >(new ConstantRGBColorTexture(val));
}
boost::shared_ptr<Texture<float> >
	ParamSet::GetFloatTexture(const string &n) const
{
	return Context::GetActive()->GetFloatTexture(FindTexture(n));
}
boost::shared_ptr<Texture<float> >
	ParamSet::GetFloatTexture(const string &n, float def) const
{
	boost::shared_ptr<Texture<float> > texture(GetFloatTexture(n));
	if (texture)
		return texture;
	float val = FindOneFloat(n, def);
	return boost::shared_ptr<Texture<float> >(new ConstantFloatTexture(val));
}
boost::shared_ptr<Texture<FresnelGeneral> >
	ParamSet::GetFresnelTexture(const string &n, float def) const
{
	boost::shared_ptr<Texture<FresnelGeneral> > texture(
		Context::GetActive()->GetFresnelTexture(FindTexture(n)));
	if (texture)
		return texture;
	float val = FindOneFloat(n, def);
	return boost::shared_ptr<Texture<FresnelGeneral> >(new ConstantFresnelTexture(val));
}
boost::shared_ptr<Material> ParamSet::GetMaterial(const string &n) const
{
	return Context::GetActive()->GetMaterial(FindOneString(n, ""));
}

}
