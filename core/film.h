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

#ifndef LUX_FILM_H
#define LUX_FILM_H
// film.h*
#include "lux.h"
#include "color.h"
#include "error.h"

#include <boost/serialization/split_member.hpp>

namespace lux {

enum ImageType {
    IMAGE_NONE = 0,
    IMAGE_FILEOUTPUT = 1 << 1,
    IMAGE_FRAMEBUFFER = 1 << 2,
    IMAGE_ALL = IMAGE_FILEOUTPUT | IMAGE_FRAMEBUFFER
};

// Buffer types

enum BufferType {
    BUF_TYPE_PER_PIXEL = 0, // Per pixel normalized
    BUF_TYPE_PER_SCREEN, // Per screen normalized
    BUF_TYPE_RAW, // 
    NUM_OF_BUFFER_TYPES
};

enum BufferOutputConfig {
    BUF_FRAMEBUFFER = 1 << 0,
    BUF_STANDALONE = 1 << 1,
    BUF_RAWDATA = 1 << 2
};

class FlexImageFilm;

class ArrSample {
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & sX;
        ar & sY;
        ar & xyz;
        ar & alpha;
        ar & buf_id;
        ar & bufferGroup;
    }

public:
    void Sample() {
        sX = 0;
        sY = 0;
        xyz = XYZColor(0.);
        alpha = 0;
        buf_id = 0;
        bufferGroup = 0;
    }

    float sX;
    float sY;
    XYZColor xyz;
    float alpha;
    int buf_id;
    int bufferGroup;
};

// Film Declarations

class Film {
public:
    // Film Interface

    Film(int xres, int yres, int haltspp) :
		xResolution(xres), yResolution(yres), haltSamplePerPixel(haltspp),
		enoughSamplePerPixel(false) {
		invSamplePerPass =  1.0 / (xResolution * yResolution);
	}
    virtual ~Film() { }

    virtual void AddSample(float sX, float sY, const XYZColor &L, float alpha, int buffer = 0, int bufferGroup = 0) = 0;

    virtual void AddSampleCount(float count, int bufferGroup = 0) { }
    virtual void WriteImage(ImageType type) = 0;
    virtual void GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const = 0;

    virtual int RequestBuffer(BufferType type, BufferOutputConfig output, const string& filePostfix) {
        return 0;
    }

    virtual void CreateBuffers() {
    }
    virtual unsigned char* getFrameBuffer() = 0;
    virtual void updateFrameBuffer() = 0;
    virtual float getldrDisplayInterval() = 0;

    void SetScene(Scene *scene1) {
        scene = scene1;
    }

    // Film Public Data
    int xResolution, yResolution;
    float* flux2radiance;

	// Dade - Samplers will check this flag to know if we have enough samples per
	// pixel and it is time to stop
	int haltSamplePerPixel;
	bool enoughSamplePerPixel;

protected:
    Scene *scene;

	// Dade - 1.0 / (xResolution * yResolution)
	float invSamplePerPass;
};

// Image Pipeline Declarations
extern void ApplyImagingPipeline(float *rgb, int xResolution, int yResolution,
        float *yWeight = NULL, float bloomRadius = .2f,
        float bloomWeight = 0.f, const char *tonemap = NULL,
        const ParamSet *toneMapParams = NULL, float gamma = 2.2,
        float dither = 0.5f, int maxDisplayValue = 255);

}//namespace lux;

#endif // LUX_FILM_H
