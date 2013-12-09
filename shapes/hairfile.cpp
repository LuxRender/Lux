/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#include <boost/foreach.hpp>
#include <vector>

#include "hairfile.h"
#include "sphere.h"
#include "dynload.h"

using namespace lux;
using namespace luxrays;

//------------------------------------------------------------------------------
// CatmullRomCurve class definition
//------------------------------------------------------------------------------

class CatmullRomCurve {
public:
	CatmullRomCurve() {
	}
	~CatmullRomCurve() {
	}

	void AddPoint(const Point &p, const float size, const RGBColor &col,
			const float transp, const luxrays::UV &uv) {
		points.push_back(p);
		sizes.push_back(size);
		cols.push_back(col);
		transps.push_back(transp);
		uvs.push_back(uv);
	}

	void AdaptiveTessellate(const u_int maxDepth, const float error, vector<float> &values) {
		values.push_back(0.f);
		AdaptiveTessellate(0, maxDepth, error, values, 0.f, 1.f);
		values.push_back(1.f);

		std::sort(values.begin(), values.end());
	}

	Point EvaluatePoint(const float t) {
		const int count = (int)points.size();
		if (count > 2) {
			int segment = Floor2Int((count - 1) * t);
			segment = max(segment, 0);
			segment = min(segment, count - 2);

			const float ct = t * (count - 1) - segment;

			if (segment == 0)
				return CatmullRomSpline(points[0], points[0], points[1], points[2], ct);
			if (segment == count - 2)
				return CatmullRomSpline(points[count - 3], points[count - 2], points[count - 1], points[count - 1], ct);

			return CatmullRomSpline(points[segment - 1], points[segment], points[segment + 1], points[segment + 2], ct);
		} else if (count == 2)
			return (1.f - t) * points[0] + t * points[1];
		else if (count == 1)
			return points[0];
		else
			throw std::runtime_error("Internal error in CatmullRomCurve::EvaluatePoint()");
	}

	float EvaluateSize(const float t) {
		int count = (int)sizes.size();
		if (count > 2) {
			int segment = Floor2Int((count - 1) * t);
			segment = max(segment, 0);
			segment = min(segment, count - 2);

			const float ct = t * (count - 1) - segment;

			if (segment == 0)
				return CatmullRomSpline(sizes[0], sizes[0], sizes[1], sizes[2], ct);
			if (segment == count - 2)
				return CatmullRomSpline(sizes[count - 3], sizes[count - 2], sizes[count - 1], sizes[count - 1], ct);

			return CatmullRomSpline(sizes[segment - 1], sizes[segment], sizes[segment + 1], sizes[segment + 2], ct);
		} else if (count == 2)
			return (1.f - t) * sizes[0] + t * sizes[1];
		else if (count == 1)
			return sizes[0];
		else
			throw std::runtime_error("Internal error in CatmullRomCurve::EvaluateSize()");
	}

	RGBColor EvaluateColor(const float t) {
		int count = (int)cols.size();
		if (count > 2) {
			int segment = Floor2Int((count - 1) * t);
			segment = max(segment, 0);
			segment = min(segment, count - 2);

			const float ct = t * (count - 1) - segment;

			if (segment == 0)
				return CatmullRomSpline(cols[0], cols[0], cols[1], cols[2], ct);
			if (segment == count - 2)
				return CatmullRomSpline(cols[count - 3], cols[count - 2], cols[count - 1], cols[count - 1], ct);

			return CatmullRomSpline(cols[segment - 1], cols[segment], cols[segment + 1], cols[segment + 2], ct);
		} else if (count == 2)
			return (1.f - t) * cols[0] + t * cols[1];
		else if (count == 1)
			return cols[0];
		else
			throw std::runtime_error("Internal error in CatmullRomCurve::EvaluateColor()");
	}

	float EvaluateTransparency(const float t) {
		int count = (int)transps.size();
		if (count > 2) {
			int segment = Floor2Int((count - 1) * t);
			segment = max(segment, 0);
			segment = min(segment, count - 2);

			const float ct = t * (count - 1) - segment;

			if (segment == 0)
				return CatmullRomSpline(transps[0], transps[0], transps[1], transps[2], ct);
			if (segment == count - 2)
				return CatmullRomSpline(transps[count - 3], transps[count - 2], transps[count - 1], transps[count - 1], ct);

			return CatmullRomSpline(transps[segment - 1], transps[segment], transps[segment + 1], transps[segment + 2], ct);
		} else if (count == 2)
			return (1.f - t) * transps[0] + t * transps[1];
		else if (count == 1)
			return transps[0];
		else
			throw std::runtime_error("Internal error in CatmullRomCurve::EvaluateTransparency()");
	}

	luxrays::UV EvaluateUV(const float t) {
		int count = (int)uvs.size();
		if (count > 2) {
			int segment = Floor2Int((count - 1) * t);
			segment = max(segment, 0);
			segment = min(segment, count - 2);

			const float ct = t * (count - 1) - segment;

			if (segment == 0)
				return CatmullRomSpline(uvs[0], uvs[0], uvs[1], uvs[2], ct);
			if (segment == count - 2)
				return CatmullRomSpline(uvs[count - 3], uvs[count - 2], uvs[count - 1], uvs[count - 1], ct);

			return CatmullRomSpline(uvs[segment - 1], uvs[segment], uvs[segment + 1], uvs[segment + 2], ct);
		} else if (count == 2)
			return (1.f - t) * uvs[0] + t * uvs[1];
		else if (count == 1)
			return uvs[0];
		else
			throw std::runtime_error("Internal error in CatmullRomCurve::EvaluateUV()");
	}

private:
	bool AdaptiveTessellate(const u_int depth, const u_int maxDepth, const float error,
			vector<float> &values, const float t0, const float t1) {
		if (depth >= maxDepth)
			return false;

		const float tmid = (t0 + t1) * .5f;

		const Point p0 = EvaluatePoint(t0);
		const Point pmid = EvaluatePoint(tmid);
		const Point p1 = EvaluatePoint(t1);
		
		const Vector vmid = pmid - p0;
		const Vector v = p1 - p0;

		// Check if the vectors are nearly parallel
		if (AbsDot(Normalize(vmid), Normalize(v)) < 1.f - .05f) {
			// Tessellate left side too
			const bool leftSide = AdaptiveTessellate(depth + 1, maxDepth, error, values, t0, tmid);
			const bool rightSide = AdaptiveTessellate(depth + 1, maxDepth, error, values, tmid, t1);

			if (leftSide || rightSide)
				values.push_back(tmid);

			return false;
		}

		//----------------------------------------------------------------------
		// Curve flatness check
		//----------------------------------------------------------------------

		// Calculate the distance between vmid and the segment
		const float distance = Cross(v, vmid).Length() / vmid.Length();

		// Check if the distance normalized with the segment length is
		// over the required error
		const float segmentLength = v.Length();
		if (distance / segmentLength > error) {
			// Tessellate left side too
			AdaptiveTessellate(depth + 1, maxDepth, error, values, t0, tmid);
			
			values.push_back(tmid);

			// Tessellate right side too
			AdaptiveTessellate(depth + 1, maxDepth, error, values, tmid, t1);

			return true;
		}
		
		//----------------------------------------------------------------------
		// Curve size check
		//----------------------------------------------------------------------

		const float s0 = EvaluateSize(t0);
		const float smid = EvaluateSize(tmid);
		const float s1 = EvaluateSize(t1);

		const float expectedSize = (s0 + s1) * .5f;
		if (fabsf(expectedSize - smid) > error) {
			// Tessellate left side too
			AdaptiveTessellate(depth + 1, maxDepth, error, values, t0, tmid);
			
			values.push_back(tmid);

			// Tessellate right side too
			AdaptiveTessellate(depth + 1, maxDepth, error, values, tmid, t1);

			return true;
		}

		return false;
	}

	float CatmullRomSpline(const float a, const float b, const float c, const float d, const float t) {
		const float t1 = (c - a) * .5f;
		const float t2 = (d - b) * .5f;

		const float h1 = +2 * t * t * t - 3 * t * t + 1;
		const float h2 = -2 * t * t * t + 3 * t * t;
		const float h3 = t * t * t - 2 * t * t + t;
		const float h4 = t * t * t - t * t;

		return b * h1 + c * h2 + t1 * h3 + t2 * h4;
	}

	Point CatmullRomSpline(const Point a, const Point b, const Point c, const Point d, const float t) {
		return Point(
				CatmullRomSpline(a.x, b.x, c.x, d.x, t),
				CatmullRomSpline(a.y, b.y, c.y, d.y, t),
				CatmullRomSpline(a.z, b.z, c.z, d.z, t));
	}

	RGBColor CatmullRomSpline(const RGBColor a, const RGBColor b, const RGBColor c, const RGBColor d, const float t) {
		return RGBColor(
				Clamp(CatmullRomSpline(a.c[0], b.c[0], c.c[0], d.c[0], t), 0.f, 1.f),
				Clamp(CatmullRomSpline(a.c[1], b.c[1], c.c[1], d.c[1], t), 0.f, 1.f),
				Clamp(CatmullRomSpline(a.c[2], b.c[2], c.c[2], d.c[2], t), 0.f, 1.f));
	}

	luxrays::UV CatmullRomSpline(const luxrays::UV a, const luxrays::UV b, const luxrays::UV c, const luxrays::UV d, const float t) {
		return luxrays::UV(
				Clamp(CatmullRomSpline(a.u, b.u, c.u, d.u, t), 0.f, 1.f),
				Clamp(CatmullRomSpline(a.v, b.v, c.v, d.v, t), 0.f, 1.f));
	}

	vector<Point> points;
	vector<float> sizes;
	vector<RGBColor> cols;
	vector<float> transps;
	vector<luxrays::UV> uvs;
};

//------------------------------------------------------------------------------
// HairFile methods
//------------------------------------------------------------------------------

HairFile::HairFile(const Transform &o2w, bool ro, const string &name, const Point *cameraPos,
		const string &aType,  const TessellationType tType, const u_int aMaxDepth,
		const float aError, const u_int sSideCount,
		const bool sCapBottom, const bool sCapTop, const float gamma,
		boost::shared_ptr<cyHairFile> &hair) : Shape(o2w, ro, name) {
	hasCameraPosition = (cameraPos != NULL);
	if (hasCameraPosition) {
		// Transform the camera position in local coordinate
		cameraPosition = Inverse(ObjectToWorld) * (*cameraPos);
	}

	accelType = aType;
	tesselType = tType;
	colorGamma = gamma;
	adaptiveMaxDepth = aMaxDepth;
	adaptiveError = aError;
	solidSideCount = sSideCount;
	solidCapBottom = sCapBottom;
	solidCapTop = sCapTop;
	hairFile = hair;
}

HairFile::~HairFile() {
}

BBox HairFile::ObjectBound() const {
	BBox objectBound;
	
	const cyHairFileHeader &header = hairFile->GetHeader();
	const float *points = hairFile->GetPointsArray();
	const float *thickness = hairFile->GetThicknessArray();

	for (u_int i = 0; i < header.point_count; ++i) {
		const unsigned int index = i * 3;
		const Point p(points[index], points[index + 1], points[index + 2]);
		BBox pointBBox(p);

		const float radius = ((thickness) ? thickness[i] : header.d_thickness) * .5f;
		pointBBox.Expand(radius);

		objectBound = Union(objectBound, pointBBox);
	}

	return objectBound;
}

void HairFile::TessellateRibbon(const vector<Point> &hairPoints,
		const vector<float> &hairSizes, const vector<RGBColor> &hairCols,
		const vector<luxrays::UV> &hairUVs, const vector<float> &hairTransps,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs, vector<float> &meshCols,
		vector<float> &meshTransps) const {
	// Create the mesh vertices
	const u_int baseOffset = meshVerts.size();

	Vector previousDir;
	Vector previousX;
	// I'm using quaternion here in order to avoid Gimbal lock problem
	Quaternion trans;
	for (int i = 0; i < (int)hairPoints.size(); ++i) {
		Vector dir;
		// I need a special case for the very last point
		if (i == (int)hairPoints.size() - 1)
			dir = Normalize(hairPoints[i] - hairPoints[i - 1]);
		else
			dir = Normalize(hairPoints[i + 1] - hairPoints[i]);

		if (i == 0) {
			// Build the initial quaternion by establishing an initial (arbitrary)
			// frame

			// Check if I have to face the ribbon in a specific direction
			Vector up;
			if (hasCameraPosition)
				up = Normalize(cameraPosition - hairPoints[i]);
			else
				up = Vector(1.f, 0.f, 0.f);

			if (AbsDot(dir, up) > 1.f - .05f) {
				up = Vector(0.f, 1.f, 0.f);
				if (AbsDot(dir, up) > 1.f - .05f)
					up = Vector(0.f, 0.f, 1.f);
			}

			const Transform dirTrans = LookAt(hairPoints[0], hairPoints[1], up);
			trans = Quaternion(dirTrans.m);
		} else {
			// Compose the new delta transformation with all old one
			trans = GetRotationBetween(previousDir, dir) * trans;
		}
		previousDir = dir;

		const Vector newPreviousX = trans.RotateVector(Vector(1.f, 0.f, 0.f));

		// Using this trick to have a section half way between previous and new one
		const Vector x = (i == 0) ? newPreviousX : (previousX + newPreviousX) * .5;

		previousX = newPreviousX;
		
		const Point p0 = hairPoints[i] + hairSizes[i] * x;
		const Point p1 = hairPoints[i] - hairSizes[i] * x;
		meshVerts.push_back(p0);
		meshNorms.push_back(Normal());
		meshVerts.push_back(p1);
		meshNorms.push_back(Normal());

		meshUVs.push_back(hairUVs[i].u);
		meshUVs.push_back(hairUVs[i].v);
		meshUVs.push_back(hairUVs[i].u);
		meshUVs.push_back(hairUVs[i].v);

		meshCols.push_back(hairCols[i].c[0]);
		meshCols.push_back(hairCols[i].c[1]);
		meshCols.push_back(hairCols[i].c[2]);
		meshCols.push_back(hairCols[i].c[0]);
		meshCols.push_back(hairCols[i].c[1]);
		meshCols.push_back(hairCols[i].c[2]);

		meshTransps.push_back(hairTransps[i]);
		meshTransps.push_back(hairTransps[i]);
	}

	// Triangulate the vertex mesh
	for (int i = 0; i < (int)hairPoints.size() - 1; ++i) {
		const u_int index = baseOffset + i * 2;

		const u_int i0 = index;
		const u_int i1 = index + 1;
		const u_int i2 = index + 2;
		const u_int i3 = index + 3;

		// First triangle
		meshTris.push_back(i0);
		meshTris.push_back(i1);
		meshTris.push_back(i2);
		// First triangle normal
		const Normal n0 = Normal(Cross(meshVerts[i2] - meshVerts[i0], meshVerts[i1] - meshVerts[i0]));
		meshNorms[i0] += n0;
		meshNorms[i1] += n0;
		meshNorms[i2] += n0;

		// Second triangle
		meshTris.push_back(i1);
		meshTris.push_back(i3);
		meshTris.push_back(i2);
		// Second triangle normal
		const Normal n1 = Normal(Cross(meshVerts[i2] - meshVerts[i1], meshVerts[i3] - meshVerts[i1]));
		meshNorms[i1] += n1;
		meshNorms[i2] += n1;
		meshNorms[i3] += n1;
	}
}

void HairFile::TessellateAdaptive(const bool solid, const vector<Point> &hairPoints,
		const vector<float> &hairSizes, const vector<RGBColor> &hairCols,
		const vector<luxrays::UV> &hairUVs, const vector<float> &hairTransps,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs, vector<float> &meshCols,
		vector<float> &meshTransps) const {
	// Interpolate the hair segments
	CatmullRomCurve curve;
	for (int i = 0; i < (int)hairPoints.size(); ++i)
		curve.AddPoint(hairPoints[i], hairSizes[i], hairCols[i],
				hairTransps[i], hairUVs[i]);

	// Tessellate the curve
	vector<float> values;
	curve.AdaptiveTessellate(adaptiveMaxDepth, adaptiveError, values);

	// Create the ribbon
	vector<Point> tesselPoints;
	vector<float> tesselSizes;
	vector<RGBColor> tesselCols;
	vector<float> tesselTransps;
	vector<luxrays::UV> tesselUVs;
	for (u_int i = 0; i < values.size(); ++i) {
		tesselPoints.push_back(curve.EvaluatePoint(values[i]));
		tesselSizes.push_back(curve.EvaluateSize(values[i]));
		tesselCols.push_back(curve.EvaluateColor(values[i]));
		tesselTransps.push_back(curve.EvaluateTransparency(values[i]));
		tesselUVs.push_back(curve.EvaluateUV(values[i]));
	}

	if (solid)
		TessellateSolid(tesselPoints, tesselSizes, tesselCols, tesselUVs, tesselTransps,
			meshVerts, meshNorms, meshTris, meshUVs, meshCols, meshTransps);
	else
		TessellateRibbon(tesselPoints, tesselSizes, tesselCols, tesselUVs, tesselTransps,
			meshVerts, meshNorms, meshTris, meshUVs, meshCols, meshTransps);
}

void HairFile::TessellateSolid(const vector<Point> &hairPoints,
		const vector<float> &hairSizes, const vector<RGBColor> &hairCols,
		const vector<luxrays::UV> &hairUVs, const vector<float> &hairTransps,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs, vector<float> &meshCols,
		vector<float> &meshTransps) const {
	// Create the mesh vertices
	const u_int baseOffset = meshVerts.size();
	const float angleStep = Radians(360.f / solidSideCount);

	Vector previousDir;
	Vector previousX, previousY, previousZ;
	// I'm using quaternion here in order to avoid Gimbal lock problem
	Quaternion trans;
	for (int i = 0; i < (int)hairPoints.size(); ++i) {
		Vector dir;
		// I need a special case for the very last point
		if (i == (int)hairPoints.size() - 1)
			dir = Normalize(hairPoints[i] - hairPoints[i - 1]);
		else
			dir = Normalize(hairPoints[i + 1] - hairPoints[i]);

		if (i == 0) {
			// Build the initial quaternion by establishing an initial (arbitrary)
			// frame

			Vector up(0.f, 0.f, 1.f);
			if (AbsDot(dir, up) > 1.f - .05f)
				up = Vector(1.f, 0.f, 0.f);

			const Transform dirTrans = LookAt(hairPoints[0], hairPoints[1], up);
			trans = Quaternion(dirTrans.m);
		} else {
			// Compose the new delta transformation with all old one
			trans = GetRotationBetween(previousDir, dir) * trans;
		}
		previousDir = dir;

		const Vector newPreviousX = trans.RotateVector(Vector(1.f, 0.f, 0.f));
		const Vector newPreviousY = trans.RotateVector(Vector(0.f, 1.f, 0.f));
		const Vector newPreviousZ = trans.RotateVector(Vector(0.f, 0.f, 1.f));

		// Using this trick to have a section half way between previous and new one
		const Vector x = (i == 0) ? newPreviousX : (previousX + newPreviousX) * .5;
		const Vector y = (i == 0) ? newPreviousY : (previousY + newPreviousY) * .5;
		const Vector z = (i == 0) ? newPreviousZ : (previousZ + newPreviousZ) * .5;

		previousX = newPreviousX;
		previousY = newPreviousY;
		previousZ = newPreviousZ;

		float angle = 0.f;
		for (u_int j = 0; j < solidSideCount; ++j) {
			const Point lp(hairSizes[i] * cosf(angle), hairSizes[i] * sinf(angle), 0.f);
			const Point p(
				x.x * lp.x + y.x * lp.y + z.x * lp.z + hairPoints[i].x,
				x.y * lp.x + y.y * lp.y + z.y * lp.z + hairPoints[i].y,
				x.z * lp.x + y.z * lp.y + z.z * lp.z + hairPoints[i].z);
			
			meshVerts.push_back(p);
			meshNorms.push_back(Normal());
			meshUVs.push_back(hairUVs[i].u);
			meshUVs.push_back(hairUVs[i].v);
			meshCols.push_back(hairCols[i].c[0]);
			meshCols.push_back(hairCols[i].c[1]);
			meshCols.push_back(hairCols[i].c[2]);
			meshTransps.push_back(hairTransps[i]);

			angle += angleStep;
		}
	}

	// Triangulate the vertex mesh
	for (int i = 0; i < (int)hairPoints.size() - 1; ++i) {
		const u_int index = baseOffset + i * solidSideCount;

		for (u_int j = 0; j < solidSideCount; ++j) {
			// Side face

			const u_int i0 = index + j;
			const u_int i1 = (j == solidSideCount - 1) ? index : (index + j + 1);
			const u_int i2 = index + j + solidSideCount;
			const u_int i3 = (j == solidSideCount - 1) ? (index + solidSideCount) : (index + j + solidSideCount  + 1);

			// First triangle
			meshTris.push_back(i0);
			meshTris.push_back(i1);
			meshTris.push_back(i2);
			const Normal n0 = Normal(Cross(meshVerts[i2] - meshVerts[i0], meshVerts[i1] - meshVerts[i0]));
			meshNorms[i0] += n0;
			meshNorms[i1] += n0;
			meshNorms[i2] += n0;

			// Second triangle
			meshTris.push_back(i1);
			meshTris.push_back(i3);
			meshTris.push_back(i2);
			const Normal n1 = Normal(Cross(meshVerts[i2] - meshVerts[i0], meshVerts[i3] - meshVerts[i0]));
			meshNorms[i1] += n1;
			meshNorms[i3] += n1;
			meshNorms[i2] += n1;
		}
	}

	if (solidCapTop) {
		// Add a top fan cap

		const u_int offset = meshVerts.size();
		const Normal n = Normal(Normalize(hairPoints[hairPoints.size() - 1] - hairPoints[hairPoints.size() - 2]));
		for (u_int j = 0; j < solidSideCount; ++j) {
			meshVerts.push_back(meshVerts[offset - solidSideCount + j]);
			meshNorms.push_back(n);
			meshUVs.push_back(hairUVs.back().u);
			meshUVs.push_back(hairUVs.back().v);
			meshCols.push_back(hairCols.back().c[0]);
			meshCols.push_back(hairCols.back().c[1]);
			meshCols.push_back(hairCols.back().c[2]);
			meshTransps.push_back(hairTransps.back());
		}

		// Add the fan center
		meshVerts.push_back(hairPoints.back());
		meshNorms.push_back(n);
		meshUVs.push_back(hairUVs.back().u);
		meshUVs.push_back(hairUVs.back().v);
		meshCols.push_back(hairCols.back().c[0]);
		meshCols.push_back(hairCols.back().c[1]);
		meshCols.push_back(hairCols.back().c[2]);
		meshTransps.push_back(hairTransps.back());

		const u_int i3 = meshVerts.size() - 1;
		for (u_int j = 0; j < solidSideCount; ++j) {
			const u_int i0 = offset + j;
			const u_int i1 = (j == solidSideCount - 1) ? offset : (offset + j + 1);

			meshTris.push_back(i0);
			meshTris.push_back(i1);
			meshTris.push_back(i3);
		}
	}

	if (solidCapBottom) {
		// Add a bottom fan cap

		const u_int offset = meshVerts.size();
		const Normal n = Normal(Normalize(hairPoints[0] - hairPoints[1]));
		for (u_int j = 0; j < solidSideCount; ++j) {
			meshVerts.push_back(meshVerts[baseOffset + j]);
			meshNorms.push_back(n);
			meshUVs.push_back(hairUVs[0].u);
			meshUVs.push_back(hairUVs[0].v);
			meshCols.push_back(hairCols[0].c[0]);
			meshCols.push_back(hairCols[0].c[1]);
			meshCols.push_back(hairCols[0].c[2]);
			meshTransps.push_back(hairTransps[0]);
		}

		// Add the fan center
		meshVerts.push_back(hairPoints[0]);
		meshNorms.push_back(n);
		meshUVs.push_back(hairUVs[0].u);
		meshUVs.push_back(hairUVs[0].v);
		meshCols.push_back(hairCols[0].c[0]);
		meshCols.push_back(hairCols[0].c[1]);
		meshCols.push_back(hairCols[0].c[2]);
		meshTransps.push_back(hairTransps[0]);

		const u_int i3 = meshVerts.size() - 1;
		for (u_int j = 0; j < solidSideCount; ++j) {
			const u_int i0 = offset + j;
			const u_int i1 = (j == solidSideCount - 1) ? offset : (offset + j + 1);

			meshTris.push_back(i1);
			meshTris.push_back(i0);
			meshTris.push_back(i3);
		}
	}
}

void HairFile::Refine(vector<boost::shared_ptr<Shape> > &refined) const {
	const cyHairFileHeader &header = hairFile->GetHeader();
	if (header.hair_count == 0)
		return;

	if (refinedHairs.size() > 0) {
		refined.reserve(refined.size() + refinedHairs.size());
		for (u_int i = 0; i < refinedHairs.size(); ++i)
			refined.push_back(refinedHairs[i]);
		return;
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Refining " << header.hair_count << " strands";
	const double start = luxrays::WallClockTime();

	const float *points = hairFile->GetPointsArray();
	const float *thickness = hairFile->GetThicknessArray();
	const u_short *segments = hairFile->GetSegmentsArray();
	const float *colors = hairFile->GetColorsArray();
	const float *transparency = hairFile->GetTransparencyArray();
	const float *uvs = hairFile->GetUVsArray();

	if (segments || (header.d_segments > 0)) {
		u_int pointIndex = 0;

		vector<Point> hairPoints;
		vector<float> hairSizes;
		vector<RGBColor> hairCols;
		vector<float> hairTransps;
		vector<luxrays::UV> hairUVs;

		vector<Point> meshVerts;
		vector<Normal> meshNorms;
		vector<int> meshTris;
		vector<float> meshUVs;
		vector<float> meshCols;
		vector<float> meshTransps;
		for (u_int i = 0; i < header.hair_count; ++i) {
			// segmentSize must be a signed 
			const int segmentSize = segments ? segments[i] : header.d_segments;
			if (segmentSize == 0)
				continue;

			// Collect the segment points and size
			hairPoints.clear();
			hairSizes.clear();
			hairCols.clear();
			hairTransps.clear();
			hairUVs.clear();
			for (int j = 0; j <= segmentSize; ++j) {
				hairPoints.push_back(Point(points[pointIndex * 3], points[pointIndex * 3 + 1], points[pointIndex * 3 + 2]));
				hairSizes.push_back(((thickness) ? thickness[pointIndex] : header.d_thickness) * .5f);
				if (colors)
					hairCols.push_back(RGBColor(colors[pointIndex * 3], colors[pointIndex * 3 + 1], colors[pointIndex * 3 + 2]));
				else
					hairCols.push_back(RGBColor(header.d_color[0], header.d_color[1], header.d_color[2]));
				if (transparency)
					hairTransps.push_back(1.f - transparency[pointIndex]);
				else
					hairTransps.push_back(1.f - header.d_transparency);
				if (uvs)
					hairUVs.push_back(luxrays::UV(uvs[pointIndex * 2], uvs[pointIndex * 2 + 1]));
				else 
					hairUVs.push_back(luxrays::UV(0.f, j / (float)segmentSize));

				++pointIndex;
			}

			switch (tesselType) {
				case TESSEL_RIBBON:
					TessellateRibbon(hairPoints, hairSizes, hairCols, hairUVs,
							hairTransps, meshVerts, meshNorms, meshTris, meshUVs,
							meshCols, meshTransps);
					break;
				case TESSEL_RIBBON_ADAPTIVE:
					TessellateAdaptive(false, hairPoints, hairSizes, hairCols, hairUVs, 
							hairTransps, meshVerts, meshNorms, meshTris, meshUVs,
							meshCols, meshTransps);
					break;
				case TESSEL_SOLID:
					TessellateSolid(hairPoints, hairSizes, hairCols, hairUVs, 
							hairTransps, meshVerts, meshNorms, meshTris, meshUVs,
							meshCols, meshTransps);
					break;					
				case TESSEL_SOLID_ADAPTIVE:
					TessellateAdaptive(true, hairPoints, hairSizes, hairCols, hairUVs, 
							hairTransps, meshVerts, meshNorms, meshTris, meshUVs,
							meshCols, meshTransps);
					break;
				default:
					LOG(LUX_ERROR, LUX_RANGE)<< "Unknown tessellation  type in an HairFile Shape";
			}
		}

		// Normalize normals
		for (u_int i = 0; i < meshNorms.size(); ++i)
			meshNorms[i] = Normalize(meshNorms[i]);

		LOG(LUX_DEBUG, LUX_NOERROR) << "Strands mesh: " << meshTris.size() / 3 << " triangles";

		// Create the mesh Shape
		ParamSet paramSet;
		paramSet.AddInt("indices", &meshTris[0], meshTris.size());
		paramSet.AddFloat("uv", &meshUVs[0], meshUVs.size());
		paramSet.AddPoint("P", &meshVerts[0], meshVerts.size());
		paramSet.AddNormal("N", &meshNorms[0], meshNorms.size());
		paramSet.AddString("acceltype", &accelType, 1);
		paramSet.AddFloat("gamma", &colorGamma, 1);

		// Check if I have to include vertex color too
		bool useColor = false;
		BOOST_FOREACH(const float &c, meshCols) {
			if (c != 1.f) {
				useColor = true;
				break;
			}
		}

		if (useColor) {
			LOG(LUX_DEBUG, LUX_NOERROR) << "Strands use colors";
			paramSet.AddFloat("C", &meshCols[0], meshCols.size());
		}

		// Check if I have to include vertex alpha too
		bool useAlpha = false;
		BOOST_FOREACH(const float &a, meshTransps) {
			if (a != 1.f) {
				useAlpha = true;
				break;
			}
		}

		if (useAlpha) {
			LOG(LUX_DEBUG, LUX_NOERROR) << "Strands use alphas";
			paramSet.AddFloat("A", &meshTransps[0], meshTransps.size());
		}

		boost::shared_ptr<Shape> shape = MakeShape("trianglemesh",
				ObjectToWorld, reverseOrientation, paramSet);

		refined.reserve(refined.size() + meshTris.size() / 3);	
		refined.push_back(shape);
		refinedHairs.reserve(meshTris.size() / 3);
		refinedHairs.push_back(shape);
	} else {
		// There are not segments so it must be a particles file. The Shape
		// is refined as a set of spheres.
		for (u_int i = 0; i < header.hair_count; ++i) {
			const unsigned int index = i * 3;
			const Vector vert(points[index], points[index + 1], points[index + 2]);
			const float radius = ((thickness) ? thickness[i] : header.d_thickness) * .5f;

			boost::shared_ptr<Shape> shape(new Sphere(ObjectToWorld * Translate(vert), reverseOrientation, name, radius, -radius, radius, 360.f));
			refined.push_back(shape);
		}
	}

	const float dt = luxrays::WallClockTime() - start;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Refining time: " << std::setprecision(3) << dt << " secs";
}

void HairFile::Tessellate(vector<luxrays::TriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
	// Refine the primitive
	vector<boost::shared_ptr<Shape> > refined;
	Refine(refined);

	// Tessellate all generated primitives
	for (u_int i = 0; i < refined.size(); ++i)
		refined[i]->Tessellate(meshList, primitiveList);
}

void HairFile::ExtTessellate(vector<luxrays::ExtTriangleMesh *> *meshList,
		vector<const Primitive *> *primitiveList) const {
	// Refine the primitive
	vector<boost::shared_ptr<Shape> > refined;
	Refine(refined);

	// Tessellate all generated primitives
	for (u_int i = 0; i < refined.size(); ++i)
		refined[i]->ExtTessellate(meshList, primitiveList);
}

Shape *HairFile::CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params) {
	string name = params.FindOneString("name", "'hairfile'");
	const string filename = AdjustFilename(params.FindOneString("filename", "none"));
	u_int nItems;
	const Point *cameraPos = params.FindPoint("camerapos", &nItems);
	const string accelType = params.FindOneString("acceltype", "qbvh");
	const string tessellationTypeStr = params.FindOneString("tesseltype", "ribbon");
	TessellationType tessellationType;
	if (tessellationTypeStr == "ribbon")
		tessellationType = TESSEL_RIBBON;
	else if (tessellationTypeStr == "ribbonadaptive")
		tessellationType = TESSEL_RIBBON_ADAPTIVE;
	else if (tessellationTypeStr == "solid")
		tessellationType = TESSEL_SOLID;
	else if (tessellationTypeStr == "solidadaptive")
		tessellationType = TESSEL_SOLID_ADAPTIVE;
	else {
		SHAPE_LOG(name, LUX_WARNING, LUX_BADTOKEN) << "Tessellation type  '" << tessellationTypeStr << "' unknown. Using \"ribbon\".";
		tessellationType = TESSEL_RIBBON;
	}

	const u_int adaptiveMaxDepth = max(0, params.FindOneInt("adaptive_maxdepth", 8));
	const float adaptiveError = params.FindOneFloat("adaptive_error", 0.1f);

	const u_int solidSideCount = max(0, params.FindOneInt("solid_sidecount", 3));
	const bool solidCapBottom = params.FindOneBool("solid_capbottom", false);
	const bool solidCapTop = params.FindOneBool("solid_captop", false);
	
	

	boost::shared_ptr<cyHairFile> hairFile(new cyHairFile());
	int hairCount = hairFile->LoadFromFile(filename.c_str());
	if (hairCount <= 0) {
		SHAPE_LOG("hairfile", LUX_ERROR, LUX_SYSTEM) << "Unable to read hair file '" << filename << "'";
		return NULL;
	}

	const float colorGamma = params.FindOneFloat("gamma", 1.f);

	return new HairFile(o2w, reverseOrientation, name, cameraPos, accelType, tessellationType,
		adaptiveMaxDepth, adaptiveError, solidSideCount, solidCapBottom, solidCapTop, colorGamma,
		hairFile);
}

static DynamicLoader::RegisterShape<HairFile> r("hairfile");
