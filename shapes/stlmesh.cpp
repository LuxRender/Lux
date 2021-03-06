#include "stlmesh.h"

#include "paramset.h"
#include "context.h"
#include "dynload.h"
#include "mesh.h"

#include <climits>
#include <cfloat>
#include <cstdio>
using std::fopen;
using std::fclose;
using std::fread;
using std::ftell;

#define MAX_STL_FILE_FACES	(16*1024*1024) // 16Mb

#ifndef _MSC_VER

	inline int stricmp(const char* a, const char* b)
	{
		for( ; *a || *b ; a++, b++)
		{
			if(int d = toupper(*a) - toupper(*b))
				return d;
		}
		
		return 0;
	}

#endif // _MSC_VER

namespace lux
{

Shape* StlMesh::CreateShape(const Transform &o2w,
							bool reverseOrientation,
							const ParamSet &params)
{
	string name = params.FindOneString("name", "'stlmesh'");
	string FileName = params.FindOneString("filename", "none");

	string subdivscheme = params.FindOneString("subdivscheme", "loop");

	int nsubdivlevels = params.FindOneInt("nsubdivlevels", 0);

	bool recenterMesh = params.FindOneBool("recenter_mesh", false);

	Mesh::MeshSubdivType subdivType;

	if (subdivscheme == "loop")
		subdivType = Mesh::SUBDIV_LOOP;
	else if (subdivscheme == "microdisplacement")
		subdivType = Mesh::SUBDIV_MICRODISPLACEMENT;
	else
	{
		SHAPE_LOG(name, LUX_WARNING,LUX_BADTOKEN) << "Subdivision type  '" << subdivscheme << "' unknown. Using \"loop\".";
		subdivType = Mesh::SUBDIV_LOOP;
	}

	SHAPE_LOG(name, LUX_INFO, LUX_NOERROR) << "Loading STL mesh file: '" << FileName << "'...";

	FILE* pFile = fopen(FileName.c_str(), "rb");

	if(!pFile)
	{
		SHAPE_LOG(name, LUX_ERROR, LUX_SYSTEM) << "Unable to read STL mesh file '" << FileName << "'";
		return NULL;
	}

	uint32_t uNFaces;

	std::vector<Point> Vertices;

	// Checking file format (ASCII or binary)
	bool bIsBinary = false;	

	do
	{
		fseek(pFile, 0, SEEK_END);

		uint32_t uLength = ftell(pFile);

		if (80 + sizeof(uint32_t) >= uLength)
			break;

		fseek(pFile, 80, SEEK_SET);		

		if (fread(&uNFaces, sizeof(uNFaces), 1, pFile) != 1)
			break;

		if (!uNFaces || uNFaces > MAX_STL_FILE_FACES)
			break;

		if (80 + sizeof(uint32_t) + uNFaces * (sizeof(float) * 3 * 4 + sizeof(uint16_t)) != uLength)
			break;

		bIsBinary = true;

	}while(false);	

	if(bIsBinary) // binary mode loader
	{
		fseek(pFile, 80, SEEK_SET);

		if(fread(&uNFaces, sizeof(uNFaces), 1, pFile) != 1)
		{
			fclose(pFile);
			SHAPE_LOG(name, LUX_ERROR, LUX_SYSTEM) << "Invalid STL mesh file '" << FileName << "'";
			return NULL;
		}

		Vertices.resize(uNFaces * 3);

		const size_t szFaceSize = sizeof(float) * 3 * 4 + sizeof(uint16_t);

		// Preloading file
		std::vector<uint8_t> Data(uNFaces * szFaceSize);

		if(fread(&Data[0], 1, Data.size(), pFile) != Data.size())
		{
			fclose(pFile);
			SHAPE_LOG(name, LUX_ERROR, LUX_SYSTEM) << "Invalid STL mesh file '" << FileName << "'";
			return NULL;
		}

		for(uint32_t i = 0 ; i < uNFaces ; i++)
		{
			const float* pBaseData = reinterpret_cast<float*>(&Data[0] + i * szFaceSize);

			Vertices[i*3 + 0].x = pBaseData[3];
			Vertices[i*3 + 0].y = pBaseData[4];
			Vertices[i*3 + 0].z = pBaseData[5];

			Vertices[i*3 + 1].x = pBaseData[6];
			Vertices[i*3 + 1].y = pBaseData[7];
			Vertices[i*3 + 1].z = pBaseData[8];

			Vertices[i*3 + 2].x = pBaseData[9];
			Vertices[i*3 + 2].y = pBaseData[10];
			Vertices[i*3 + 2].z = pBaseData[11];
		}

		fclose(pFile);
	}
	else // ASCII mode loader
	{
		// Reopening in text mode
		fclose(pFile);

		if(!(pFile = fopen(FileName.c_str(), "rt")))
		{
			SHAPE_LOG(name, LUX_ERROR, LUX_SYSTEM) << "Unable to read STL mesh file '" << FileName << "'";
			return NULL;
		}

		// Reading file
		char Token[128];

		uNFaces = 0;

		while(!feof(pFile))
		{
			if(fscanf(pFile, "%127s", Token) != 1)
			{
				fclose(pFile);
				SHAPE_LOG(name, LUX_ERROR, LUX_SYSTEM) << "Invalid STL mesh file '" << FileName << "'";
				return NULL;
			}
			
			Token[127] = 0;

			if(!stricmp(Token, "endsolid"))
				break;

			if(!stricmp(Token, "facet"))
				uNFaces++;
		}

		Vertices.resize(uNFaces * 3);

		// Rewinding file and skipping 'solid ...' header
		fseek(pFile, 0, SEEK_SET);
		fgets(Token, sizeof(Token), pFile);

		// Reading faces
		for(uint32_t i = 0 ; i < uNFaces ; i++)
		{
			fscanf(pFile, "%*s %*s %*f %*f %*f %*s %*s"); // facet normal [nx] [ny] [nz] outer loop

			if(	fscanf(	pFile,
						"%*s %f %f %f",
							&Vertices[i*3 + 0].x,
							&Vertices[i*3 + 0].y,
							&Vertices[i*3 + 0].z) != 3 || // vertex [v1] [v2] [v3]

				fscanf(	pFile,
						"%*s %f %f %f",
							&Vertices[i*3 + 1].x,
							&Vertices[i*3 + 1].y,
							&Vertices[i*3 + 1].z) != 3 || // vertex [v1] [v2] [v3]

				fscanf(	pFile,
						"%*s %f %f %f",
							&Vertices[i*3 + 2].x,
							&Vertices[i*3 + 2].y,
							&Vertices[i*3 + 2].z) != 3) // vertex [v1] [v2] [v3]
			{
				fclose(pFile);
				SHAPE_LOG(name, LUX_ERROR, LUX_SYSTEM) << "Invalid STL mesh file '" << FileName << "'";
				return NULL;
			}

			fscanf(pFile, "%*s %*s"); // endloop endfacet
		}

		fclose(pFile);
	}

	if (recenterMesh)
	{
		// Bringing bounding box center to (0,0,0)
		BBox bbox;

		for(size_t i = 0 ; i < Vertices.size() ; i++)
			bbox = Union(bbox, BBox(Vertices[i]));

		Vector centerV((bbox.pMin + bbox.pMax) * 0.5f);

		for(size_t i = 0 ; i < Vertices.size() ; i++)
			Vertices[i] -= centerV;
	}

	// Filling face indices
	vector<int> Faces(uNFaces * 3);

	for(uint32_t i = 0 ; i < uNFaces ; i++)
	{
		Faces[i*3 + 0] = i*3 + 0;
		Faces[i*3 + 1] = i*3 + 1;
		Faces[i*3 + 2] = i*3 + 2;
	}

	boost::shared_ptr<Texture<float> > displacementMap;

	return new Mesh(o2w, reverseOrientation, name, Mesh::ACCEL_AUTO,
					Vertices.size(), &Vertices[0], NULL, NULL, NULL, NULL, 1.f,
					Mesh::TRI_AUTO, uNFaces, &Faces[0],
					Mesh::QUAD_QUADRILATERAL, 0, NULL,
					subdivType, nsubdivlevels, displacementMap, 0.1f, 0.0f, true, false,
					false, false);
}

static DynamicLoader::RegisterShape<StlMesh> r("stlmesh");

}//namespace lux
