/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/  

// 32 bin RGB spectra for smits RGB->Spectrum conversion using D65 whitepoint (for illuminants)
// generated with matlab by karlvb - added by radiance

#ifndef LUX_SKYCHROMA_SPECT_H
#define LUX_SKYCHROMA_SPECT_H

namespace lux
{

//300-830 10nm
static float S0Amplitudes[54] = {
0.04,6.0,29.6,55.3,57.3,
61.8,61.5,68.8,63.4,65.8,
94.8,104.8,105.9,96.8,113.9,
125.6,125.5,121.3,121.3,113.5,
113.1,110.8,106.5,108.8,105.3,
104.4,100.0,96.0,95.1,89.1,
90.5,90.3,88.4,84.0,85.1,
81.9,82.6,84.9,81.3,71.9,
74.3,76.4,63.3,71.7,77.0,
65.2,47.7,68.6,65.0,66.0,
61.0,53.3,58.9,61.9
};

static float S1Amplitudes[54] = {
0.02,4.5,22.4,42.0,40.6,
41.6,38.0,42.4,38.5,35.0,
43.4,46.3,43.9,37.1,36.7,
35.9,32.6,27.9,24.3,20.1,
16.2,13.2,8.6,6.1,4.2,
1.9,0.0,-1.6,-3.5,-3.5,
-5.8,-7.2,-8.6,-9.5,-10.9,
-10.7,-12.0,-14.0,-13.6,-12.0,
-13.3,-12.9,-10.6,-11.6,-12.2,
-10.2,-7.8,-11.2,-10.4,-10.6,
-9.7,-8.3,-9.3,-9.8
};

static float S2Amplitudes[54] = {
0.0,2.0,4.0,8.5,7.8,
6.7,5.3,6.1,3.0,1.2,
-1.1,-0.5,-0.7,-1.2,-2.6,
-2.9,-2.8,-2.6,-2.6,-1.8,
-1.5,-1.3,-1.2,-1.0,-0.5,
-0.3,0.0,0.2,0.5,2.1,
3.2,4.1,4.7,5.1,6.7,
7.3,8.6,9.8,10.2,8.3,
9.6,8.5,7.0,7.6,8.0,
6.7,5.2,7.4,6.8,7.0,
6.4,5.5,6.1,6.5
};

}//namespace lux

#endif // LUX_SKYCHROMA_SPECT_H
