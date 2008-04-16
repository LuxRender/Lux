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

inline Point Transform::operator()(const Point &pt) const {
	const float x = pt.x, y = pt.y, z = pt.z;
	const float xp = m->m[0][0]*x + m->m[0][1]*y + m->m[0][2]*z + m->m[0][3];
	const float yp = m->m[1][0]*x + m->m[1][1]*y + m->m[1][2]*z + m->m[1][3];
	const float zp = m->m[2][0]*x + m->m[2][1]*y + m->m[2][2]*z + m->m[2][3];
	const float wp = m->m[3][0]*x + m->m[3][1]*y + m->m[3][2]*z + m->m[3][3];

//	BOOST_ASSERT(wp != 0);
	if (wp == 1.) return Point(xp, yp, zp);
	else          return Point(xp, yp, zp)/wp;
}

inline void Transform::operator()(const Point &pt, Point *ptrans) const {
	const float x = pt.x, y = pt.y, z = pt.z;
	ptrans->x = m->m[0][0]*x + m->m[0][1]*y + m->m[0][2]*z + m->m[0][3];
	ptrans->y = m->m[1][0]*x + m->m[1][1]*y + m->m[1][2]*z + m->m[1][3];
	ptrans->z = m->m[2][0]*x + m->m[2][1]*y + m->m[2][2]*z + m->m[2][3];
	const float w   = m->m[3][0]*x + m->m[3][1]*y + m->m[3][2]*z + m->m[3][3];
	if (w != 1.) *ptrans /= w;
}


inline Vector Transform::operator()(const Vector &v) const {
  const float x = v.x, y = v.y, z = v.z;
  return Vector(m->m[0][0]*x + m->m[0][1]*y + m->m[0][2]*z,
			    m->m[1][0]*x + m->m[1][1]*y + m->m[1][2]*z,
			    m->m[2][0]*x + m->m[2][1]*y + m->m[2][2]*z);
}

inline void Transform::operator()(const Vector &v,
		Vector *vt) const {
  const float x = v.x, y = v.y, z = v.z;
  vt->x = m->m[0][0] * x + m->m[0][1] * y + m->m[0][2] * z;
  vt->y = m->m[1][0] * x + m->m[1][1] * y + m->m[1][2] * z;
  vt->z = m->m[2][0] * x + m->m[2][1] * y + m->m[2][2] * z;
}

inline Normal Transform::operator()(const Normal &n) const {
	const float x = n.x, y = n.y, z = n.z;
	return Normal(mInv->m[0][0]*x + mInv->m[1][0]*y + mInv->m[2][0]*z,
                  mInv->m[0][1]*x + mInv->m[1][1]*y + mInv->m[2][1]*z,
                  mInv->m[0][2]*x + mInv->m[1][2]*y + mInv->m[2][2]*z);
}

inline void Transform::operator()(const Normal &n,
		Normal *nt) const {
	const float x = n.x, y = n.y, z = n.z;
	nt->x = mInv->m[0][0]*x + mInv->m[1][0]*y + mInv->m[2][0]*z;
	nt->y = mInv->m[0][1]*x + mInv->m[1][1]*y + mInv->m[2][1]*z;
	nt->z = mInv->m[0][2]*x + mInv->m[1][2]*y + mInv->m[2][2]*z;
}

inline bool Transform::SwapsHandedness() const {
	const float det = ((m->m[0][0] *
                  (m->m[1][1] * m->m[2][2] -
                   m->m[1][2] * m->m[2][1])) -
                 (m->m[0][1] *
                  (m->m[1][0] * m->m[2][2] -
                   m->m[1][2] * m->m[2][0])) +
                 (m->m[0][2] *
                  (m->m[1][0] * m->m[2][1] -
                   m->m[1][1] * m->m[2][0])));
	return det < 0.f;
}


