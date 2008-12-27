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

#ifndef LUX_MOTIONSYSTEM_H
#define LUX_MOTIONSYSTEM_H

// motionsystem.h*
#include "lux.h"
#include "geometry.h"
#include "primitive.h"
#include "error.h"
// MotionSystem Declarations

namespace lux
{

class Transforms {
public:
	// Scaling
	float Sx, Sy, Sz;
	// Shearing
	float Sxy, Sxz, Syz;
	// Rotation
	float Rx, Ry, Rz;
	// Translation
	float Tx, Ty, Tz;
	// Perspective
	float Px, Py, Pz, Pw;
};

class  MotionSystem {
public:
	MotionSystem(float st, float et,
		const Transform &s, const Transform &e) {
		
		startTime = st;
		endTime = et;
		start = s;
		end = e;
		startMat = start.GetMatrix();
		endMat = end.GetMatrix();

		if (!DecomposeMatrix(startMat, startT)) {
			luxError(LUX_MATH, LUX_WARNING, "Singular start matrix in MotionSystem, interpolation disabled");
			return;
		}
		if (!DecomposeMatrix(endMat, endT)) {
			luxError(LUX_MATH, LUX_WARNING, "Singular end matrix in MotionSystem, interpolation disabled");
			return;
		}

		Transform rot;
		
		rot = RotateX(Degrees(startT.Rx)) * 
			RotateY(Degrees(startT.Ry)) * 
			RotateZ(Degrees(startT.Rz));

		startQ = Quaternion(rot.GetMatrix());
		startQ.Normalize();

		rot = RotateX(Degrees(endT.Rx)) * 
			RotateY(Degrees(endT.Ry)) * 
			RotateZ(Degrees(endT.Rz));

		endQ = Quaternion(rot.GetMatrix());
		endQ.Normalize();	

		hasTranslationX = startT.Tx != endT.Tx;
		hasTranslationY = startT.Ty != endT.Ty;
		hasTranslationZ = startT.Tz != endT.Tz;

		hasScaleX = startT.Sx != endT.Sx;
		hasScaleY = startT.Sy != endT.Sy;
		hasScaleZ = startT.Sz != endT.Sz;

		hasRotation = (startT.Rx != endT.Rx) ||
			(startT.Ry != endT.Ry) ||
			(startT.Rz != endT.Rz);
	}
	~MotionSystem() {};

	Transform Sample(float time) {
		// Determine interpolation value
		if(time <= startTime)
			return start;
		if(time >= endTime)
			return end;

		float w = endTime - startTime;
		float d = time - startTime;
		float le = d / w; 		

		// Initialize Identity interMatrix
		float interMatrix[4][4];
		for (int i = 0; i < 4; i++)
			interMatrix[i][0] = interMatrix[i][1] = 
				interMatrix[i][2] = interMatrix[i][3] = 0;
		interMatrix[0][0] = interMatrix[1][1] = 
			interMatrix[2][2] = interMatrix[3][3] = 1;

		if(hasRotation) {
			// Quaternion interpolation of rotation / also does scale
			Quaternion between_quat = slerp(le, startQ, endQ);
			toMatrix(between_quat, interMatrix);
		};

		// Interpolate Scale linearly
		if (hasScaleX) {
			float Sx = Lerp(le, startT.Sx, endT.Sx);
			for (int j = 0; j < 4; j++) 
				interMatrix[0][j] *= Sx;
		}
		if (hasScaleY) {
			float Sy = Lerp(le, startT.Sy, endT.Sy);
			for (int j = 0; j < 4; j++) 
				interMatrix[1][j] *= Sy;
		}
		if (hasScaleZ) {
			float Sz = Lerp(le, startT.Sz, endT.Sz);
			for (int j = 0; j < 4; j++) 
				interMatrix[2][j] *= Sz;
		}

		// Interpolate Translation linearly
		if (hasTranslationX) interMatrix[0][3] = Lerp(le, startT.Tx, endT.Tx);
		if (hasTranslationY) interMatrix[1][3] = Lerp(le, startT.Ty, endT.Ty);
		if (hasTranslationZ) interMatrix[2][3] = Lerp(le, startT.Tz, endT.Tz);

		return Transform(interMatrix);
	}

	BBox Bound(BBox ibox) {
      		// Compute total bounding box by naive unions.
		// NOTE - radiance - this needs some work.
       	BBox tbox;
		const float s = 1.f/1024;
       	for(u_int i=0; i<1024; i++) {
              		Transform t = Sample(s*i);
             		tbox = Union(tbox, t(ibox));
       	}
       	return tbox;
	}

protected:

	// decomposes the matrix m into a series of transformations
	// [Sx][Sy][Sz][Shearx/y][Sx/z][Sz/y][Rx][Ry][Rz][Tx][Ty][Tz][P(x,y,z,w)]
	// based on unmatrix() by Spencer W. Thomas from Graphic Gems II
	// TODO - lordcrc - implement extraction of perspective transform
	bool DecomposeMatrix(const boost::shared_ptr<Matrix4x4> m, Transforms &trans) const {

		boost::shared_ptr<Matrix4x4> locmat(new Matrix4x4(m->m));

		/* Normalize the matrix. */
		if (locmat->m[3][3] == 0)
			return false;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				locmat->m[i][j] /= locmat->m[3][3];
		/* pmat is used to solve for perspective, but it also provides
		 * an easy way to test for singularity of the upper 3x3 component.
		 */
		boost::shared_ptr<Matrix4x4> pmat(new Matrix4x4(locmat->m));
		for (int i = 0; i < 3; i++)
			pmat->m[i][3] = 0;
		pmat->m[3][3] = 1;

		if ( pmat->Determinant() == 0.0 )
			return false;

		/* First, isolate perspective.  This is the messiest. */
		if ( locmat->m[3][0] != 0 || locmat->m[3][1] != 0 ||
			locmat->m[3][2] != 0 ) {
			/* prhs is the right hand side of the equation. */
			//float prhs[4];
			//prhs[0] = locmat->m[3][0];
			//prhs[1] = locmat->m[3][1];
			//prhs[2] = locmat->m[3][2];
			//prhs[3] = locmat->m[3][3];

			/* Solve the equation by inverting pmat and multiplying
			 * prhs by the inverse.  (This is the easiest way, not
			 * necessarily the best.)
			 * inverse function (and det4x4, above) from the Matrix
			 * Inversion gem in the first volume.
			 */
			//boost::shared_ptr<Matrix4x4> tinvpmat = pmat->Inverse()->Transpose();
			//V4MulPointByMatrix(&prhs, &tinvpmat, &psol);
	 
			/* Stuff the answer away. */
			//tran[U_PERSPX] = psol.x;
			//tran[U_PERSPY] = psol.y;
			//tran[U_PERSPZ] = psol.z;
			//tran[U_PERSPW] = psol.w;
			trans.Px = trans.Py = trans.Pz = trans.Pw = 0;
			/* Clear the perspective partition. */
			locmat->m[0][3] = locmat->m[1][3] =
				locmat->m[2][3] = 0;
			locmat->m[3][3] = 1;
		};
	//	else		/* No perspective. */
	//		tran[U_PERSPX] = tran[U_PERSPY] = tran[U_PERSPZ] =
	//			tran[U_PERSPW] = 0;

		/* Next take care of translation (easy). */
		trans.Tx = locmat->m[0][3];
		trans.Ty = locmat->m[1][3];
		trans.Tz = locmat->m[2][3];
		for (int i = 0; i < 3; i++ )
			locmat->m[i][3] = 0;
		
		Vector row[3];
		/* Now get scale and shear. */
		for (int i = 0; i < 3; i++ ) {
			row[i].x = locmat->m[i][0];
			row[i].y = locmat->m[i][1];
			row[i].z = locmat->m[i][2];
		}

		/* Compute X scale factor and normalize first row. */
		trans.Sx = row[0].Length();
		row[0] *= 1 / trans.Sx;

		/* Compute XY shear factor and make 2nd row orthogonal to 1st. */
		trans.Sxy = Dot(row[0], row[1]);
		row[1] -= trans.Sxy * row[0];

		/* Now, compute Y scale and normalize 2nd row. */
		trans.Sy = row[1].Length();
		row[1] *= 1.0 / trans.Sy;
		trans.Sxy /= trans.Sy;

		/* Compute XZ and YZ shears, orthogonalize 3rd row. */
		trans.Sxz = Dot(row[0], row[2]);
		row[2] -= trans.Sxz * row[0];
		trans.Syz = Dot(row[1], row[2]);
		row[2] -= trans.Syz * row[1];

		/* Next, get Z scale and normalize 3rd row. */
		trans.Sz = row[2].Length();
		row[2] *= 1.0 / trans.Sz;
		trans.Sxz /= trans.Sz;
		trans.Syz /= trans.Sz;

		/* At this point, the matrix (in rows[]) is orthonormal.
		 * Check for a coordinate system flip.  If the determinant
		 * is -1, then negate the matrix and the scaling factors.
		 */
		if (Dot(row[0], Cross(row[1], row[2])) < 0) {
			trans.Sx *= -1;
			trans.Sy *= -1;
			trans.Sz *= -1;
			for (int i = 0; i < 3; i++ ) {
				row[i].x *= -1;
				row[i].y *= -1;
				row[i].z *= -1;
			}
		}

		/* Now, get the rotations out, as described in the gem. */
		trans.Ry = -asinf(-row[0].z);
		if ( cosf(trans.Ry) != 0 ) {
			trans.Rx = -atan2f(row[1].z, row[2].z);
			trans.Rz = -atan2f(row[0].y, row[0].x);
		} else {
			trans.Rx = -atan2f(-row[2].x, row[1].y);
			trans.Rz = 0;
		}
		/* All done! */
		return true;
	}


	// MotionSystem Protected Data
	float startTime, endTime;
	Transform start, end;
	Transforms startT, endT;
	boost::shared_ptr<Matrix4x4> startMat, endMat;
	Quaternion startQ, endQ;
	bool hasRotation;
	bool hasTranslationX, hasTranslationY, hasTranslationZ;
	bool hasScaleX, hasScaleY, hasScaleZ;
};

}//namespace lux

#endif // LUX_MOTIONSYSTEM_H
