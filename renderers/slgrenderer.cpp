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

#include <iomanip>

#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "slgrenderer.h"
#include "context.h"
#include "renderers/statistics/slgstatistics.h"

#include "luxrays/core/context.h"
#include "luxrays/core/virtualdevice.h"
#include "rendersession.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <fstream>

using namespace lux;

//------------------------------------------------------------------------------
// SLGHostDescription
//------------------------------------------------------------------------------

SLGHostDescription::SLGHostDescription(SLGRenderer *r, const string &n) : renderer(r), name(n) {
	SLGDeviceDescription *desc = new SLGDeviceDescription(this, "Test");
	devs.push_back(desc);
}

SLGHostDescription::~SLGHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

void SLGHostDescription::AddDevice(SLGDeviceDescription *devDesc) {
	devs.push_back(devDesc);
}

//------------------------------------------------------------------------------
// SLGRenderer
//------------------------------------------------------------------------------

SLGRenderer::SLGRenderer() : Renderer() {
	state = INIT;

	SLGHostDescription *host = new SLGHostDescription(this, "Localhost");
	hosts.push_back(host);

	preprocessDone = false;
	suspendThreadsWhenDone = false;

	AddStringConstant(*this, "name", "Name of current renderer", "slg");

	rendererStatistics = new SLGStatistics(this);
}

SLGRenderer::~SLGRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	delete rendererStatistics;

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SLGRenderer::~SLGRenderer() while not in TERMINATE or INIT state.");

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererState SLGRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &SLGRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void SLGRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

static void CreateBox(luxrays::sdl::Scene *scene, const string &objName, const string &matName,
		const string &texName, const BBox &bbox) {
	Point *p = new Point[24];
	// Bottom face
	p[0] = Point(bbox.pMin.x, bbox.pMin.y, bbox.pMin.z);
	p[1] = Point(bbox.pMin.x, bbox.pMax.y, bbox.pMin.z);
	p[2] = Point(bbox.pMax.x, bbox.pMax.y, bbox.pMin.z);
	p[3] = Point(bbox.pMax.x, bbox.pMin.y, bbox.pMin.z);
	// Top face
	p[4] = Point(bbox.pMin.x, bbox.pMin.y, bbox.pMax.z);
	p[5] = Point(bbox.pMax.x, bbox.pMin.y, bbox.pMax.z);
	p[6] = Point(bbox.pMax.x, bbox.pMax.y, bbox.pMax.z);
	p[7] = Point(bbox.pMin.x, bbox.pMax.y, bbox.pMax.z);
	// Side left
	p[8] = Point(bbox.pMin.x, bbox.pMin.y, bbox.pMin.z);
	p[9] = Point(bbox.pMin.x, bbox.pMin.y, bbox.pMax.z);
	p[10] = Point(bbox.pMin.x, bbox.pMax.y, bbox.pMax.z);
	p[11] = Point(bbox.pMin.x, bbox.pMax.y, bbox.pMin.z);
	// Side right
	p[12] = Point(bbox.pMax.x, bbox.pMin.y, bbox.pMin.z);
	p[13] = Point(bbox.pMax.x, bbox.pMax.y, bbox.pMin.z);
	p[14] = Point(bbox.pMax.x, bbox.pMax.y, bbox.pMax.z);
	p[15] = Point(bbox.pMax.x, bbox.pMin.y, bbox.pMax.z);
	// Side back
	p[16] = Point(bbox.pMin.x, bbox.pMin.y, bbox.pMin.z);
	p[17] = Point(bbox.pMax.x, bbox.pMin.y, bbox.pMin.z);
	p[18] = Point(bbox.pMax.x, bbox.pMin.y, bbox.pMax.z);
	p[19] = Point(bbox.pMin.x, bbox.pMin.y, bbox.pMax.z);
	// Side front
	p[20] = Point(bbox.pMin.x, bbox.pMax.y, bbox.pMin.z);
	p[21] = Point(bbox.pMin.x, bbox.pMax.y, bbox.pMax.z);
	p[22] = Point(bbox.pMax.x, bbox.pMax.y, bbox.pMax.z);
	p[23] = Point(bbox.pMax.x, bbox.pMax.y, bbox.pMin.z);

	luxrays::Triangle *vi = new luxrays::Triangle[12];
	// Bottom face
	vi[0] = luxrays::Triangle(0, 1, 2);
	vi[1] = luxrays::Triangle(2, 3, 0);
	// Top face
	vi[2] = luxrays::Triangle(4, 5, 6);
	vi[3] = luxrays::Triangle(6, 7, 4);
	// Side left
	vi[4] = luxrays::Triangle(8, 9, 10);
	vi[5] = luxrays::Triangle(10, 11, 8);
	// Side right
	vi[6] = luxrays::Triangle(12, 13, 14);
	vi[7] = luxrays::Triangle(14, 15, 12);
	// Side back
	vi[8] = luxrays::Triangle(16, 17, 18);
	vi[9] = luxrays::Triangle(18, 19, 16);
	// Side back
	vi[10] = luxrays::Triangle(20, 21, 22);
	vi[11] = luxrays::Triangle(22, 23, 20);

	if (texName == "") {
		// Define the object
		scene->DefineObject(objName, 24, 12, p, vi, NULL, NULL, false);

		// Add the object to the scene
		scene->AddObject(objName, matName,
				"scene.objects." + matName + "." + objName + ".useplynormals = 0\n"
			);
	} else {
		luxrays::UV *uv = new luxrays::UV[24];
		// Bottom face
		uv[0] = luxrays::UV(0.f, 0.f);
		uv[1] = luxrays::UV(1.f, 0.f);
		uv[2] = luxrays::UV(1.f, 1.f);
		uv[3] = luxrays::UV(0.f, 1.f);
		// Top face
		uv[4] = luxrays::UV(0.f, 0.f);
		uv[5] = luxrays::UV(1.f, 0.f);
		uv[6] = luxrays::UV(1.f, 1.f);
		uv[7] = luxrays::UV(0.f, 1.f);
		// Side left
		uv[8] = luxrays::UV(0.f, 0.f);
		uv[9] = luxrays::UV(1.f, 0.f);
		uv[10] = luxrays::UV(1.f, 1.f);
		uv[11] = luxrays::UV(0.f, 1.f);
		// Side right
		uv[12] = luxrays::UV(0.f, 0.f);
		uv[13] = luxrays::UV(1.f, 0.f);
		uv[14] = luxrays::UV(1.f, 1.f);
		uv[15] = luxrays::UV(0.f, 1.f);
		// Side back
		uv[16] = luxrays::UV(0.f, 0.f);
		uv[17] = luxrays::UV(1.f, 0.f);
		uv[18] = luxrays::UV(1.f, 1.f);
		uv[19] = luxrays::UV(0.f, 1.f);
		// Side front
		uv[20] = luxrays::UV(0.f, 0.f);
		uv[21] = luxrays::UV(1.f, 0.f);
		uv[22] = luxrays::UV(1.f, 1.f);
		uv[23] = luxrays::UV(0.f, 1.f);

		// Define the object
		scene->DefineObject(objName, 24, 12, p, vi, NULL, uv, false);

		// Add the object to the scene
		scene->AddObject(objName, matName,
				"scene.objects." + matName + "." + objName + ".useplynormals = 0\n"
				"scene.objects." + matName + "." + objName + ".texmap = " + texName + "\n"
			);
	}
}

luxrays::sdl::Scene *SLGRenderer::CreateSLGScene() {
	luxrays::sdl::Scene *slgScene = new luxrays::sdl::Scene();

	// Setup the camera
	slgScene->CreateCamera(
		"scene.camera.lookat = 1.0 6.0 3.0  0.0 0.0 0.5\n"
		"scene.camera.fieldofview = 60.0\n"
		);

	// Define texture maps
	const u_int size = 500;
	luxrays::Spectrum *tm = new luxrays::Spectrum[size * size];
	for (u_int y = 0; y < size; ++y) {
		for (u_int x = 0; x < size; ++x) {
			if ((x % 50 < 25) ^ (y % 50 < 25))
				tm[x + y * size] = luxrays::Spectrum(1.f, 0.f, 0.f);
			else
				tm[x + y * size] = luxrays::Spectrum(1.f, 1.f, 0.f);
		}
	}
	slgScene->DefineTexMap("check_texmap", tm, size, size);

	// Setup materials
	slgScene->AddMaterials(
		"scene.materials.light.whitelight = 300.0 300.0 300.0\n"
		"scene.materials.matte.mat_white = 0.75 0.75 0.75\n"
		"scene.materials.matte.mat_red = 0.75 0.0 0.0\n"
		"scene.materials.glass.mat_glass = 0.9 0.9 0.9 0.9 0.9 0.9 1 1.4 1 1\n"
		);

	// Create the ground
	CreateBox(slgScene, "ground", "mat_white", "check_texmap", BBox(Point(-3.f,-3.f,-.1f), Point(3.f, 3.f, 0.f)));
	// Create the red box
	CreateBox(slgScene, "box01", "mat_red", "", BBox(Point(-.5f,-.5f, .2f), Point(.5f, .5f, 0.7f)));
	// Create the glass box
	CreateBox(slgScene, "box02", "mat_glass", "", BBox(Point(1.5f, 1.5f, .3f), Point(2.f, 1.75f, 1.5f)));
	// Create the light
	CreateBox(slgScene, "box03", "whitelight", "", BBox(Point(-1.75f, 1.5f, .75f), Point(-1.5f, 1.75f, .5f)));

	/*// Create an InfiniteLight loaded from file
	scene->AddInfiniteLight(
			"scene.infinitelight.file = scenes/simple-mat/arch.exr\n"
			"scene.infinitelight.gamma = 1.0\n"
			"scene.infinitelight.gain = 3.0 3.0 3.0\n"
			);*/

	// Create a SkyLight & SunLight
	slgScene->AddSkyLight(
			"scene.skylight.dir = 0.166974 0.59908 0.783085\n"
			"scene.skylight.turbidity = 2.2\n"
			"scene.skylight.gain = 0.8 0.8 0.8\n"
			);
	slgScene->AddSunLight(
			"scene.sunlight.dir = 0.166974 0.59908 0.783085\n"
			"scene.sunlight.turbidity = 2.2\n"
			"scene.sunlight.gain = 0.8 0.8 0.8\n"
			);

	return slgScene;
}

void SLGRenderer::Render(Scene *s) {
	luxrays::sdl::Scene *slgScene = NULL;

	{
		// Section under mutex
		boost::mutex::scoped_lock lock(classWideMutex);

		scene = s;

		if (scene->IsFilmOnly()) {
			state = TERMINATE;
			return;
		}

		if (scene->lights.size() == 0) {
			LOG( LUX_SEVERE,LUX_MISSINGDATA)<< "No light sources defined in scene; nothing to render.";
			state = TERMINATE;
			return;
		}

		state = RUN;

		// Initialize the stats
		rendererStatistics->reset();
	
		// Dade - I have to do initiliaziation here for the current thread.
		// It can be used by the Preprocess() methods.

		// initialize the thread's rangen
		u_long seed = scene->seedBase - 1;
		LOG( LUX_DEBUG,LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

		RandomGenerator rng(seed);

		// integrator preprocessing
		scene->sampler->SetFilm(scene->camera->film);
		scene->surfaceIntegrator->Preprocess(rng, *scene);
		scene->volumeIntegrator->Preprocess(rng, *scene);
		scene->camera->film->CreateBuffers();

		scene->surfaceIntegrator->RequestSamples(scene->sampler, *scene);
		scene->volumeIntegrator->RequestSamples(scene->sampler, *scene);

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		// TODO: extend SLG library to accept an handler for each rendering session
		luxrays::sdl::LuxRaysSDLDebugHandler = SDLDebugHandler;

		// Build the SLG scene to render
		slgScene = CreateSLGScene();

		// start the timer
		rendererStatistics->start();

		// Dade - preprocessing done
		preprocessDone = true;
		scene->SetReady();
	}

	//----------------------------------------------------------------------
	// Do the render
	//----------------------------------------------------------------------

	int xStart, xEnd, yStart, yEnd;
	scene->camera->film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	const int imageWidth = xEnd - xStart;
	const int imageHeight = yEnd - yStart;

	slg::RenderConfig *config = new slg::RenderConfig(
			"renderengine.type = PATHOCL\n"
			"sampler.type = INLINED_RANDOM\n"
			"opencl.platform.index = -1\n"
			"opencl.cpu.use = 0\n"
			"opencl.gpu.use = 1\n"
			"image.width = " + boost::lexical_cast<string>(imageWidth) + "\n"
			"image.height = " + boost::lexical_cast<string>(imageHeight) + "\n",
			*slgScene);
	slg::RenderSession *session = new slg::RenderSession(config);
	slg::RenderEngine *engine = session->renderEngine;

	const unsigned int haltTime = config->cfg.GetInt("batch.halttime", 0);
	const unsigned int haltSpp = config->cfg.GetInt("batch.haltspp", 0);
	const float haltThreshold = config->cfg.GetFloat("batch.haltthreshold", -1.f);

	// Start the rendering
	session->Start();
	const double startTime = luxrays::WallClockTime();

	double lastFilmUpdate = startTime;
	char buf[512];
	Film *film = scene->camera->film;
	const luxrays::utils::Film *slgFilm = session->film; 
	for (;;) {
		if (state == PAUSE) {
			session->BeginEdit();
			while (state == PAUSE && !boost::this_thread::interruption_requested())
				boost::this_thread::sleep(boost::posix_time::seconds(1));
			session->EndEdit();
		}
		if ((state == TERMINATE) || boost::this_thread::interruption_requested())
			break;

		boost::this_thread::sleep(boost::posix_time::millisec(1000));

		// Film update may be required by some render engine to
		// update statistics, convergence test and more
		if (luxrays::WallClockTime() - lastFilmUpdate > film->getldrDisplayInterval()) {
			session->renderEngine->UpdateFilm();

			// Update LuxRender film too
			// TODO: use Film write mutex
			ColorSystem colorSpace = film->GetColorSpace();
			for (int y = yStart; y <= yEnd; ++y) {
				for (int x = xStart; x <= xEnd; ++x) {
					const luxrays::utils::SamplePixel *sp = slgFilm->GetSamplePixel(
						luxrays::utils::PER_PIXEL_NORMALIZED, x - xStart, y - yStart);
					const float alpha = slgFilm->IsAlphaChannelEnabled() ?
						(slgFilm->GetAlphaPixel(x - xStart, y - yStart)->alpha) : 0.f;

					XYZColor xyz = colorSpace.ToXYZ(RGBColor(sp->radiance.r, sp->radiance.g, sp->radiance.b));
					// Flip the image upside down
					Contribution contrib(x, imageHeight - 1 - y + yStart, xyz, alpha, 0.f, sp->weight);
					film->SetSample(&contrib);
				}
			}
			film->SetSampleCount(session->renderEngine->GetTotalSampleCount());

			lastFilmUpdate =  luxrays::WallClockTime();
		}

		const double now = luxrays::WallClockTime();
		const double elapsedTime = now - startTime;
		if ((haltTime > 0) && (elapsedTime >= haltTime))
			break;

		const unsigned int pass = engine->GetPass();
		if ((haltSpp > 0) && (pass >= haltSpp))
			break;

		// Convergence test is update inside UpdateFilm()
		const float convergence = engine->GetConvergence();
		if ((haltThreshold >= 0.f) && (1.f - convergence <= haltThreshold))
			break;

		// Print some information about the rendering progress
		sprintf(buf, "[Elapsed time: %3d/%dsec][Samples %4d/%d][Convergence %f%%][Avg. samples/sec % 3.2fM on %.1fK tris]",
				int(elapsedTime), int(haltTime), pass, haltSpp, 100.f * convergence, engine->GetTotalSamplesSec() / 1000000.0,
				config->scene->dataSet->GetTotalTriangleCount() / 1000.0);

		SLG_LOG(buf);

		film->CheckWriteOuputInterval();
	}

	// Stop the rendering
	session->Stop();

	delete session;
	SLG_LOG("Done.");

	// I change the current signal to exit in order to disable the creation
	// of new threads after this point
	Terminate();

	// Flush the contribution pool
	scene->camera->film->contribPool->Flush();
	scene->camera->film->contribPool->Delete();
}

void SLGRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	rendererStatistics->stop();
}

void SLGRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	rendererStatistics->start();
}

void SLGRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------

void DebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "[LuxRays] " << msg;
}

void SDLDebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "[LuxRays::SDL] " << msg;
}

void SLGDebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "[SLG] " << msg;
}

Renderer *SLGRenderer::CreateRenderer(const ParamSet &params) {
	return new SLGRenderer();
}

static DynamicLoader::RegisterRenderer<SLGRenderer> r("slg");
