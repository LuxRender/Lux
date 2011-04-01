// Dade - sources imported from Blender with authors permission

/*
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
 *
 */

#include <math.h>

#include "blender_texlib.h"

namespace blender {

typedef unsigned long long r_uint64;

#define RNG_MULTIPLIER	0x5DEECE66Dll
#define RNG_MASK		0x0000FFFFFFFFFFFFll
#define RNG_ADDEND		0xB

struct RNG {
    r_uint64 X;
};

static RNG theBLI_rng = {0};

int rng_getInt(RNG *rng) {
    rng->X = (RNG_MULTIPLIER * rng->X + RNG_ADDEND) & RNG_MASK;
    return (int) (rng->X >> 17);
}

int BLI_rand(void) {
    return rng_getInt(&theBLI_rng);
}

//------------------------------------------------------------------------------

static int blend(const Tex *tex, const float *texvec, TexResult *texres) {
    float x, y, t;

    if (tex->flag & TEX_FLIPBLEND) {
        x = texvec[1];
        y = texvec[0];
    } else {
        x = texvec[0];
        y = texvec[1];
    }

    if (tex->stype == TEX_LIN) { /* lin */
        texres->tin = (1.0 + x) / 2.0;
    } else if (tex->stype == TEX_QUAD) { /* quad */
        texres->tin = (1.0 + x) / 2.0;
        if (texres->tin < 0.0) texres->tin = 0.0;
        else texres->tin *= texres->tin;
    } else if (tex->stype == TEX_EASE) { /* ease */
        texres->tin = (1.0 + x) / 2.0;
        if (texres->tin <= .0) texres->tin = 0.0;
        else if (texres->tin >= 1.0) texres->tin = 1.0;
        else {
            t = texres->tin * texres->tin;
            texres->tin = (3.0 * t - 2.0 * t * texres->tin);
        }
    } else if (tex->stype == TEX_DIAG) { /* diag */
        texres->tin = (2.0 + x + y) / 4.0;
    } else if (tex->stype == TEX_RAD) { /* radial */
        texres->tin = (atan2(y, x) / (2 * M_PI) + 0.5);
    } else { /* sphere TEX_SPHERE */
        texres->tin = 1.0 - sqrt(x * x + y * y + texvec[2] * texvec[2]);
        if (texres->tin < 0.0) texres->tin = 0.0;
        if (tex->stype == TEX_HALO) texres->tin *= texres->tin; /* halo */
    }

    BRICONT;

    return TEX_INT;
}

/* ------------------------------------------------------------------------- */
/* ************************************************************************* */

/* newnoise: all noisebased types now have different noisebases to choose from */

static int clouds(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;

    texres->tin = BLI_gTurbulence(tex->noisesize, texvec[0], texvec[1], texvec[2], tex->noisedepth, (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);

    if (tex->stype == TEX_COLOR) {
        // in this case, int. value should really be computed from color,
        // and bumpnormal from that, would be too slow, looks ok as is
        texres->tr = texres->tin;
        texres->tg = BLI_gTurbulence(tex->noisesize, texvec[1], texvec[0], texvec[2], tex->noisedepth, (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);
        texres->tb = BLI_gTurbulence(tex->noisesize, texvec[1], texvec[2], texvec[0], tex->noisedepth, (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);
        BRICONTRGB;
        texres->ta = 1.0;
        return (rv | TEX_RGB);
    }

    BRICONT;

    return rv;

}

/* creates a sine wave */
static float tex_sin(float a) {
    a = 0.5 + 0.5 * sin(a);

    return a;
}

/* creates a saw wave */
static float tex_saw(float a) {
    const float b = 2 * M_PI;

    int n = (int) (a / b);
    a -= n*b;
    if (a < 0) a += b;
    return a / b;
}

/* creates a triangle wave */
static float tex_tri(float a) {
    const float b = 2 * M_PI;
    const float rmax = 1.0;

    a = rmax - 2.0 * fabs(floor((a * (1.0 / b)) + 0.5) - (a * (1.0 / b)));

    return a;
}

/* computes basic wood intensity value at x,y,z */
static float wood_int(const Tex *tex, float x, float y, float z) {
    float wi = 0;
    short wf = tex->noisebasis2; /* wave form:	TEX_SIN=0,  TEX_SAW=1,  TEX_TRI=2						 */
    short wt = tex->stype; /* wood type:	TEX_BAND=0, TEX_RING=1, TEX_BANDNOISE=2, TEX_RINGNOISE=3 */

    float (*waveform[3])(float); /* create array of pointers to waveform functions */
    waveform[0] = tex_sin; /* assign address of tex_sin() function to pointer array */
    waveform[1] = tex_saw;
    waveform[2] = tex_tri;

    if ((wf > TEX_TRI) || (wf < TEX_SIN)) wf = 0; /* check to be sure noisebasis2 is initialized ahead of time */

    if (wt == TEX_BAND) {
        wi = waveform[wf]((x + y + z)*10.0);
    } else if (wt == TEX_RING) {
        wi = waveform[wf](sqrt(x * x + y * y + z * z)*20.0);
    } else if (wt == TEX_BANDNOISE) {
        wi = tex->turbul * BLI_gNoise(tex->noisesize, x, y, z, (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);
        wi = waveform[wf]((x + y + z)*10.0 + wi);
    } else if (wt == TEX_RINGNOISE) {
        wi = tex->turbul * BLI_gNoise(tex->noisesize, x, y, z, (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);
        wi = waveform[wf](sqrt(x * x + y * y + z * z)*20.0 + wi);
    }

    return wi;
}

static int wood(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;

    texres->tin = wood_int(tex, texvec[0], texvec[1], texvec[2]);

    BRICONT;

    return rv;
}
#include <stdio.h>
/* computes basic marble intensity at x,y,z */
static float marble_int(const Tex *tex, float x, float y, float z) {
    float n, mi;
    short wf = tex->noisebasis2; /* wave form:	TEX_SIN=0,  TEX_SAW=1,  TEX_TRI=2						*/
    short mt = tex->stype; /* marble type:	TEX_SOFT=0,	TEX_SHARP=1,TEX_SHAPER=2 					*/

    float (*waveform[3])(float); /* create array of pointers to waveform functions */
    waveform[0] = tex_sin; /* assign address of tex_sin() function to pointer array */
    waveform[1] = tex_saw;
    waveform[2] = tex_tri;

    if ((wf > TEX_TRI) || (wf < TEX_SIN)) wf = 0; /* check to be sure noisebasis2 isn't initialized ahead of time */

    n = 5.0 * (x + y + z);

    mi = n + tex->turbul * BLI_gTurbulence(tex->noisesize, x, y, z, tex->noisedepth, (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);

    if (mt >= TEX_SOFT) { /* TEX_SOFT always true */
        mi = waveform[wf](mi);
        if (mt == TEX_SHARP) {
            mi = sqrt(mi);
        }
        else if (mt == TEX_SHARPER) {
            mi = sqrt(sqrt(mi));
        }
    }

    return mi;
}

static int marble(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;

    texres->tin = marble_int(tex, texvec[0], texvec[1], texvec[2]);

    BRICONT;

    return rv;
}

/* ------------------------------------------------------------------------- */

static int magic(const Tex *tex, const float *texvec, TexResult *texres) {
    float x, y, z, turb = 1.0;
    int n;

    n = tex->noisedepth;
    turb = tex->turbul / 5.0;

    x = sin((texvec[0] + texvec[1] + texvec[2])*5.0);
    y = cos((-texvec[0] + texvec[1] - texvec[2])*5.0);
    z = -cos((-texvec[0] - texvec[1] + texvec[2])*5.0);
    if (n > 0) {
        x *= turb;
        y *= turb;
        z *= turb;
        y = -cos(x - y + z);
        y *= turb;
        if (n > 1) {
            x = cos(x - y - z);
            x *= turb;
            if (n > 2) {
                z = sin(-x - y - z);
                z *= turb;
                if (n > 3) {
                    x = -cos(-x + y - z);
                    x *= turb;
                    if (n > 4) {
                        y = -sin(-x + y + z);
                        y *= turb;
                        if (n > 5) {
                            y = -cos(-x + y + z);
                            y *= turb;
                            if (n > 6) {
                                x = cos(x + y + z);
                                x *= turb;
                                if (n > 7) {
                                    z = sin(x + y - z);
                                    z *= turb;
                                    if (n > 8) {
                                        x = -cos(-x - y + z);
                                        x *= turb;
                                        if (n > 9) {
                                            y = -sin(x - y + z);
                                            y *= turb;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (turb != 0.0) {
        turb *= 2.0;
        x /= turb;
        y /= turb;
        z /= turb;
    }
    texres->tr = 0.5 - x;
    texres->tg = 0.5 - y;
    texres->tb = 0.5 - z;

    texres->tin = 0.3333 * (texres->tr + texres->tg + texres->tb);

    BRICONTRGB;
    texres->ta = 1.0;

    return TEX_RGB;
}

/* ------------------------------------------------------------------------- */

/* newnoise: stucci also modified to use different noisebasis */
static int stucci(const Tex *tex, const float *texvec, TexResult *texres) {
    float nor[3], b2, ofs;
    int retval = TEX_INT;

    b2 = BLI_gNoise(tex->noisesize, texvec[0], texvec[1], texvec[2], (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);

    ofs = tex->turbul / 200.0;

    if (tex->stype) ofs *= (b2 * b2);
    nor[0] = BLI_gNoise(tex->noisesize, texvec[0] + ofs, texvec[1], texvec[2], (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);
    nor[1] = BLI_gNoise(tex->noisesize, texvec[0], texvec[1] + ofs, texvec[2], (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);
    nor[2] = BLI_gNoise(tex->noisesize, texvec[0], texvec[1], texvec[2] + ofs, (tex->noisetype != TEX_NOISESOFT), tex->noisebasis);

    texres->tin = nor[2];

    if (tex->stype == TEX_WALLOUT)
        texres->tin = 1.0f - texres->tin;

    if (texres->tin < 0.0f)
        texres->tin = 0.0f;

    return retval;
}

/* ------------------------------------------------------------------------- */

/* newnoise: musgrave terrain noise types */

static float mg_mFractalOrfBmTex(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;
    float (*mgravefunc)(float, float, float, float, float, float, int);

    if (tex->stype == TEX_MFRACTAL)
        mgravefunc = mg_MultiFractal;
    else
        mgravefunc = mg_fBm;

    texres->tin = tex->ns_outscale * mgravefunc(texvec[0], texvec[1], texvec[2], tex->mg_H, tex->mg_lacunarity, tex->mg_octaves, tex->noisebasis);

    BRICONT;

    return rv;
}

static float mg_ridgedOrHybridMFTex(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;
    float (*mgravefunc)(float, float, float, float, float, float, float, float, int);

    if (tex->stype == TEX_RIDGEDMF)
        mgravefunc = mg_RidgedMultiFractal;
    else
        mgravefunc = mg_HybridMultiFractal;

    texres->tin = tex->ns_outscale * mgravefunc(texvec[0], texvec[1], texvec[2], tex->mg_H, tex->mg_lacunarity, tex->mg_octaves, tex->mg_offset, tex->mg_gain, tex->noisebasis);

    BRICONT;

    return rv;
}

static float mg_HTerrainTex(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;

    texres->tin = tex->ns_outscale * mg_HeteroTerrain(texvec[0], texvec[1], texvec[2], tex->mg_H, tex->mg_lacunarity, tex->mg_octaves, tex->mg_offset, tex->noisebasis);

    BRICONT;

    return rv;

}

static float mg_distNoiseTex(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;

    texres->tin = mg_VLNoise(texvec[0], texvec[1], texvec[2], tex->dist_amount, tex->noisebasis, tex->noisebasis2);

    BRICONT;

    return rv;
}


/* ------------------------------------------------------------------------- */

/* newnoise: Voronoi texture type, probably the slowest, especially with minkovsky, bumpmapping, could be done another way */

static float voronoiTex(const Tex *tex, const float *texvec, TexResult *texres) {
    int rv = TEX_INT;
    float da[4], pa[12]; /* distance and point coordinate arrays of 4 nearest neighbours */
    float aw1 = fabs(tex->vn_w1);
    float aw2 = fabs(tex->vn_w2);
    float aw3 = fabs(tex->vn_w3);
    float aw4 = fabs(tex->vn_w4);
    float sc = (aw1 + aw2 + aw3 + aw4);
    if (sc != 0.f) sc = tex->ns_outscale / sc;

    voronoi(texvec[0], texvec[1], texvec[2], da, pa, tex->vn_mexp, tex->vn_distm);
    texres->tin = sc * fabs(tex->vn_w1 * da[0] + tex->vn_w2 * da[1] + tex->vn_w3 * da[2] + tex->vn_w4 * da[3]);

    if (tex->vn_coltype) {
        float ca[3]; /* cell color */
        cellNoiseV(pa[0], pa[1], pa[2], ca);
        texres->tr = aw1 * ca[0];
        texres->tg = aw1 * ca[1];
        texres->tb = aw1 * ca[2];
        cellNoiseV(pa[3], pa[4], pa[5], ca);
        texres->tr += aw2 * ca[0];
        texres->tg += aw2 * ca[1];
        texres->tb += aw2 * ca[2];
        cellNoiseV(pa[6], pa[7], pa[8], ca);
        texres->tr += aw3 * ca[0];
        texres->tg += aw3 * ca[1];
        texres->tb += aw3 * ca[2];
        cellNoiseV(pa[9], pa[10], pa[11], ca);
        texres->tr += aw4 * ca[0];
        texres->tg += aw4 * ca[1];
        texres->tb += aw4 * ca[2];
        if (tex->vn_coltype >= 2) {
            float t1 = (da[1] - da[0])*10;
            if (t1 > 1) t1 = 1;
            if (tex->vn_coltype == 3) t1 *= texres->tin;
            else t1 *= sc;
            texres->tr *= t1;
            texres->tg *= t1;
            texres->tb *= t1;
        } else {
            texres->tr *= sc;
            texres->tg *= sc;
            texres->tb *= sc;
        }
    }

    if (tex->vn_coltype) {
        BRICONTRGB;
        texres->ta = 1.0;
        return (rv | TEX_RGB);
    }

    BRICONT;

    return rv;

}

/* ------------------------------------------------------------------------- */

static int texnoise(const Tex *tex, TexResult *texres) {
    float div = 3.0;
    int val, ran, loop;

    ran = BLI_rand();
    val = (ran & 3);

    loop = tex->noisedepth;
    while (loop--) {
        ran = (ran >> 2);
        val *= (ran & 3);
        div *= 3.0;
    }

    texres->tin = ((float) val) / div;
    ;

    BRICONT;
    return TEX_INT;
}

/* ************************************** */

int multitex(const Tex *tex, const float *texvec, TexResult *texres) {
    float tmpvec[3];
    int retval = 0; /* return value, int:0, col:1, nor:2, everything:3 */

    texres->talpha = 0; /* is set when image texture returns alpha (considered premul) */

    switch (tex->type) {

        case 0:
            texres->tin = 0.0f;
            return 0;
        case TEX_CLOUDS:
            retval = clouds(tex, texvec, texres);
            break;
        case TEX_WOOD:
            retval = wood(tex, texvec, texres);
            break;
        case TEX_MARBLE:
            retval = marble(tex, texvec, texres);
            break;
        case TEX_MAGIC:
            retval = magic(tex, texvec, texres);
            break;
        case TEX_BLEND:
            retval = blend(tex, texvec, texres);
            break;
        case TEX_STUCCI:
            retval = stucci(tex, texvec, texres);
            break;
        case TEX_NOISE:
            retval = texnoise(tex, texres);
            break;
        case TEX_MUSGRAVE:
            /* newnoise: musgrave types */

            /* ton: added this, for Blender convention reason. 
             * artificer: added the use of tmpvec to avoid scaling texvec
             */
            VECCOPY(tmpvec, texvec);
            VecMulf(tmpvec, 1.0 / tex->noisesize);

            switch (tex->stype) {
                case TEX_MFRACTAL:
                case TEX_FBM:
                    retval = mg_mFractalOrfBmTex(tex, tmpvec, texres);
                    break;
                case TEX_RIDGEDMF:
                case TEX_HYBRIDMF:
                    retval = mg_ridgedOrHybridMFTex(tex, tmpvec, texres);
                    break;
                case TEX_HTERRAIN:
                    retval = mg_HTerrainTex(tex, tmpvec, texres);
                    break;
            }
            break;
            /* newnoise: voronoi type */
        case TEX_VORONOI:
            /* ton: added this, for Blender convention reason.
             * artificer: added the use of tmpvec to avoid scaling texvec
             */
            VECCOPY(tmpvec, texvec);
            VecMulf(tmpvec, 1.0 / tex->noisesize);

            retval = voronoiTex(tex, tmpvec, texres);
            break;
        case TEX_DISTNOISE:
            /* ton: added this, for Blender convention reason.
             * artificer: added the use of tmpvec to avoid scaling texvec
             */
            VECCOPY(tmpvec, texvec);
            VecMulf(tmpvec, 1.0 / tex->noisesize);

            retval = mg_distNoiseTex(tex, tmpvec, texres);
            break;
    }

    return retval;
}

} // namespace blender
