
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

%{
#include "api.h"
#include "lux.h"
#include "error.h"
#include "paramset.h"
#include <stdarg.h>
#include <sstream>

using namespace lux;

extern int yylex( void );
int line_num = 0;
string current_file;

#define YYMAXDEPTH 100000000

void yyerror( char *str ) {
	std::stringstream ss;
	ss<<"Parsing error: "<<str;
	luxError( LUX_SYNTAX,LUX_SEVERE,ss.str().c_str());
	//Severe( "Parsing error: %s", str);
}

/*
void ParseError( const char *format, ... ) PRINTF_FUNC;

void ParseError( const char *format, ... ) {
	char error[4096];
	va_list args;
	va_start( args, format );
	vsnprintf(error, 4096, format, args);
	yyerror(error);
	va_end( args );
}*/

int cur_paramlist_allocated = 0;
int cur_paramlist_size = 0;
const char **cur_paramlist_tokens = NULL;
void **cur_paramlist_args = NULL;
int *cur_paramlist_sizes = NULL;
bool *cur_paramlist_texture_helper = NULL;

#define CPS cur_paramlist_size
#define CPT cur_paramlist_tokens
#define CPA cur_paramlist_args
#define CPTH cur_paramlist_texture_helper
#define CPSZ cur_paramlist_sizes

typedef struct ParamArray {
	int element_size;
	int allocated;
	int nelems;
	void *array;
} ParamArray;

ParamArray *cur_array = NULL;
bool array_is_single_string = false;

#define NA(r) ((float *) r->array)
#define SA(r) ((const char **) r->array)

void AddArrayElement( void *elem ) {
	if (cur_array->nelems >= cur_array->allocated) {
		cur_array->allocated = 2*cur_array->allocated + 1;
		cur_array->array = realloc( cur_array->array,
			cur_array->allocated*cur_array->element_size );
	}
	char *next = ((char *)cur_array->array) + cur_array->nelems *
		cur_array->element_size;
	memcpy( next, elem, cur_array->element_size );
	cur_array->nelems++;
}

ParamArray *ArrayDup( ParamArray *ra )
{
	ParamArray *ret = new ParamArray;
	ret->element_size = ra->element_size;
	ret->allocated = ra->allocated;
	ret->nelems = ra->nelems;
	ret->array = malloc(ra->nelems * ra->element_size);
	memcpy( ret->array, ra->array, ra->nelems * ra->element_size );
	return ret;
}

void ArrayFree( ParamArray *ra )
{
	free(ra->array);
	delete ra;
}

void FreeArgs()
{
	for (int i = 0; i < cur_paramlist_size; ++i)
		delete[] ((char *)cur_paramlist_args[i]);
}

static bool VerifyArrayLength( ParamArray *arr, int required,
	const char *command ) {
	if (arr->nelems != required) {
		std::stringstream ss;
		ss<<command<<" requires a(n) "<<required<<" element array!";
		//ParseError( "%s requires a(n) %d element array!", command, required);
		return false;
	}
	return true;
}
enum { PARAM_TYPE_INT, PARAM_TYPE_BOOL, PARAM_TYPE_FLOAT, PARAM_TYPE_POINT,
	PARAM_TYPE_VECTOR, PARAM_TYPE_NORMAL, PARAM_TYPE_COLOR,
	PARAM_TYPE_STRING, PARAM_TYPE_TEXTURE };
static void InitParamSet(ParamSet &ps, int count, const char **tokens,
	void **args, int *sizes, bool *texture_helper);
static bool lookupType(const char *token, int *type, string &name);
#define YYPRINT(file, type, value)  \
{ \
	if ((type) == ID || (type) == STRING) \
		fprintf ((file), " %s", (value).string); \
	else if ((type) == NUM) \
		fprintf ((file), " %f", (value).num); \
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
%token CAMERA CONCATTRANSFORM COORDINATESYSTEM COORDSYSTRANSFORM
%token FILM IDENTITY LIGHTSOURCE LOOKAT MATERIAL
%token OBJECTBEGIN OBJECTEND OBJECTINSTANCE
%token PIXELFILTER REVERSEORIENTATION ROTATE SAMPLER SCALE
%token SEARCHPATH PORTALSHAPE SHAPE SURFACEINTEGRATOR TEXTURE TRANSFORMBEGIN TRANSFORMEND
%token TRANSFORM TRANSLATE VOLUME VOLUMEINTEGRATOR WORLDBEGIN WORLDEND

%token HIGH_PRECEDENCE

%type<ribarray> array num_array string_array
%type<ribarray> real_num_array real_string_array
%%
start: ri_stmt_list
{
};

array_init: %prec HIGH_PRECEDENCE
{
	if (cur_array) ArrayFree( cur_array );
	cur_array = new ParamArray;
	cur_array->allocated = 0;
	cur_array->nelems = 0;
	cur_array->array = NULL;
	array_is_single_string = false;
};

string_array_init: %prec HIGH_PRECEDENCE
{
	cur_array->element_size = sizeof( const char * );
};

num_array_init: %prec HIGH_PRECEDENCE
{
	cur_array->element_size = sizeof( float );
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
	$$ = ArrayDup(cur_array);
	array_is_single_string = true;
};

real_string_array: array_init LBRACK string_list RBRACK
{
	$$ = ArrayDup(cur_array);
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
	char *to_add = strdup($2);
	AddArrayElement( &to_add );
};

num_array: real_num_array
{
	$$ = $1;
}
| single_element_num_array
{
	$$ = ArrayDup(cur_array);
};

real_num_array: array_init LBRACK num_list RBRACK
{
	$$ = ArrayDup(cur_array);
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
	float to_add = $2;
	AddArrayElement( &to_add );
};

paramlist: paramlist_init paramlist_contents
{
};

paramlist_init: %prec HIGH_PRECEDENCE
{
	cur_paramlist_size = 0;
};

paramlist_contents: paramlist_entry paramlist_contents
{
}
|
{
};

paramlist_entry: STRING array
{
	void *arg = new char[ $2->nelems * $2->element_size ];
	memcpy(arg, $2->array, $2->nelems * $2->element_size);
	if (cur_paramlist_size >= cur_paramlist_allocated) {
		cur_paramlist_allocated = 2*cur_paramlist_allocated + 1;
		cur_paramlist_tokens = (const char **) realloc(cur_paramlist_tokens, cur_paramlist_allocated*sizeof(const char *) );
		cur_paramlist_args = (void * *) realloc( cur_paramlist_args, cur_paramlist_allocated*sizeof(void *) );
		cur_paramlist_sizes = (int *) realloc( cur_paramlist_sizes, cur_paramlist_allocated*sizeof(int) );
		cur_paramlist_texture_helper = (bool *) realloc( cur_paramlist_texture_helper, cur_paramlist_allocated*sizeof(bool) );
	}
	cur_paramlist_tokens[cur_paramlist_size] = $1;
	cur_paramlist_sizes[cur_paramlist_size] = $2->nelems;
	cur_paramlist_texture_helper[cur_paramlist_size] = array_is_single_string;
	cur_paramlist_args[cur_paramlist_size++] = arg;
	ArrayFree( $2 );
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
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxAccelerator($2, params);
	FreeArgs();
}
| AREALIGHTSOURCE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxAreaLightSource($2, params);
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
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxCamera($2, params);
	FreeArgs();
}
| CONCATTRANSFORM num_array
{
	if (VerifyArrayLength( $2, 16, "ConcatTransform" ))
		luxConcatTransform( (float *) $2->array );
	ArrayFree( $2 );
}
| COORDINATESYSTEM STRING
{
	luxCoordinateSystem( $2 );
}
| COORDSYSTRANSFORM STRING
{
	luxCoordSysTransform( $2 );
}
| FILM STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxFilm($2, params);
	FreeArgs();
}
| IDENTITY
{
	luxIdentity();
}
| LIGHTSOURCE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxLightSource($2, params);
	FreeArgs();
}
| LOOKAT NUM NUM NUM NUM NUM NUM NUM NUM NUM
{
	luxLookAt($2, $3, $4, $5, $6, $7, $8, $9, $10);
}
| MATERIAL STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxMaterial($2, params);
	FreeArgs();
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
| PIXELFILTER STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxPixelFilter($2, params);
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
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxSampler($2, params);
	FreeArgs();
}
| SCALE NUM NUM NUM
{
	luxScale($2, $3, $4);
}
| SEARCHPATH STRING
{
	;//luxSearchPath($2);
}
| SHAPE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxShape($2, params);
	FreeArgs();
}
| PORTALSHAPE STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxPortalShape($2, params);
	FreeArgs();
}
| SURFACEINTEGRATOR STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxSurfaceIntegrator($2, params);
	FreeArgs();
}
| TEXTURE STRING STRING STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxTexture($2, $3, $4, params);
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
	if (VerifyArrayLength( $2, 16, "Transform" ))
		luxTransform( (float *) $2->array );
	ArrayFree( $2 );
}
| TRANSLATE NUM NUM NUM
{
	luxTranslate($2, $3, $4);
}
| VOLUMEINTEGRATOR STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxVolumeIntegrator($2, params);
	FreeArgs();
}
| VOLUME STRING paramlist
{
	ParamSet params;
	InitParamSet(params, CPS, CPT, CPA, CPSZ, CPTH);
	luxVolume($2, params);
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
static void InitParamSet(ParamSet &ps, int count, const char **tokens,
		void **args, int *sizes, bool *texture_helper) {
	ps.Clear();
	for (int i = 0; i < count; ++i) {
		int type;
		string name;
		if (lookupType(tokens[i], &type, name)) {
			if (texture_helper && texture_helper[i] && type != PARAM_TYPE_TEXTURE && type != PARAM_TYPE_STRING)
			{
				std::stringstream ss;
				ss<<"Bad type for "<<name<<". Changing it to a texture.";
				luxError( LUX_SYNTAX,LUX_WARNING,ss.str().c_str());
				//Warning( "Bad type for %s. Changing it to a texture.", name.c_str());
				type = PARAM_TYPE_TEXTURE;
			}
			void *data = args[i];
			int nItems = sizes[i];
			if (type == PARAM_TYPE_INT) {
				// parser doesn't handle ints, so convert from floats here....
				int nAlloc = sizes[i];
				int *idata = new int[nAlloc];
				float *fdata = (float *)data;
				for (int j = 0; j < nAlloc; ++j)
					idata[j] = int(fdata[j]);
				ps.AddInt(name, idata, nItems);
				delete[] idata;
			}
			else if (type == PARAM_TYPE_BOOL) {
				// strings -> bools
				int nAlloc = sizes[i];
				bool *bdata = new bool[nAlloc];
				for (int j = 0; j < nAlloc; ++j) {
					string s(*((const char **)data));
					if (s == "true") bdata[j] = true;
					else if (s == "false") bdata[j] = false;
					else {
						std::stringstream ss;
						ss<<"Value '"<<s<<"' unknown for boolean parameter '"<<tokens[i]<<"'. Using 'false'.";
						luxError( LUX_SYNTAX,LUX_WARNING,ss.str().c_str());
						//Warning("Value \"%s\" unknown for boolean parameter \"%s\"."
						//	"Using \"false\".", s.c_str(), tokens[i]);
						bdata[j] = false;
					}
				}
				ps.AddBool(name, bdata, nItems);
				delete[] bdata;
			}
			else if (type == PARAM_TYPE_FLOAT) {
				ps.AddFloat(name, (float *)data, nItems);
			} else if (type == PARAM_TYPE_POINT) {
				ps.AddPoint(name, (Point *)data, nItems / 3);
			} else if (type == PARAM_TYPE_VECTOR) {
				ps.AddVector(name, (Vector *)data, nItems / 3);
			} else if (type == PARAM_TYPE_NORMAL) {
				ps.AddNormal(name, (Normal *)data, nItems / 3);
			} else if (type == PARAM_TYPE_COLOR) {
				ps.AddSpectrum(name, (Spectrum *)data, nItems / COLOR_SAMPLES);
			} else if (type == PARAM_TYPE_STRING) {
				string *strings = new string[nItems];
				for (int j = 0; j < nItems; ++j)
					strings[j] = string(*((const char **)data+j));
				ps.AddString(name, strings, nItems);
				delete[] strings;
			}
			else if (type == PARAM_TYPE_TEXTURE) {
				if (nItems == 1) {
					string val(*((const char **)data));
					ps.AddTexture(name, val);
				}
				else
				{
						//Error("Only one string allowed for \"texture\" parameter \"%s\"", name.c_str());
						std::stringstream ss;
						ss<<"Only one string allowed for 'texture' parameter "<<name;
						luxError( LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
				}
			}
		}
		else
		{
			//Warning("Type of parameter \"%s\" is unknown", tokens[i]);
			std::stringstream ss;
			ss<<"Type of parameter '"<<tokens[i]<<"' is unknown";
			luxError( LUX_SYNTAX,LUX_WARNING,ss.str().c_str());
		}
	}
}
static bool lookupType(const char *token, int *type, string &name) {
	BOOST_ASSERT(token != NULL);
	*type = 0;
	const char *strp = token;
	while (*strp && isspace(*strp))
		++strp;
	if (!*strp) {
		//Error("Parameter \"%s\" doesn't have a type declaration?!", token);
		std::stringstream ss;
		ss<<"Parameter '"<<token<<"' doesn't have a type declaration?!";
		luxError( LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
		return false;
	}
	#define TRY_DECODING_TYPE(name, mask) \
		if (strncmp(name, strp, strlen(name)) == 0) { \
			*type = mask; strp += strlen(name); \
		}
	     TRY_DECODING_TYPE("float",    PARAM_TYPE_FLOAT)
	else TRY_DECODING_TYPE("integer",  PARAM_TYPE_INT)
	else TRY_DECODING_TYPE("bool",     PARAM_TYPE_BOOL)
	else TRY_DECODING_TYPE("point",    PARAM_TYPE_POINT)
	else TRY_DECODING_TYPE("vector",   PARAM_TYPE_VECTOR)
	else TRY_DECODING_TYPE("normal",   PARAM_TYPE_NORMAL)
	else TRY_DECODING_TYPE("string",   PARAM_TYPE_STRING)
	else TRY_DECODING_TYPE("texture",  PARAM_TYPE_TEXTURE)
	else TRY_DECODING_TYPE("color",    PARAM_TYPE_COLOR)
	else {
		//Error("Unable to decode type for token \"%s\"", token);
		std::stringstream ss;
		ss<<"Unable to decode type for token '"<<token<<"'";
		luxError( LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
		return false;
	}
	while (*strp && isspace(*strp))
		++strp;
	name = string(strp);
	return true;
}
