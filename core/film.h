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
#include "error.h"

#include <boost/serialization/split_member.hpp>

namespace lux
{

class MultiImageFilm;

// Film Declarations
class Film {
	friend class boost::serialization::access;
public:
	// Film Interface
	Film(int xres, int yres) :
		xResolution(xres), yResolution(yres) {
	}
	virtual ~Film() {
	}
	virtual void AddSample(float sX, float sY, const SWCSpectrum &L, float alpha) = 0;
	virtual void AddSample(float sX, float sY, const XYZColor &L, float alpha) = 0;
	virtual void WriteImage() = 0;
	virtual void WriteImage(int oType) = 0;
	virtual void
			GetSampleExtent(int *xstart, int *xend, int *ystart, int *yend) const = 0;
	virtual unsigned char* getFrameBuffer() = 0;
	virtual void updateFrameBuffer() = 0;
	virtual float getldrDisplayInterval() = 0;

	virtual void merge(MultiImageFilm &f) {luxError(LUX_BUG,LUX_ERROR,"Invalid call to Film::merge()");}
	virtual void clean() {luxError(LUX_BUG,LUX_ERROR,"Invalid call to Film::clean()");}
	// Film Public Data
	int xResolution, yResolution;

private:

	//template<class Archive>
	//void serialize(Archive & ar, const unsigned int version)
	/*void serialize(std::string & ar, const unsigned int version)
	 {
	 //ar & xResolution;
	 //ar & yResolution;
	 }*/

	virtual void save(boost::archive::text_oarchive & ar, const unsigned int version) const {
		ar & xResolution;
		ar & yResolution;
	}

	virtual void load(boost::archive::text_iarchive & ar, const unsigned int version)
	{
		ar & xResolution;
		ar & yResolution;
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()
	
	//template<class Archive>
	//	virtual void serialize(Archive & ar, const unsigned int version)=0;

};
// Image Pipeline Declarations
extern void ApplyImagingPipeline(float *rgb, int xResolution, int yResolution,
		float *yWeight = NULL, float bloomRadius = .2f,
		float bloomWeight = 0.f, const char *tonemap = NULL,
		const ParamSet *toneMapParams = NULL, float gamma = 2.2,
		float dither = 0.5f, int maxDisplayValue = 255);

}//namespace lux;

#endif // LUX_FILM_H
