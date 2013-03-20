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

//------------------------------------------------------------------------------
// CatmullRomCurve class definition
//------------------------------------------------------------------------------

class CatmullRomCurve {
public:
	CatmullRomCurve() {
	}
	~CatmullRomCurve() {
	}

	void AddPoint(const Point &p, const float size, const RGBColor &col, const float transp) {
		points.push_back(p);
		sizes.push_back(size);
		cols.push_back(col);
		transps.push_back(transp);
	}

	void AdaptiveTessellate(const u_int maxDepth, const float error, vector<float> &values) {
		values.push_back(0.f);
		AdaptiveTessellate(0, maxDepth, error, values, 0.f, 1.f);
		values.push_back(1.f);

		std::sort(values.begin(), values.end());
	}

	Point EvaluatePoint(const float t) {
		int count = (int)points.size();
		int segment = Floor2Int((count - 1) * t);
		segment = max(segment, 0);
		segment = min(segment, count - 2);

		const float ct = t * (count - 1) - segment;

		if (segment == 0)
			return CatmullRomSpline(points[0], points[0], points[1], points[2], ct);
		if (segment == count - 2)
			return CatmullRomSpline(points[count - 3], points[count - 2], points[count - 1], points[count - 1], ct);

		return CatmullRomSpline(points[segment - 1], points[segment], points[segment + 1], points[segment + 2], ct);
	}

	float EvaluateSize(const float t) {
		int count = (int)sizes.size();
		int segment = Floor2Int((count - 1) * t);
		segment = max(segment, 0);
		segment = min(segment, count - 2);

		const float ct = t * (count - 1) - segment;

		if (segment == 0)
			return CatmullRomSpline(sizes[0], sizes[0], sizes[1], sizes[2], ct);
		if (segment == count - 2)
			return CatmullRomSpline(sizes[count - 3], sizes[count - 2], sizes[count - 1], sizes[count - 1], ct);

		return CatmullRomSpline(sizes[segment - 1], sizes[segment], sizes[segment + 1], sizes[segment + 2], ct);
	}

	RGBColor EvaluateColor(const float t) {
		int count = (int)cols.size();
		int segment = Floor2Int((count - 1) * t);
		segment = max(segment, 0);
		segment = min(segment, count - 2);

		const float ct = t * (count - 1) - segment;

		if (segment == 0)
			return CatmullRomSpline(cols[0], cols[0], cols[1], cols[2], ct);
		if (segment == count - 2)
			return CatmullRomSpline(cols[count - 3], cols[count - 2], cols[count - 1], cols[count - 1], ct);

		return CatmullRomSpline(cols[segment - 1], cols[segment], cols[segment + 1], cols[segment + 2], ct);
	}

	float EvaluateTransparency(const float t) {
		int count = (int)transps.size();
		int segment = Floor2Int((count - 1) * t);
		segment = max(segment, 0);
		segment = min(segment, count - 2);

		const float ct = t * (count - 1) - segment;

		if (segment == 0)
			return CatmullRomSpline(transps[0], transps[0], transps[1], transps[2], ct);
		if (segment == count - 2)
			return CatmullRomSpline(transps[count - 3], transps[count - 2], transps[count - 1], transps[count - 1], ct);

		return CatmullRomSpline(transps[segment - 1], transps[segment], transps[segment + 1], transps[segment + 2], ct);
	}

private:
	bool AdaptiveTessellate(const u_int depth, const u_int maxDepth, const float error,
			vector<float> &values, const float t0, const float t1) {
		if (depth >= maxDepth)
			return false;

		const float tmid = (t0 + t1) * .5f;

		const Point p0 = EvaluateSize(t0);
		const Point pmid = EvaluatePoint(tmid);
		const Point p1 = EvaluatePoint(t1);

		const Vector vmid = pmid - p0;
		const Vector v = p1 - p0;

		// Check if the vectors are nearly parallel
		if (AbsDot(Normalize(vmid), Normalize(v)) < 1.f - 0.05f) {
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
		const float segmentLength = (p1 - p0).Length();
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

	vector<Point> points;
	vector<float> sizes;
	vector<RGBColor> cols;
	vector<float> transps;
};

//------------------------------------------------------------------------------
// HairFile methods
//------------------------------------------------------------------------------

HairFile::HairFile(const Transform &o2w, bool ro, const string &name, const Point *cameraPos,
		const string &aType,  const TessellationType tType, const u_int rAdaptiveMaxDepth,
		const float rAdaptiveError, const u_int sSideCount,  const bool sCap,
		boost::shared_ptr<cyHairFile> &hair) : Shape(o2w, ro, name) {
	hasCameraPosition = (cameraPos != NULL);
	if (hasCameraPosition) {
		// Transform the camera position in local coordinate
		cameraPosition = Inverse(ObjectToWorld) * (*cameraPos);
	}

	accelType = aType;
	tesselType = tType;
	ribbonAdaptiveMaxDepth = rAdaptiveMaxDepth;
	ribbonAdaptiveError = rAdaptiveError;
	solidSideCount = sSideCount;
	solidCap = sCap;
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
		const vector<float> &hairTransps,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs, vector<float> &meshCols,
		vector<float> &meshTransps) const {
	// Create the mesh vertices
	const u_int baseOffset = meshVerts.size();
	for (int i = 0; i < (int)hairPoints.size(); ++i) {
		Vector z;
		// I need a special case for the very last point
		if (i == (int)hairPoints.size() - 1)
			z = Normalize(hairPoints[i] - hairPoints[i - 1]);
		else
			z = Normalize(hairPoints[i + 1] - hairPoints[i]);

		Vector x, y;
		// Check if I have to face the ribbon in a specific direction
		if (hasCameraPosition) {
			y = Normalize(cameraPosition - hairPoints[i]);

			if (AbsDot(z, y) < 1.f - 0.05f) {
				x = Normalize(Cross(z, y));
				y = Normalize(Cross(x, z));
			} else
				CoordinateSystem(z, &x, &y);
		} else
			CoordinateSystem(z, &x, &y);

		const Point p0 = hairPoints[i] + hairSizes[i] * x;
		const Point p1 = hairPoints[i] - hairSizes[i] * x;
		meshVerts.push_back(p0);
		meshNorms.push_back(Normal());
		meshVerts.push_back(p1);
		meshNorms.push_back(Normal());

		const float v = i / (float)hairPoints.size();
		meshUVs.push_back(1.f);
		meshUVs.push_back(v);
		meshUVs.push_back(-1.f);
		meshUVs.push_back(v);

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
		const Normal n0 = Normal(Cross(meshVerts[i1] - meshVerts[i0], meshVerts[i2] - meshVerts[i0]));
		meshNorms[i0] += n0;
		meshNorms[i1] += n0;
		meshNorms[i2] += n0;

		// Second triangle
		meshTris.push_back(i1);
		meshTris.push_back(i3);
		meshTris.push_back(i2);
		// Second triangle normal
		const Normal n1 = Normal(Cross(meshVerts[i3] - meshVerts[i1], meshVerts[i2] - meshVerts[i1]));
		meshNorms[i1] += n1;
		meshNorms[i2] += n1;
		meshNorms[i3] += n1;
	}
}

void HairFile::TessellateRibbonAdaptive(const vector<Point> &hairPoints,
		const vector<float> &hairSizes, const vector<RGBColor> &hairCols,
		const vector<float> &hairTransps,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs, vector<float> &meshCols,
		vector<float> &meshTransps) const {
	// Interpolate the hair segments
	CatmullRomCurve curve;
	for (int i = 0; i < (int)hairPoints.size(); ++i)
		curve.AddPoint(hairPoints[i], hairSizes[i], hairCols[i], hairTransps[i]);

	// Tessellate the curve
	vector<float> values;
	curve.AdaptiveTessellate(ribbonAdaptiveMaxDepth, ribbonAdaptiveError, values);

	// Create the ribbon
	vector<Point> tesselPoints;
	vector<float> tesselSizes;
	vector<RGBColor> tesselCols;
	vector<float> tesselTransps;
	for (u_int i = 0; i < values.size(); ++i) {
		tesselPoints.push_back(curve.EvaluatePoint(values[i]));
		tesselSizes.push_back(curve.EvaluateSize(values[i]));
		tesselCols.push_back(curve.EvaluateColor(values[i]));
		tesselTransps.push_back(curve.EvaluateTransparency(values[i]));
	}
	TessellateRibbon(tesselPoints, tesselSizes, tesselCols, tesselTransps,
		meshVerts, meshNorms, meshTris, meshUVs, meshCols, meshTransps);
}

void HairFile::TessellateSolid(const vector<Point> &hairPoints,
		const vector<float> &hairSizes, const vector<RGBColor> &hairCols,
		const vector<float> &hairTransps,
		vector<Point> &meshVerts, vector<Normal> &meshNorms,
		vector<int> &meshTris, vector<float> &meshUVs, vector<float> &meshCols,
		vector<float> &meshTransps) const {
	// Create the mesh vertices
	const u_int baseOffset = meshVerts.size();
	const float angleStep = Radians(360.f / solidSideCount);
	for (int i = 0; i < (int)hairPoints.size(); ++i) {
		Vector z;
		// I need a special case for the very last point
		if (i == (int)hairPoints.size() - 1)
			z = Normalize(hairPoints[i] - hairPoints[i - 1]);
		else
			z = Normalize(hairPoints[i + 1] - hairPoints[i]);

		Vector x, y;
		CoordinateSystem(z, &x, &y);

		float angle = 0.f;
		for (uint j = 0; j < solidSideCount; ++j) {
			const Point lp(hairSizes[i] * cosf(angle), hairSizes[i] * sinf(angle), 0.f);
			const Point p(
				x.x * lp.x + y.x * lp.y + z.x * lp.z + hairPoints[i].x,
				x.y * lp.x + y.y * lp.y + z.y * lp.z + hairPoints[i].y,
				x.z * lp.x + y.z * lp.y + z.z * lp.z + hairPoints[i].z);
			
			meshVerts.push_back(p);
			meshNorms.push_back(Normal());
			meshUVs.push_back(j / (float)solidSideCount);
			meshUVs.push_back(i / (float)hairPoints.size());
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

		for (uint j = 0; j < solidSideCount; ++j) {
			// Side face

			const u_int i0 = index + j;
			const u_int i1 = (j == solidSideCount - 1) ? index : (index + j + 1);
			const u_int i2 = index + j + solidSideCount;
			const u_int i3 = (j == solidSideCount - 1) ? (index + solidSideCount) : (index + j + solidSideCount  + 1);

			// First triangle
			meshTris.push_back(i0);
			meshTris.push_back(i1);
			meshTris.push_back(i2);
			const Normal n0 = Normal(Cross(meshVerts[i1] - meshVerts[i0], meshVerts[i2] - meshVerts[i0]));
			meshNorms[i0] += n0;
			meshNorms[i1] += n0;
			meshNorms[i2] += n0;

			// Second triangle
			meshTris.push_back(i1);
			meshTris.push_back(i3);
			meshTris.push_back(i2);
			const Normal n1 = Normal(Cross(meshVerts[i3] - meshVerts[i0], meshVerts[i2] - meshVerts[i0]));
			meshNorms[i1] += n1;
			meshNorms[i3] += n1;
			meshNorms[i2] += n1;
		}
	}

	if (solidCap) {
		// Add a fan cap

		const u_int offset = meshVerts.size();
		const Normal n = Normal(Normalize(hairPoints[hairPoints.size() - 1] - hairPoints[hairPoints.size() - 2]));
		for (uint j = 0; j < solidSideCount; ++j) {
			meshVerts.push_back(meshVerts[offset - solidSideCount + j]);
			meshNorms.push_back(n);
			meshUVs.push_back(j / (float)solidSideCount);
			meshUVs.push_back(1.f);
			meshCols.push_back(hairCols.back().c[0]);
			meshCols.push_back(hairCols.back().c[1]);
			meshCols.push_back(hairCols.back().c[2]);
			meshTransps.push_back(hairTransps.back());
		}

		// Add the fan center
		meshVerts.push_back(hairPoints.back());
		meshNorms.push_back(n);
		meshUVs.push_back(0.f);
		meshUVs.push_back(1.f);
		meshCols.push_back(hairCols.back().c[0]);
		meshCols.push_back(hairCols.back().c[1]);
		meshCols.push_back(hairCols.back().c[2]);
		meshTransps.push_back(hairTransps.back());

		const u_int i3 = meshVerts.size() - 1;
		for (uint j = 0; j < solidSideCount; ++j) {
			const u_int i0 = offset + j;
			const u_int i1 = (j == solidSideCount - 1) ? offset : (offset + j + 1);

			meshTris.push_back(i0);
			meshTris.push_back(i1);
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

	if (segments || (header.d_segments > 0)) {
		u_int pointIndex = 0;

		vector<Point> hairPoints;
		vector<float> hairSizes;
		vector<RGBColor> hairCols;
		vector<float> hairTransps;

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
			for (int j = 0; j <= segmentSize; ++j) {
				hairPoints.push_back(Point(points[pointIndex], points[pointIndex + 1], points[pointIndex + 2]));
				hairSizes.push_back(((thickness) ? thickness[i] : header.d_thickness) * .5f);
				if (colors)
					hairCols.push_back(RGBColor(colors[pointIndex], colors[pointIndex + 1], colors[pointIndex + 2]));
				else
					hairCols.push_back(RGBColor(header.d_color[0], header.d_color[1], header.d_color[2]));
				if (transparency)
					hairTransps.push_back(1.f - transparency[pointIndex]);
				else
					hairTransps.push_back(1.f - header.d_transparency);

//				if (i % 200 < 100)
//					hairCols.push_back(RGBColor(0.65f, 0.65f, 0.65f));
//				else
//					hairCols.push_back(RGBColor(0.65f, 0.f, 0.f));
//				hairTransps.push_back(1.f - j / (float)segmentSize);

				pointIndex += 3;
			}

			switch (tesselType) {
				case TESSEL_RIBBON:
					TessellateRibbon(hairPoints, hairSizes, hairCols, hairTransps,
							meshVerts, meshNorms, meshTris, meshUVs, meshCols,
							meshTransps);
					break;
				case TESSEL_RIBBON_ADAPTIVE:
					TessellateRibbonAdaptive(hairPoints, hairSizes, hairCols, hairTransps,
							meshVerts, meshNorms, meshTris, meshUVs, meshCols,
							meshTransps);
					break;
				case TESSEL_SOLID:
					TessellateSolid(hairPoints, hairSizes, hairCols, hairTransps,
							meshVerts, meshNorms, meshTris, meshUVs, meshCols,
							meshTransps);
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
	else {
		SHAPE_LOG(name, LUX_WARNING, LUX_BADTOKEN) << "Tessellation type  '" << tessellationTypeStr << "' unknown. Using \"ribbon\".";
		tessellationType = TESSEL_RIBBON;
	}

	const u_int ribbonAdaptiveMaxDepth = max(0, params.FindOneInt("ribbonadaptive_maxdepth", 8));
	const float ribbonAdaptiveError = params.FindOneFloat("ribbonadaptive_error", 0.1f);

	const u_int solidSideCount = max(0, params.FindOneInt("solid_sidecount", 3));
	const bool solidCap = params.FindOneBool("solid_cap", false);

	boost::shared_ptr<cyHairFile> hairFile(new cyHairFile());
	int hairCount = hairFile->LoadFromFile(filename.c_str());
	if (hairCount <= 0) {
		SHAPE_LOG("hairfile", LUX_ERROR, LUX_SYSTEM) << "Unable to read hair file '" << filename << "'";
		return NULL;
	}

	return new HairFile(o2w, reverseOrientation, name, cameraPos, accelType, tessellationType,
		ribbonAdaptiveMaxDepth, ribbonAdaptiveError, solidSideCount, solidCap, hairFile);
}

static DynamicLoader::RegisterShape<HairFile> r("hairfile");
