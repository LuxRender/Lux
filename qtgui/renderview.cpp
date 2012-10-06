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

#include "renderview.hxx"
#include "api.h"
#include "error.h"
#include "guiutil.h"

#include <math.h>
#include <iostream>
#include <algorithm>

using namespace std;

RenderView::RenderView(QWidget *parent) : QGraphicsView(parent) {
	renderscene = new QGraphicsScene();
	renderscene->setBackgroundBrush(QColor(127,127,127));
	luxlogo = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb->hide ();
	userSamplingMap = NULL;
	userSamplingPixmap = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	userSamplingPixmap->hide ();
	renderscene->setSceneRect (0.0f, 0.0f, 416, 389);
	centerOn(luxlogo);
	setScene(renderscene);
	zoomfactor = 100.0f;
	overlayStats = false;
	showAlpha = false;

	showUserSamplingMap = false;
	userSamplingAddictivePen = true;
	userSamplingPenPressed = false;
	userSamplingPenX = 0;
	userSamplingPenY = 0;
	userSamplingPenSize = 50;
}

RenderView::~RenderView () {
	delete[] userSamplingMap;
	delete userSamplingPixmap;
	delete luxfb;
	delete luxlogo;
	delete renderscene;
}

void RenderView::copyToClipboard()
{
	if ((luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) && luxfb->isVisible()) {
		QImage image = getFramebufferImage(overlayStats, showAlpha);
		if (image.isNull()) {
			LOG(LUX_ERROR, LUX_SYSTEM) << tr("Error getting framebuffer").toLatin1().data();
			return;
		}

		QClipboard *clipboard = QApplication::clipboard();
		// QT assumes 32bpp images for clipboard (DIBs)
		if (!clipboard) {
			LOG(LUX_ERROR, LUX_SYSTEM) << tr("Copy to clipboard failed, unable to open clipboard").toLatin1().data();
			return;
		}
		clipboard->setImage(image.convertToFormat(QImage::Format_ARGB32));
	}
}

void RenderView::setShowUserSamplingMap(bool value) {
	if (showUserSamplingMap == value)
		return;

	showUserSamplingMap = value;
	userSamplingPenPressed = false;
	userSamplingPenX = 0;
	userSamplingPenY = 0;
}

void RenderView::reload () {
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		int w = luxGetIntAttribute("film", "xResolution");
		int h = luxGetIntAttribute("film", "yResolution");
			
		QImage image = getFramebufferImage(overlayStats, showAlpha);
		if (showAlpha == true) {
			QPixmap checkerboard(":/images/checkerboard.png");
			renderscene->setBackgroundBrush(checkerboard);
		} else {
			renderscene->setBackgroundBrush(QColor(127,127,127));
		}
		
		if (image.isNull())
			return;

		if (luxlogo->isVisible ())
			luxlogo->hide ();
		
		luxfb->setPixmap(QPixmap::fromImage(image));

		if (!luxfb->isVisible()) {
			resetTransform ();
			luxfb->show ();
			renderscene->setSceneRect (0.0f, 0.0f, w, h);
			centerOn(luxfb);
			//fitInView(luxfb, Qt::KeepAspectRatio);
		}

		if (showUserSamplingMap) {
			// User driven sampling is active

			setDragMode(QGraphicsView::NoDrag);
			RenderView::setCursor(QCursor(Qt::CrossCursor));
			if (!userSamplingMap) {
				// Get the current sampling map

				userSamplingMap = luxGetUserSamplingMap();
				if (!userSamplingMap) {
					// There isn't an existing map, create the default
					userSamplingMap = new float[w * h];
					std::fill(userSamplingMap, userSamplingMap + w * h, .1f);

					// Enable user driven sampling
					luxSetUserSamplingMap(userSamplingMap);
				}
			}

			updateUserSamplingPixmap();

			if (!userSamplingPixmap->isVisible())
				userSamplingPixmap->show();
		} else {
			if (userSamplingPixmap->isVisible())
				userSamplingPixmap->hide();
				setDragMode(QGraphicsView::ScrollHandDrag);
				RenderView::unsetCursor();
		}
		
		zoomEnabled = true;
		setInteractive(true);
	}
}

void  RenderView::setUserSamplingPen(const bool addictive) {
	userSamplingAddictivePen = addictive;
	updateUserSamplingPixmap();
}

void  RenderView::setUserSamplingPensize(const uint size) {
	userSamplingPenSize = std::max(1u, size);
	updateUserSamplingPixmap();
}

void  RenderView::applyUserSampling() {
	luxSetUserSamplingMap(userSamplingMap);
}

void  RenderView::undoUserSampling() {
	delete[] userSamplingMap;
	userSamplingMap = luxGetUserSamplingMap();
	if (!userSamplingMap) {
		int w = luxGetIntAttribute("film", "xResolution");
		int h = luxGetIntAttribute("film", "yResolution");
		// There isn't an existing map, create the default
		userSamplingMap = new float[w * h];
		std::fill(userSamplingMap, userSamplingMap + w * h, .1f);
	}

	updateUserSamplingPixmap();
}

void  RenderView::resetUserSampling() {
	int w = luxGetIntAttribute("film", "xResolution");
	int h = luxGetIntAttribute("film", "yResolution");

	if (!userSamplingMap) {
		// There isn't an existing map, create the default
		userSamplingMap = new float[w * h];
	}
	std::fill(userSamplingMap, userSamplingMap + w * h, .1f);

	updateUserSamplingPixmap();
}

void RenderView::updateUserSamplingPixmap() {
	int w = luxGetIntAttribute("film", "xResolution");
	int h = luxGetIntAttribute("film", "yResolution");

	// Convert from float to ARGB32
	QImage samplingMapImage(w, h, QImage::Format_ARGB32);
	for (int y = 0; y < h; y++) {
		QRgb *scanline = reinterpret_cast<QRgb*>(samplingMapImage.scanLine(y));
		for (int x = 0; x < w; x++) {
			const float value = userSamplingMap[x + y * w] * .5f + .25f;
			const int fba = static_cast<int>(min(max(255.f * value, 0.f), 255.f));
			scanline[x] = qRgba(255, 255, 255, fba);
		}
	}

	QPainter painter(&samplingMapImage);

	QPen pen;
	pen.setWidth(3);
	pen.setBrush(Qt::black);
	painter.setPen(pen);

	painter.drawEllipse(userSamplingPenX - userSamplingPenSize / 2, userSamplingPenY - userSamplingPenSize / 2,
			userSamplingPenSize, userSamplingPenSize);
	painter.drawLine(userSamplingPenX - userSamplingPenSize / 4, userSamplingPenY,
			userSamplingPenX + userSamplingPenSize / 4, userSamplingPenY);
	if (userSamplingAddictivePen) {
		painter.drawLine(userSamplingPenX, userSamplingPenY - userSamplingPenSize / 4,
				userSamplingPenX, userSamplingPenY + userSamplingPenSize / 4);
	}
	
	userSamplingPixmap->setPixmap(QPixmap::fromImage(samplingMapImage));
}

void RenderView::setLogoMode () {
	resetTransform ();
	if (luxfb->isVisible()) {
		luxfb->hide ();
		zoomEnabled = false;
		zoomfactor = 100.0f;
	}
	if (userSamplingPixmap->isVisible()) {
		userSamplingPixmap->hide();
		delete[] userSamplingMap;
		userSamplingMap = NULL;
		userSamplingPenPressed = false;
		userSamplingPenX = 0;
		userSamplingPenY = 0;
	}
	if (!luxlogo->isVisible ()) {
		luxlogo->show ();
		renderscene->setSceneRect (0.0f, 0.0f, 416, 389);
		centerOn(luxlogo);
	}
	setInteractive(false);
}

void RenderView::resizeEvent(QResizeEvent *event) {
	QGraphicsView::resizeEvent(event);
	emit viewChanged ();
}

int RenderView::getZoomFactor () {
	return zoomfactor;
}

int RenderView::getWidth () {
	return width();
}

int RenderView::getHeight () {
	return height();
}

void RenderView::wheelEvent (QWheelEvent* event) {
   if (!zoomEnabled)
	   return;

	const float zoomsteps[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12.5, 17, 25, 33, 45, 50, 67, 75, 100, 
		125, 150, 175, 200, 250, 300, 400, 500, 600, 700, 800, 1000, 1200, 1600 };
	
	size_t numsteps = sizeof(zoomsteps) / sizeof(*zoomsteps);

	size_t index = min<size_t>(std::upper_bound(zoomsteps, zoomsteps + numsteps, zoomfactor) - zoomsteps, numsteps-1);
	if (event->delta() < 0) {
		// if zoomfactor is equal to zoomsteps[index-1] we need index-2
		while (index > 0 && zoomsteps[--index] == zoomfactor);		
	}
	zoomfactor = zoomsteps[index];

	resetTransform();
	scale(zoomfactor / 100.f, zoomfactor / 100.f);

	emit viewChanged ();
}

void RenderView::drawPenOnUserSamplingMap(const int xPos, const int yPos) {
	for (uint y = 0; y <= userSamplingPenSize; ++y) {
		for (uint x = 0; x <= userSamplingPenSize; ++x) {
			// Check if the point is inside the circle
			const float cx = 2.f * x / userSamplingPenSize - 1.f;
			const float cy = 2.f * y / userSamplingPenSize - 1.f;

			const float dist = sqrtf(cx * cx + cy * cy);
			if (dist <= 1.f) {
				// We are inside the circle
				int xRes = luxGetIntAttribute("film", "xResolution");
				int yRes = luxGetIntAttribute("film", "yResolution");
				int px = xPos + x - userSamplingPenSize / 2;
				int py = yPos + y - userSamplingPenSize / 2;

				if ((px >= 0) && (px < xRes) && (py >= 0) && (py < yRes)) {
					const float value = userSamplingMap[px + py * xRes];

					// The * .5 is used to have a spray-like effect
					if (userSamplingAddictivePen)
						userSamplingMap[px + py * xRes] = Clamp(value + (1.f - dist) * .05f, .1f, 1.f);
					else
						userSamplingMap[px + py * xRes] = Clamp(value + (dist - 1.f) * .05f, .1f, 1.f);
				}
			}
		}
	}
}

void RenderView::mousePressEvent (QMouseEvent *event) {
	if (luxfb->isVisible()) {
		switch (event->button()) {
			case Qt::LeftButton:
				if (showUserSamplingMap) {
					userSamplingPenPressed = true;
					QPointF pos = mapToScene(event->pos());
					userSamplingPenX = pos.x();
					userSamplingPenY = pos.y();

					drawPenOnUserSamplingMap(userSamplingPenX, userSamplingPenY);
					updateUserSamplingPixmap();
				}
				break;
			case Qt::MidButton:
				setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
				setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
				fitInView(renderscene->sceneRect(), Qt::KeepAspectRatio);
				// compute correct zoomfactor
				origw = (qreal)luxGetIntAttribute("film", "xResolution")/(qreal)width();
				origh = (qreal)luxGetIntAttribute("film", "yResolution")/(qreal)height();
				if (origh > origw)
					zoomfactor = 100.0f/(origh);
				else
					zoomfactor = 100.0f/(origw);
				setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
				setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
				
				emit viewChanged ();
				break;
			case Qt::RightButton:
				resetTransform ();
				zoomfactor = 100.0f;
				emit viewChanged ();
			default:
				break;
		}
	}

	QGraphicsView::mousePressEvent(event);
}

void RenderView::mouseReleaseEvent (QMouseEvent *event) {
	if (luxfb->isVisible() && showUserSamplingMap) {
		switch (event->button()) {
			case Qt::LeftButton:
				userSamplingPenPressed = false;
				break;
			default:
				break;
		}
	}

	QGraphicsView::mouseReleaseEvent(event);
}

void RenderView::mouseMoveEvent (QMouseEvent *event) {
	if (luxfb->isVisible() && showUserSamplingMap && userSamplingPenPressed) {
		QPointF pos = mapToScene(event->pos());
		int x = pos.x();
		int y = pos.y();
		
		if ((userSamplingPenX != x) || (userSamplingPenY != y)) {
			userSamplingPenX = x;
			userSamplingPenY = y;

			drawPenOnUserSamplingMap(userSamplingPenX, userSamplingPenY);
			updateUserSamplingPixmap();
		}
	}

	QGraphicsView::mouseMoveEvent(event);
}
