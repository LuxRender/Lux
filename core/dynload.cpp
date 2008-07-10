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

// dynload.cpp*
#include "kdtree.h"

#include "dynload.h"
#include "paramset.h"
#include "light.h"
#include "shape.h"
#include "material.h"
#include "texture.h"
/*
 * #ifndef WIN32
 * #ifndef __APPLE__
 * #include <dlfcn.h>
 * #else
 * #include <mach-o/dyld.h>
 * #endif
 * #endif
 * #include <map>*/



#include "cone.h"
#include "cylinder.h"
#include "disk.h"
#include "heightfield.h"
#include "hyperboloid.h"
#include "loopsubdiv.h"
#include "nurbs.h"
#include "paraboloid.h"
#include "sphere.h"
#include "torus.h"
#include "barytrianglemesh.h"
#include "waldtrianglemesh.h"
#include "plymesh.h"
#include "lenscomponent.h"
#include "quad.h"

#include "lowdiscrepancy.h"
#include "halton.h"
#include "random.h"
#include "metrosampler.h"
#include "erpt.h"

#include "environment.h"
#include "orthographic.h"
#include "perspective.h"
#include "realistic.h"

#include "fleximage.h"

#include "box.h"
#include "gaussian.h"
#include "mitchell.h"
#include "sinc.h"
#include "triangle.h"

#include "directlighting.h"
#include "path.h"
#include "path2.h"
#include "particletracing.h"
#include "bidirectional.h"
#include "exphotonmap.h"
#include "importancepath.h"
#include "distributedpath.h"

#include "emission.h"
#include "single.h"

#include "distant.h"
#include "goniometric.h"
#include "infinitesample.h"
#include "infinite.h"
#include "point.h"
#include "projection.h"
#include "spot.h"
#include "sun.h"
#include "sky.h"

#include "glass.h"
#include "roughglass.h"
#include "matte.h"
#include "mattetranslucent.h"
#include "mirror.h"
#include "plastic.h"
#include "shinymetal.h"
#include "substrate.h"
#include "carpaint.h"
#include "metal.h"
#include "null.h"
#include "mixmaterial.h"

#include "bilerp.h"
#include "checkerboard.h"
#include "constant.h"
#include "dots.h"
#include "fbm.h"
#include "imagemap.h"
#include "marble.h"
#include "mix.h"
#include "scale.h"
#include "uv.h"
#include "windy.h"
#include "wrinkled.h"
#include "harlequin.h"
#include "blender_musgrave.h"
#include "blender_marble.h"
#include "blender_wood.h"
#include "blender_clouds.h"
#include "blender_blend.h"
#include "blender_distortednoise.h"
#include "blender_noise.h"
#include "blender_magic.h"
#include "blender_stucci.h"
#include "blender_voronoi.h"

#include "contrast.h"
#include "highcontrast.h"
#include "maxwhite.h"
#include "nonlinear.h"
#include "reinhard.h"

#include "exponential.h"
#include "homogeneous.h"
#include "volumegrid.h"

#include "grid.h"
#include "unsafekdtreeaccel.h"
#include "tabreckdtreeaccel.h"
#include "bruteforce.h"

//
// Radiance - The following plugins are currently retired to /PBRT_Attic
//
/*
 * #include "debug.h"
 * #include "exphotonmap.h"
 * #include "igi.h"
 * #include "irradiancecache.h"
 * #include "photonmap.h"
 * #include "whitted.h"
 * #include "bidirectional.h"
 *
 * #include "image.h"
 *
 * #include "stratified.h"
 * #include "bestcandidate.h"
 */

using std::map;

namespace lux {
    
    
boost::shared_ptr<Shape> MakeShape(const string &name,
        const Transform &object2world,
        bool reverseOrientation,
        const ParamSet &paramSet,
		map<string, boost::shared_ptr<Texture<float> > > *floatTextures) {
    /*
ShapePlugin *plugin =
GetPlugin<ShapePlugin>(name,
                               shapePlugins,
                               PluginSearchPath);
if (plugin)
return plugin->CreateShape(object2world,
                         reverseOrientation,
                                      paramSet);*/

    if(name=="cone")
        return boost::shared_ptr<Shape>(Cone::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="cylinder")
        return boost::shared_ptr<Shape>(Cylinder::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="disk")
        return boost::shared_ptr<Shape>(Disk::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="heightfield")
        return boost::shared_ptr<Shape>(Heightfield::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="hyperboloid")
        return boost::shared_ptr<Shape>(Hyperboloid::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="loopsubdiv")
        return boost::shared_ptr<Shape>(LoopSubdiv::CreateShape(object2world, reverseOrientation, paramSet, floatTextures));
    if(name=="nurbs")
        return boost::shared_ptr<Shape>(NURBS::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="paraboloid")
        return boost::shared_ptr<Shape>(Paraboloid::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="sphere")
        return boost::shared_ptr<Shape>(Sphere::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="torus")
        return boost::shared_ptr<Shape>(Torus::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="barytrianglemesh")
        return boost::shared_ptr<Shape>(BaryTriangleMesh::CreateShape(object2world, reverseOrientation, paramSet));
    if((name=="waldtrianglemesh") || (name=="trianglemesh"))
        return boost::shared_ptr<Shape>(WaldTriangleMesh::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="plymesh")
        return boost::shared_ptr<Shape>(PlyMesh::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="lenscomponent")
        return boost::shared_ptr<Shape>(LensComponent::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="quad")
        return boost::shared_ptr<Shape>(Quad::CreateShape(object2world, reverseOrientation, paramSet));
    //Error("Static loading of shape '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of shape '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    boost::shared_ptr<Shape> o;
    return o;
}

/*
static string SearchPath(const string &searchpath,
                     const string &filename)
{
const char *start = searchpath.c_str();
const char *end = start;
while (*start)
{
    while (*end && *end != LUX_PATH_SEP[0])
        ++end;
    string component(start, end);

    string fn = component + "/" + filename;
    FILE *f = fopen(fn.c_str(), "r");
    if (f)
    {
        fclose(f);
        return fn;
    }
    if (*end == LUX_PATH_SEP[0])
        ++end;
    start = end;
}
return "";
}*/

boost::shared_ptr<Material> MakeMaterial(const string &name,
        const Transform &mtl2world,
        const TextureParams &mp) {
    /*MaterialPlugin *plugin = GetPlugin<MaterialPlugin>(name, materialPlugins,
                         PluginSearchPath);
if (plugin)
{
    boost::shared_ptr<Material> ret =
        plugin->CreateMaterial(mtl2world, mp);
    mp.ReportUnused();
    return ret;
}*/
    if(name=="glass") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(Glass::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="roughglass") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(RoughGlass::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="matte") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(Matte::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="mattetranslucent") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(MatteTranslucent::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="mirror") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(Mirror::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="plastic") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(Plastic::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="shinymetal") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(ShinyMetal::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="substrate") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(Substrate::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="carpaint") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(CarPaint::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="metal") {
        boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(Metal::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="null")
    {
    	boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(Null::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }    
    if(name=="mix")
    {
    	boost::shared_ptr<Material> ret = boost::shared_ptr<Material>(MixMaterial::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }    


    //Error("Static loading of material '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of material '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    boost::shared_ptr<Material> o;
    return o;
}

boost::shared_ptr< Texture<float> > MakeFloatTexture(const string &name,
        const Transform &tex2world, const TextureParams &tp) {
    /*TexturePlugin *plugin = GetPlugin<TexturePlugin>(name, texturePlugins,
                        PluginSearchPath);
if (plugin)
{
    boost::shared_ptr<Texture<float> > ret =
        plugin->CreateFloatTex(tex2world, tp);
    tp.ReportUnused();
    return ret;
}*/
    if(name=="bilerp") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BilerpTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="checkerboard") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(Checkerboard::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="constant") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(Constant::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="dots") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(DotsTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="fbm") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(FBmTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="imagemap") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(ImageTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="marble") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(MarbleTexture::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="mix") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(MixTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="scale") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(ScaleTexture<float, float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="uv") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(UVTexture::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="windy") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(WindyTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="wrinkled") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(WrinkledTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_musgrave") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderMusgraveTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_marble") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderMarbleTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_wood") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderWoodTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_clouds") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderCloudsTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_blend") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderBlendTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_distortednoise") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderDistortedNoiseTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_stucci") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderStucciTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_noise") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderNoiseTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_magic") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderMagicTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_voronoi") {
        boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BlenderVoronoiTexture3D<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    //Error("Static loading of float texture '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of texture '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    boost::shared_ptr<Texture<float> > o;
    return o;
}

boost::shared_ptr<Texture<Spectrum> > MakeSpectrumTexture(const string &name,
        const Transform &tex2world, const TextureParams &tp) {
    /*TexturePlugin *plugin = GetPlugin<TexturePlugin>(name, texturePlugins,
                        PluginSearchPath);
if (plugin)
{
    boost::shared_ptr<Texture<Spectrum> > ret =
        plugin->CreateSpectrumTex(tex2world, tp);
    tp.ReportUnused();
    return ret;
}*/
    if(name=="bilerp") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BilerpTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="checkerboard") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(Checkerboard::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="constant") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(Constant::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="dots") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(DotsTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="fbm") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(FBmTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="imagemap") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(ImageTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="marble") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(MarbleTexture::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="mix") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(MixTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="scale") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(ScaleTexture<Spectrum, Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="uv") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(UVTexture::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="windy") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(WindyTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="wrinkled") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(WrinkledTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="harlequin") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(HarlequinTexture::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_musgrave") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderMusgraveTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_marble") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderMarbleTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_wood") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderWoodTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_clouds") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderCloudsTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_blend") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderBlendTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_distortednoise") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderDistortedNoiseTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_stucci") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderStucciTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_noise") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderNoiseTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_magic") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderMagicTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="blender_voronoi") {
        boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BlenderVoronoiTexture3D<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    //Error("Static loading of spectrum texture '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of spectrum texture '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    boost::shared_ptr<Texture<Spectrum> > o;
    return o;
}

Light *MakeLight(const string &name,
        const Transform &light2world, const ParamSet &paramSet) {
    /*LightPlugin *plugin = GetPlugin<LightPlugin>(name, lightPlugins, PluginSearchPath);
if (plugin)
{
    Light *ret = plugin->CreateLight(light2world, paramSet);
    paramSet.ReportUnused();
    return ret;
}*/
    if(name=="distant") {
        Light *ret = DistantLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="goniometric") {
        Light *ret = GonioPhotometricLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="infinitesample") {
        Light *ret = InfiniteAreaLightIS::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="infinite") {
        Light *ret = InfiniteAreaLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="point") {
        Light *ret = PointLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="projection") {
        Light *ret = ProjectionLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="spot") {
        Light *ret = SpotLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="sun") {
        Light *ret = SunLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="sky") {
        Light *ret = SkyLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    //Error("Static loading of light '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of light '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

AreaLight *MakeAreaLight(const string &name,
        const Transform &light2world, const ParamSet &paramSet,
        const boost::shared_ptr<Shape> &shape) {
    /*AreaLightPlugin *plugin = GetPlugin<AreaLightPlugin>(name, arealightPlugins,
                          PluginSearchPath);
if (plugin)
{
    AreaLight *ret = plugin->CreateAreaLight(light2world, paramSet, shape);
    paramSet.ReportUnused();
    return ret;
}*/

    if(name=="area") {
        AreaLight *ret = AreaLight::CreateAreaLight(light2world, paramSet, shape);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of area light '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of area light '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

VolumeRegion *MakeVolumeRegion(const string &name,
        const Transform &volume2world, const ParamSet &paramSet) {
    /*VolumeRegionPlugin *plugin = GetPlugin<VolumeRegionPlugin>(name, volumePlugins,
                             PluginSearchPath);
if (plugin)
{
    VolumeRegion *ret = plugin->CreateVolumeRegion(volume2world, paramSet);
    paramSet.ReportUnused();
    return ret;
}*/

    if(name=="exponential") {
        VolumeRegion *ret = ExponentialDensity::CreateVolumeRegion(volume2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="homogeneous") {
        VolumeRegion *ret = HomogeneousVolume::CreateVolumeRegion(volume2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="volumegrid") {
        VolumeRegion *ret = VolumeGrid::CreateVolumeRegion(volume2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of volume region '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of volume region '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

SurfaceIntegrator *MakeSurfaceIntegrator(const string &name,
        const ParamSet &paramSet) {
    /*SurfaceIntegratorPlugin *plugin = GetPlugin<SurfaceIntegratorPlugin>(name,
                                  surf_integratorPlugins, PluginSearchPath);
if (plugin)
{
    SurfaceIntegrator *ret = plugin->CreateSurfaceIntegrator(paramSet);
    paramSet.ReportUnused();
    return ret;
}*/

    if(name=="directlighting") {
        SurfaceIntegrator *ret=DirectLighting::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="path") {
        SurfaceIntegrator *ret=PathIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="path2") {
        SurfaceIntegrator *ret=Path2Integrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="particletracing") {
        SurfaceIntegrator *ret=ParticleTracingIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="bidirectional") {
        SurfaceIntegrator *ret=BidirIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="exphotonmap") {
        SurfaceIntegrator *ret=ExPhotonIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="importancepath") {
        SurfaceIntegrator *ret=ImportancePathIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="distributedpath") {
        SurfaceIntegrator *ret=DistributedPath::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

	//
    // Radiance - Volume integrators currently disabled
    // due to multithreading implementation which is in progress.
    //
    /*if(name=="single")
{
    SurfaceIntegrator *ret=SingleScattering::CreateSurfaceIntegrator(paramSet);
    paramSet.ReportUnused();
    return ret;
}*/
    /*if(name=="emission")
{
    SurfaceIntegrator *ret=EmissionIntegrator::CreateSurfaceIntegrator(paramSet);
    paramSet.ReportUnused();
    return ret;
}*/

    //Error("Static loading of surface integrator '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of surface integrator '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

VolumeIntegrator *MakeVolumeIntegrator(const string &name,
        const ParamSet &paramSet) {
    /*
VolumeIntegratorPlugin *plugin = GetPlugin<VolumeIntegratorPlugin>(name, vol_integratorPlugins,
                                 PluginSearchPath);
if (plugin)
{
    VolumeIntegrator *ret = plugin->CreateVolumeIntegrator(paramSet);
    paramSet.ReportUnused();
    return ret;
}*/

    if(name=="single") {
        VolumeIntegrator *ret=SingleScattering::CreateVolumeIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    if(name=="emission") {
        VolumeIntegrator *ret=EmissionIntegrator::CreateVolumeIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of volume integrator '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of volume integrator '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

Primitive *MakeAccelerator(const string &name, const vector<Primitive* > &prims, const ParamSet &paramSet) {
    /*
   AcceleratorPlugin *plugin = GetPlugin<AcceleratorPlugin>(name, acceleratorPlugins,
                               PluginSearchPath);
   if (plugin)
   {
       Primitive *ret = plugin->CreateAccelerator(prims, paramSet);
       paramSet.ReportUnused();
       return ret;
   }*/
    //Primitive* ret;

    if(name=="unsafekdtree") {
        Primitive* ret=UnsafeKdTreeAccel::CreateAccelerator(prims, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if((name=="tabreckdtree") || (name=="kdtree")) {
        Primitive* ret=TaBRecKdTreeAccel::CreateAccelerator(prims, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="grid") {
        Primitive* ret=GridAccel::CreateAccelerator(prims, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    // NOTE - ratow - Added a brute force (de)accelerator for debugging purposes.
    if(name=="none") {
        Primitive* ret=BruteForceAccel::CreateAccelerator(prims, paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of accelerator '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of accelerator '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

Camera *MakeCamera(const string &name,
        const ParamSet &paramSet,
        const Transform &world2cam, Film *film) {
    /*
CameraPlugin *plugin = GetPlugin<CameraPlugin>(name, cameraPlugins, PluginSearchPath);
if (plugin)
{
    Camera *ret = plugin->CreateCamera(paramSet, world2cam, film);
    paramSet.ReportUnused();
    return ret;
}*/
    if(name=="environment") {
        Camera *ret=EnvironmentCamera::CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="orthographic") {
        Camera *ret=OrthoCamera::CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="perspective") {
        Camera *ret=PerspectiveCamera::CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="realistic") {
        Camera *ret=RealisticCamera::CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of camera '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of camera '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

Sampler *MakeSampler(const string &name,
        const ParamSet &paramSet, const Film *film) {
    /*
SamplerPlugin *plugin = GetPlugin<SamplerPlugin>(name, samplerPlugins, PluginSearchPath);
if (plugin) {
Sampler *ret = plugin->CreateSampler(paramSet, film);
paramSet.ReportUnused();
return ret;
}*/
    //Sampler *ret;

    if(name=="lowdiscrepancy") {
        Sampler *ret=LDSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="random") {
        Sampler *ret=RandomSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="halton") {
        Sampler *ret=HaltonSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="metropolis") {
        Sampler *ret=MetropolisSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="erpt") {
        Sampler *ret=ERPTSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }

    //
    // Radiance - The following samplers are currently retired to /PBRT_Attic
    //
    /*
if(name=="bestcandidate")
{
    Sampler *ret=BestCandidateSampler::CreateSampler(paramSet, film);
    paramSet.ReportUnused();
    return ret;
}
if(name=="stratified")
{
    Sampler *ret=StratifiedSampler::CreateSampler(paramSet, film);
    paramSet.ReportUnused();
    return ret;
} */

    //Error("Static loading of sampler '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of sampler '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

Filter *MakeFilter(const string &name,
        const ParamSet &paramSet) {
    /*FilterPlugin *plugin = GetPlugin<FilterPlugin>(name, filterPlugins, PluginSearchPath);
if (plugin)
{
    Filter *ret = plugin->CreateFilter(paramSet);
    paramSet.ReportUnused();
    return ret;
}*/

    if(name=="box") {
        Filter *ret=BoxFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    if(name=="gaussian") {
        Filter *ret=GaussianFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    if(name=="mitchell") {
        Filter *ret=MitchellFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    if(name=="sinc") {
        Filter *ret=LanczosSincFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    if(name=="triangle") {
        Filter *ret=TriangleFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of filter '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of fiter '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

ToneMap *MakeToneMap(const string &name,
        const ParamSet &paramSet) {
    /*ToneMapPlugin *plugin = GetPlugin<ToneMapPlugin>(name, tonemapPlugins, PluginSearchPath);
if (plugin)
{
    ToneMap *ret = plugin->CreateToneMap(paramSet);
     * paramSet.ReportUnused();
     * return ret;
     * }*/
    if(name=="contrast") {
        ToneMap *ret=ContrastOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="highcontrast") {
        ToneMap *ret=HighContrastOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="maxwhite") {
        ToneMap *ret=MaxWhiteOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="nonlinear") {
        ToneMap *ret=NonLinearOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="reinhard") {
        ToneMap *ret=ReinhardOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of tonemap '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of tonemap '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}

Film *MakeFilm(const string &name,
        const ParamSet &paramSet, Filter *filter) {
    /*FilmPlugin *plugin = GetPlugin<FilmPlugin>(name, filmPlugins, PluginSearchPath);
     * if (plugin)
     * {
     * Film *ret = plugin->CreateFilm(paramSet, filter);
     * paramSet.ReportUnused();
     * return ret;
     * }*/

    // note - radiance - MultiImageFilm is removed, leaving this for backward scenefile compatibility

    if((name == "fleximage") || (name == "multiimage")) {
        Film *ret = FlexImageFilm::CreateFilm(paramSet, filter);
        paramSet.ReportUnused();
        return ret;
    }

    //
    // Radiance - The following films are currently retired to /PBRT_Attic
    //
    /*
     * if(name=="image")
     * {
     * Film *ret=ImageFilm::CreateFilm(paramSet, filter);
     * paramSet.ReportUnused();
     * return ret;
     * }
     */
    //Error("Static loading of film '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of film '"<<name<<"' failed.";
    luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
    return NULL;
}
    
}//namespace lux

