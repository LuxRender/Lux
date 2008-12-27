
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

#ifndef LUX_MATRIX4X4_H
#define LUX_MATRIX4X4_H

namespace lux
{

class  Matrix4x4 {
public:
	// Matrix4x4 Public Methods
	Matrix4x4() {
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				if (i == j) m[i][j] = 1.;
				else m[i][j] = 0.;
	}
	Matrix4x4(float mat[4][4]);
	Matrix4x4(float t00, float t01, float t02, float t03,
	          float t10, float t11, float t12, float t13,
	          float t20, float t21, float t22, float t23,
	          float t30, float t31, float t32, float t33);
	boost::shared_ptr<Matrix4x4> Transpose() const;
	float Determinant() const;
	void Print(ostream &os) const {
		os << "[ ";
		for (int i = 0; i < 4; ++i) {
			os << "[ ";
			for (int j = 0; j < 4; ++j)  {
				os << m[i][j];
				if (j != 3) os << ", ";
			}
			os << " ] ";
		}
		os << " ] ";
	}
	static boost::shared_ptr<Matrix4x4>
		Mul(const boost::shared_ptr<Matrix4x4> &m1,
	        const boost::shared_ptr<Matrix4x4> &m2) {
		float r[4][4];
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				r[i][j] = m1->m[i][0] * m2->m[0][j] +
				          m1->m[i][1] * m2->m[1][j] +
				          m1->m[i][2] * m2->m[2][j] +
				          m1->m[i][3] * m2->m[3][j];
		boost::shared_ptr<Matrix4x4> o (new Matrix4x4(r));
		return o;
	}
	boost::shared_ptr<Matrix4x4> Inverse() const;
	float m[4][4];
};

}//namespace lux

#endif //LUX_MATRIX4X4_H
