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
// MotionSystem Declarations

namespace lux
{

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

		startQ = Quaternion(startMat);
		startQ.Normalize();
		endQ = Quaternion(endMat);
		endQ.Normalize();	

		hasRotation = false;
		hasScaleX = hasScaleY = hasScaleZ = false;
		hasTranslationX = hasTranslationY = hasTranslationZ = false;

		// Precheck for translation
		if(startMat->m[0][3] != endMat->m[0][3]) hasTranslationX = true;
		if(startMat->m[1][3] != endMat->m[1][3]) hasTranslationY = true;
		if(startMat->m[2][3] != endMat->m[2][3]) hasTranslationZ = true;

		// Precheck for scale
		if(startMat->m[0][0] != endMat->m[0][0]) hasScaleX = true;
		if(startMat->m[1][1] != endMat->m[1][1]) hasScaleY = true;
		if(startMat->m[2][2] != endMat->m[2][2]) hasScaleZ = true;

		// precheck for rotation
		if(startMat->m[1][0] != endMat->m[1][0]) hasRotation = true;
		if(startMat->m[2][0] != endMat->m[2][0]) hasRotation = true;
		if(startMat->m[0][1] != endMat->m[0][1]) hasRotation = true;
		if(startMat->m[2][1] != endMat->m[2][1]) hasRotation = true;
		if(startMat->m[0][2] != endMat->m[0][2]) hasRotation = true;
		if(startMat->m[1][2] != endMat->m[1][2]) hasRotation = true;

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
		memcpy(interMatrix, startMat->m, 16 * sizeof(float));	

		if(hasRotation) {
			// Quaternion interpolation of rotation / also does scale
			Quaternion between_quat = slerp(le, startQ, endQ);
			toMatrix(between_quat, interMatrix);
		} else {
			// Interpolate Scale linearly
			if(hasScaleX) interMatrix[0][0] = Lerp(le, startMat->m[0][0], endMat->m[0][0]);
			if(hasScaleY) interMatrix[1][1] = Lerp(le, startMat->m[1][1], endMat->m[1][1]);
			if(hasScaleZ) interMatrix[2][2] = Lerp(le, startMat->m[2][2], endMat->m[2][2]);
		}

		// Interpolate Translation linearly
		if(hasTranslationX) interMatrix[0][3] = Lerp(le, startMat->m[0][3], endMat->m[0][3]);
		if(hasTranslationY) interMatrix[1][3] = Lerp(le, startMat->m[1][3], endMat->m[1][3]);
		if(hasTranslationZ) interMatrix[2][3] = Lerp(le, startMat->m[2][3], endMat->m[2][3]);

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
	// MotionSystem Protected Data
	float startTime, endTime;
	Transform start, end;
	boost::shared_ptr<Matrix4x4> startMat, endMat;
	Quaternion startQ, endQ;
	bool hasRotation;
	bool hasTranslationX, hasTranslationY, hasTranslationZ;
	bool hasScaleX, hasScaleY, hasScaleZ;
};

}//namespace lux

#endif // LUX_MOTIONSYSTEM_H
