/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// dynload.cpp*
#include "kdtree.h"

#include "dynload.h"
#include "paramset.h"
#include "shape.h"
#include "material.h"
#include "texture.h"
/*
#ifndef WIN32
#ifndef __APPLE__
#include <dlfcn.h>
#else
#include <mach-o/dyld.h>
#endif
#endif
#include <map>*/



#include "cone.h"
#include "cylinder.h"
#include "disk.h"
#include "heightfield.h"
#include "hyperboloid.h"
#include "loopsubdiv.h"
#include "nurbs.h"
#include "paraboloid.h"
#include "sphere.h"
#include "trianglemesh.h"

//#include "bestcandidate.h"
#include "lowdiscrepancy.h"
#include "halton.h"
#include "random.h"
//#include "stratified.h"

#include "environment.h"
#include "orthographic.h"
#include "perspective.h"

#include "image.h"
#include "multiimage.h"

#include "box.h"
#include "gaussian.h"
#include "mitchell.h"
#include "sinc.h"
#include "triangle.h"

#include "bidirectional.h"
#include "debug.h"
#include "directlighting.h"
#include "emission.h"
#include "exphotonmap.h"
#include "igi.h"
#include "irradiancecache.h"
#include "path.h"
#include "photonmap.h"
#include "single.h"
#include "whitted.h"

#include "distant.h"
#include "goniometric.h"
#include "infinitesample.h"
#include "infinite.h"
#include "point.h"
#include "projection.h"
#include "spot.h"
#include "sun.h"
#include "sun2.h"
#include "sun3.h"
#include "sky.h"

#include "bluepaint.h"
#include "brushedmetal.h"
#include "clay.h"
#include "felt.h"
#include "glass.h"
#include "roughglass.h"
#include "matte.h"
#include "mirror.h"
#include "plastic.h"
#include "primer.h"
#include "shinymetal.h"
#include "skin.h"
#include "substrate.h"
#include "translucent.h"
#include "uber.h"
#include "carpaint.h"
#include "metal.h"

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

#include "contrast.h"
#include "highcontrast.h"
#include "maxwhite.h"
#include "nonlinear.h"
#include "reinhard.h"

#include "exponential.h"
#include "homogeneous.h"
#include "volumegrid.h"

#include "grid.h"
#include "kdtreeaccel.h"
#include "bruteforce.h"

using std::map;
/*
// Runtime Loading Forward Declarations
static string SearchPath(const string &searchpath,  // NOBOOK
                         const string &filename); // NOBOOK
template <class D>
D *GetPlugin(const string &name,
             map<string, D *> &loadedPlugins,
             const string &searchPath)
{
    if (loadedPlugins.find(name) != loadedPlugins.end())
        return loadedPlugins[name];
    string filename = name;
    // Add platform-specific shared library filename suffix
#ifdef WIN32

    filename += ".dll";
#else

    filename += ".so";
#endif

    string path = SearchPath(searchPath, filename);
    D *plugin = NULL;
    if (path != "")
        loadedPlugins[name] = plugin = new D(path.c_str());
    else
        Error("Unable to find Plugin/DLL for \"%s\"",
              name.c_str());
    return plugin;
}
class ShapePlugin;
class FilterPlugin;
class MaterialPlugin;
class TexturePlugin;
class FilmPlugin;
class LightPlugin;
class AreaLightPlugin;
class VolumeRegionPlugin;
class SurfaceIntegratorPlugin;
class VolumeIntegratorPlugin;
class ToneMapPlugin;
class AcceleratorPlugin;
class CameraPlugin;
class SamplerPlugin;
static string SearchPath(const string &searchpath,
                         const string &filename);
// Runtime Loading Static Data
static string PluginSearchPath;
static map<string, ShapePlugin *> shapePlugins;
static map<string, FilterPlugin *> filterPlugins;
static map<string, MaterialPlugin *> materialPlugins;
static map<string, TexturePlugin *> texturePlugins;
static map<string, FilmPlugin *> filmPlugins;
static map<string, LightPlugin *> lightPlugins;
static map<string, AreaLightPlugin *> arealightPlugins;
static map<string, VolumeRegionPlugin *> volumePlugins;
static map<string, SurfaceIntegratorPlugin *> surf_integratorPlugins;
static map<string, VolumeIntegratorPlugin *> vol_integratorPlugins;
static map<string, ToneMapPlugin *> tonemapPlugins;
static map<string, AcceleratorPlugin *> acceleratorPlugins;
static map<string, CameraPlugin *> cameraPlugins;
static map<string, SamplerPlugin *> samplerPlugins;
// Runtime Loading Local Classes
class Plugin
{
public:
    // Plugin Public Methods
    Plugin(const string &fname);
    ~Plugin();
    void *GetSymbol(const string &symname);
private:
    // Plugin Private Data
#if defined(WIN32)

    HMODULE hinstLib;
#elif defined(__APPLE__)

    NSModule hinstLib;
#else

    void *hinstLib;
#endif

    string pluginName;
};
class ShapePlugin : public Plugin
{
public:
    // ShapePlugin Constructor
    ShapePlugin(const string &name)
            : Plugin(name)
    {
        CreateShape =
            (CreateShapeFunc)(GetSymbol("CreateShape"));
    }
    typedef Shape * (*CreateShapeFunc)(const Transform &o2w,
                                       bool reverseOrientation, const ParamSet &params);
    CreateShapeFunc CreateShape;
};
class MaterialPlugin : public Plugin
{
    typedef Material * (*CreateMaterialFunc)(const Transform &shader2world, const TextureParams &);
public:
    MaterialPlugin( const string &name ): Plugin( name )
    {
        CreateMaterial =
            (CreateMaterialFunc)(GetSymbol("CreateMaterial"));
    }
    CreateMaterialFunc CreateMaterial;
};
class TexturePlugin : public Plugin
{
    typedef Texture<float> *
    (*CreateFloatFunc)(const Transform &shader2world, const TextureParams &);
    typedef Texture<Spectrum> *
    (*CreateSpectrumFunc)(const Transform &shader2world, const TextureParams &);
public:
    TexturePlugin( const string &name ): Plugin( name )
    {
        CreateFloatTex =
            (CreateFloatFunc)(GetSymbol("CreateFloatTexture"));
        CreateSpectrumTex =
            (CreateSpectrumFunc)(GetSymbol("CreateSpectrumTexture"));
    }
    CreateFloatFunc CreateFloatTex;
    CreateSpectrumFunc CreateSpectrumTex;
};
class LightPlugin : public Plugin
{
    typedef Light *(*CreateLightFunc)(
        const Transform &light2world, const ParamSet &params);
public:
    LightPlugin( const string &name ): Plugin( name )
    {
        CreateLight =
            (CreateLightFunc)(GetSymbol("CreateLight"));
    }
    CreateLightFunc CreateLight;
};
class VolumeRegionPlugin : public Plugin
{
    typedef VolumeRegion *(*CreateVolumeFunc)(const Transform &volume2world,
            const ParamSet &params);
public:
    VolumeRegionPlugin( const string &name ): Plugin( name )
    {
        CreateVolumeRegion =
            (CreateVolumeFunc)(GetSymbol("CreateVolumeRegion"));
    }
    CreateVolumeFunc CreateVolumeRegion;
};
class AreaLightPlugin : public Plugin
{
    typedef AreaLight *(*CreateAreaLightFunc)(
        const Transform &light2world, const ParamSet &params,
        const ShapePtr &shape);
public:
    AreaLightPlugin( const string &name ): Plugin( name )
    {
        CreateAreaLight =
            (CreateAreaLightFunc)(GetSymbol("CreateAreaLight"));
    }
    CreateAreaLightFunc CreateAreaLight;
};
class SurfaceIntegratorPlugin : public Plugin
{
    typedef SurfaceIntegrator *(*CreateSurfaceIntegratorFunc)(const ParamSet &params);
public:
    SurfaceIntegratorPlugin( const string &name ): Plugin( name )
    {
        CreateSurfaceIntegrator =
            (CreateSurfaceIntegratorFunc)(GetSymbol("CreateSurfaceIntegrator"));
    }
    CreateSurfaceIntegratorFunc CreateSurfaceIntegrator;
};
class VolumeIntegratorPlugin : public Plugin
{
    typedef VolumeIntegrator *(*CreateVolumeIntegratorFunc)(const ParamSet &params);
public:
    VolumeIntegratorPlugin( const string &name ): Plugin( name )
    {
        CreateVolumeIntegrator =
            (CreateVolumeIntegratorFunc)(GetSymbol("CreateVolumeIntegrator"));
    }
    CreateVolumeIntegratorFunc CreateVolumeIntegrator;
};
class AcceleratorPlugin : public Plugin
{
    typedef Primitive *(*CreateAcceleratorFunc)(const vector<Primitive* > &prims, const ParamSet &params);
public:
    AcceleratorPlugin( const string &name ): Plugin( name )
    {
        CreateAccelerator =
            (CreateAcceleratorFunc)(GetSymbol("CreateAccelerator"));
    }
    CreateAcceleratorFunc CreateAccelerator;
};
class SamplerPlugin : public Plugin
{
    typedef Sampler *(*CreateSamplerFunc)(const ParamSet &params, const Film *film);
public:
    SamplerPlugin( const string &name ): Plugin( name )
    {
        CreateSampler =
            (CreateSamplerFunc)(GetSymbol("CreateSampler"));
    }
    CreateSamplerFunc CreateSampler;
};
class CameraPlugin : public Plugin
{
    typedef Camera *(*CreateCameraFunc)(const ParamSet &params, const Transform &world2cam, Film *film);
public:
    CameraPlugin( const string &name ): Plugin( name )
    {
        CreateCamera =
            (CreateCameraFunc)(GetSymbol("CreateCamera"));
    }
    CreateCameraFunc CreateCamera;
};
class FilterPlugin : public Plugin
{
    typedef Filter *(*CreateFilterFunc)(const ParamSet &params);
public:
    FilterPlugin( const string &name ): Plugin( name )
    {
        CreateFilter =
            (CreateFilterFunc)(GetSymbol("CreateFilter"));
    }
    CreateFilterFunc CreateFilter;
};
class ToneMapPlugin : public Plugin
{
    typedef ToneMap *(*CreateToneMapFunc)(const ParamSet &params);
public:
    ToneMapPlugin( const string &name ): Plugin( name )
    {
        CreateToneMap =
            (CreateToneMapFunc)(GetSymbol("CreateToneMap"));
    }
    CreateToneMapFunc CreateToneMap;
};
class FilmPlugin : public Plugin
{
    typedef Film *(*CreateFilmFunc)(const ParamSet &params, Filter *filter);
public:
    FilmPlugin( const string &name ): Plugin( name )
    {
        CreateFilm =
            (CreateFilmFunc)(GetSymbol("CreateFilm"));
    }
    CreateFilmFunc CreateFilm;
};*/
// Runtime Loading Method Definitions
/*
 void UpdatePluginPath(const string &newpath)
{
    string ret;
    for (u_int i = 0; i < newpath.size(); ++i)
    {
        if (newpath[i] != '&')
            ret += newpath[i];
        else
            ret += PluginSearchPath;
    }
    PluginSearchPath = ret;
}*/
 ShapePtr MakeShape(const string &name,
                                   const Transform &object2world,
                                   bool reverseOrientation,
                                   const ParamSet &paramSet)
{
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
        return ShapePtr(Cone::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="cylinder")
        return ShapePtr(Cylinder::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="disk")
        return ShapePtr(Disk::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="heightfield")
        return ShapePtr(Heightfield::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="hyperboloid")
        return ShapePtr(Hyperboloid::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="loopsubdiv")
        return ShapePtr(LoopSubdiv::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="nurbs")
        return ShapePtr(NURBS::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="paraboloid")
        return ShapePtr(Paraboloid::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="sphere")
        return ShapePtr(Sphere::CreateShape(object2world, reverseOrientation, paramSet));
    if(name=="trianglemesh")
        return ShapePtr(TriangleMesh::CreateShape(object2world, reverseOrientation, paramSet));
    //Error("Static loading of shape '%s' failed.",name.c_str());
	std::stringstream ss;
	ss<<"Static loading of shape '"<<name<<"' failed.";
	luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    ShapePtr o;
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
 MaterialPtr MakeMaterial(const string &name,
        const Transform &mtl2world,
        const TextureParams &mp)
{
    /*MaterialPlugin *plugin = GetPlugin<MaterialPlugin>(name, materialPlugins,
                             PluginSearchPath);
    if (plugin)
    {
        MaterialPtr ret =
            plugin->CreateMaterial(mtl2world, mp);
        mp.ReportUnused();
        return ret;
    }*/
    if(name=="bluepaint")
    {
    	MaterialPtr ret = MaterialPtr(BluePaint::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="brushedmetal")
    {
    	MaterialPtr ret = MaterialPtr(BrushedMetal::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="clay")
    {
    	MaterialPtr ret = MaterialPtr(Clay::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="felt")
    {
    	MaterialPtr ret = MaterialPtr(Felt::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="glass")
    {
    	MaterialPtr ret = MaterialPtr(Glass::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="roughglass")
    {
    	MaterialPtr ret = MaterialPtr(RoughGlass::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="matte")
    {
    	MaterialPtr ret = MaterialPtr(Matte::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="mirror")
    {
    	MaterialPtr ret = MaterialPtr(Mirror::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="plastic")
    {
    	MaterialPtr ret = MaterialPtr(Plastic::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="primer")
    {
    	MaterialPtr ret = MaterialPtr(Primer::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="shinymetal")
    {
    	MaterialPtr ret = MaterialPtr(ShinyMetal::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="skin")
    {
    	MaterialPtr ret = MaterialPtr(Skin::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="substrate")
    {
    	MaterialPtr ret = MaterialPtr(Substrate::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="translucent")
    {
    	MaterialPtr ret = MaterialPtr(Translucent::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="uber")
    {
    	MaterialPtr ret = MaterialPtr(UberMaterial::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="carpaint")
    {
    	MaterialPtr ret = MaterialPtr(CarPaint::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    if(name=="metal")
    {
    	MaterialPtr ret = MaterialPtr(Metal::CreateMaterial(mtl2world, mp));
        mp.ReportUnused();
        return ret;
    }
    
    
    //Error("Static loading of material '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of material '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
	MaterialPtr o;
    return o;
}
 boost::shared_ptr< Texture<float> > MakeFloatTexture(const string &name,
        const Transform &tex2world, const TextureParams &tp)
{
    /*TexturePlugin *plugin = GetPlugin<TexturePlugin>(name, texturePlugins,
                            PluginSearchPath);
    if (plugin)
    {
        boost::shared_ptr<Texture<float> > ret =
            plugin->CreateFloatTex(tex2world, tp);
        tp.ReportUnused();
        return ret;
    }*/
    if(name=="bilerp")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(BilerpTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="checkerboard")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(Checkerboard::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="constant")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(Constant::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="dots")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(DotsTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="fbm")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(FBmTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="imagemap")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(ImageTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="marble")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(MarbleTexture::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="mix")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(MixTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="scale")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(ScaleTexture<float,float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    } 
    if(name=="uv")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(UVTexture::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="windy")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(WindyTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="wrinkled")
    {
    	boost::shared_ptr<Texture<float> >  ret = boost::shared_ptr<Texture<float> >(WrinkledTexture<float>::CreateFloatTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of float texture '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of texture '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
	boost::shared_ptr<Texture<float> > o;
    return o;
}
 boost::shared_ptr<Texture<Spectrum> > MakeSpectrumTexture(const string &name,
        const Transform &tex2world, const TextureParams &tp)
{
    /*TexturePlugin *plugin = GetPlugin<TexturePlugin>(name, texturePlugins,
                            PluginSearchPath);
    if (plugin)
    {
        boost::shared_ptr<Texture<Spectrum> > ret =
            plugin->CreateSpectrumTex(tex2world, tp);
        tp.ReportUnused();
        return ret;
    }*/
    if(name=="bilerp")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(BilerpTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="checkerboard")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(Checkerboard::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="constant")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(Constant::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="dots")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(DotsTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="fbm")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(FBmTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="imagemap")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(ImageTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="marble")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(MarbleTexture::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="mix")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(MixTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="scale")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(ScaleTexture<Spectrum,Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    } 
    if(name=="uv")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(UVTexture::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="windy")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(WindyTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    if(name=="wrinkled")
    {
    	boost::shared_ptr<Texture<Spectrum> >  ret = boost::shared_ptr<Texture<Spectrum> >(WrinkledTexture<Spectrum>::CreateSpectrumTexture(tex2world, tp));
        tp.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of spectrum texture '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of specturm texture '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
	boost::shared_ptr<Texture<Spectrum> > o;
    return o;
}
 Light *MakeLight(const string &name,
                         const Transform &light2world, const ParamSet &paramSet)
{
    /*LightPlugin *plugin = GetPlugin<LightPlugin>(name, lightPlugins, PluginSearchPath);
    if (plugin)
    {
        Light *ret = plugin->CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    if(name=="distant")
    {
    	Light *ret = DistantLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="goniometric")
    {
    	Light *ret = GonioPhotometricLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="infinitesample")
    {
    	Light *ret = InfiniteAreaLightIS::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="infinite")
    {
    	Light *ret = InfiniteAreaLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="point")
    {
    	Light *ret = PointLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="projection")
    {
    	Light *ret = ProjectionLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="spot")
    {
    	Light *ret = SpotLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="sun")
    {
    	Light *ret = SunLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="sun2")
    {
    	Light *ret = NSunLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="sun3")
    {
    	Light *ret = Sun3Light::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="sky")
    {
    	Light *ret = SkyLight::CreateLight(light2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    //Error("Static loading of light '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of light '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 AreaLight *MakeAreaLight(const string &name,
                                 const Transform &light2world, const ParamSet &paramSet,
                                 const ShapePtr &shape)
{
    /*AreaLightPlugin *plugin = GetPlugin<AreaLightPlugin>(name, arealightPlugins,
                              PluginSearchPath);
    if (plugin)
    {
        AreaLight *ret = plugin->CreateAreaLight(light2world, paramSet, shape);
        paramSet.ReportUnused();
        return ret;
    }*/
    
    if(name=="area")
    {
    	AreaLight *ret = AreaLight::CreateAreaLight(light2world, paramSet, shape);
        paramSet.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of area light '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of area light '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 VolumeRegion *MakeVolumeRegion(const string &name,
                                       const Transform &volume2world, const ParamSet &paramSet)
{
    /*VolumeRegionPlugin *plugin = GetPlugin<VolumeRegionPlugin>(name, volumePlugins,
                                 PluginSearchPath);
    if (plugin)
    {
        VolumeRegion *ret = plugin->CreateVolumeRegion(volume2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    
    if(name=="exponential")
    {
    	VolumeRegion *ret = ExponentialDensity::CreateVolumeRegion(volume2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="homogeneous")
    {
    	VolumeRegion *ret = HomogeneousVolume::CreateVolumeRegion(volume2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="volumegrid")
    {
    	VolumeRegion *ret = VolumeGrid::CreateVolumeRegion(volume2world, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of volume region '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of volume region '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 SurfaceIntegrator *MakeSurfaceIntegrator(const string &name,
        const ParamSet &paramSet)
{
    /*SurfaceIntegratorPlugin *plugin = GetPlugin<SurfaceIntegratorPlugin>(name,
                                      surf_integratorPlugins, PluginSearchPath);
    if (plugin)
    {
        SurfaceIntegrator *ret = plugin->CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    
    if(name=="bidirectional")
    {
        SurfaceIntegrator *ret=BidirIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="debug")
    {
        SurfaceIntegrator *ret=DebugIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="directlighting")
    {
        SurfaceIntegrator *ret=DirectLighting::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    /*if(name=="emission")
    {
        SurfaceIntegrator *ret=EmissionIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    if(name=="exphotonmap")
    {
        SurfaceIntegrator *ret=ExPhotonIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    } 
    if(name=="igi")
    {
        SurfaceIntegrator *ret=IGIIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="irradiancecache")
    {
        SurfaceIntegrator *ret=IrradianceCache::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="path")
    {
        SurfaceIntegrator *ret=PathIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="photonmap")
    {
        SurfaceIntegrator *ret=PhotonIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    } 
    /*if(name=="single")
    {
        SurfaceIntegrator *ret=SingleScattering::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    if(name=="whitted")
    {
        SurfaceIntegrator *ret=WhittedIntegrator::CreateSurfaceIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of surface integrator '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of surface integrator '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 VolumeIntegrator *MakeVolumeIntegrator(const string &name,
        const ParamSet &paramSet)
{
    /*
    VolumeIntegratorPlugin *plugin = GetPlugin<VolumeIntegratorPlugin>(name, vol_integratorPlugins,
                                     PluginSearchPath);
    if (plugin)
    {
        VolumeIntegrator *ret = plugin->CreateVolumeIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    
    if(name=="single")
    {
        VolumeIntegrator *ret=SingleScattering::CreateVolumeIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    if(name=="emission")
    {
        VolumeIntegrator *ret=EmissionIntegrator::CreateVolumeIntegrator(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of volume integrator '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of volume integrator '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 Primitive *MakeAccelerator(const string &name, const vector<Primitive* > &prims, const ParamSet &paramSet)
{
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

    if(name=="kdtree")
    {
        Primitive* ret=KdTreeAccel::CreateAccelerator(prims, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="grid")
    {
        Primitive* ret=GridAccel::CreateAccelerator(prims, paramSet);
        paramSet.ReportUnused();
        return ret;
    }
		// NOTE - ratow - Added a brute force (de)accelerator for debugging purposes.
    if(name=="none")
		{
        Primitive* ret=BruteForceAccel::CreateAccelerator(prims, paramSet);
        paramSet.ReportUnused();
        return ret;
		}

    //Error("Static loading of accelerator '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of accelerator '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 Camera *MakeCamera(const string &name,
                           const ParamSet &paramSet,
                           const Transform &world2cam, Film *film)
{
    /*
    CameraPlugin *plugin = GetPlugin<CameraPlugin>(name, cameraPlugins, PluginSearchPath);
    if (plugin)
    {
        Camera *ret = plugin->CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }*/
    if(name=="environment")
    {
        Camera *ret=EnvironmentCamera::CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="orthographic")
    {
        Camera *ret=OrthoCamera::CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="perspective")
    {
        Camera *ret=PerspectiveCamera::CreateCamera(paramSet, world2cam, film);
        paramSet.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of camera '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of camera '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 Sampler *MakeSampler(const string &name,
                             const ParamSet &paramSet, const Film *film)
{
    /*
    SamplerPlugin *plugin = GetPlugin<SamplerPlugin>(name, samplerPlugins, PluginSearchPath);
    if (plugin) {
    Sampler *ret = plugin->CreateSampler(paramSet, film);
    paramSet.ReportUnused();
    return ret;
}*/
    //Sampler *ret;

    if(name=="lowdiscrepancy")
    {
        Sampler *ret=LDSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="random")
    {
        Sampler *ret=RandomSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="halton")
    {
        Sampler *ret=HaltonSampler::CreateSampler(paramSet, film);
        paramSet.ReportUnused();
        return ret;
    }

	// NOTE - Radiance - currently disabled due to reimplementation of pixelsampling, will fix
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
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 Filter *MakeFilter(const string &name,
                           const ParamSet &paramSet)
{
    /*FilterPlugin *plugin = GetPlugin<FilterPlugin>(name, filterPlugins, PluginSearchPath);
    if (plugin)
    {
        Filter *ret = plugin->CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    
    if(name=="box")
    {
        Filter *ret=BoxFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    if(name=="gaussian")
    {
        Filter *ret=GaussianFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    if(name=="mitchell")
    {
        Filter *ret=MitchellFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    if(name=="sinc")
    {
        Filter *ret=LanczosSincFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    if(name=="triangle")
    {
        Filter *ret=TriangleFilter::CreateFilter(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    
    //Error("Static loading of filter '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of fiter '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 ToneMap *MakeToneMap(const string &name,
                             const ParamSet &paramSet)
{
    /*ToneMapPlugin *plugin = GetPlugin<ToneMapPlugin>(name, tonemapPlugins, PluginSearchPath);
    if (plugin)
    {
        ToneMap *ret = plugin->CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }*/
    if(name=="contrast")
    {
        ToneMap *ret=ContrastOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="highcontrast")
    {
        ToneMap *ret=HighContrastOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="maxwhite")
    {
        ToneMap *ret=MaxWhiteOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="nonlinear")
    {
        ToneMap *ret=NonLinearOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }
    if(name=="reinhard")
    {
        ToneMap *ret=ReinhardOp::CreateToneMap(paramSet);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of tonemap '%s' failed.",name.c_str());
    std::stringstream ss;
    ss<<"Static loading of tonemap '"<<name<<"' failed.";
    luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
 Film *MakeFilm(const string &name,
                       const ParamSet &paramSet, Filter *filter)
{
    /*FilmPlugin *plugin = GetPlugin<FilmPlugin>(name, filmPlugins, PluginSearchPath);
    if (plugin)
    {
        Film *ret = plugin->CreateFilm(paramSet, filter);
        paramSet.ReportUnused();
        return ret;
    }*/
    
    if(name=="image")
    {
        Film *ret=ImageFilm::CreateFilm(paramSet, filter);
        paramSet.ReportUnused();
        return ret;
    }

	if(name=="multiimage")
    {
		Film *ret=MultiImageFilm::CreateFilm(paramSet, filter);
        paramSet.ReportUnused();
        return ret;
    }

    //Error("Static loading of film '%s' failed.",name.c_str());
	std::stringstream ss;
	ss<<"Static loading of film '"<<name<<"' failed.";
	luxError(LUX_BUG,LUX_ERROR,ss.str().c_str());
    return NULL;
}
// Plugin Method Definitions
/*
Plugin::Plugin(const string &fname)
{
    pluginName = fname;
#if defined(WIN32)

    hinstLib = LoadLibrary(fname.c_str());
    if (!hinstLib)
        Error("Can't open plug-in \"%s\"", fname.c_str());
#elif defined(__APPLE__)

    NSObjectFileImage fileImage;
    NSObjectFileImageReturnCode returnCode =
                                          NSCreateObjectFileImageFromFile(fname.c_str(), &fileImage);
    if(returnCode == NSObjectFileImageSuccess)
    {
        hinstLib = NSLinkModule(fileImage,fname.c_str(),
                                NSLINKMODULE_OPTION_RETURN_ON_ERROR
                                |  NSLINKMODULE_OPTION_PRIVATE);
        NSDestroyObjectFileImage(fileImage);
        if (!hinstLib)
        {
            Error("Can't open plug-in \"%s\"", fname.c_str());
        }
    }
    else
    {
        Error("Can't open plug-in \"%s\"", fname.c_str());
    }
#else
    hinstLib = dlopen(fname.c_str(), RTLD_LAZY);
    if (!hinstLib)
        Error("Can't open plug-in \"%s\" (%s)", fname.c_str(),
              dlerror());
#endif
}
Plugin::~Plugin()
{
#if defined(WIN32)
    FreeLibrary(hinstLib);
#elif defined(__APPLE__)

    NSUnLinkModule(hinstLib,0);
#else

    dlclose(hinstLib);
#endif
}
void *Plugin::GetSymbol(const string &symname)
{
    void *data;
#if defined(WIN32)

    data = GetProcAddress(hinstLib, symname.c_str());
#elif defined(__APPLE__)

    string apple_lossage = string("_") + symname;
    NSSymbol nssym = NSLookupSymbolInModule(hinstLib,apple_lossage.c_str());
    data = NSAddressOfSymbol(nssym);
#else

    data = dlsym(hinstLib, symname.c_str());
#endif

    if (!data)
        Severe("Couldn't get symbol \"%s\" in Plugin %s.", symname.c_str(), pluginName.c_str());
    return data;
}
*/

