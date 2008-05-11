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

// 32 bin RGB spectra for smits RGB->Spectrum conversion using D65 whitepoint (for illuminants)
// generated with matlab by karlvb - added by radiance

#ifndef LUX_SUN_SPECT_H
#define LUX_SUN_SPECT_H

namespace lux
{

// -------------------------------------------------------------------------------------------------------------

/* All data lifted from MI */
/* Units are either [] or cm^-1. refer when in doubt MI */

// k_o Spectrum table from pg 127, MI.
static float sun_k_oWavelengths[64] = {
300, 305, 310, 315, 320,
325, 330, 335, 340, 345,
350, 355,

445, 450, 455, 460, 465,
470, 475, 480, 485, 490,
495,

500, 505, 510, 515, 520,
525, 530, 535, 540, 545,
550, 555, 560, 565, 570,
575, 580, 585, 590, 595,

600, 605, 610, 620, 630,
640, 650, 660, 670, 680,
690,

700, 710, 720, 730, 740,
750, 760, 770, 780, 790
};

static float sun_k_oAmplitudes[65] = {
  10.0,
  4.8,
  2.7,
  1.35,
  .8,
  .380,
  .160,
  .075,
  .04,
  .019,
  .007,
  .0,
  
  .003,
  .003,
  .004,
  .006,
  .008,
  .009,
  .012,
  .014,
  .017,
  .021,
  .025,

  .03,
  .035,
  .04,
  .045,
  .048,
  .057,
  .063,
  .07,
  .075,
  .08,
  .085,
  .095,
  .103,
  .110,
  .12,
  .122,
  .12,
  .118,
  .115,
  .12,

  .125,
  .130,
  .12,
  .105,
  .09,
  .079,
  .067,
  .057,
  .048,
  .036,
  .028,
  
  .023,
  .018,
  .014,
  .011,
  .010,
  .009,
  .007,
  .004,
  .0,
  .0
};

// k_g Spectrum table from pg 130, MI.
static float sun_k_gWavelengths[2] = {
  760,
  770,
};

static float sun_k_gAmplitudes[2] = {
  3.0,
  0.210,
};

// k_wa Spectrum table from pg 130, MI.
static float sun_k_waWavelengths[13] = {
  689,
  690,
  700,
  710,
  720,
  730,
  740,
  750,
  760,
  770,
  780,
  790,
  800
};

static float sun_k_waAmplitudes[13] = {
  0,
  0.160e-1,
  0.240e-1,
  0.125e-1,
  0.100e+1,
  0.870,
  0.610e-1,
  0.100e-2,
  0.100e-4,
  0.100e-4,
  0.600e-3,
  0.175e-1,
  0.360e-1
};

// 380-750 by 10nm
static float sun_solAmplitudes[38] = {
    165.5, 162.3, 211.2, 258.8, 258.2,
    242.3, 267.6, 296.6, 305.4, 300.6,
    306.6, 288.3, 287.1, 278.2, 271.0,
    272.3, 263.6, 255.0, 250.6, 253.1,
    253.5, 251.3, 246.3, 241.7, 236.8,
    232.1, 228.2, 223.4, 219.7, 215.3,
    211.0, 207.3, 202.4, 198.7, 194.3,
    190.7, 186.3, 182.6
};

// The suns irradiance as sampled outside the atmosphere from 380 - 770 nm at 5nm intervals
static float sun_sun_irradiance[79] = {
    1.1200E09,
    1.0980E09,
    1.0980E09,
    1.1890E09,
    1.4290E09,
    1.6440E09,
    1.7510E09,
    1.7740E09,
    1.7470E09,
    1.6930E09,
    1.6390E09,
    1.6630E09,
    1.8100E09,
    1.9220E09,
    2.0060E09,
    2.0570E09,
    2.0660E09,
    2.0480E09,
    2.0330E09,
    2.0440E09,
    2.0740E09,
    1.9760E09,
    1.9500E09,
    1.9600E09,
    1.9420E09,
    1.9200E09,
    1.8820E09,
    1.8330E09,
    1.8330E09,
    1.8520E09,
    1.8420E09,
    1.8180E09,
    1.7830E09,
    1.7540E09,
    1.7250E09,
    1.7200E09,
    1.6950E09,
    1.7050E09,
    1.7120E09,
    1.7190E09,
    1.7150E09,
    1.7120E09,
    1.7000E09,
    1.6820E09,
    1.6660E09,
    1.6470E09,
    1.6350E09,
    1.6150E09,
    1.6020E09,
    1.5900E09,
    1.5700E09,
    1.5550E09,
    1.5440E09,
    1.5270E09,
    1.5110E09,
    1.4985E09,
    1.4860E09,
    1.4710E09,
    1.4560E09,
    1.4415E09,
    1.4270E09,
    1.4145E09,
    1.4020E09,
    1.4855E09,
    1.3690E09,
    1.3565E09,
    1.3440E09,
    1.3290E09,
    1.3140E09,
    1.3020E09,
    1.2900E09,
    1.2750E09,
    1.2600E09,
    1.2475E09,
    1.2350E09,
    1.2230E09,
    1.2110E09,
    1.1980E09,
    1.1850E09
};

}//namespace lux

#endif // LUX_SUN_SPECT_H
