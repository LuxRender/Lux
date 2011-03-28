/***************************************************************************
 *   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 *
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

#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QString>
#include <QImage>
#include <QFontMetrics>

#include "api.h"

template<class T> inline T Clamp(T val, T low, T high) {
	return val > low ? (val < high ? val : high) : low;
}

int ValueToLogSliderVal(float value, const float logLowerBound, const float logUpperBound, const float slider_resolution);
float LogSliderValToValue(int sliderval, const float logLowerBound, const float logUpperBound, const float slider_resolution);

QString pathElidedText(const QFontMetrics &fm, const QString &text, int width, int flags = 0);

void overlayStatistics(QImage *image);	
bool saveCurrentImageTonemapped(const QString &outFile, bool overlayStats = false, bool outputAlpha = false);

#endif //GUIUTIL_H