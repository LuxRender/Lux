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

#include <iostream>

using namespace std;

RenderView::RenderView(QWidget *parent, bool opengl) : QGraphicsView(parent) {
#if !defined(__APPLE__)
    if (opengl)
		setViewport(new QGLWidget);
#endif

	renderscene = new QGraphicsScene();
	renderscene->setBackgroundBrush(QColor(127,127,127));
	luxlogo = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb->hide ();
	renderscene->setSceneRect (0.0f, 0.0f, 416, 389);
	centerOn(luxlogo);
	setScene(renderscene);
	zoomfactor = 100.0f;
}

RenderView::~RenderView () {
	delete luxfb;
	delete luxlogo;
	delete renderscene;
}

void RenderView::copyToClipboard()
{
	if ((luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) && luxfb->isVisible()) {
		int w = luxStatistics("filmXres"), h = luxStatistics("filmYres");
		unsigned char* fb = luxFramebuffer();
		QImage image = QImage(fb, w, h, QImage::Format_RGB888);
		QClipboard *clipboard = QApplication::clipboard();
		// QT assumes 32bpp images for clipboard (DIBs)
		clipboard->setImage(image.convertToFormat(QImage::Format_RGB32));
	}
}

void RenderView::reload () {
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		int w = luxStatistics("filmXres"), h = luxStatistics("filmYres");
		unsigned char* fb = luxFramebuffer();

		if (!fb)
			return;
			
		if (luxlogo->isVisible ())
			luxlogo->hide ();

		luxfb->setPixmap(QPixmap::fromImage(QImage(fb, w, h, w * 3, QImage::Format_RGB888)));

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

	qreal factor = 1.2;
	if (event->delta() < 0)
		factor = 1.0 / factor;
	zoomfactor *= factor;
	scale(factor, factor);

	emit viewChanged ();
}

void RenderView::mousePressEvent (QMouseEvent *event) {
	
	if (luxfb->isVisible()) {
		switch (event->button()) {
			case Qt::LeftButton:
				currentpos = event->pos();
				break;
			case Qt::MidButton:
				fitInView(renderscene->sceneRect(), Qt::KeepAspectRatio);
				// compute correct zoomfactor
				origw = (luxStatistics("filmXres")/width());
				origh = (luxStatistics("filmYres")/height());
				if (origh > origw)
					zoomfactor = 100.0f/(origh);
				else
					zoomfactor = 100.0f/(origw);
				
				emit viewChanged ();
				break;
			case Qt::RightButton:
				resetTransform ();
				zoomfactor = 100.0f;
				emit viewChanged ();
				break;
		}
	}
	QGraphicsView::mousePressEvent(event);
}
