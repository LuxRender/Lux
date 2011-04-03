#ifndef stlmesh_h
#define stlmesh_h

#include "shape.h"
#include "paramset.h"

namespace lux
{

// PlyMesh Declarations
class StlMesh {
public:
	// StlMesh Public Methods
	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
};

}//namespace lux

#endif // stlmesh_h
