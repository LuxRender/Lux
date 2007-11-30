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

// Transform Inline Functions
inline Point Transform::operator()(const Point &pt) const {
	__m128 ptvec=_mm_set_ps(0,pt.z,pt.y,pt.x);
	
	union{
		__m128 vec;
		float f[4];
	};

      vec = m->_L4;
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x00) , m->_L1));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x55) , m->_L2));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0xAA) , m->_L3));

	BOOST_ASSERT(f[3] != 0);
      //BOOST_ASSERT(result.w != 0);
      
      //std::cout<<"Pt:"<<f[0]<<','<< f[1] <<','<< f[2]<< ',' << f[3] << std::endl;
      
      if (f[3] == 1.) return Point(f[0], f[1], f[2]);
	else          return Point(f[0], f[1], f[2])/f[3];
}


inline void Transform::operator()(const Point &pt, Point *ptrans) const {
	__m128 ptvec=_mm_set_ps(0,pt.z,pt.y,pt.x);
	
	union{
		__m128 vec;
		float f[4];
	};

      vec = m->_L4;
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x00) , m->_L1));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x55) , m->_L2));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0xAA) , m->_L3));

	BOOST_ASSERT(f[3] != 0);
    ptrans->x=f[0];
    ptrans->y=f[1];
    ptrans->z=f[2];
    if (f[3] != 1.) *ptrans /= f[3];
}

inline Vector Transform::operator()(const Vector &v) const {    
	__m128 vec=_mm_set_ps(0,v.z,v.y,v.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    return Vector(f[0], f[1], f[2]);
}

inline void Transform::operator()(const Vector &v, Vector *vt) const
{
    __m128 vec=_mm_set_ps(0,v.z,v.y,v.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    vt->x=f[0];
    vt->y=f[1];
    vt->z=f[2];
}

inline Normal Transform::operator()(const Normal &n) const {
	__m128 vec=_mm_set_ps(0,n.z,n.y,n.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    return Normal(f[0], f[1], f[2]);
}

inline void Transform::operator()(const Normal &n,
		Normal *nt) const {
	__m128 vec=_mm_set_ps(0,n.z,n.y,n.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    nt->x=f[0];
    nt->y=f[1];
    nt->z=f[2];
}

inline bool Transform::SwapsHandedness() const {
	float det = ((m->_11 *
                  (m->_22 * m->_33 -
                   m->_23 * m->_32)) -
                 (m->_12 *
                  (m->_21 * m->_33 -
                   m->_23 * m->_31)) +
                 (m->_13 *
                  (m->_21 * m->_32 -
                   m->_22 * m->_31)));
	//std::cout<<"Det:"<<det<<std::endl;
	return det < 0.f;
}

