
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

/* state used for include file stuff */
%{

#define YY_MAIN 0
#define YY_NEVER_INTERACTIVE 1

#include <sstream>
#include "lux.h"
#include "api.h"
#include "error.h"

struct ParamArray;

#include "luxparse.hpp"
/*
#ifdef WIN32
#include "luxparse.hpp"
#else
#include "luxparse.h"
#endif*/

#if defined(WIN32) && !defined(__CYGWIN__)
#pragma warning ( disable: 4244 )
#endif

struct IncludeInfo {
	string filename;
	YY_BUFFER_STATE bufState;
	u_int lineNum;
};
vector<IncludeInfo> includeStack;

extern u_int line_num;
extern string current_file;
int str_pos;

void add_string_char( char c )
{
	if (str_pos == 1023) {
		luxError(LUX_LIMIT, LUX_ERROR, "String is too long.");
		++str_pos;
	} else if (str_pos < 1023) {
		yylval.string[str_pos++] = c;
		yylval.string[str_pos] = '\0';
	}
}

extern void yyerror( const char *str );
//extern void ParseError( const char *format, ... ) PRINTF_FUNC;

void include_push(char *filename) {
	if (includeStack.size() > 32)
		luxError(LUX_NESTING,LUX_SEVERE,"Only 32 levels of nested Include allowed in scene files.");
    else {
        // Dade - Fix for BUG 570: including not existing files doesn't lead
        // anymore to a crash

        FILE *newFile = fopen(filename, "r");
        if (!newFile) {
            //Severe("Unable to open included scene file \"%s\"", filename);
            std::stringstream ss;
            ss<<"Unable to open included scene file "<<filename;
            luxError(LUX_NOFILE,LUX_SEVERE,ss.str().c_str());
        } else {
            IncludeInfo ii;
            ii.filename = current_file;
            ii.bufState = YY_CURRENT_BUFFER;
            ii.lineNum = line_num;
            includeStack.push_back(ii);

            current_file = filename;
            line_num = 1;

        	yyin = newFile;
        	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
        }
    }
}

void include_pop(void)
{
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer(includeStack.back().bufState);
	current_file = includeStack.back().filename;
	line_num = includeStack.back().lineNum;
	includeStack.pop_back();
}
%}
%option nounput
WHITESPACE [ \t\0xa]+
NUMBER [-+]?([0-9]+|(([0-9]+\.[0-9]*)|(\.[0-9]+)))([eE][-+]?[0-9]+)?
IDENT [a-zA-Z_][a-zA-Z_0-9]*
%x STR COMMENT INCL INCL_FILE
%%
Include { BEGIN(INCL); }
<INCL>{WHITESPACE} ;
<INCL>\" { BEGIN(INCL_FILE); }
<INCL>. { luxError( LUX_SYNTAX,LUX_SEVERE,"Illegal character following Include directive" ); }
<INCL_FILE>\" { BEGIN INITIAL; }
<INCL_FILE>. { luxError( LUX_SYNTAX,LUX_SEVERE,"Illegal character in Include file name" ); }
<INCL_FILE>[^\n\"]+ {
	BEGIN(INITIAL);
	include_push(yytext);
}
"#" { BEGIN COMMENT; }
<COMMENT>. /* eat it up */
<COMMENT>\n { line_num++; BEGIN INITIAL; }
Accelerator		{ return ACCELERATOR; }
AreaLightSource		{ return AREALIGHTSOURCE;	}
AttributeBegin 		{ return ATTRIBUTEBEGIN; 	}
AttributeEnd 		{ return ATTRIBUTEEND; 		}
Camera			{ return CAMERA;		}
ConcatTransform 	{ return CONCATTRANSFORM; 	}
CoordinateSystem	{ return COORDINATESYSTEM;	}
CoordSysTransform	{ return COORDSYSTRANSFORM;	}
SetEpsilon			{ return SETEPSILON; }
Film	 		{ return FILM;	 		}
Identity		{ return IDENTITY;		}
LightGroup		{ return LIGHTGROUP;		}
LightSource 		{ return LIGHTSOURCE;		}
LookAt			{ return LOOKAT;		}
Material		{ return MATERIAL;		}
MakeNamedMaterial	{ return MAKENAMEDMATERIAL;	}
NamedMaterial		{ return NAMEDMATERIAL;		}
ObjectBegin 		{ return OBJECTBEGIN;		}
ObjectEnd 		{ return OBJECTEND;		}
ObjectInstance 		{ return OBJECTINSTANCE;	}
MotionInstance 		{ return MOTIONINSTANCE;	}
PixelFilter 		{ return PIXELFILTER; 		}
ReverseOrientation 	{ return REVERSEORIENTATION;	}
Rotate			{ return ROTATE;		}
Sampler			{ return SAMPLER;		}
SearchPath		{ return SEARCHPATH;		}
Scale 			{ return SCALE;			}
PortalShape		{ return PORTALSHAPE;           }
Shape			{ return SHAPE;			}
SurfaceIntegrator	{ return SURFACEINTEGRATOR;	}
Texture			{ return TEXTURE;		}
TransformBegin	{ return TRANSFORMBEGIN;	}
TransformEnd	{ return TRANSFORMEND;	}
Transform		{ return TRANSFORM;		}
Translate		{ return TRANSLATE; 		}
Volume			{ return VOLUME;		}
VolumeIntegrator	{ return VOLUMEINTEGRATOR;	}
WorldBegin 		{ return WORLDBEGIN; 		}
WorldEnd 		{ return WORLDEND;		}
{WHITESPACE} /* do nothing */
\r /* jromang - do nothing */
\n { line_num++; }
{NUMBER} {
  yylval.num = static_cast<float>(atof(yytext));
  return NUM;
}
{IDENT} {
	strcpy( yylval.string, yytext );
	return ID;
}
"[" { return LBRACK; }
"]" { return RBRACK; }
\" { BEGIN STR; str_pos = 0; }
<STR>\\n {add_string_char('\n');}
<STR>\\t {add_string_char('\t');}
<STR>\\r {add_string_char('\r');}
<STR>\\b {add_string_char('\b');}
<STR>\\f {add_string_char('\f');}
<STR>\\\" {add_string_char('\"');}
<STR>\\\\ {add_string_char('\\');}
<STR>\\[0-9]{3} {
  char val = static_cast<char>(atoi(yytext+1) & 0xff);
  add_string_char(val);
}
<STR>\\\n {line_num++;}
<STR>\\. { add_string_char(yytext[1]);}
<STR>\" {BEGIN INITIAL; return STRING;}
<STR>. {add_string_char(yytext[0]);}
<STR>\n {luxError( LUX_SYNTAX,LUX_SEVERE,"Unterminated string!");}

. { std::stringstream ss; ss<<"Illegal character: "<<yytext[0]; luxError( LUX_SYNTAX,LUX_SEVERE,ss.str().c_str()); }
%%
int yywrap(void)
{
	if (includeStack.size() ==0) return 1;
	include_pop();
	BEGIN(INCL_FILE);
	return 0;
}

