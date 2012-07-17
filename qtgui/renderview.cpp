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

#include <iostream>
#include <algorithm>

using namespace std;

RenderView::RenderView(QWidget *parent) : QGraphicsView(parent) {
	renderscene = new QGraphicsScene();
	renderscene->setBackgroundBrush(QColor(127,127,127));
	luxlogo = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb->hide ();
	renderscene->setSceneRect (0.0f, 0.0f, 416, 389);
	centerOn(luxlogo);
	setScene(renderscene);
	zoomfactor = 100.0f;
	overlayStats = false;
	showAlpha = false;
}

RenderView::~RenderView () {
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

void RenderView::reload () {
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		int w = luxGetIntAttribute("film", "xPixelCount");
		int h = luxGetIntAttribute("film", "yPixelCount");
			
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
//			fitInView(luxfb, Qt::KeepAspectRatio);
		}
		zoomEnabled = true;
		setDragMode(QGraphicsView::ScrollHandDrag);
		setInteractive(true);
	}
}

void RenderView::setLogoMode () {
	resetTransform ();
	if (luxfb->isVisible()) {
		luxfb->hide ();
		zoomEnabled = false;
		zoomfactor = 100.0f;
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

void RenderView::mousePressEvent (QMouseEvent *event) {
	
	if (luxfb->isVisible()) {
		switch (event->button()) {
			case Qt::LeftButton:
				currentpos = event->pos();
				break;
			case Qt::MidButton:
				setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
				setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
				fitInView(renderscene->sceneRect(), Qt::KeepAspectRatio);
				// compute correct zoomfactor
				origw = (qreal)luxGetIntAttribute("film", "xPixelCount")/(qreal)width();
				origh = (qreal)luxGetIntAttribute("film", "yPixelCount")/(qreal)height();
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
