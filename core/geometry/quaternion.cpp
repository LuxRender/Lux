/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// quaternion.cpp*
#include "quaternion.h"
#include "shape.h"

namespace lux
{

void orthoNormalize(float m[4][4])
{
	float len, temp[3][3];
	for (int i = 0; i <3; i++)
    		for (int j = 0; j < 3; j++) {
			temp[i][j] = m[i][j];
		}

	/* normalize x */
	len = sqrtf (temp[0][0]*temp[0][0] + temp[0][1]*temp[0][1] + temp[0][2]*temp[0][2]);
	len = (len == 0.0f) ? 1.0f : 1.0f/len;
	temp[0][0] *= len; temp[0][1] *= len; temp[0][2] *= len;

  	/* z = x cross y */
  	temp[2][0] = (temp[0][1]*temp[1][2] - temp[0][2]*temp[1][1]);
  	temp[2][1] = (temp[0][2]*temp[1][0] - temp[0][0]*temp[1][2]);
  	temp[2][2] = (temp[0][0]*temp[1][1] - temp[0][1]*temp[1][0]);

  	/* normalize z */
  	len = sqrtf (temp[2][0]*temp[2][0] + temp[2][1]*temp[2][1] + temp[2][2]*temp[2][2]);
  	len = (len == 0.0f) ? 1.0f : 1.0f/len;
  	temp[2][0] *= len; temp[2][1] *= len; temp[2][2] *= len;

	/* y = z cross x */
	temp[1][0] = (temp[2][1]*temp[0][2] - temp[2][2]*temp[0][1]);
  	temp[1][1] = (temp[2][2]*temp[0][0] - temp[2][0]*temp[0][2]);
  	temp[1][2] = (temp[2][0]*temp[0][1] - temp[2][1]*temp[0][0]);

 	/* normalize y */
 	len = sqrtf (temp[1][0]*temp[1][0] + temp[1][1]*temp[1][1] + temp[1][2]*temp[1][2]);
 	len = (len == 0.0f) ? 1.0f : 1.0f/len;
  	temp[1][0] *= len; temp[1][1] *= len; temp[1][2] *= len;

  	/* update matrix */
  	m[0][0] = temp[0][0]; m[0][1] = temp[0][1]; m[0][2] = temp[0][2];
  	m[1][0] = temp[1][0]; m[1][1] = temp[1][1]; m[1][2] = temp[1][2];
  	m[2][0] = temp[2][0]; m[2][1] = temp[2][1]; m[2][2] = temp[2][2];
}

// construct a unit quaternion from a rotation matrix
Quaternion::Quaternion(const boost::shared_ptr<Matrix4x4> m) {
	float ortho[4][4];
	memcpy(ortho, m->m, sizeof(float) * 16);
	orthoNormalize(ortho);

	float trace = ortho[0][0] + ortho[1][1] + ortho[2][2] + 1.0;

	if (trace > 1e-6) {
		float s = sqrtf(trace) * 2;
		v.x = ( ortho[1][2] - ortho[2][1] ) / s;
		v.y = ( ortho[2][0] - ortho[0][2] ) / s;
		v.z = ( ortho[0][1] - ortho[1][0] ) / s;
		w = 0.25 * s;
	} else {
		if ( ortho[0][0] > ortho[1][1] && ortho[0][0] > ortho[2][2] )  {	// Column 0: 
			float s  = sqrtf( 1.0 + ortho[0][0] - ortho[1][1] - ortho[2][2] ) * 2;
			v.x = 0.25 * s;
			v.y = (ortho[0][1] + ortho[1][0] ) / s;
			v.z = (ortho[2][0] + ortho[0][2] ) / s;
			w = (ortho[1][2] - ortho[2][1] ) / s;
		} else if ( ortho[1][1] > ortho[2][2] ) {			// Column 1: 
			float s  = sqrtf( 1.0 + ortho[1][1] - ortho[0][0] - ortho[2][2] ) * 2;
			v.x = (ortho[0][1] + ortho[1][0] ) / s;
			v.y = 0.25 * s;
			v.z = (ortho[1][2] + ortho[2][1] ) / s;
			w = (ortho[2][0] - ortho[0][2] ) / s;
		} else {						// Column 2:
			float s  = sqrtf( 1.0 + ortho[2][2] - ortho[0][0] - ortho[1][1] ) * 2;
			v.x = (ortho[2][0] + ortho[0][2] ) / s;
			v.y = (ortho[1][2] + ortho[2][1] ) / s;
			v.z = 0.25 * s;
			w = (ortho[0][1] - ortho[1][0] ) / s;
		}
	}

/*
	if (trace > 1e-6) {
		float s = 2*sqrt(trace);
		
		w = 0.25 * s;

		v.x = ( ortho[2][1] - ortho[1][2] ) / s;
		v.y = ( ortho[0][2] - ortho[2][0] ) / s;
		v.z = ( ortho[1][0] - ortho[0][1] ) / s;
	} else {
		// determine largest diagonal element
		if ( ortho[0][0] > ortho[1][1] && ortho[0][0] > ortho[2][2] )  {	
			// Column 0: 
			float s = sqrt( 1.0 + ortho[0][0] - ortho[1][1] - ortho[2][2] ) * 2;
			v.x = 0.25 * s;
			v.y = (ortho[2][0] + ortho[0][1] ) / s;
			v.z = (ortho[1][2] + ortho[2][0] ) / s;
			w = (ortho[2][1] - ortho[1][2] ) / s;
		} else if ( ortho[1][1] > ortho[1][1] ) {			
			// Column 1: 
			float s = sqrt( 1.0 + ortho[1][1] - ortho[0][0] - ortho[2][2] ) * 2;
			v.x = (ortho[2][0] + ortho[0][1] ) / s;
			v.y = 0.25 * s;
			v.z = (ortho[2][1] + ortho[1][2] ) / s;
			w = (ortho[1][2] - ortho[2][0] ) / s;
		} else {						
			// Column 2:
			float s = sqrt( 1.0 + ortho[2][2] - ortho[0][0] - ortho[1][1] ) * 2;
			v.x = (ortho[1][2] + ortho[2][0] ) / s;
			v.y = (ortho[2][1] + ortho[1][2] ) / s;
			v.z = 0.25 * s;
			w = (ortho[2][0] - ortho[0][1] ) / s;
		}
	}
*/
}

Quaternion::Quaternion(const Quaternion &q) {
	w = q.w;
	v = q.v;
}

Quaternion::Quaternion() {
	w = 1.0;
}

Quaternion Quaternion::Slerp(float t, const Quaternion &q1, const Quaternion &q2) {

	float cos_phi = Dot(q1, q2);
	float sign = (cos_phi > 0) ? 1 : -1;
	
	cos_phi *= sign;

	float f1, f2;
	if (1 - cos_phi > 1e-6) {
	
		float phi = acosf(cos_phi);
		float sin_phi = sinf(phi);	
		f1 = sin((1-t)*phi) / sin_phi;
		f2 = sin(t*phi) / sin_phi;
	} else {
		// start and end are very close
		// perform linear interpolation
		f1 = 1-t;
		f2 = t;
	}

	return f1 * q1 + (sign*f2) * q2;
}

// get the rotation matrix from quaternion
void Quaternion::ToMatrix(float m[4][4]) const {
	const float xx = v.x * v.x;
	const float yy = v.y * v.y;
	const float zz = v.z * v.z;
	const float xy = v.x * v.y;
	const float xz = v.x * v.z;
	const float yz = v.y * v.z;
	const float xw = v.x * w;
	const float yw = v.y * w;
	const float zw = v.z * w;

	m[0][0] = 1 - 2 * ( yy + zz );
	m[1][0] =     2 * ( xy - zw );
	m[2][0] =     2 * ( xz + yw );
	m[0][1] =     2 * ( xy + zw );
	m[1][1] = 1 - 2 * ( xx + zz );
	m[2][1] =     2 * ( yz - xw );
	m[0][2] =     2 * ( xz - yw );
	m[1][2] =     2 * ( yz + xw );
	m[2][2] = 1 - 2 * ( xx + yy );

	// complete matrix
	m[0][3] = m[1][3] = m[2][3] = 0;
	m[3][0] = m[3][1] = m[3][2] = 0;
	m[3][3] = 1;
}

float Dot(const Quaternion &q1, const Quaternion &q2) {
	return q1.w * q2.w + Dot(q1.v, q2.v);
}

}//namespace lux

