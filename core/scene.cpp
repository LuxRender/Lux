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

// scene.cpp*
#include <sstream>
#include <stdlib.h>

#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "volume.h"
#include "error.h"
#include "bxdf.h"
#include "light.h"
#include "spectrumwavelengths.h"
#include "transport.h"

#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/random/mersenne_twister.hpp>
boost::hash<boost::uuids::uuid> uuid_hasher;
boost::mt19937 scene_rng(uuid_hasher(boost::uuids::random_generator()()));
boost::mutex scene_rand_mutex;

using namespace lux;

void Scene::SaveEXR(const string& filename, bool useHalfFloat, bool includeZBuffer, int compressionType, bool tonemapped) {
	camera()->film->SaveEXR(filename, useHalfFloat, includeZBuffer, compressionType, tonemapped);
}

// Framebuffer Access for GUI
void Scene::UpdateFramebuffer() {
    camera()->film->updateFrameBuffer();
}

unsigned char* Scene::GetFramebuffer() {
    return camera()->film->getFrameBuffer();
}

float* Scene::GetFloatFramebuffer() {
    return camera()->film->getFloatFrameBuffer();
}

float* Scene::GetAlphaBuffer() {
    return camera()->film->getAlphaBuffer();
}

float* Scene::GetZBuffer() {
    return camera()->film->getZBuffer();
}

// histogram access for GUI
void Scene::GetHistogramImage(unsigned char *outPixels, u_int width, u_int height, int options){
	camera()->film->getHistogramImage(outPixels, width, height, options);
}


// Parameter Access functions
void Scene::SetParameterValue(luxComponent comp, luxComponentParameters param, double value, u_int index) { 
	if(comp == LUX_FILM)
		camera()->film->SetParameterValue(param, value, index);
}
double Scene::GetParameterValue(luxComponent comp, luxComponentParameters param, u_int index) {
	if(comp == LUX_FILM)
		return camera()->film->GetParameterValue(param, index);
	else
		return 0.;
}
double Scene::GetDefaultParameterValue(luxComponent comp, luxComponentParameters param, u_int index) {
	if(comp == LUX_FILM)
		return camera()->film->GetDefaultParameterValue(param, index);
	else
		return 0.;
}
void Scene::SetStringParameterValue(luxComponent comp, luxComponentParameters param, const string& value, u_int index) { 
}
string Scene::GetStringParameterValue(luxComponent comp, luxComponentParameters param, u_int index) {
	if(comp == LUX_FILM)
		return camera()->film->GetStringParameterValue(param, index);
	else
		return "";
}
string Scene::GetDefaultStringParameterValue(luxComponent comp, luxComponentParameters param, u_int index) {
	return "";
}

int Scene::DisplayInterval() {
    return camera()->film->getldrDisplayInterval();
}

u_int Scene::FilmXres() {
    return camera()->film->GetXPixelCount();
}

u_int Scene::FilmYres() {
    return camera()->film->GetYPixelCount();
}

Scene::~Scene() {
	delete sampler;
	delete surfaceIntegrator;
	delete volumeIntegrator;
	delete volumeRegion;
	for (u_int i = 0; i < lights.size(); ++i)
		delete lights[i];
}

Scene::Scene(Camera *cam, SurfaceIntegrator *si, VolumeIntegrator *vi,
	Sampler *s, vector<boost::shared_ptr<Primitive> > prims, boost::shared_ptr<Primitive> &accel,
	const vector<Light *> &lts, const vector<string> &lg, Region *vr) :
	ready(false), aggregate(accel), lights(lts),
	lightGroups(lg), camera(cam), volumeRegion(vr), surfaceIntegrator(si),
	volumeIntegrator(vi), sampler(s), terminated(false), primitives(prims),
	filmOnly(false)
{
	// Scene Constructor Implementation
	bound = Union(aggregate->WorldBound(), camera()->Bounds());
	if (volumeRegion)
		bound = Union(bound, volumeRegion->WorldBound());

	// Dade - Initialize the base seed with the standard C lib random number generator
	scene_rand_mutex.lock();
	seedBase = scene_rng();
	scene_rand_mutex.unlock();

	camera()->film->RequestBufferGroups(lightGroups);
}

Scene::Scene(Camera *cam) :
	camera(cam), volumeRegion(NULL), surfaceIntegrator(NULL),
	volumeIntegrator(NULL), sampler(NULL),
	filmOnly(true)
{
	for(u_int i = 0; i < cam->film->GetNumBufferGroups(); i++)
		lightGroups.push_back( cam->film->GetGroupName(i) );

	// Dade - Initialize the base seed with the standard C lib random number generator
	scene_rand_mutex.lock();
	seedBase = scene_rng();
	scene_rand_mutex.unlock();
}

SWCSpectrum Scene::Li(const Ray &ray, const Sample &sample, float *alpha) const
{
//  NOTE - radiance - leave these off for now, should'nt be used (broken with multithreading)
//  TODO - radiance - cleanup / reimplement into integrators
//	SWCSpectrum Lo = surfaceIntegrator->Li(this, ray, sample, alpha);
//	SWCSpectrum T = volumeIntegrator->Transmittance(this, ray, sample, alpha);
//	SWCSpectrum Lv = volumeIntegrator->Li(this, ray, sample, alpha);
//	return T * Lo + Lv;
	return 0.f;
}

void Scene::Transmittance(const Ray &ray, const Sample &sample,
	SWCSpectrum *const L) const {
	volumeIntegrator->Transmittance(*this, ray, sample, NULL, L);
}
