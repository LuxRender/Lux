#ifndef LUX_PREVIEW_SCENES_STANDARD_MATERIAL_H
#define LUX_PREVIEW_SCENES_STANDARD_MATERIAL_H

#include <vector>

#include "lux_instance.h"
#include "lux_paramset.h"

namespace lux { namespace scenes {

/*

This file assumes that the calling app has already correctly loaded the shared library
to make the CreateLuxParamSet and CreateLuxInstance functions available.

In order to make this preview work, the calling app should execute the following:

// Assume include path contains "lux/cpp_api"
#include "lux_instance.h"
#include "lux_paramset.h"
#include "preview_scenes/standard_material.h"

(you'd normally run all this in a thread)
void mythread::run()
{
	mutex scene_setup;
	lux_instance* context = dynamic_cast<lux_instance*>(CreateLuxInstance("preview"));
	std::vector<lux_paramset*> setup_paramsets;
	std::vector<lux_paramset*> render_paramsets;

	// Scene setup is not thread safe
	scene_setup.lock();

	// Setup the rendering parameters, and open the World block
	// Takes arguments: pointer to paramset create function, context object, filename, x resolution, y resolution, and haltspp
	setup_paramsets = lux::scenes::standard_material_setup(CreateLuxParamSet, context, "my-preview", 120, 75, 64);

	// Now insert your material/texture/volume definitions
	char* preview_material_name = "preview_mat";
	context->makeNamedMaterial( ..., preview_material_name, ... );

	// Then finish creating the scene and objects.
	// Takes arguments: pointer to paramset create function, context object, preview material name, interior volume name, exterior volume name
	// This function will close the World block, and rendering will start shortly afterwards
	render_paramsets = lux::scenes::standard_material_render(CreateLuxParamSet, context, preview_material_name, "", "");

	// Scene setup is finished
	scene_setup.unlock();

	// You should now wait for the scene to be ready before issuing further commands to context
	// NB. waiting for "sceneIsReady" != waiting for render to finish. This is still part of the
	// rendering startup phase.
	while (!context->statistics("sceneIsReady")) {
		sleep( .. some appropriate value, perhaps 0.1 seconds? .. );
	}

	// You can add more threads to the renderer here
	for(int i=0; i<3; i++)	// Add 3 more rendering threads
		context->addThread();

	// Here, you can wait in a loop to get statistics ...
	while (!context->statistics("terminated") && !context->statistics("enoughSamples"))
	{
		float samples_per_px = context->statistics("samplePx");
		float progress = samples_per_px / 64;
		log( context->printableStatistics(false) );
	}

	// ... or just simply wait. Regardless of whether you print stats or not, calling wait()
	// is a really good idea.
	context->wait();

	// If you want to grab the framebuffer directly, now is the time, otherwise, load
	// from the filename specified in the setup phase above
	const char* preview_image = context->framebuffer();

	// Finally, clean up to free memory
	context->exit();
	context->cleanup();

	// You are now responsible for disposing of the contents of setup_paramsets and render_paramsets
	// and to delete the context itself.
}

*/

// Returns a vector of the created ParamSets for deletion later
std::vector<lux_paramset*> standard_material_setup(CreateLuxParamSetPtr CreateLuxParamSet, lux_instance* ctx, const char* filename, int xres, int yres, int haltspp)
{
	std::vector<lux_paramset*> paramsets;

	const bool b_true = true;
	const bool b_false = false;

	// Camera
	ctx->lookAt(0.0f, -3.0f, 0.5f,  0.0f, -2.0f, 0.5f,  0.0f, 0.0f, 1.0f);
	lux_paramset* ps_camera = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_camera);
	float fov = 22.5f;
	ps_camera->AddFloat("fov", &fov);
	ctx->camera("perspective", ps_camera);

	// Film
	lux_paramset* ps_film = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_film);
	ps_film->AddInt("xresolution", &xres);
	ps_film->AddInt("yresolution", &yres);
	ps_film->AddString("filename", filename);
	ps_film->AddBool("write_exr_ZBuf", &b_true);
	ps_film->AddBool("write_exr_applyimaging", &b_true);
	ps_film->AddString("write_exr_channels", "RGBA");
	ps_film->AddBool("write_exr_halftype", &b_false);
	float gamma = 2.2f;
	ps_film->AddFloat("gamma", &gamma);
	ps_film->AddBool("write_png", &b_true);
	ps_film->AddBool("write_tga", &b_false);
	ps_film->AddBool("write_resume_flm", &b_false);
	int display_interval = 3;
	ps_film->AddInt("displayinterval", &display_interval);
	ps_film->AddInt("haltspp", &haltspp);
	ps_film->AddString("tonemapkernel", "linear");
	int reject_warmup = 64;
	ps_film->AddInt("reject_warmup", &reject_warmup);
	ps_film->AddBool("write_exr", &b_false);
	int write_interval = 3600;
	ps_film->AddInt("writeinterval", &write_interval);
	ctx->film("fleximage", ps_film);

	// Pixel Filter
	float pxf_width = 1.5f;
	float pxf_b_c = 0.333f;
	lux_paramset* ps_pxf = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_pxf);
	ps_pxf->AddFloat("xwidth", &pxf_width);
	ps_pxf->AddFloat("ywidth", &pxf_width);
	ps_pxf->AddFloat("B", &pxf_b_c);
	ps_pxf->AddFloat("C", &pxf_b_c);
	ps_pxf->AddBool("supersample", &b_true);
	ctx->pixelFilter("mitchell", ps_pxf);

	// Sampler
	lux_paramset* ps_sampler = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_sampler);
	ctx->sampler("metropolis", ps_sampler);

	// Surface Integrator
	lux_paramset* ps_sintegrator = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_sintegrator);
	ctx->surfaceIntegrator("bidirectional", ps_sintegrator);

	// Volume Integrator
	lux_paramset* ps_vintegrator = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_vintegrator);
	ctx->volumeIntegrator("multi", ps_vintegrator);

	ctx->worldBegin();

	return paramsets;
}

// Returns a vector of the created ParamSets for deletion later
std::vector<lux_paramset*> standard_material_render(CreateLuxParamSetPtr CreateLuxParamSet, lux_instance* ctx, const char* preview_material_name, const char* preview_interior, const char* preview_exterior)
{
	std::vector<lux_paramset*> paramsets;

	const bool b_true = true;
	const bool b_false = false;

	// Main Area Light
	ctx->attributeBegin();
	// light transform
	float light_transform[16] = {
		 0.599f,  0.800f, 0.000f, 0.0f,
		-0.605f,  0.453f, 0.653f, 0.0f,
		 0.522f, -0.391f, 0.757f, 0.0f,
		 4.076f, -3.054f, 5.903f, 1.0f
	};
	ctx->transform( light_transform );
	// light color emission texture
	lux_paramset* ps_tex_bb = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_tex_bb);
	float tex_bb_temp = 6500.0f;
	ps_tex_bb->AddFloat("temperature", &tex_bb_temp);
	ctx->texture("pL", "color", "blackbody", ps_tex_bb);
	// light source
	lux_paramset* ps_light = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_light);
	ps_light->AddTexture("L", "pL");
	float light_unity = 1.0f;
	ps_light->AddFloat("gain", &light_unity);
	ps_light->AddFloat("importance", &light_unity);
	ctx->exterior(preview_exterior);
	ctx->areaLightSource("area", ps_light);
	// light shape
	lux_paramset* ps_light_shape = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_light_shape);
	int light_shape_indices[6] = {0,1,2, 0,2,3};
	ps_light_shape->AddInt("indices", light_shape_indices, 6);
	int light_size = 1;
	float light_shape_points[12] = {
		-light_size/2.0f,  light_size/2.0f, 0.0f,
		 light_size/2.0f,  light_size/2.0f, 0.0f,
		 light_size/2.0f, -light_size/2.0f, 0.0f,
		-light_size/2.0f, -light_size/2.0f, 0.0f
	};
	ps_light_shape->AddPoint("P", light_shape_points, 12);
	ctx->shape("trianglemesh", ps_light_shape);
	ctx->attributeEnd();

	// Background Infinite Light
	ctx->attributeBegin();
	ctx->exterior(preview_exterior);
	lux_paramset* ps_bg_light = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_bg_light);
	float bg_light_val = 0.1f;
	ps_bg_light->AddFloat("gain", &bg_light_val);
	ps_bg_light->AddFloat("importance", &bg_light_val);
	ctx->lightSource("infinite", ps_bg_light);
	ctx->attributeEnd();

	// Checkered Backdrop
	ctx->attributeBegin();
	// object transform
	float bd_transform[16] = {
		5.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 5.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 5.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	ctx->transform(bd_transform);
	ctx->scale(4.0f, 1.0f, 1.0f);
	// checkered pattern for color mix
	lux_paramset* ps_bd_checks1 = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_bd_checks1);
	int bd_checks_dim = 2;
	float bd_checks_uscale = 36.8f;
	float bd_checks_vscale = 144.0f;
	ps_bd_checks1->AddInt("dimension", &bd_checks_dim);
	ps_bd_checks1->AddString("mapping", "uv");
	ps_bd_checks1->AddFloat("uscale", &bd_checks_uscale);
	ps_bd_checks1->AddFloat("vscale", &bd_checks_vscale);
	ctx->texture("checks::pattern", "float", "checkerboard", ps_bd_checks1);
	// color mix
	lux_paramset* ps_bd_checks2 = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_bd_checks2);
	float bd_checks2_tex1[3] = {0.9f, 0.9f, 0.9f};
	float bd_checks2_tex2[3] = {0.0f, 0.0f, 0.0f};
	ps_bd_checks2->AddTexture("amount", "checks::pattern");
	ps_bd_checks2->AddRGBColor("tex1", bd_checks2_tex1);
	ps_bd_checks2->AddRGBColor("tex2", bd_checks2_tex2);
	ctx->texture("checks", "color", "mix", ps_bd_checks2);
	// backdrop material
	lux_paramset* ps_bd_mat = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_bd_mat);
	ps_bd_mat->AddTexture("Kd", "checks");
	ctx->material("matte", ps_bd_mat);
	// backdrop shape
	lux_paramset* ps_bd_shape = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_bd_shape);
	int bd_shape_subdiv = 3;
	ps_bd_shape->AddInt("nlevels", &bd_shape_subdiv);
	ps_bd_shape->AddBool("dmnormalsmooth", &b_true);
	ps_bd_shape->AddBool("dmsharpboundary", &b_true);
	int bd_shape_indices[18] = {0,1,2,0,2,3,1,0,4,1,4,5,5,4,6,5,6,7};
	ps_bd_shape->AddInt("indices", bd_shape_indices, 18);
	float bd_shape_points[24] = {
		 1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  3.0f, 0.0f,
		-1.0f,  3.0f, 0.0f,
		 1.0f,  3.0f, 2.0f,
		-1.0f,  3.0f, 2.0f
	};
	ps_bd_shape->AddPoint("P", bd_shape_points, 24);
	float bd_shape_normals[24] = {
		0.0f,  0.000000f, 1.000000f,
		0.0f,  0.000000f, 1.000000f,
		0.0f,  0.000000f, 1.000000f,
		0.0f,  0.000000f, 1.000000f,
		0.0f, -0.707083f, 0.707083f,
		0.0f, -0.707083f, 0.707083f,
		0.0f, -1.000000f, 0.000000f,
		0.0f, -1.000000f, 0.000000f
	};
	ps_bd_shape->AddNormal("N", bd_shape_normals, 24);
	float bd_shape_uvs[16] = {
		0.333334f, 0.000000f,
		0.333334f, 0.333334f,
		0.000000f, 0.333334f,
		0.000000f, 0.000000f,
		0.666667f, 0.000000f,
		0.666667f, 0.333333f,
		1.000000f, 0.000000f,
		1.000000f, 0.333333f
	};
	ps_bd_shape->AddFloat("uv", bd_shape_uvs, 16);
	ctx->shape("loopsubdiv", ps_bd_shape);
	ctx->exterior(preview_exterior);
	ctx->attributeEnd();

	// Preview object
	// TODO; make switchable between sphere/cube/monkey
	ctx->attributeBegin();
	float po_transform[16] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	};
	ctx->transform(po_transform);

	ctx->interior(preview_interior);
	ctx->exterior(preview_exterior);
	ctx->namedMaterial(preview_material_name);

	lux_paramset* ps_po_shape = dynamic_cast<lux_paramset*>(CreateLuxParamSet());
	paramsets.push_back(ps_po_shape);
	float po_sphere_radius = 1.0f;
	ps_po_shape->AddFloat("radius", &po_sphere_radius);
	ctx->shape("sphere", ps_po_shape);
	ctx->attributeEnd();

	ctx->worldEnd();

	return paramsets;
}


} } // namespaces

#endif // LUX_PREVIEW_SCENES_STANDARD_MATERIAL_H
