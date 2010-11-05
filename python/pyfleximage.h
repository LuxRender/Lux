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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_PYFLEXIMAGE_H
#define LUX_PYFLEXIMAGE_H

void export_PyFlexImageFilm()
{
	using namespace boost::python;
	using namespace lux;

	// Set up a fake module for these items
	object flexImageFilm(handle<>(borrowed(PyImport_AddModule("pylux.FlexImageFilm"))));
	scope().attr("FlexImageFilm") = flexImageFilm;
	scope flexImageFilmScope = flexImageFilm;

	/*
	enum_<FlexImageFilm::OutputChannels>("OutputChannels", "")
		.value("Y", FlexImageFilm::Y)
		.value("YA", FlexImageFilm::YA)
		.value("RGB", FlexImageFilm::RGB)
		.value("RGBA", FlexImageFilm::RGBA)
		;

	enum_<FlexImageFilm::ZBufNormalization>("ZBufNormalization", "")
		.value("None", FlexImageFilm::None)
		.value("CameraStartEnd", FlexImageFilm::CameraStartEnd)
		.value("MinMax", FlexImageFilm::MinMax)
		;
	*/

	// TODO: don't rely on arbitrary int values, or having to
	// include half the core headers
	enum_<int>("TonemapKernels", "")
		.value("Reinhard",	0) //FlexImageFilm::TMK_Reinhard)
		.value("Linear",	1) //FlexImageFilm::TMK_Linear)
		.value("Contrast",	2) //FlexImageFilm::TMK_Contrast)
		.value("MaxWhite",	3) //FlexImageFilm::TMK_MaxWhite)
		.value("AutoLinear",4) //FlexImageFilm::TMK_AutoLinear)
		;
};

#endif	// LUX_PYFLEXIMAGE_H