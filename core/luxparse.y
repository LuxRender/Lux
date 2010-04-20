
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

%{
#include "api.h"
#include "lux.h"
#include "error.h"
#include "paramset.h"
#include "context.h"
#include <stdarg.h>
#include <sstream>

using namespace lux;

extern int yylex(void);
u_int lineNum = 0;
string currentFile;

#define YYMAXDEPTH 100000000

void yyerror(const char *str)
{
	std::stringstream ss;
	ss << "Parsing error";
	if (currentFile != "")
		ss << " in file '" << currentFile << "'";
	if (lineNum > 0)
		ss << " at line " << lineNum;
	ss << ": " << str;
	luxError(LUX_SYNTAX, LUX_SEVERE, ss.str().c_str());
}

class ParamListElem {
public:
	ParamListElem() : token(NULL), arg(NULL), size(0), textureHelper(false)
	{ }
	const char *token;
	void *arg;
	u_int size;
	bool textureHelper;
};
u_int curParamlistAllocated = 0;
u_int curParamlistSize = 0;
ParamListElem *curParamlists = NULL;

#define CPAL curParamlistAllocated
#define CPS curParamlistSize
#define CP curParamlists
#define CPT(__n) curParamlists[(__n)].token
#define CPA(__n) curParamlists[(__n)].arg
#define CPSZ(__n) curParamlists[(__n)].size
#define CPTH(__n) curParamlists[(__n)].textureHelper

class ParamArray {
public:
	ParamArray() : elementSize(0), allocated(0), nelems(0), array(NULL) { }
	u_int elementSize;
	u_int allocated;
	u_int nelems;
	void *array;
};

ParamArray *curArray = NULL;
bool arrayIsSingleString = false;

#define NA(r) ((float *) r->array)
#define SA(r) ((const char **) r->array)

void AddArrayElement(void *elem)
{
	if (curArray->nelems >= curArray->allocated) {
		curArray->allocated = 2 * curArray->allocated + 1;
		curArray->array = realloc(curArray->array,
			curArray->allocated * curArray->elementSize);
	}
	char *next = static_cast<char *>(curArray->array) +
		curArray->nelems * curArray->elementSize;
	memcpy(next, elem, curArray->elementSize);
	curArray->nelems++;
}

ParamArray *ArrayDup(ParamArray *ra)
{
	ParamArray *ret = new ParamArray;
	ret->elementSize = ra->elementSize;
	ret->allocated = ra->allocated;
	ret->nelems = ra->nelems;
	ret->array = malloc(ra->nelems * ra->elementSize);
	memcpy(ret->array, ra->array, ra->nelems * ra->elementSize);
	return ret;
}

void ArrayFree(ParamArray *ra)
{
	free(ra->array);
	delete ra;
}

void FreeArgs()
{
	for (u_int i = 0; i < CPS; ++i) {
		// NOTE - Ratow - freeing up strings inside string type args
		if (memcmp("string", CPT(i), 6) == 0 ||
			memcmp("texture", CPT(i), 7) == 0) {
			for (u_int j = 0; j < CPSZ(i); ++j)
				free(static_cast<char **>(CPA(i))[j]);
		}
		delete[] static_cast<char *>(CPA(i));
	}
}

static bool VerifyArrayLength(ParamArray *arr, u_int required,
	const char *command)
{
	if (arr->nelems != required) {
		std::stringstream ss;
		ss << command << " requires a(n) " << required <<
			" element array!";
		luxError(LUX_SYNTAX, LUX_SEVERE, ss.str().c_str());
		return false;
	}
	return true;
}
static void InitParamSet(ParamSet &ps, u_int count, ParamListElem *list);
#define YYPRINT(file, type, value)  \
{ \
	if ((type) == ID || (type) == STRING) \
		fprintf((file), " %s", (value).string); \
	else if ((type) == NUM) \
		fprintf((file), " %f", (value).num); \
}
%}

%union {
char string[1024];
float num;
ParamArray *ribarray;
}
%token <string> STRING ID
%token <num> NUM
%token LBRACK RBRACK

%token ACCELERATOR AREALIGHTSOURCE ATTRIBUTEBEGIN ATTRIBUTEEND
%token CAMERA CONCATTRANSFORM COORDINATESYSTEM COORDSYSTRANSFORM EXTERIOR
%token FILM IDENTITY INTERIOR LIGHTSOURCE LOOKAT MATERIAL MAKENAMEDMATERIAL
%token MAKENAMEDVOLUME NAMEDMATERIAL OBJECTBEGIN OBJECTEND OBJECTINSTANCE
%token MOTIONINSTANCE LIGHTGROUP PIXELFILTER REVERSEORIENTATION ROTATE SAMPLER
%token SCALE SEARCHPATH PORTALSHAPE SHAPE SURFACEINTEGRATOR TEXTURE
%token TRANSFORMBEGIN TRANSFORMEND TRANSFORM TRANSLATE VOLUME VOLUMEINTEGRATOR
%token WORLDBEGIN WORLDEND

%token HIGH_PRECEDENCE

%type<ribarray> array num_array string_array
%type<ribarray> real_num_array real_string_array
%%
start: ri_stmt_list
{
};

array_init: %prec HIGH_PRECEDENCE
{
	if (curArray)
		ArrayFree(curArray);
	curArray = new ParamArray;
	arrayIsSingleString = false;
};

string_array_init: %prec HIGH_PRECEDENCE
{
	curArray->elementSize = sizeof(const char *);
};

num_array_init: %prec HIGH_PRECEDENCE
{
	curArray->elementSize = sizeof(float);
};

array: string_array
{
	$$ = $1;
}
| num_array
{
	$$ = $1;
};

string_array: real_string_array
{
	$$ = $1;
}
| single_element_string_array
{
	$$ = ArrayDup(curArray);
	arrayIsSingleString = true;
};

real_string_array: array_init LBRACK string_list RBRACK
{
	$$ = ArrayDup(curArray);
};

single_element_string_array: array_init string_list_entry
{
};

string_list: string_list string_list_entry
{
}
| string_list_entry
{
};

string_list_entry: string_array_init STRING
{
	char *toAdd = strdup($2);
	AddArrayElement(&toAdd);
};

num_array: real_num_array
{
	$$ = $1;
}
| single_element_num_array
{
	$$ = ArrayDup(curArray);
};

real_num_array: array_init LBRACK num_list RBRACK
{
	$$ = ArrayDup(curArray);
};

single_element_num_array: array_init num_list_entry
{
};

num_list: num_list num_list_entry
{
}
| num_list_entry
{
};

num_list_entry: num_array_init NUM
{
	float toAdd = $2;
	AddArrayElement(&toAdd);
};

paramlist: paramlist_init paramlist_contents
{
};

paramlist_init: %prec HIGH_PRECEDENCE
{
	CPS = 0;
};

paramlist_contents: paramlist_entry paramlist_contents
{
}
|
{
};

paramlist_entry: STRING array
{
	void *arg = new char[$2->nelems * $2->elementSize];
	memcpy(arg, $2->array, $2->nelems * $2->elementSize);
	if (CPS >= CPAL) {
		CPAL = 2 * CPAL + 1;
		CP = static_cast<ParamListElem *>(realloc(CP,
			CPAL * sizeof(ParamListElem)));
	}
	CPT(CPS) = $1;
	CPA(CPS) = arg;
	CPSZ(CPS) = $2->nelems;
	CPTH(CPS) = arrayIsSingleString;
	++CPS;
	ArrayFree($2);
};

ri_stmt_list: ri_stmt_list ri_stmt
{
}
| ri_stmt
{
};

ri_stmt: ACCELERATOR STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Accelerator($2, params);
	FreeArgs();
}
| AREALIGHTSOURCE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->AreaLightSource($2, params);
	FreeArgs();
}
| ATTRIBUTEBEGIN
{
	luxAttributeBegin();
}
| ATTRIBUTEEND
{
	luxAttributeEnd();
}
| CAMERA STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Camera($2, params);
	FreeArgs();
}
| CONCATTRANSFORM num_array
{
	if (VerifyArrayLength($2, 16, "ConcatTransform"))
		luxConcatTransform(static_cast<float *>($2->array));
	ArrayFree($2);
}
| COORDINATESYSTEM STRING
{
	luxCoordinateSystem($2);
}
| COORDSYSTRANSFORM STRING
{
	luxCoordSysTransform($2);
}
| EXTERIOR STRING
{
	Context::GetActive()->Exterior($2);
}
| FILM STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Film($2, params);
	FreeArgs();
}
| IDENTITY
{
	luxIdentity();
}
| INTERIOR STRING
{
	Context::GetActive()->Interior($2);
}
| LIGHTGROUP STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->LightGroup($2, params);
	FreeArgs();
}
| LIGHTSOURCE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->LightSource($2, params);
	FreeArgs();
}
| LOOKAT NUM NUM NUM NUM NUM NUM NUM NUM NUM
{
	luxLookAt($2, $3, $4, $5, $6, $7, $8, $9, $10);
}
| MATERIAL STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Material($2, params);
	FreeArgs();
}
| MAKENAMEDMATERIAL STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->MakeNamedMaterial($2, params);
	FreeArgs();
}
| MAKENAMEDVOLUME STRING STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->MakeNamedVolume($2, $3, params);
	FreeArgs();
}
| NAMEDMATERIAL STRING
{
	Context::GetActive()->NamedMaterial($2);
}
| OBJECTBEGIN STRING
{
	luxObjectBegin($2);
}
| OBJECTEND
{
	luxObjectEnd();
}
| OBJECTINSTANCE STRING
{
	luxObjectInstance($2);
}
| MOTIONINSTANCE STRING NUM NUM STRING
{
	luxMotionInstance($2, $3, $4, $5);
}
| PIXELFILTER STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->PixelFilter($2, params);
	FreeArgs();
}
| REVERSEORIENTATION
{
	luxReverseOrientation();
}
| ROTATE NUM NUM NUM NUM
{
	luxRotate($2, $3, $4, $5);
}
| SAMPLER STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Sampler($2, params);
	FreeArgs();
}
| SCALE NUM NUM NUM
{
	luxScale($2, $3, $4);
}
| SEARCHPATH STRING
{
	;//luxSearchPath($2);//FIXME - Unimplemented
}
| SHAPE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Shape($2, params);
	FreeArgs();
}
| PORTALSHAPE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->PortalShape($2, params);
	FreeArgs();
}
| SURFACEINTEGRATOR STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->SurfaceIntegrator($2, params);
	FreeArgs();
}
| TEXTURE STRING STRING STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Texture($2, $3, $4, params);
	FreeArgs();
}
| TRANSFORMBEGIN
{
	luxTransformBegin();
}
| TRANSFORMEND
{
	luxTransformEnd();
}
| TRANSFORM real_num_array
{
	if (VerifyArrayLength($2, 16, "Transform"))
		luxTransform(static_cast<float *>($2->array));
	ArrayFree($2);
}
| TRANSLATE NUM NUM NUM
{
	luxTranslate($2, $3, $4);
}
| VOLUMEINTEGRATOR STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->VolumeIntegrator($2, params);
	FreeArgs();
}
| VOLUME STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CP);
	Context::GetActive()->Volume($2, params);
	FreeArgs();
}
| WORLDBEGIN
{
	luxWorldBegin();
}
| WORLDEND
{
	luxWorldEnd();
};
%%
static void InitParamSet(ParamSet &ps, u_int count, ParamListElem *list) {
	ps.Clear();
	for (u_int i = 0; i < count; ++i) {
		ParamType type;
		string name;
		if (!LookupType(list[i].token, &type, name)) {
			std::stringstream ss;
			ss << "Type of parameter '" << list[i].token <<
				"' is unknown";
			luxError(LUX_SYNTAX, LUX_WARNING, ss.str().c_str());
			continue;
		}
		if (list[i].textureHelper && type != PARAM_TYPE_TEXTURE &&
			type != PARAM_TYPE_STRING) {
			std::stringstream ss;
			ss << "Bad type for " << name <<
				". Changing it to a texture.";
			luxError(LUX_SYNTAX, LUX_WARNING, ss.str().c_str());
			type = PARAM_TYPE_TEXTURE;
		}
		void *data = list[i].arg;
		u_int nItems = list[i].size;
		if (type == PARAM_TYPE_INT) {
			// parser doesn't handle ints, so convert from floats
			int *idata = new int[nItems];
			float *fdata = static_cast<float *>(data);
			for (u_int j = 0; j < nItems; ++j)
				idata[j] = static_cast<int>(fdata[j]);
			ps.AddInt(name, idata, nItems);
			delete[] idata;
		} else if (type == PARAM_TYPE_BOOL) {
			// strings -> bools
			bool *bdata = new bool[nItems];
			for (u_int j = 0; j < nItems; ++j) {
				string s(*static_cast<const char **>(data));
				if (s == "true")
					bdata[j] = true;
				else if (s == "false")
					bdata[j] = false;
				else {
					std::stringstream ss;
					ss << "Value '" << s << "' unknown for boolean parameter '" <<
						list[i].token <<
						"'. Using 'false'.";
					luxError(LUX_SYNTAX, LUX_WARNING,
						ss.str().c_str());
					bdata[j] = false;
				}
			}
			ps.AddBool(name, bdata, nItems);
			delete[] bdata;
		} else if (type == PARAM_TYPE_FLOAT) {
			ps.AddFloat(name, static_cast<float *>(data), nItems);
		} else if (type == PARAM_TYPE_POINT) {
			ps.AddPoint(name, static_cast<Point *>(data), nItems / 3);
		} else if (type == PARAM_TYPE_VECTOR) {
			ps.AddVector(name, static_cast<Vector *>(data), nItems / 3);
		} else if (type == PARAM_TYPE_NORMAL) {
			ps.AddNormal(name, static_cast<Normal *>(data), nItems / 3);
		} else if (type == PARAM_TYPE_COLOR) {
			ps.AddRGBColor(name, static_cast<RGBColor *>(data), nItems / COLOR_SAMPLES);
		} else if (type == PARAM_TYPE_STRING) {
			string *strings = new string[nItems];
			for (u_int j = 0; j < nItems; ++j)
				strings[j] = string(static_cast<const char **>(data)[j]);
			ps.AddString(name, strings, nItems);
			delete[] strings;
		} else if (type == PARAM_TYPE_TEXTURE) {
			if (nItems == 1) {
				string val(*static_cast<const char **>(data));
				ps.AddTexture(name, val);
			} else {
				std::stringstream ss;
				ss << "Only one string allowed for 'texture' parameter " << name;
				luxError(LUX_SYNTAX, LUX_ERROR,
					ss.str().c_str());
			}
		}
	}
}
