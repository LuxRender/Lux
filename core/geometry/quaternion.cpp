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

	float s = 2*sqrt(ortho[0][0] + ortho[1][1] + ortho[2][2] + 1.0);
	
	w = 0.25 * s;

	v.x = ( ortho[2][1] - ortho[1][2] ) / s;
	v.y = ( ortho[0][2] - ortho[2][0] ) / s;
	v.z = ( ortho[1][0] - ortho[0][1] ) / s;

}

Quaternion::Quaternion(const Quaternion &q) {
	w = q.w;
	v = q.v;
}

Quaternion::Quaternion() {
	w = 1.0;
}

float dot(const Quaternion &q1, const Quaternion &q2) {
        return q1.w *q2.w + Dot(q1.v, q2.v);
}

Quaternion slerp(float t, const Quaternion &q1, const Quaternion &q2) {

        float cos_o = dot(q1, q2);
        float o = acos(cos_o);

        return (sin( (1-t)*o )/sin(o))*q1 + (sin( t*o )/sin(o))*q2;
}

// get the rotation matrix from quaternion
void toMatrix(const Quaternion &q, float m[4][4]) {
  float xx = q.v.x * q.v.x;
  float yy = q.v.y * q.v.y;
  float zz = q.v.z * q.v.z;
  float xy = q.v.x * q.v.y;
  float xz = q.v.x * q.v.z;
  float yz = q.v.y * q.v.z;
  float wx = q.w * q.v.x;
  float wy = q.w * q.v.y;
  float wz = q.w * q.v.z;
  m[0][0] = 1 - 2*(yy + zz); m[0][1] =  2.0 * (xy - wz); m[0][2] = 2.0 * (xz + wy);
  m[1][0] = 2.0 * (xy + wz); m[1][1] =  1 - 2*(xx + zz); m[1][2] = 2.0 * (yz - wx);
  m[2][0] = 2.0 * (wy - xz); m[2][1] = -2.0 * (yz + wx); m[2][2] = 2.0*(xx + yy)-1;

  // flip signs on third row 
  m[2][0] = -m[2][0];
  m[2][1] = -m[2][1];
  m[2][2] = -m[2][2];

  // complete matrix
  m[0][3] = m[1][3] = m[2][3] = 0;
  m[3][0] =  m[3][1] = m[3][2] = 0;
  m[3][3] = 1;
}


}//namespace lux

