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

// cie.cpp*
#include "cie.h"

using namespace lux;

const float DIlluminant::S0[nFS] = {
  0.04, 6.00, 29.60, 55.30, 57.30,
  61.80, 61.50, 68.80, 63.40, 65.80,
  94.80, 104.80, 105.90, 96.80, 113.90, 
  125.60, 125.50, 121.30, 121.30, 113.50, 
  113.10, 110.80, 106.50, 108.80, 105.30, 
  104.40, 100.00, 96.00, 95.10, 89.10, 
  90.50, 90.30, 88.40, 84.00, 85.10, 
  81.90, 82.60, 84.90, 81.30, 71.90, 
  74.30, 76.40, 63.30, 71.70, 77.00, 
  65.20, 47.70, 68.60, 65.00, 66.00, 
  61.00, 53.30, 58.90, 61.90 
};

const float DIlluminant::S1[nFS] = {
  0.02, 4.50, 22.40, 42.00, 40.60, 
  41.60, 38.00, 43.40, 38.50, 35.00, 
  43.40, 46.30, 43.90, 37.10, 36.70, 
  35.90, 32.60, 27.90, 24.30, 20.10, 
  16.20, 13.20, 8.60, 6.10, 4.20, 
  1.90, 0.00, -1.60, -3.50, -3.50, 
  -5.80, -7.20, -8.60, -9.50, -10.90, 
  -10.70, -12.00, -14.00, -13.60, -12.00, 
  -13.30, -12.90, -10.60, -11.60, -12.20, 
  -10.20, -7.80, -11.20, -10.40, -10.60, 
  -9.70, -8.30, -9.30, -9.80
};

const float DIlluminant::S2[nFS] = {
  0.0, 2.0, 4.0, 8.5, 7.8, 
  6.7, 5.3, 6.1, 3.0, 1.2, 
  -1.1, -0.5, -0.7, -1.2, -2.6, 
  -2.9, -2.8, -2.6, -2.6, -1.8, 
  -1.5, -1.3, -1.2, -1.0, -0.5, 
  -0.3, 0.0, 0.2, 0.5, 2.1, 
  3.2, 4.1, 4.7, 5.1, 6.7, 
  7.3, 8.6, 9.8, 10.2, 8.3, 
  9.6, 8.5, 7.0, 7.6, 8.0, 
  6.7, 5.2, 7.4, 6.8, 7.0, 
  6.4, 5.5, 6.1, 6.5
};