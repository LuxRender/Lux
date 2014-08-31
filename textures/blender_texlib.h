// Dade - sources imported from Blender with authors permission

/**
 * blenlib/DNA_texture_types.h (mar-2001 nzc)
 *
 * $Id$ 
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef LUX_BLENDER_TEXLIB_H
#define LUX_BLENDER_TEXLIB_H 1

// blender_texlib.cpp*
#include "lux.h"
#include "slg/sdl/blender_noiselib.h"

namespace blender
{

#define VECCOPY(v1,v2)          {*(v1)= *(v2); *(v1+1)= *(v2+1); *(v1+2)= *(v2+2);}

inline void VecMulf(float *v1, float f)
{
        v1[0]*= f;
        v1[1]*= f;
        v1[2]*= f;
}

#define BRICONT         texres->tin= (texres->tin-0.5)*tex->contrast+tex->bright-0.5; \
    if(texres->tin<0.0) texres->tin= 0.0; else if(texres->tin>1.0) texres->tin= 1.0;

#define BRICONTRGB      texres->tr= tex->rfac*((texres->tr-0.5)*tex->contrast+tex->bright-0.5); \
    if(texres->tr<0.0) texres->tr= 0.0; \
    texres->tg= tex->gfac*((texres->tg-0.5)*tex->contrast+tex->bright-0.5); \
    if(texres->tg<0.0) texres->tg= 0.0; \
    texres->tb= tex->bfac*((texres->tb-0.5)*tex->contrast+tex->bright-0.5); \
    if(texres->tb<0.0) texres->tb= 0.0; 

typedef struct Tex {
    float noisesize, turbul;
    float bright, contrast, rfac, gfac, bfac;
    float filtersize;

    /* newnoise: musgrave parameters */
    float mg_H, mg_lacunarity, mg_octaves, mg_offset, mg_gain;

    /* newnoise: distorted noise amount, musgrave & voronoi output scale */
    float dist_amount, ns_outscale;

    /* newnoise: voronoi nearest neighbour weights, minkovsky exponent, distance metric & color type */
    float vn_w1, vn_w2, vn_w3, vn_w4, vn_mexp;
    short vn_distm, vn_coltype;

    short noisedepth, noisetype;

    /* newnoise: noisebasis type for clouds/marble/etc, noisebasis2 only used for distorted noise */
    short noisebasis, noisebasis2;

    short imaflag, flag;
    short type, stype;

    float cropxmin, cropymin, cropxmax, cropymax;
    short xrepeat, yrepeat;
    short extend;

    /* variables disabled, moved to struct iuser */
    short fie_ima;
    int len;
    int frames, offset, sfra;

    float checkerdist, nabla;
    float norfac;
} Tex;

/* localized texture result data */
/* note; tr tg tb ta has to remain in this order */
typedef struct TexResult {
        float tin, tr, tg, tb, ta;
        int talpha;
} TexResult;

int multitex(const Tex *tex, const float *texvec, TexResult *texres);

/* **************** TEX ********************* */

/* type */
#define TEX_CLOUDS		1
#define TEX_WOOD		2
#define TEX_MARBLE		3
#define TEX_MAGIC		4
#define TEX_BLEND		5
#define TEX_STUCCI		6
#define TEX_NOISE		7
#define TEX_IMAGE		8
#define TEX_PLUGIN		9
#define TEX_ENVMAP		10
#define TEX_MUSGRAVE	11
#define TEX_VORONOI		12
#define TEX_DISTNOISE	13

/* musgrave stype */
/*
#define TEX_MFRACTAL		0
#define TEX_RIDGEDMF		1
#define TEX_HYBRIDMF		2
#define TEX_FBM				3
#define TEX_HTERRAIN		4
*/
/* newnoise: noisebasis 1 & 2 */
/*
#define TEX_BLENDER			0
#define TEX_STDPERLIN		1
#define TEX_NEWPERLIN		2
#define TEX_VORONOI_F1		3
#define TEX_VORONOI_F2		4
#define TEX_VORONOI_F3		5
#define TEX_VORONOI_F4		6
#define TEX_VORONOI_F2_F1	7
#define TEX_VORONOI_CRACKLE		8
#define TEX_CELLNOISE		14
*/
/* newnoise: Voronoi distance metrics, vn_distm */
/*
#define TEX_DISTANCE		0
#define TEX_DISTANCE_SQUARED		1
#define TEX_MANHATTAN		2
#define TEX_CHEBYCHEV		3
#define TEX_MINKOVSKY_HALF		4
#define TEX_MINKOVSKY_FOUR		5
#define TEX_MINKOVSKY		6
*/
/* imaflag */
#define TEX_INTERPOL	1
#define TEX_USEALPHA	2
#define TEX_MIPMAP		4
#define TEX_IMAROT		16
#define TEX_CALCALPHA	32
#define TEX_NORMALMAP	2048
#define TEX_GAUSS_MIP	4096
#define TEX_FILTER_MIN	8192

/* imaflag unused, only for version check */
#define TEX_FIELDS_		8
#define TEX_ANIMCYCLIC_	64
#define TEX_ANIM5_		128
#define TEX_ANTIALI_	256
#define TEX_ANTISCALE_	512
#define TEX_STD_FIELD_	1024

/* flag */
#define TEX_COLORBAND		1
#define TEX_FLIPBLEND		2
#define TEX_NEGALPHA		4
#define TEX_CHECKER_ODD		8
#define TEX_CHECKER_EVEN	16
#define TEX_PRV_ALPHA		32
#define TEX_PRV_NOR			64
#define TEX_REPEAT_XMIR		128
#define TEX_REPEAT_YMIR		256
#define TEX_FLAG_MASK		( TEX_COLORBAND | TEX_FLIPBLEND | TEX_NEGALPHA | TEX_CHECKER_ODD | TEX_CHECKER_EVEN | TEX_PRV_ALPHA | TEX_PRV_NOR | TEX_REPEAT_XMIR | TEX_REPEAT_YMIR ) 

/* extend (starts with 1 because of backward comp.) */
#define TEX_EXTEND		1
#define TEX_CLIP		2
#define TEX_REPEAT		3
#define TEX_CLIPCUBE	4
#define TEX_CHECKER		5

/* noisetype */
#define TEX_NOISESOFT	0
#define TEX_NOISEPERL	1
/*
// tex->noisebasis2 in texture.c - wood waveforms
#define TEX_SIN			0
#define TEX_SAW			1
#define TEX_TRI			2

// tex->stype in texture.c - wood types
#define TEX_BAND		0
#define TEX_RING		1
#define TEX_BANDNOISE	2
#define TEX_RINGNOISE	3
*/
/* tex->stype in texture.c - cloud types */
#define TEX_DEFAULT		0
#define TEX_COLOR		1
/*
// tex->stype in texture.c - marble types 
#define TEX_SOFT		0
#define TEX_SHARP		1
#define TEX_SHARPER		2

// tex->stype in texture.c - blend types 
#define TEX_LIN			0
#define TEX_QUAD		1
#define TEX_EASE		2
#define TEX_DIAG		3
#define TEX_SPHERE		4
#define TEX_HALO		5
#define TEX_RAD			6

// tex->stype in texture.c - stucci types 
#define TEX_PLASTIC		0
#define TEX_WALLIN		1
#define TEX_WALLOUT		2
*/
/* tex->stype in texture.c - voronoi types */
#define TEX_INTENSITY	0
#define TEX_COL1		1
#define TEX_COL2		2
#define TEX_COL3		3

/* mtex->normapspace */
#define MTEX_NSPACE_CAMERA	0
#define MTEX_NSPACE_WORLD	1
#define MTEX_NSPACE_OBJECT	2
#define MTEX_NSPACE_TANGENT	3

/* wrap */
#define MTEX_FLAT		0
#define MTEX_CUBE		1
#define MTEX_TUBE		2
#define MTEX_SPHERE		3

/* return value */
#define TEX_INT		0
#define TEX_RGB		1

} // namespace blender

#endif // LUX_BLENDER_TEXLIB_H
