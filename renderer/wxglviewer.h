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

#ifndef LUX_WXGLVIEWER_H
#define LUX_WXGLVIEWER_H

#ifdef LUX_USE_OPENGL
#include "wx/wx.h"
#include "wx/glcanvas.h"
#endif // LUX_USE_OPENGL

namespace lux
{

#ifdef LUX_USE_OPENGL

class LuxGLViewer : public wxGLCanvas {
public:
	LuxGLViewer(wxWindow *parent, int textureW = 256, int textureH = 256);

protected:
	DECLARE_EVENT_TABLE()
	virtual void Refresh(bool eraseBackground = true, const wxRect* rect = NULL);
	void OnPaint(wxPaintEvent &event);
	void OnSize(wxSizeEvent &event);
	void OnMouse(wxMouseEvent &event);

	wxGLContext m_glContext;

	int m_imageW, m_imageH;
	int m_tilesX, m_tilesY, m_tilesNr;
	bool m_firstDraw;
	bool m_imageChanged;
	const int m_textureW;
	const int m_textureH;
	int m_offsetX, m_offsetY, m_scaleXo2, m_scaleYo2, m_scaleXo, m_scaleYo, m_lastX, m_lastY;
	float m_scale;
	float m_scaleExp;
	int m_lastW, m_lastH;

};

#else // LUX_USE_OPENGL

//dummy class
class LuxGLViewer : public wxWindow {
public:
	LuxGLViewer(wxWindow *parent, int textureW = 256, int textureH = 256) {}
};

#endif // LUX_USE_OPENGL

}//namespace lux

#endif // LUX_WXGLVIEWER_H
