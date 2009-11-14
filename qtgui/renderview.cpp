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

RenderView::RenderView(QWidget *parent, bool opengl) : QGraphicsView(parent) {
    if (opengl)
		setViewport(new QGLWidget);

	renderscene = new QGraphicsScene();
	renderscene->setBackgroundBrush(QColor(127,127,127));
	luxlogo = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb = renderscene->addPixmap(QPixmap(":/images/luxlogo_bg.png"));
	luxfb->hide ();
	setScene(renderscene);
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

		if (luxlogo->isVisible ())
			luxlogo->hide ();

		luxfb->setPixmap( QPixmap::fromImage(QImage(fb, w, h, w*3, QImage::Format_RGB888)));

		if (!luxfb->isVisible())
			luxfb->show ();
		zoomEnabled = true;
		setDragMode(QGraphicsView::ScrollHandDrag);
		setInteractive(true);
	}
}

void RenderView::setLogoMode () {
	if (luxfb->isVisible()) {
		luxfb->hide ();
		zoomEnabled = false;
	}
	if (!luxlogo->isVisible ())
		luxlogo->show ();
	resetZoom ();
	setInteractive(false);
}

void RenderView::resetZoom () {
	setMatrix (QMatrix());
} 

void RenderView::wheelEvent (QWheelEvent* event) {
   if (!zoomEnabled)
	   return;

	qreal factor = 1.2;
	if (event->delta() < 0)
		factor = 1.0 / factor;
	scale(factor, factor);
}

void RenderView::mousePressEvent (QMouseEvent *event) {
	
	if (luxfb->isVisible()) {
		switch (event->button()) {
			case Qt::LeftButton:
				currentpos = event->pos();
	//			setCursor(Qt::ClosedHandCursor);
				break;
			case Qt::MidButton:
			case Qt::RightButton:
				resetZoom ();
				break;
		}
	}
	QGraphicsView::mousePressEvent(event);
}
