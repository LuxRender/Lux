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

#ifndef LUX_MATRIX4X4_H
#define LUX_MATRIX4X4_H

#include <xmmintrin.h>

namespace lux {
    static float _matrix44_sse_ident[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    class  Matrix4x4  {
    public:
        // Matrix4x4 Public Methods
        Matrix4x4() {
            // Dade - the following code leads to a coredump under Linux (and
            // it is slower of the memcpy anyway)
            /*__m128 zerone = _mm_set_ps(1.f, 0.f, 0.f, 1.f);//_V1001;
             * _mm_storel_pi((__m64 *)&_11, zerone);
             * _mm_storel_pi((__m64 *)&_13, _mm_setzero_ps());
             * _mm_storeh_pi((__m64 *)&_21, zerone);
             * _mm_storel_pi((__m64 *)&_23, _mm_setzero_ps());
             * _mm_storel_pi((__m64 *)&_31, _mm_setzero_ps());
             * _mm_storel_pi((__m64 *)&_33, zerone);
             * _mm_storel_pi((__m64 *)&_41, _mm_setzero_ps());
             * _mm_storeh_pi((__m64 *)&_43, zerone);*/
            
            memcpy(&_11, _matrix44_sse_ident, sizeof(_matrix44_sse_ident));
        }
        Matrix4x4(const Matrix4x4 & m):_L1(m._L1), _L2(m._L2), _L3(m._L3),
                _L4(m._L4)
        {}
        
        Matrix4x4(const __m128 &l1, const __m128 &l2, const __m128 &l3, const __m128 &l4):_L1(l1), _L2(l2), _L3(l3), _L4(l4)
        {}
        
        Matrix4x4(float mat[4][4]);
        Matrix4x4(float t00, float t01, float t02, float t03,
                float t10, float t11, float t12, float t13,
                float t20, float t21, float t22, float t23,
                float t30, float t31, float t32, float t33);
        boost::shared_ptr<Matrix4x4> Transpose() const;
        void Print(ostream &os) const {
            os<<"[ ";
            for(int i=0;i<4;i++) {
                os<<"[ ";
                for(int j=0;j<4;j++) {
                    os<<(*this)[j][i];
                    if(j!=3) os<<", ";
                }
                os<<" ] ";
            }
            os<<"]";
        }
        static boost::shared_ptr<Matrix4x4>
        Mul(const boost::shared_ptr<Matrix4x4> &A,
                const boost::shared_ptr<Matrix4x4> &B) {
            __m128 r1, r2, r3, r4;
            __m128 B1 = A->_L1, B2 = A->_L2, B3 = A->_L3, B4 = A->_L4;
            
            r1 = _mm_mul_ps(_mm_set_ps1(B->_11) , B1);
            r2 = _mm_mul_ps(_mm_set_ps1(B->_21) , B1);
            r1 = _mm_add_ps(r1, _mm_mul_ps(_mm_set_ps1(B->_12) , B2));
            r2 = _mm_add_ps(r2, _mm_mul_ps(_mm_set_ps1(B->_22) , B2));
            r1 = _mm_add_ps(r1, _mm_mul_ps(_mm_set_ps1(B->_13) , B3));
            r2 = _mm_add_ps(r2, _mm_mul_ps(_mm_set_ps1(B->_23) , B3));
            r1 = _mm_add_ps(r1, _mm_mul_ps(_mm_set_ps1(B->_14) , B4));
            r2 = _mm_add_ps(r2, _mm_mul_ps(_mm_set_ps1(B->_24) , B4));
            //res._L1 = r1;
            //res._L2 = r2;
            
            r3 = _mm_mul_ps(_mm_set_ps1(B->_31) , B1);
            r4 = _mm_mul_ps(_mm_set_ps1(B->_41) , B1);
            r3 = _mm_add_ps(r3, _mm_mul_ps(_mm_set_ps1(B->_32) , B2));
            r4 = _mm_add_ps(r4, _mm_mul_ps(_mm_set_ps1(B->_42) , B2));
            r3 = _mm_add_ps(r3, _mm_mul_ps(_mm_set_ps1(B->_33) , B3));
            r4 = _mm_add_ps(r4, _mm_mul_ps(_mm_set_ps1(B->_43) , B3));
            r3 = _mm_add_ps(r3, _mm_mul_ps(_mm_set_ps1(B->_34) , B4));
            r4 = _mm_add_ps(r4, _mm_mul_ps(_mm_set_ps1(B->_44) , B4));
            //res._L3 = r1;
            //res._L4 = r2;
            
            //return new Matrix4x4(r1,r2,r3,r4);
            
            boost::shared_ptr<Matrix4x4> o(new Matrix4x4(r1, r2, r3, r4));
            return o;
        }
        boost::shared_ptr<Matrix4x4> Inverse() const;
        //float m[4][4];
        
        union {
            struct {
                __m128 _L1, _L2, _L3, _L4;
            };
            struct {
                float _11, _12, _13, _14;
                float _21, _22, _23, _24;
                float _31, _32, _33, _34;
                float _41, _42, _43, _44;
            };
            struct {
                float _t11, _t21, _t31, _t41;
                float _t12, _t22, _t32, _t42;
                float _t13, _t23, _t33, _t43;
                float _t14, _t24, _t34, _t44;
            };
        };
        
        void* operator new(size_t t) { return _mm_malloc(t, 16); }
        void operator delete(void* ptr, size_t t) { _mm_free(ptr); }
        void* operator new[](size_t t) { return _mm_malloc(t, 16); }
        void operator delete[] (void* ptr) { _mm_free(ptr); }
        
    private:
        //Acessing elements
        float* operator [] (int i) const {
            //BOOST_ASSERT((0<=i) && (i<=3));
            return (float*)((&_L1)+i);
        }
    };
    
}//namespace lux

#endif //LUX_MATRIX4X4_H
