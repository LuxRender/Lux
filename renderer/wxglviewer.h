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

#include "wxviewer.h"

#ifdef LUX_USE_OPENGL
#include "wx/wx.h"
#include "wx/glcanvas.h"
#endif // LUX_USE_OPENGL

#include "point.h"

namespace lux
{

#ifdef LUX_USE_OPENGL

#define ID_ANIMATIONUPDATE	3000

class LuxGLViewer : public wxGLCanvas, public wxViewerBase {
public:
	LuxGLViewer(wxWindow *parent, int textureW = 256, int textureH = 256);
	~LuxGLViewer();

protected:
	DECLARE_EVENT_TABLE()
	void OnPaint(wxPaintEvent &event);
	void OnEraseBackground(wxEraseEvent &event);
	void OnSize(wxSizeEvent &event);
	void OnMouse(wxMouseEvent &event);
	void OnTimer(wxTimerEvent &event);

	// wxViewerBase methods
	virtual wxWindow* GetWindow();
	virtual wxViewerSelection GetSelection();
	virtual void SetMode(wxViewerMode mode);
	virtual void SetRulersEnabled(bool enabled);
	virtual void SetZoom(const wxViewerSelection *selection);
	virtual void SetSelection(const wxViewerSelection *selection);
	virtual void SetHighlight(const wxViewerSelection *selection);
	virtual void Reload();
	virtual void Reset();

private:
	void CreateTextures();
	Point TransformPoint(const Point &p);
	Point InverseTransformPoint(const Point &p);
	void DrawMarchingAnts(const wxViewerSelection &selection, float red, float green, float blue);
	void DrawRulers();

	wxGLContext        m_glContext;

	int                m_imageW, m_imageH;
	const int          m_textureW, m_textureH;
	int                m_tilesX, m_tilesY, m_tilesNr;
	unsigned int      *m_tileTextureNames;
	int                m_postScaleOffsetX, m_postScaleOffsetY, m_preScaleOffsetX, m_preScaleOffsetY;
	float              m_scale;
	float              m_scaleExp;
	int                m_viewX, m_viewY;
	int                m_viewW, m_viewH;
	int                m_windowW, m_windowH;
	int                m_prevWindowW, m_prevWindowH;
	int                m_prevMouseX, m_prevMouseY;
	bool               m_firstDraw;
	bool               m_imageChanged;

	wxTimer*           m_animTimer;
	int                m_stipple;
	wxViewerSelection  m_selection;
	bool               m_selectionChanged;
	wxViewerSelection  m_highlightSel;
	bool               m_refreshMarchingAntsOnly;
	bool               m_trackMousePos;
	bool               m_rulersEnabled;
	int                m_rulerSize;

	class FontGenerator {
	public:
		FontGenerator();
		~FontGenerator();
		void Init();
		void DrawText(const char* text, int x=0, int y=0, bool vertical=false);
		bool isInitialized;
		unsigned int m_texName, m_texW, m_texH;

	} m_fontgen;

};

#else // LUX_USE_OPENGL

//dummy class
class LuxGLViewer : public wxWindow, public wxViewerBase {
public:
	LuxGLViewer(wxWindow *parent, int textureW = 256, int textureH = 256) {}
	virtual wxWindow* GetWindow() { return this; }
};

#endif // LUX_USE_OPENGL

}//namespace lux

#endif // LUX_WXGLVIEWER_H
