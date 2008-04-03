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

/*
 * FlexImage Film class
 *
 */

#include "fleximage.h"
#include "error.h"
#include "scene.h"		// for Scene
#include "camera.h"		// for Camera
#include "transport.h"	// for SurfaceIntegrator::IsFluxBased()
#include "filter.h"

using namespace lux;

// FlexImageFilm Method Definitions
FlexImageFilm::FlexImageFilm(int xres, int yres, Filter *filt, const float crop[4],
	bool outhdr, bool outldr,
	const string &filename1, bool premult, int wI,
	const string &tm, float c_dY, float n_MY,
	float reinhard_prescale, float reinhard_postscale, float reinhard_burn,
	float bw, float br, float g, float d) :
	Film(xres, yres), filter(filt), writeInterval(wI), filename(filename1),
	outputType(IMAGE_NONE), premultiplyAlpha(premult), buffersInited(false),
	toneMapper(tm), bloomWidth(bw), bloomRadius(br), gamma(g), dither(d),
	framebuffer(NULL), imageLock(false), factor(NULL)
{
	// Compute film image extent
	memcpy(cropWindow, crop, 4 * sizeof(float));
	xPixelStart = Ceil2Int(xResolution * cropWindow[0]);
	xPixelCount = max(1, Ceil2Int(xResolution * cropWindow[1]) - xPixelStart);
	yPixelStart = Ceil2Int(yResolution * cropWindow[2]);
	yPixelCount = max(1, Ceil2Int(yResolution * cropWindow[3]) - yPixelStart);

	if (outhdr)
		outputType = (ImageType)(outputType | IMAGE_HDR);
	if (outldr)
		outputType = (ImageType)(outputType | IMAGE_LDR);

	// Set tonemapper params
	if (toneMapper == "contrast") {
		toneParams.AddFloat("displayadaptationY", &c_dY, 1);
	} else if (toneMapper == "nonlinear") {
		toneParams.AddFloat("maxY", &n_MY, 1);
	} else if (toneMapper == "reinhard") {
		toneParams.AddFloat("prescale", &reinhard_prescale, 1);
		toneParams.AddFloat("postscale", &reinhard_postscale, 1);
		toneParams.AddFloat("burn", &reinhard_burn, 1);
	}

	// init timer
	timer.restart();

	// Precompute filter weight table
	#define FILTER_TABLE_SIZE 16
	filterTable = new float[FILTER_TABLE_SIZE * FILTER_TABLE_SIZE];
	float *ftp = filterTable;
	for (int y = 0; y < FILTER_TABLE_SIZE; ++y) {
		float fy = ((float)y + .5f) * filter->yWidth / FILTER_TABLE_SIZE;
		for (int x = 0; x < FILTER_TABLE_SIZE; ++x) {
			float fx = ((float)x + .5f) * filter->xWidth / FILTER_TABLE_SIZE;
			*ftp++ = filter->Evaluate(fx, fy);
		}
	}
}

void FlexImageFilm::GetSampleExtent(int *xstart, int *xend,
	int *ystart, int *yend) const
{
	*xstart = Floor2Int(xPixelStart + .5f - filter->xWidth);
	*xend   = Floor2Int(xPixelStart + .5f + xPixelCount + filter->xWidth);
	*ystart = Floor2Int(yPixelStart + .5f - filter->yWidth);
	*yend   = Floor2Int(yPixelStart + .5f + yPixelCount + filter->yWidth);
}

int FlexImageFilm::RequestBuffer(BufferType type, BufferOutputConfig output,
	const string& filePostfix)
{
	bufferConfigs.push_back(BufferConfig(type, output, filePostfix));
	return bufferConfigs.size() - 1;
}

void FlexImageFilm::CreateBuffers()
{
	// TODO: more groups for multilight
	bufferGroups.push_back(BufferGroup());
	bufferGroups.back().CreateBuffers(bufferConfigs,xPixelCount,yPixelCount);
}
void FlexImageFilm::AddSample(float sX, float sY, const XYZColor &xyz, float alpha, int buf_id, int bufferGroup)
{
	// Issue warning if unexpected radiance value returned
//	assert(!xyz.IsNaN() && xyz.y() >= -1e-5f && !isinf(xyz.y()));
	if (xyz.IsNaN() || xyz.y() < -1e-5f || isinf(xyz.y())) {
		std::stringstream ss;
		ss << "Out of bound intensity in FlexImageFilm::AddSample: "
		   << xyz.y() << ", sample discarded";
		luxError(LUX_LIMIT, LUX_WARNING, ss.str().c_str());
		return;
	}

	// TODO: Find a group
	if (bufferGroups.empty())
	{
		RequestBuffer(BUF_TYPE_PER_SCREEN, BUF_FRAMEBUFFER, "");
		CreateBuffers();
	}

	BufferGroup &currentGroup = bufferGroups[bufferGroup];
	Buffer *buffer = currentGroup.getBuffer(buf_id);

	// Compute sample's raster extent
	float dImageX = sX - 0.5f;
	float dImageY = sY - 0.5f;
	int x0 = Ceil2Int (dImageX - filter->xWidth);
	int x1 = Floor2Int(dImageX + filter->xWidth);
	int y0 = Ceil2Int (dImageY - filter->yWidth);
	int y1 = Floor2Int(dImageY + filter->yWidth);
	x0 = max(x0, xPixelStart);
	x1 = min(x1, xPixelStart + xPixelCount - 1);
	y0 = max(y0, yPixelStart);
	y1 = min(y1, yPixelStart + yPixelCount - 1);
	if (x1 < x0 || y1 < y0) return;
	// Loop over filter support and add sample to pixel arrays
	// Precompute $x$ and $y$ filter table offsets
	int *ifx = (int *)alloca((x1-x0+1) * sizeof(int));				// TODO - radiance - pre alloc memory in constructor for speedup ?
	for (int x = x0; x <= x1; ++x) {
		float fx = fabsf((x - dImageX) *
			filter->invXWidth * FILTER_TABLE_SIZE);
		ifx[x-x0] = min(Floor2Int(fx), FILTER_TABLE_SIZE-1);
	}
	int *ify = (int *)alloca((y1-y0+1) * sizeof(int));				// TODO - radiance - pre alloc memory in constructor for speedup ?
	for (int y = y0; y <= y1; ++y) {
		float fy = fabsf((y - dImageY) *
			filter->invYWidth * FILTER_TABLE_SIZE);
		ify[y-y0] = min(Floor2Int(fy), FILTER_TABLE_SIZE-1);
	}

	{
		boost::mutex::scoped_lock lock(addSampleMutex);
		for (int y = y0; y <= y1; ++y) {
			for (int x = x0; x <= x1; ++x) {
				// Evaluate filter value at $(x,y)$ pixel
				int offset = ify[y-y0]*FILTER_TABLE_SIZE + ifx[x-x0];
				float filterWt = filterTable[offset];
				// Update pixel values with filtered sample contribution
				buffer->Add(x - xPixelStart,y - yPixelStart,
					xyz, alpha, filterWt);
			}
		}
	}
	// Possibly write out in-progress image
	// File output
	// TODO: Use only a output timer.
	// TODO: use a real lock
	if( outputType && !imageLock && Floor2Int(timer.elapsed()) > writeInterval)
	{
		imageLock = true;
		WriteImage(outputType);
		timer.restart();
		imageLock = false;
	}
}

void FlexImageFilm::WriteImage2(ImageType type, float* rgb, float* alpha, string postfix)
{
	if (type & IMAGE_HDR)
		WriteEXRImage(rgb, alpha, filename+postfix+".exr");
	if ((type & IMAGE_LDR) || (type & IMAGE_FRAMEBUFFER && framebuffer))
	{
		ApplyImagingPipeline(rgb,xPixelCount,yPixelCount,NULL,
			bloomRadius,bloomWidth,toneMapper.c_str(),
			&toneParams,gamma,dither,255);
		// Write tonemapped ldr TGA file
		if (type & IMAGE_LDR)
			WriteTGAImage(rgb, alpha, filename+postfix+".tga");
		// Update gui film display
		if (type & IMAGE_FRAMEBUFFER && framebuffer)
		{
			// Copy to framebuffer pixels
			u_int nPix = xPixelCount * yPixelCount;
			for (u_int i=0;  i < nPix*3 ; i++)
				framebuffer[i] = (unsigned char) rgb[i];
		}
	}
}

void FlexImageFilm::ScaleOutput(float *rgb, float *alpha, float *scale)
{
	int x,y;
	int offset = 0;
	for (y = 0; y < yPixelCount; ++y)
	{
		for (x = 0; x < xPixelCount; ++x,++offset)
		{
			rgb[3*offset  ] *= scale[offset];
			rgb[3*offset+1] *= scale[offset];
			rgb[3*offset+2] *= scale[offset];
			alpha[offset] *= scale[offset];
		}
	}
}
void FlexImageFilm::WriteImage(ImageType type)
{
	boost::mutex::scoped_lock lock(addSampleMutex);

	int x,y,offset;
	int nPix = xPixelCount * yPixelCount;
	float *rgb0 = new float[3*nPix], *alpha0 = new float[nPix];
	float *rgb = new float[3*nPix], *alpha = new float[nPix];
	if (factor==NULL)
	{
		factor = new float[nPix];
		scene->camera->GetFlux2RadianceFactors(this,factor,xPixelCount,yPixelCount);
	}
	memset(rgb0,0,nPix*3*sizeof(*rgb0));
	memset(alpha0,0,nPix*sizeof(*alpha0));

	for(u_int j=0;j<bufferGroups.size();++j)
	{
		for(u_int i=0;i<bufferConfigs.size();++i)
		{
			bufferGroups[j].buffers[i]->GetData(rgb, alpha);
			if (bufferConfigs[i].output & BUF_FRAMEBUFFER)
			{
				for (offset = 0, y = 0; y < yPixelCount; ++y)
				{
					for (x = 0; x < xPixelCount; ++x,++offset)
					{
						rgb0[3*offset  ] += rgb[3*offset  ];
						rgb0[3*offset+1] += rgb[3*offset+1];
						rgb0[3*offset+2] += rgb[3*offset+2];
						alpha0[offset] += alpha[offset];
					}
				}
			}
			if (bufferConfigs[i].output & BUF_STANDALONE)
			{
				if (!(bufferConfigs[i].output & BUF_RAWDATA) && scene->surfaceIntegrator->IsFluxBased())
					ScaleOutput(rgb,alpha,factor);
				WriteImage2(type, rgb, alpha, bufferConfigs[i].postfix);
			}
		}

		if (scene->surfaceIntegrator->IsFluxBased())
		{
			ScaleOutput(rgb0,alpha0,factor);
		}
		WriteImage2(type, rgb0, alpha0, "");
	}

	delete[] rgb;
	delete[] alpha;
	delete[] rgb0;
	delete[] alpha0;
}


// GUI display methods
void FlexImageFilm::createFrameBuffer()
{
	// allocate pixels
	unsigned int nPix = xPixelCount * yPixelCount;
	framebuffer = new unsigned char[3*nPix];			// TODO delete data

	// zero it out
	memset(framebuffer,0,sizeof(*framebuffer)*3*nPix);
}

void FlexImageFilm::updateFrameBuffer()
{
	if(!framebuffer)
		createFrameBuffer();

	WriteImage(IMAGE_FRAMEBUFFER);
}

unsigned char* FlexImageFilm::getFrameBuffer()
{
	if(!framebuffer)
		createFrameBuffer();

	return framebuffer;
}

void FlexImageFilm::WriteTGAImage(float *rgb, float *alpha, const string &filename)
{
	//printf("\nWriting Tonemapped TGA image to file \"%s\"...\n", filename.c_str());
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing Tonemapped TGA image to file ")+filename).c_str());

	// Open file
	FILE* tgaFile = fopen(filename.c_str(),"wb");
	if (!tgaFile) {
		std::stringstream ss;
		ss<< "Cannot open file '"<<filename<<"' for output";
		luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());
		return;
	}

	// write the header
	// make sure its platform independent of little endian and big endian
	char header[18];
	memset(header, 0,sizeof(char)*18);

	header[2] = 2;							// set the data type of the targa (2 = uncompressed)
	short xResShort = xResolution;			// set the resolution and make sure the bytes are in the right order
	header[13] = (char) (xResShort >> 8);
	header[12] = xResShort;
	short yResShort = yResolution;
	header[15] = (char) (yResShort >> 8);
	header[14] = yResShort;
	header[16] = 32;						// set the bitdepth

	// put the header data into the file
	for (int i=0; i < 18; i++)
		fputc(header[i],tgaFile);

	// write the bytes of data out
	for (int i=yPixelCount-1;  i >= 0 ; i--) {
		for (int j=0;  j < xPixelCount; j++) {
			fputc((int) rgb[(i*xPixelCount+j)*3+2], tgaFile);
			fputc((int) rgb[(i*xPixelCount+j)*3+1], tgaFile);
			fputc((int) rgb[(i*xPixelCount+j)*3+0], tgaFile);
			// NOTE - radiance - removed alpha output in TGA files due to errors
			fputc((int) 255.0, tgaFile);
			//fputc((int) (255.0*alpha[(i*xPixelCount+j)]), tgaFile);
		}
	}

	fclose(tgaFile);
	//printf("Done.\n");
}

void FlexImageFilm::WriteEXRImage(float *rgb, float *alpha, const string &filename)
{
	// Write OpenEXR RGBA image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing OpenEXR image to file ")+filename).c_str());
	WriteRGBAImageFloat(filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart);
}

void FlexImageFilm::clean() {

}

void FlexImageFilm::merge(FlexImageFilm &f) {

}

// params / creation

Film* FlexImageFilm::CreateFilm(const ParamSet &params, Filter *filter)
{
	// General
	bool premultiplyAlpha = params.FindOneBool("premultiplyalpha", true);

	int xres = params.FindOneInt("xresolution", 800);
	int yres = params.FindOneInt("yresolution", 600);

	float crop[4] = { 0, 1, 0, 1 };
	int cwi;
	const float *cr = params.FindFloat("cropwindow", &cwi);
	if (cr && cwi == 4) {
		crop[0] = Clamp(min(cr[0], cr[1]), 0.f, 1.f);
		crop[1] = Clamp(max(cr[0], cr[1]), 0.f, 1.f);
		crop[2] = Clamp(min(cr[2], cr[3]), 0.f, 1.f);
		crop[3] = Clamp(max(cr[2], cr[3]), 0.f, 1.f);
	}

	bool outhdr = params.FindOneBool("outhdr", true);
	bool outldr = params.FindOneBool("outldr", false);
	// output filenames
	string filename = params.FindOneString("filename", "-");

	// intervals
	int writeInterval = params.FindOneInt("writeinterval", 30);

	// tonemapper
	string toneMapper = params.FindOneString("tonemapper", "reinhard");
	float contrast_displayAdaptationY = params.FindOneFloat("contrast_displayadaptationY", 50.0f);
	float nonlinear_MaxY = params.FindOneFloat("nonlinear_maxY", 0.0f);
	float reinhard_prescale = params.FindOneFloat("reinhard_prescale", 1.0f);
	float reinhard_postscale = params.FindOneFloat("reinhard_postscale", 1.0f);
	float reinhard_burn = params.FindOneFloat("reinhard_burn", 6.0f);

	// Pipeline correction
	float bloomWidth = params.FindOneFloat("bloomwidth", 0.0f);
	float bloomRadius = params.FindOneFloat("bloomradius", 0.0f);
	float gamma = params.FindOneFloat("gamma", 2.2f);
	float dither = params.FindOneFloat("dither", 0.0f);

	return new FlexImageFilm(xres, yres, filter, crop, outhdr, outldr,
		filename, premultiplyAlpha, writeInterval, 
		toneMapper, contrast_displayAdaptationY, nonlinear_MaxY,
		reinhard_prescale, reinhard_postscale, reinhard_burn,
		bloomWidth, bloomRadius, gamma, dither);
}



