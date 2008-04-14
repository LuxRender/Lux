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

/*
 * MultiImage Film class
 *
 * This extended Film class allows for configurable multiple outputs
 * in hdr and ldr (tonemapped) image file formats, aswell as updating the
 * lux gui film display.
 * The original 'Image' film class was left intact as to ensure
 * backwards compatibility with .pbrt scene files.
 *
 * 30/09/07 - Radiance - Initial Version
 */

#include "multiimage.h"
#include "error.h"

using namespace lux;

// MultiImageFilm Method Definitions
MultiImageFilm::MultiImageFilm(int xres, int yres,
	                     Filter *filt, const float crop[4], bool hdr_out, bool igi_out, bool ldr_out,
		             const string &hdr_filename, const string &igi_filename, const string &ldr_filename, bool premult,
		             int hdr_wI, int igi_wI, int ldr_wI, int ldr_dI,
					 const string &tm, float c_dY, float n_MY,
					 float reinhard_prescale, float reinhard_postscale, float reinhard_burn,
					 float bw, float br, float g, float d)
	: Film(xres, yres) {
	filter = filt;
	memcpy(cropWindow, crop, 4 * sizeof(float));
	hdrFilename = hdr_filename;
	igiFilename = igi_filename;
	ldrFilename = ldr_filename;
	premultiplyAlpha = premult;
	sampleCount = 0;

	hdrWriteInterval = hdr_wI;
	igiWriteInterval = hdr_wI;
    ldrWriteInterval = ldr_wI;
    ldrDisplayInterval = ldr_dI;
	hdrOut = hdr_out;
	igiOut = igi_out;
	ldrOut = ldr_out;
	//ldrDisplayOut = ldrDisplay_out;

    contrastDisplayAdaptationY = c_dY;
    nonlinearMaxY = n_MY;
	reinhardPrescale = reinhard_prescale;
	reinhardPostscale = reinhard_postscale;
	reinhardBurn = reinhard_burn;
	toneMapper = tm;
	bloomWidth = bw;
	bloomRadius = br;
	gamma = g;
	dither = d;

	framebuffer = NULL;

	// Set tonemapper params
	if( toneMapper == "contrast" ) {
		string st = "displayadaptationY";
		toneParams.AddFloat(st, &contrastDisplayAdaptationY, 1);
	} else if( toneMapper == "nonlinear" ) {
		string st = "maxY";
		toneParams.AddFloat(st, &nonlinearMaxY, 1);
	} else if( toneMapper == "reinhard" ) {
		string st = "prescale";
		toneParams.AddFloat(st, &reinhardPrescale, 1);
		string st2 = "postscale";
		toneParams.AddFloat(st2, &reinhardPostscale, 1);
		string st3 = "burn";
        toneParams.AddFloat(st3, &reinhardBurn, 1);
	}

	// init locks and timers
	ldrLock = false;
	hdrLock = false;
	igiLock = false;
	ldrDisplayLock = false;
	ldrTimer.restart();
	igiTimer.restart();
	hdrTimer.restart();
	ldrDisplayTimer.restart();

	// Compute film image extent
	xPixelStart = Ceil2Int(xResolution * cropWindow[0]);
	xPixelCount =
		max(1, Ceil2Int(xResolution * cropWindow[1]) - xPixelStart);
	yPixelStart =
		Ceil2Int(yResolution * cropWindow[2]);
	yPixelCount =
		max(1, Ceil2Int(yResolution * cropWindow[3]) - yPixelStart);
	// Allocate film image storage
	pixels = new BlockedArray<Pixel>(xPixelCount, yPixelCount);
	// Precompute filter weight table
	#define FILTER_TABLE_SIZE 16
	filterTable =
		new float[FILTER_TABLE_SIZE * FILTER_TABLE_SIZE];
	float *ftp = filterTable;
	for (int y = 0; y < FILTER_TABLE_SIZE; ++y) {
		float fy = ((float)y + .5f) * filter->yWidth /
			FILTER_TABLE_SIZE;
		for (int x = 0; x < FILTER_TABLE_SIZE; ++x) {
			float fx = ((float)x + .5f) * filter->xWidth /
				FILTER_TABLE_SIZE;
			*ftp++ = filter->Evaluate(fx, fy);
		}
	}
}

void MultiImageFilm::AddSample(float sX, float sY, const XYZColor &L, float alpha, int buf_id, int bufferGroup)
{
	//boost::mutex::scoped_lock lock(addSampleMutex);

	XYZColor xyz = L;

	// Issue warning if unexpected radiance value returned
	if (xyz.IsNaN()) {
		//std::stringstream error;
		//error<<"THR"<<myThread->n+1<<": Nan radiance value returned.";
		//luxError(LUX_BUG,LUX_ERROR,error.str().c_str());
		//L = SWCSpectrum(0.f);
		return;
	}
	else if (xyz.y() < -1e-5) {
		//std::stringstream error;
		//error<<"THR"<<myThread->n+1<<": NegLum value, "<<Ls.y()<<" returned.";
		//luxError(LUX_BUG,LUX_ERROR,error.str().c_str());
		//L = SWCSpectrum(0.f);
		return;
	}
	else if (isinf(xyz.y())) {
		//luxError(LUX_BUG,LUX_ERROR,"InfinLum value returned.");
		//L = SWCSpectrum(0.f);
		return;
	}

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
	if ((x1-x0) < 0 || (y1-y0) < 0) return;
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
	for (int y = y0; y <= y1; ++y)
		for (int x = x0; x <= x1; ++x) {
			// Evaluate filter value at $(x,y)$ pixel
			int offset = ify[y-y0]*FILTER_TABLE_SIZE + ifx[x-x0];
			float filterWt = filterTable[offset];
			// Update pixel values with filtered sample contribution
			Pixel &pixel = (*pixels)(x - xPixelStart, y - yPixelStart);
			pixel.L.AddWeighted(filterWt, xyz);
			pixel.alpha += alpha * filterWt;
			pixel.weightSum += filterWt;
		}

    // Possibly write out in-progress image

/*	// LDR Display buffer
	if(ldrDisplayOut && !ldrDisplayLock)
		if(Floor2Int(ldrDisplayTimer.elapsed()) > ldrDisplayInterval) {
			ldrDisplayLock = true;
			WriteImage( IMAGE_FRAMEBUFFER );
			ldrDisplayTimer.restart();
			ldrDisplayLock = false;
		} */

	// LDR File output
	if(ldrOut && !ldrLock)
		if(Floor2Int(ldrTimer.elapsed()) > ldrWriteInterval) {
			ldrLock = true;
			WriteImage( IMAGE_LDR );
			ldrTimer.restart();
			ldrLock = false;
		}

	// HDR File output
	if(hdrOut && !hdrLock)
	    if (Floor2Int(hdrTimer.elapsed()) > hdrWriteInterval) {
			hdrLock = true;
			WriteImage( IMAGE_HDR );
			hdrTimer.restart();
			hdrLock = false;
		}
	//// IGI File output
	//if(igiOut && !igiLock)
	//    if (Floor2Int(igiTimer.elapsed()) > igiWriteInterval) {
	//		igiLock = true;
	//		WriteImage( WI_IGI );
	//		igiTimer.restart();
	//		igiLock = false;
	//	}
}

void MultiImageFilm::GetSampleExtent(int *xstart,
		int *xend, int *ystart, int *yend) const {
	*xstart = Floor2Int(xPixelStart + .5f - filter->xWidth);
	*xend   = Floor2Int(xPixelStart + .5f + xPixelCount  +
		filter->xWidth);
	*ystart = Floor2Int(yPixelStart + .5f - filter->yWidth);
	*yend   = Floor2Int(yPixelStart + .5f + yPixelCount +
		filter->yWidth);
}

void MultiImageFilm::WriteImage(ImageType type) {
	// Convert image to RGB and compute final pixel values
	int nPix = xPixelCount * yPixelCount;
	float *rgb = new float[3*nPix], *alpha = new float[nPix];
	int offset = 0;
	for (int y = 0; y < yPixelCount; ++y) {
		for (int x = 0; x < xPixelCount; ++x) {
			// Convert pixel XYZ radiance to RGB
			float xyz[3];
			xyz[0] = (*pixels)(x, y).L.c[0];
			xyz[1] = (*pixels)(x, y).L.c[1];
			xyz[2] = (*pixels)(x, y).L.c[2];
			const float
				rWeight[3] = { 3.240479f, -1.537150f, -0.498535f };
			const float
				gWeight[3] = {-0.969256f,  1.875991f,  0.041556f };
			const float
				bWeight[3] = { 0.055648f, -0.204043f,  1.057311f };
			rgb[3*offset  ] = rWeight[0]*xyz[0] +
			                  rWeight[1]*xyz[1] +
				              rWeight[2]*xyz[2];
			rgb[3*offset+1] = gWeight[0]*xyz[0] +
			                  gWeight[1]*xyz[1] +
				              gWeight[2]*xyz[2];
			rgb[3*offset+2] = bWeight[0]*xyz[0] +
			                  bWeight[1]*xyz[1] +
				              bWeight[2]*xyz[2];
			alpha[offset] = (*pixels)(x, y).alpha;
			// Normalize pixel with weight sum
			float weightSum = (*pixels)(x, y).weightSum;
			if (weightSum != 0.f) {
				float invWt = 1.f / weightSum;
				rgb[3*offset  ] =
					max(rgb[3*offset  ] * invWt, 0.f);
				rgb[3*offset+1] =
					max(rgb[3*offset+1] * invWt, 0.f);
				rgb[3*offset+2] =
					max(rgb[3*offset+2] * invWt, 0.f);
				// NOTE - radiance - leave off for now, creates rounding error and square filter blocks/artifacts
				//alpha[offset] = Clamp(alpha[offset] * invWt, 0.f, 1.f);
			}
			// Compute premultiplied alpha color
			if (premultiplyAlpha) {
				rgb[3*offset  ] *= alpha[offset];
				rgb[3*offset+1] *= alpha[offset];
				rgb[3*offset+2] *= alpha[offset];
			}
			++offset;
		}
	}

	if (type & IMAGE_HDR) // Write hdr EXR file
		WriteEXRImage(rgb, alpha, hdrFilename);
	if (type & IMAGE_LDR) // Write tonemapped ldr TGA file
	{
	    ApplyImagingPipeline(rgb,xPixelCount,yPixelCount,NULL,
		  bloomRadius,bloomWidth,toneMapper.c_str(),
		  &toneParams,gamma,dither,255);
		WriteTGAImage(rgb, alpha, ldrFilename);
	}
	if (type & IMAGE_FRAMEBUFFER)
	{
		// Update gui film display
		ApplyImagingPipeline(rgb,xPixelCount,yPixelCount,NULL,
		  bloomRadius,bloomWidth,toneMapper.c_str(),
		  &toneParams,gamma,dither,255);
		// Copy to framebuffer pixels
		u_int nPix = xPixelCount * yPixelCount;
		for (u_int i=0;  i < nPix*3 ; i++) {
				framebuffer[i] = (unsigned char) rgb[i];
		}
	}

	// Release temporary image memory
	delete[] alpha;
	delete[] rgb;
}

void MultiImageFilm::WriteTGAImage(float *rgb, float *alpha, const string &filename)
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

void MultiImageFilm::WriteEXRImage(float *rgb, float *alpha, const string &filename)
{
	// Write OpenEXR RGBA image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing OpenEXR image to file ")+filename).c_str());
	WriteRGBAImage(filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart);
}

void MultiImageFilm::WriteIGIImage(float *rgb, float *alpha, const string &filename)
{
	// Write IGI image
	luxError(LUX_NOERROR, LUX_INFO, (std::string("Writing IGI image to file ")+filename).c_str());
	WriteIgiImage(filename, rgb, alpha,
		xPixelCount, yPixelCount,
		xResolution, yResolution,
		xPixelStart, yPixelStart);
}

void MultiImageFilm::createFrameBuffer()
{
	// allocate pixels
	unsigned int nPix = xPixelCount * yPixelCount;
    framebuffer = new unsigned char[3*nPix];			// TODO delete data

	// zero it out
	for(unsigned int i = 0; i < 3*nPix; i++)
		framebuffer[i] = (unsigned char)(0);
}

void MultiImageFilm::updateFrameBuffer()
{
	if(!framebuffer)
		createFrameBuffer();

	WriteImage(IMAGE_FRAMEBUFFER);
}

unsigned char* MultiImageFilm::getFrameBuffer()
{
	if(!framebuffer)
		createFrameBuffer();

	return framebuffer;
}

Film* MultiImageFilm::CreateFilm(const ParamSet &params, Filter *filter)
{
	bool hdr_out, igi_out, ldr_out;

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

	// output filenames
	string hdr_filename = params.FindOneString("hdr_filename", "-");
	string igi_filename = params.FindOneString("igi_filename", "-");
	string ldr_filename = params.FindOneString("ldr_filename", "-");
	if(igi_filename == "-")
		if(hdr_filename == "-")
			if(ldr_filename == "-")
				ldr_filename = "luxout.tga"; // default to ldr output
	hdr_out = igi_out = ldr_out = false;
	if( hdr_filename != "-") hdr_out = true;
	if( igi_filename != "-") igi_out = true;
	if( ldr_filename != "-") ldr_out = true;

	// intervals
	int hdr_writeInterval = params.FindOneInt("hdr_writeinterval", 60);
	int igi_writeInterval = params.FindOneInt("igi_writeinterval", 60);
	int ldr_writeInterval = params.FindOneInt("ldr_writeinterval", 30);
	int ldr_displayInterval = params.FindOneInt("ldr_displayinterval", 10);

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

	return new MultiImageFilm(xres, yres, filter, crop,
		hdr_out, igi_out, ldr_out, hdr_filename, igi_filename, ldr_filename, premultiplyAlpha,
		hdr_writeInterval, igi_writeInterval, ldr_writeInterval, ldr_displayInterval,
        toneMapper, contrast_displayAdaptationY, nonlinear_MaxY,
		reinhard_prescale, reinhard_postscale, reinhard_burn,
		bloomWidth, bloomRadius, gamma, dither);
}

void MultiImageFilm::clean() {
	boost::mutex::scoped_lock lock(addSampleMutex);
	//std::cout<<"cleaning film"<<::std::endl;
       for (int y = 0; y < yPixelCount; ++y) {
          for (int x = 0; x < xPixelCount; ++x) {
             (*pixels)(x, y).L = 0.;
             (*pixels)(x, y).alpha = 0.;
             (*pixels)(x, y).weightSum = 0.;
          }
       }
    }

void MultiImageFilm::merge(MultiImageFilm &f) {
	boost::mutex::scoped_lock lock(addSampleMutex);
	//std::cout<<"merging film"<<::std::endl;
       for (int y = 0; y < yPixelCount; ++y) {
          for (int x = 0; x < xPixelCount; ++x) {
             (*pixels)(x, y).L += (*f.pixels)(x, y).L;
             (*pixels)(x, y).alpha += (*f.pixels)(x, y).alpha;
             (*pixels)(x, y).weightSum += (*f.pixels)(x, y).weightSum;
          }
       }
    }


