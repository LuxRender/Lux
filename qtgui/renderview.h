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

#ifndef RENDERVIEW_H
#define RENDERVIEW_H

#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QApplication>
#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMatrix>
#include <QPoint>
#include <QClipboard>
#include <QtOpenGL/QGLWidget>

#include "api.h"
#include "lux.h"

class RenderView : public QGraphicsView
{
	Q_OBJECT

public:

	RenderView(QWidget *parent = 0, bool opengl = false);
    ~RenderView ();

	void resetZoom ();
	void setZoomEnabled (bool enabled = true) { zoomEnabled = enabled; };
	void reload ();
	void setLogoMode ();

	void copyToClipboard ();

private:

	bool zoomEnabled;
	QPoint currentpos;

	QGraphicsScene *renderscene;
	QGraphicsPixmapItem *luxlogo;
	QGraphicsPixmapItem *luxfb;

	void wheelEvent (QWheelEvent *event);
	void mousePressEvent (QMouseEvent *event);
	//void mouseReleaseEvent (QMouseEvent* event);
	//void mouseMoveEvent (QMouseEvent* event);
};

#endif // RENDERVIEW_H