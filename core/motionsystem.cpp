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

// motionsystem.cpp*
#include "motionsystem.h"
#include "error.h"

#include <cstring>
using std::memset;

using namespace lux;

MotionSystem::MotionSystem(float st, float et,
		const Transform &s, const Transform &e) {
	
	startTime = st;
	endTime = et;
	start = s;
	end = e;
	startMat = start.GetMatrix();
	endMat = end.GetMatrix();

	// initialize to false
	hasTranslationX = hasTranslationY = hasTranslationZ =
		hasScaleX = hasScaleY = hasScaleZ = hasRotation = isActive = false;

	if (!DecomposeMatrix(startMat, &startT)) {
		LOG( LUX_WARNING,LUX_MATH)<< "Singular start matrix in MotionSystem, interpolation disabled";
		return;
	}
	if (!DecomposeMatrix(endMat, &endT)) {
		LOG( LUX_WARNING,LUX_MATH)<< "Singular end matrix in MotionSystem, interpolation disabled";
		return;
	}

	startQ = Quaternion(startT.R);
	startQ.Normalize();

	endQ = Quaternion(endT.R);
	endQ.Normalize();

	hasTranslationX = startT.Tx != endT.Tx;
	hasTranslationY = startT.Ty != endT.Ty;
	hasTranslationZ = startT.Tz != endT.Tz;
	hasTranslation = hasTranslationX || 
		hasTranslationY || hasTranslationZ;

	hasScaleX = startT.Sx != endT.Sx;
	hasScaleY = startT.Sy != endT.Sy;
	hasScaleZ = startT.Sz != endT.Sz;
	hasScale = hasScaleX || hasScaleY || hasScaleZ;

	hasRotation = fabsf(Dot(startQ, endQ) - 1.f) >= 1e-6f;

	isActive = hasTranslation ||
		hasScale || hasRotation;
}

 BBox MotionSystem::Bound(BBox ibox) const {
	// Compute total bounding box by naive unions.
	BBox tbox;
	const float N = 1024;
	for (float i = 0; i <= N; i++) {
		const float t = Lerp(static_cast<float>(i) / N, startTime, endTime);
		Transform subT = Sample(t);
		tbox = Union(tbox, subT(ibox));
	}
	return tbox;
}

Transform MotionSystem::Sample(float time) const {
	if (!isActive)
		return start;

	// Determine interpolation value
	if(time <= startTime)
		return start;
//		time = startTime;
	if(time >= endTime)
		return end;

	const float w = endTime - startTime;
	const float d = time - startTime;
	const float le = d / w;

	float interMatrix[4][4];

	// if translation only, just modify start matrix
	if (hasTranslation && !(hasScale || hasRotation)) {
		memcpy(interMatrix, startMat->m, sizeof(float) * 16);
		if (hasTranslationX)
			interMatrix[0][3] = Lerp(le, startT.Tx, endT.Tx);
		if (hasTranslationY)
			interMatrix[1][3] = Lerp(le, startT.Ty, endT.Ty);
		if (hasTranslationZ)
			interMatrix[2][3] = Lerp(le, startT.Tz, endT.Tz);
		return Transform(interMatrix);
	}

	if (hasRotation) {
		// Quaternion interpolation of rotation
		Quaternion interQ = Quaternion::Slerp(le, startQ, endQ);
		interQ.ToMatrix(interMatrix);
	} else
		memcpy(interMatrix, startT.R->m, sizeof(float) * 16);

	if (hasScale) {
		const float Sx = Lerp(le, startT.Sx, endT.Sx);
		const float Sy = Lerp(le, startT.Sy, endT.Sy); 
		const float Sz = Lerp(le, startT.Sz, endT.Sz);

		// T * S * R
		for (u_int j = 0; j < 3; ++j) {
			interMatrix[0][j] = Sx * interMatrix[0][j];
			interMatrix[1][j] = Sy * interMatrix[1][j];
			interMatrix[2][j] = Sz * interMatrix[2][j];
		}
	} else {
		for (u_int j = 0; j < 3; ++j) {
			interMatrix[0][j] = startT.Sx * interMatrix[0][j];
			interMatrix[1][j] = startT.Sy * interMatrix[1][j];
			interMatrix[2][j] = startT.Sz * interMatrix[2][j];
		}
	}

	if (hasTranslationX) {
		interMatrix[0][3] = Lerp(le, startT.Tx, endT.Tx);
	} else {
		interMatrix[0][3] = startT.Tx;
	}

	if (hasTranslationY) {
		interMatrix[1][3] = Lerp(le, startT.Ty, endT.Ty);
	} else {
		interMatrix[1][3] = startT.Ty;
	}

	if (hasTranslationZ) {
		interMatrix[2][3] = Lerp(le, startT.Tz, endT.Tz);
	} else {
		interMatrix[2][3] = startT.Tz;
	}

	return Transform(interMatrix);
}

void V4MulByMatrix(const boost::shared_ptr<Matrix4x4> &A, const float x[4], float b[4]) {
	b[0] = A->m[0][0]*x[0] + A->m[0][1]*x[1] + A->m[0][2]*x[2] + A->m[0][3]*x[3];
	b[1] = A->m[1][0]*x[0] + A->m[1][1]*x[1] + A->m[1][2]*x[2] + A->m[1][3]*x[3];
	b[2] = A->m[2][0]*x[0] + A->m[2][1]*x[1] + A->m[2][2]*x[2] + A->m[2][3]*x[3];
	b[3] = A->m[3][0]*x[0] + A->m[3][1]*x[1] + A->m[3][2]*x[2] + A->m[3][3]*x[3];
}

bool MotionSystem::DecomposeMatrix(const boost::shared_ptr<Matrix4x4> &m, Transforms *trans) const {

	boost::shared_ptr<Matrix4x4> locmat(new Matrix4x4(m->m));

	// Normalize the matrix. 
	if (locmat->m[3][3] == 0)
		return false;
	for (u_int i = 0; i < 4; ++i) {
		for (u_int j = 0; j < 4; ++j)
			locmat->m[i][j] /= locmat->m[3][3];
	}
	// pmat is used to solve for perspective, but it also provides
	// an easy way to test for singularity of the upper 3x3 component.	 
	boost::shared_ptr<Matrix4x4> pmat(new Matrix4x4(locmat->m));
	for (u_int i = 0; i < 3; i++)
		pmat->m[i][3] = 0.f;
	pmat->m[3][3] = 1.f;

	if (pmat->Determinant() == 0.f)
		return false;

	// First, isolate perspective.  This is the messiest. 
	if (locmat->m[3][0] != 0.f || locmat->m[3][1] != 0.f ||
		locmat->m[3][2] != 0.f) {
		// prhs is the right hand side of the equation. 
		float prhs[4];
		prhs[0] = locmat->m[3][0];
		prhs[1] = locmat->m[3][1];
		prhs[2] = locmat->m[3][2];
		prhs[3] = locmat->m[3][3];

		// Solve the equation by inverting pmat and multiplying
		// prhs by the inverse. This is the easiest way, not
		// necessarily the best.
		boost::shared_ptr<Matrix4x4> tinvpmat(pmat->Inverse()->Transpose());
		float psol[4];
		V4MulByMatrix(tinvpmat, prhs, psol);
 
		// Stuff the answer away. 
		trans->Px = psol[0];
		trans->Py = psol[1];
		trans->Pz = psol[2];
		trans->Pw = psol[3];
		//trans->Px = trans->Py = trans->Pz = trans->Pw = 0;
		// Clear the perspective partition. 
		locmat->m[3][0] = locmat->m[3][1] = locmat->m[3][2] = 0.f;
		locmat->m[3][3] = 1.f;
	};
//	else		// No perspective. 
//		tran[U_PERSPX] = tran[U_PERSPY] = tran[U_PERSPZ] =
//			tran[U_PERSPW] = 0;

	// Next take care of translation (easy). 
	trans->Tx = locmat->m[0][3];
	trans->Ty = locmat->m[1][3];
	trans->Tz = locmat->m[2][3];
	for (u_int i = 0; i < 3; ++i)
		locmat->m[i][3] = 0.f;
	
	Vector row[3];
	// Now get scale and shear. 
	for (u_int i = 0; i < 3; ++i) {
		row[i].x = locmat->m[i][0];
		row[i].y = locmat->m[i][1];
		row[i].z = locmat->m[i][2];
	}

	// Compute X scale factor and normalize first row. 
	trans->Sx = row[0].Length();
	row[0] *= 1.f / trans->Sx;

	// Compute XY shear factor and make 2nd row orthogonal to 1st. 
	trans->Sxy = Dot(row[0], row[1]);
	row[1] -= trans->Sxy * row[0];

	// Now, compute Y scale and normalize 2nd row. 
	trans->Sy = row[1].Length();
	row[1] *= 1.f / trans->Sy;
	trans->Sxy /= trans->Sy;

	// Compute XZ and YZ shears, orthogonalize 3rd row. 
	trans->Sxz = Dot(row[0], row[2]);
	row[2] -= trans->Sxz * row[0];
	trans->Syz = Dot(row[1], row[2]);
	row[2] -= trans->Syz * row[1];

	// Next, get Z scale and normalize 3rd row. 
	trans->Sz = row[2].Length();
	row[2] *= 1.f / trans->Sz;
	trans->Sxz /= trans->Sz;
	trans->Syz /= trans->Sz;

	// At this point, the matrix (in rows[]) is orthonormal.
	// Check for a coordinate system flip.  If the determinant
	// is -1, then negate the matrix and the scaling factors.	 
	if (Dot(row[0], Cross(row[1], row[2])) < 0.f) {
		trans->Sx *= -1.f;
		trans->Sy *= -1.f;
		trans->Sz *= -1.f;
		for (u_int i = 0; i < 3; ++i) {
			row[i].x *= -1.f;
			row[i].y *= -1.f;
			row[i].z *= -1.f;
		}
	}

	// Now, get the rotations out 
	for (u_int i = 0; i < 3; ++i) {
		locmat->m[i][0] = row[i].x;
		locmat->m[i][1] = row[i].y;
		locmat->m[i][2] = row[i].z;
	}
	trans->R = locmat;
	// All done! 
	return true;
}

