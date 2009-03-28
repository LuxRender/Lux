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

#ifdef LUX_USE_OPENGL

#include "wxglviewer.h"

// include OpenGL
#if defined(__WXOSX_COCOA__) || defined(__WXCOCOA__) || defined(__WXMAC__)
#include "OpenGL/glu.h"
#include "OpenGL/gl.h"
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

#include <cmath>
#include <sstream>

#if defined (__WXMAC__) && !(__WXOSX_COCOA__)
#define cimg_display_type  3
#endif
#define cimg_debug 0     // Disable modal window in CImg exceptions.
#include "cimg.h"

#include "lux.h"
#include "api.h"
#include "wx/mstream.h"

using namespace lux;

BEGIN_EVENT_TABLE(LuxGLViewer, wxWindow)
		EVT_PAINT            (LuxGLViewer::OnPaint)
		EVT_SIZE             (LuxGLViewer::OnSize)
		EVT_MOUSE_EVENTS     (LuxGLViewer::OnMouse)
		EVT_ERASE_BACKGROUND (LuxGLViewer::OnEraseBackground)
		EVT_TIMER            (wxID_ANY, LuxGLViewer::OnTimer)
END_EVENT_TABLE()

int glAttribList[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0};

LuxGLViewer::LuxGLViewer(wxWindow *parent, int textureW, int textureH)
#if defined(__WXOSX_COCOA__) || defined(__WXCOCOA__)
      : wxGLCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxGLCanvasName, glAttribList), wxViewerBase(), m_glContext(this), m_textureW(textureW), m_textureH(textureH){
#elif defined(__WXMAC__)
      : wxGLCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxGLCanvasName, glAttribList), wxViewerBase(), m_glContext(NULL,this,wxNullPalette,NULL), m_textureW(textureW), m_textureH(textureH){
#else
      : wxGLCanvas(parent, wxID_ANY, glAttribList), wxViewerBase(), m_glContext(this), m_textureW(textureW), m_textureH(textureH) {
#endif
	m_imageW = m_imageH = 0;
	m_tilesX = m_tilesY = m_tilesNr = 0;
	m_tileTextureNames = NULL;
	m_preScaleOffsetX  = m_preScaleOffsetY  = 0;
	m_postScaleOffsetX = m_postScaleOffsetY = 0;
	m_scale = 1.0f;
	m_scaleExp = 0;
	m_viewX = m_viewY = m_viewW = m_viewH = 0;
	m_windowW = m_windowH = m_prevWindowW = m_prevWindowH = 0;
	m_prevMouseX = m_prevMouseY = 0;
	m_logoData = NULL;
	m_logoDataSize = 0;

	m_stipple = 0x00FF; // Stipple pattern - dashes
	m_animTimer = new wxTimer(this, ID_ANIMATIONUPDATE);
	m_animTimer->Start(125, wxTIMER_CONTINUOUS); // Animation at 8fps

	m_useAlpha = false;
	m_texturesReady = false;
	m_selectionChanged = false;
	m_refreshMarchingAntsOnly = false;
	m_trackMousePos = false;
	m_rulersEnabled = true;
	m_rulerSize = 13;

	m_controlMode = PANZOOM;
	m_displayMode = EMPTY_VIEW;

	SetCursor(wxCURSOR_CROSS);
}

LuxGLViewer::~LuxGLViewer() {
	if(m_tileTextureNames!=NULL) delete [] m_tileTextureNames;
	if(m_animTimer->IsRunning())
		m_animTimer->Stop();
	delete m_animTimer;
}

void LuxGLViewer::OnPaint(wxPaintEvent& event) {
#if defined(__WXMAC__)
	SetCurrent();
#else
	SetCurrent(m_glContext);
#endif
	wxPaintDC(this);

	if (!m_refreshMarchingAntsOnly) {
		glClearColor(0.5, 0.5, 0.5, 1.0);
	}
	glViewport(0, 0, m_windowW, m_windowH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, m_windowW, 0, m_windowH, -1, 1);
	if (!m_refreshMarchingAntsOnly) {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	if( !m_texturesReady && ((m_displayMode==RENDER_VIEW && ( luxStatistics("sceneIsReady") || luxStatistics("filmIsReady") )) || m_displayMode==LOGO_VIEW) ) {
		m_fontgen.Init();
		CreateTextures();
		m_texturesReady = true;
		m_imageChanged = true;
		if( m_displayMode==RENDER_VIEW ){
			//move to center of window
			m_preScaleOffsetX = m_preScaleOffsetY = 0;
			m_scale = 1.0f;
			m_scaleExp = 0.0f;
			m_postScaleOffsetX = -m_imageW/2 + m_viewW/2 + m_viewX;
			m_postScaleOffsetY = -m_imageH/2 + m_viewH/2 + m_viewY;
		}else{ //m_displayMode==LOGO_VIEW
			//zoom and offset slightly for dramatic effect!
			m_preScaleOffsetX = m_imageW/2;
			m_preScaleOffsetY = m_imageH/2;
			m_scale = 2.0f;
			m_scaleExp = 1.0f;
			m_postScaleOffsetX = m_viewW*3/4;
			m_postScaleOffsetY = m_viewH*1/4;
		}
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//transform and roll out!
	glTranslatef(m_postScaleOffsetX, m_postScaleOffsetY, 0.f);
	glScalef(m_scale, m_scale, 1.f);
	glTranslatef(-m_preScaleOffsetX, -m_preScaleOffsetY, 0.f);

	if( m_texturesReady && (m_displayMode==RENDER_VIEW || m_displayMode==LOGO_VIEW) && !m_refreshMarchingAntsOnly ) {
		//draw the texture tiles
		glEnable(GL_TEXTURE_2D);
		for(int y = 0; y < m_tilesY; y++){
			for(int x = 0; x < m_tilesX; x++){
				int offX = x*m_textureW;
				int offY = y*m_textureH;
				int tileW = min(m_textureW, m_imageW - offX);
				int tileH = min(m_textureH, m_imageH - offY);
				glBindTexture (GL_TEXTURE_2D, m_tileTextureNames[y*m_tilesX+x]);
				if( m_imageChanged && m_displayMode==RENDER_VIEW && m_texturesReady && ( luxStatistics("sceneIsReady") || luxStatistics("filmIsReady") ) ) { // upload/refresh textures
					// NOTE - Ratow - loading texture tile in one pass
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glPixelStorei(GL_UNPACK_SKIP_PIXELS, offX);
					glPixelStorei(GL_UNPACK_SKIP_ROWS, offY);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, m_imageW);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tileW, tileH, GL_RGB, GL_UNSIGNED_BYTE, luxFramebuffer());
				}
				if( m_useAlpha ){
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glEnable(GL_BLEND);
				}else{
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
				}
				glBegin(GL_QUADS);
				glTexCoord2f(                 0.0f,                             0.0f);
				glVertex3f(  x*m_textureW +   0.0f, m_imageH -  (y*m_textureH + 0.0f), 0.0f);
				glTexCoord2f(1.0f*tileW/m_textureW,                             0.0f);
				glVertex3f(  x*m_textureW +  tileW, m_imageH -  (y*m_textureH + 0.0f), 0.0f);
				glTexCoord2f(1.0f*tileW/m_textureW,            1.0f*tileH/m_textureH);
				glVertex3f(  x*m_textureW +  tileW, m_imageH - (y*m_textureH + tileH), 0.0f);
				glTexCoord2f(                 0.0f,            1.0f*tileH/m_textureH);
				glVertex3f(  x*m_textureW +   0.0f, m_imageH - (y*m_textureH + tileH), 0.0f);
				glEnd();
				if( m_useAlpha ) glDisable(GL_BLEND);
			}
		}
		glDisable(GL_TEXTURE_2D);
		m_imageChanged = false;
		if( m_rulersEnabled && m_displayMode==RENDER_VIEW ) DrawRulers();
	}

	if (m_selection.HasSize()) {
		// Draw current (white) selection area
		DrawMarchingAnts(m_selection, 1.0, 1.0, 1.0); 
	}
	if (m_highlightSel.HasSize()) {
		// Draw active (red) work area
		DrawMarchingAnts(m_highlightSel, 1.0, 0.0, 0.0); 
	}
	m_refreshMarchingAntsOnly = false;

	glFlush();
	SwapBuffers();
}

void LuxGLViewer::OnEraseBackground(wxEraseEvent &event) {
	// Method overridden for less flicker:
	// http://wiki.wxwidgets.org/WxGLCanvas#Animation_flicker:_wxGLCanvas-as-member_vs_subclass-as-member
	return;
}

void LuxGLViewer::OnSize(wxSizeEvent &event) {
	wxGLCanvas::OnSize(event);

	GetClientSize(&m_windowW, &m_windowH);
	if(m_rulersEnabled){
		m_viewX = m_rulerSize;
		m_viewY = 0;
		m_viewW = m_windowW-m_rulerSize;
		if(m_viewW<0) m_viewW = 0;
		m_viewH = m_windowH-m_rulerSize;
		if(m_viewH<0) m_viewH = 0;
	}else{
		m_viewX = 0;
		m_viewY = 0;
		m_viewW = m_windowW;
		m_viewH = m_windowH;
	}

	if(m_texturesReady) {
		//calculate new offset
		m_postScaleOffsetX += (m_windowW - m_prevWindowW)/2;
		m_postScaleOffsetY += (m_windowH - m_prevWindowH)/2;
		//make sure new offset is in bounds
		Point p0 = TransformPoint(Point(0, 0));
		Point p1 = TransformPoint(Point(m_imageW, m_imageH));
		if(p1.x-30<0)               	m_postScaleOffsetX =              30 -(p1.x - m_postScaleOffsetX);
		else if(p0.x+30>m_windowW-1)	m_postScaleOffsetX = m_windowW-1 -30 -(p0.x - m_postScaleOffsetX);
		if(p1.y-30<0)               	m_postScaleOffsetY =              30 -(p1.y - m_postScaleOffsetY);
		else if(p0.y+30>m_windowH-1)	m_postScaleOffsetY = m_windowH-1 -30 -(p0.y - m_postScaleOffsetY);
	}
	m_prevWindowW = m_windowW;
	m_prevWindowH = m_windowH;

	Refresh();
}

void LuxGLViewer::OnMouse(wxMouseEvent &event) {
	if( !(m_displayMode==RENDER_VIEW && m_texturesReady) ) {
		event.Skip();
		return;
	}
	if(m_controlMode == PANZOOM) {
		if(event.GetEventType() == wxEVT_LEFT_DOWN) {
			SetCursor(wxCURSOR_HAND);
			event.Skip();
		} else if(event.GetEventType() == wxEVT_LEFT_UP) {
			SetCursor(wxCURSOR_CROSS);
		} else if(event.GetEventType() == wxEVT_MIDDLE_DOWN) {
			m_preScaleOffsetX = 0;
			m_preScaleOffsetY = 0;
			m_scale = min(m_viewW/(float)m_imageW, m_viewH/(float)m_imageH);
			if(m_scale<=0) m_scale = 1;
			m_scaleExp = floor(log((float)m_scale)/log(2.0f)*2+0.5f)/2;
			m_postScaleOffsetX = (int)((m_viewW - m_imageW*m_scale)/2 + 0.5f) + m_viewX;
			m_postScaleOffsetY = (int)((m_viewH - m_imageH*m_scale)/2 + 0.5f) + m_viewY;
			Refresh();
		} else if(event.GetEventType() == wxEVT_RIGHT_DOWN) {
			//calculate the scaling point in image space
			Point p = InverseTransformPoint(Point(event.GetX(), m_windowH-1 -event.GetY()));
			m_preScaleOffsetX = p.x;
			m_preScaleOffsetY = p.y;
			//make sure the scaling point is in bounds
			if(m_preScaleOffsetX<0) m_preScaleOffsetX = 0;
			if(m_preScaleOffsetX>m_imageW-1) m_preScaleOffsetX = m_imageW-1;
			if(m_preScaleOffsetY<0) m_preScaleOffsetY = 0;
			if(m_preScaleOffsetY>m_imageH-1) m_preScaleOffsetY = m_imageH-1;
			//new scale
			m_scaleExp = 0;
			m_scale = 1.0f;
			//get the scaling point in window space
			m_postScaleOffsetX =               event.GetX();
			m_postScaleOffsetY = m_windowH-1 - event.GetY();
			Refresh();
		} else if(event.GetEventType() == wxEVT_MOTION) {
			if(event.Dragging() && event.m_leftDown) {
				//calculate new offset
				int newOffsetX = m_postScaleOffsetX + (event.GetX()-m_prevMouseX);
				int newOffsetY = m_postScaleOffsetY - (event.GetY()-m_prevMouseY);
				//check if new offset in bounds
				Point p0 = TransformPoint(Point(0, 0));
				Point p1 = TransformPoint(Point(m_imageW, m_imageH));
				if( p1.x -m_postScaleOffsetX +newOffsetX -30 >= 0 &&
					p0.x -m_postScaleOffsetX +newOffsetX +30 <= m_windowW-1)
					m_postScaleOffsetX = newOffsetX;
				if( p1.y -m_postScaleOffsetY +newOffsetY -30 >= 0 &&
					p0.y -m_postScaleOffsetY +newOffsetY +30 <= m_windowH-1)
					m_postScaleOffsetY = newOffsetY;
				if(!m_rulersEnabled) Refresh(); //avoid redrawing twice
			}
		} else if(event.GetEventType() == wxEVT_MOUSEWHEEL) {
			if( (event.GetWheelRotation() > 0 && m_scale < 8) || //zoom in up to 8x
			    (event.GetWheelRotation() < 0 && (m_scale > 1 || m_scale*m_imageW > m_windowW*0.5f || m_scale*m_imageH > m_windowH*0.5f))) { //zoom out up to 50% of window size
				//calculate the scaling point in image space
				Point p = InverseTransformPoint(Point(event.GetX(), m_windowH-1 -event.GetY()));
				m_preScaleOffsetX = p.x;
				m_preScaleOffsetY = p.y;
				//make sure the scaling point is in bounds
				if(m_preScaleOffsetX < 0) m_preScaleOffsetX = 0;
				if(m_preScaleOffsetX > m_imageW-1) m_preScaleOffsetX = m_imageW-1;
				if(m_preScaleOffsetY < 0) m_preScaleOffsetY = 0;
				if(m_preScaleOffsetY > m_imageH-1) m_preScaleOffsetY = m_imageH-1;
				//calculate new scale
				m_scaleExp += (event.GetWheelRotation()>0?1:-1)*0.5f;
				m_scale = pow(2.0f, m_scaleExp);
				//get the scaling point in window space
				m_postScaleOffsetX =               event.GetX();
				m_postScaleOffsetY = m_windowH-1 - event.GetY();
				Refresh();
			}
		} else {
			event.Skip();
		}
	} else if(m_controlMode == SELECTION) {
		if(event.GetEventType() == wxEVT_LEFT_DOWN) {
			Point p = InverseTransformPoint(Point(event.GetX(), m_windowH-1 -event.GetY()));
			m_selection.SetBounds(p.x, p.x, p.y, p.y);
		} else if(event.GetEventType() == wxEVT_LEFT_UP) {
			boost::shared_ptr<wxViewerSelection> selection(new wxViewerSelection(m_selection));
			Point p = InverseTransformPoint(Point(event.GetX(), m_windowH-1 -event.GetY()));
			selection->SetCorner2(p.x, p.y);
			wxViewerEvent viewerEvent(selection, wxEVT_LUX_VIEWER_SELECTION);
			GetEventHandler()->AddPendingEvent(viewerEvent);
		} else if(event.GetEventType() == wxEVT_MOTION) {
			if(event.Dragging() && event.m_leftDown) {
				int oldX, oldY;
				m_selection.GetCorner2(oldX, oldY);
				Point p = InverseTransformPoint(Point(event.GetX(), m_windowH-1 -event.GetY()));
				if ((p.x != oldX) || (p.y != oldY)) {
					m_selection.SetCorner2(p.x, p.y);
					m_selectionChanged = true;
				}
			}
		}
	}
	
	m_prevMouseX = event.GetX();
	m_prevMouseY = event.GetY();
	if(event.GetEventType() == wxEVT_ENTER_WINDOW){
		m_trackMousePos = true;
	} else if(event.GetEventType() == wxEVT_LEAVE_WINDOW){
		m_trackMousePos = false;
		if(m_rulersEnabled) Refresh();
	} else if(event.GetEventType() == wxEVT_MOTION){
		m_trackMousePos = true;
		if(m_rulersEnabled) Refresh();
	}
}

wxWindow* LuxGLViewer::GetWindow() {
	return this;
}

wxViewerSelection LuxGLViewer::GetSelection() {
	return m_selection;
}

void LuxGLViewer::SetMode(wxViewerMode mode) {
	if( mode==EMPTY_VIEW ){
		DeleteTextures();
		m_texturesReady = false;
		m_displayMode = EMPTY_VIEW;
		Refresh();
	}else if( mode==LOGO_VIEW ){
		if(m_displayMode!=LOGO_VIEW && m_logoData!=NULL){
			DeleteTextures();
			m_texturesReady = false;
			m_displayMode = LOGO_VIEW;
			Refresh();
		}
	}else if( mode==RENDER_VIEW ){
		if(m_displayMode!=RENDER_VIEW){
			DeleteTextures();
			m_texturesReady = false;
			m_displayMode = RENDER_VIEW;
			Refresh();
		}
	}else{
		if( mode!=m_controlMode ){
			m_controlMode = mode;
			Refresh();
		}
	}

}

void LuxGLViewer::SetRulersEnabled(bool enabled){
	m_rulersEnabled = enabled;
	if(m_rulersEnabled){
		m_viewX = m_rulerSize;
		m_viewY = 0;
		m_viewW = m_windowW-m_rulerSize;
		if(m_viewW<0) m_viewW = 0;
		m_viewH = m_windowH-m_rulerSize;
		if(m_viewH<0) m_viewH = 0;
	}else{
		m_viewX = 0;
		m_viewY = 0;
		m_viewW = m_windowW;
		m_viewH = m_windowH;
	}
	Refresh();
}

void LuxGLViewer::SetZoom(const wxViewerSelection *selection) {
	int x1, x2, y1, y2;
	selection->GetBounds(x1,x2,y1,y2);
	int selMaxX = min(m_imageW, max(x1, x2));
	int selMaxY = min(m_imageH, max(y1, y2));
	int selMinX = max(0, min(x1, x2));
	int selMinY = max(0, min(y1, y2));
	float exactScale = min((float)m_viewW/(selMaxX-selMinX), (float)m_viewH/(selMaxY-selMinY));

	if(isinf(exactScale)) // If selection is too small, increase it to 1 pixel
		exactScale = min(m_viewW, m_viewH);

	// Zoom in and center selection
	m_preScaleOffsetX = (selMaxX + selMinX)/2.0;
	m_preScaleOffsetY = (selMaxY + selMinY)/2.0;
	m_scaleExp = log(exactScale)/log(2.0);
	m_scale = exactScale;
	m_postScaleOffsetX = m_viewW/2.0   + m_viewX;
	m_postScaleOffsetY = m_viewH/2.0-1 + m_viewY;
	Refresh();
}

void LuxGLViewer::SetSelection(const wxViewerSelection *selection) {
	if (!selection) {
		m_selection.Clear();
	} else {
		m_selection = *selection;
	}
	Refresh();
}

void LuxGLViewer::SetHighlight(const wxViewerSelection *selection) {
	if (!selection) {
		m_highlightSel.Clear();
	} else {
		m_highlightSel = *selection;
	}
	Refresh();
}

void LuxGLViewer::Reload() {
	m_imageChanged = true;
	Refresh();
}

void LuxGLViewer::Reset() {
	DeleteTextures();
	m_texturesReady = false;
	Refresh();
}

void LuxGLViewer::OnTimer(wxTimerEvent &event) {
	if (!m_selection.HasSize() && !m_highlightSel.HasSize())	return;

	m_stipple = ((m_stipple >> 15) | (m_stipple << 1)) & 0xFFFF;
	if (!m_selectionChanged) {
		m_refreshMarchingAntsOnly = true;
	}
	m_selectionChanged = false;
	this->Refresh();
}

void LuxGLViewer::CreateTextures(){
	if( m_texturesReady ) return;
	unsigned char* logo_buf = NULL;

	if( m_displayMode==RENDER_VIEW ){
		m_useAlpha = false;
		m_imageW = luxStatistics("filmXres");
		m_imageH = luxStatistics("filmYres");
	}else{ //m_displayMode==LOGO_VIEW
		m_useAlpha = true;
		wxMemoryInputStream stream( m_logoData, m_logoDataSize );
		wxImage logo_img( stream, wxBITMAP_TYPE_ANY, -1 );
		m_imageW = logo_img.GetWidth();
		m_imageH = logo_img.GetHeight();
		logo_buf = new unsigned char[m_imageW*m_imageH];
		for(int i=0;i<m_imageW*m_imageH;i++)
			logo_buf[i] = 255-logo_img.GetData()[i*3+0];
	}
	// NOTE - Ratow - using ceiling function so that a minimum number of tiles are created in the case of m_imageW % 256 == 0
	m_tilesX = ceil((float)m_imageW/m_textureW);
	m_tilesY = ceil((float)m_imageH/m_textureH);
	m_tilesNr = m_tilesX*m_tilesY;

	glEnable(GL_TEXTURE_2D);
	m_tileTextureNames = new unsigned int[m_tilesNr];
	glGenTextures(m_tilesNr, (GLuint*) m_tileTextureNames);
	if( m_displayMode==RENDER_VIEW ){ //create uninitialized image textures
		for(int i = 0; i < m_tilesNr; i++){
			glBindTexture(GL_TEXTURE_2D, m_tileTextureNames[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_LINEAR); //'linear' causes seams to show
			glTexImage2D(GL_TEXTURE_2D, 0,  m_useAlpha?GL_RGBA:GL_RGB, m_textureW, m_textureH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		}
	}else{ //m_displayMode==LOGO_VIEW - create and upload logo textures
		for(int y = 0; y < m_tilesY; y++){
			for(int x = 0; x < m_tilesX; x++){
				int offX = x*m_textureW;
				int offY = y*m_textureH;
				int tileW = min(m_textureW, m_imageW - offX);
				int tileH = min(m_textureH, m_imageH - offY);
				glBindTexture (GL_TEXTURE_2D, m_tileTextureNames[y*m_tilesX+x]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_UNPACK_SKIP_PIXELS, offX);
				glPixelStorei(GL_UNPACK_SKIP_ROWS, offY);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, m_imageW);

				glPixelTransferf( GL_RED_BIAS, 221/255.0f );
				glPixelTransferf( GL_GREEN_BIAS, 127/255.0f );
				glPixelTransferf( GL_BLUE_BIAS,  0.0f );
				glPixelTransferf( GL_ALPHA_SCALE,  0.3f );

				glTexImage2D(GL_TEXTURE_2D, 0, m_useAlpha?GL_RGBA:GL_RGB, m_textureW, m_textureH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tileW, tileH, GL_ALPHA, GL_UNSIGNED_BYTE, logo_buf);

				glPixelTransferf( GL_RED_BIAS,   0.0f );
				glPixelTransferf( GL_GREEN_BIAS, 0.0f );
				glPixelTransferf( GL_BLUE_BIAS,  0.0f );
				glPixelTransferf( GL_ALPHA_SCALE,1.0f );
			}
		}
	}

	glDisable(GL_TEXTURE_2D);
	if(logo_buf!=NULL) delete [] logo_buf;

}

void LuxGLViewer::DeleteTextures() {
	if(m_tilesNr>0 && m_tileTextureNames!=NULL) glDeleteTextures(m_tilesNr, (GLuint*) m_tileTextureNames);
	if(m_tileTextureNames!=NULL) delete [] m_tileTextureNames;
	m_tileTextureNames = NULL;
	m_tilesNr = m_tilesX = m_tilesY = 0;
}

lux::Point LuxGLViewer::TransformPoint(const Point &p){
	return Point( (p.x - m_preScaleOffsetX)*m_scale + m_postScaleOffsetX,
	              (p.y - m_preScaleOffsetY)*m_scale + m_postScaleOffsetY);
}

lux::Point LuxGLViewer::InverseTransformPoint(const Point &p){
	return Point( (p.x - m_postScaleOffsetX)/m_scale + m_preScaleOffsetX,
	              (p.y - m_postScaleOffsetY)/m_scale + m_preScaleOffsetY);
}

void LuxGLViewer::DrawMarchingAnts(const wxViewerSelection &selection, float red, float green, float blue) {
	int x1, x2, y1, y2;
	selection.GetBounds(x1, x2, y1, y2);

	glEnable(GL_LINE_STIPPLE);

	glLineWidth(2.0); // Perhaps we should change width according to zoom

	glLineStipple(1, m_stipple);
	glBegin(GL_LINE_STRIP);
	glColor3f(0.0, 0.0, 0.0);
	glVertex3i(x1, y1, 0.0);
	glVertex3i(x1, y2, 0.0);
	glVertex3i(x2, y2, 0.0);
	glVertex3i(x2, y1, 0.0);
	glVertex3i(x1, y1, 0.0);
	glEnd();

	glLineStipple(1, ~m_stipple);
	glBegin(GL_LINE_STRIP);
	glColor3f(red, green, blue);
	glVertex3i(x1, y1, 0.0);
	glVertex3i(x1, y2, 0.0);
	glVertex3i(x2, y2, 0.0);
	glVertex3i(x2, y1, 0.0);
	glVertex3i(x1, y1, 0.0);
	glEnd();

	glDisable(GL_LINE_STIPPLE);
}

void  LuxGLViewer::DrawRulers(){
	int tickSpacing = 10;
	float tickDist = tickSpacing*m_scale;
	//adjust the ticks to current scale
	while((tickDist<5 || tickDist>15) && tickSpacing>1){
		if(tickDist<5) tickSpacing *= 2;
		else tickSpacing /= 2;
		if(tickSpacing<1) tickSpacing = 1;
		tickDist = tickSpacing*m_scale;
	}
	Point p0 = TransformPoint(Point(0, 0));
	float x_offset=p0.x;
	float y_offset=p0.y;

	glPushMatrix();
	glLoadIdentity();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	//draw background
	glColor4f(0.2f, 0.2f, 0.2f, 0.7f);
	glBegin(GL_QUADS);
		glVertex2f(      0.0f, m_windowH-m_rulerSize );
		glVertex2f( m_windowW, m_windowH-m_rulerSize );
		glVertex2f( m_windowW, m_windowH );
		glVertex2f(      0.0f, m_windowH );

		glVertex2f(        0.0f,      0.0f );
		glVertex2f( m_rulerSize,      0.0f );
		glVertex2f( m_rulerSize, m_windowH );
		glVertex2f(        0.0f, m_windowH );
	glEnd();

	//draw borders
	glLineWidth(1);
	glColor4f(0.2f, 0.2f, 0.2f, 0.7f);
	glBegin(GL_LINES);
		glVertex2f(      0.0f, m_windowH-m_rulerSize + 0.5f );
		glVertex2f( m_windowW, m_windowH-m_rulerSize + 0.5f );

		glVertex2f( m_rulerSize - 0.5f,      0.0f );
		glVertex2f( m_rulerSize - 0.5f, m_windowH );
	glEnd();

	//HORIZONTAL RULER

	//draw ticks
	int tick_length, count;
	float x_start_offset;
	if(x_offset>=0){
		count = 0;
		x_start_offset = x_offset;
	}else{
		count = floor(-x_offset/tickDist);
		x_start_offset = fmod(x_offset, tickDist);
	}
	glScissor(m_rulerSize, m_windowH-m_rulerSize, m_windowW-m_rulerSize, m_rulerSize);
	glEnable(GL_SCISSOR_TEST);
	glLineWidth(1);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS); //using quads because lines don't seem to align properly
	for(float x=x_start_offset;x<m_windowW;x+=tickDist){
		if( count%10==0 ){
			tick_length=m_rulerSize-1;
		}else if( count%5==0 ){
			tick_length=m_rulerSize/2-1;
		}else{
			tick_length=m_rulerSize/4-1;
		}
		count++;
		glVertex2f( x + 0.0f, m_windowH-m_rulerSize );
		glVertex2f( x + 1.0f, m_windowH-m_rulerSize );
		glVertex2f( x + 1.0f, m_windowH-m_rulerSize+tick_length );
		glVertex2f( x + 0.0f, m_windowH-m_rulerSize+tick_length );
	}
	glEnd();
	//draw mouse marker
	if(m_trackMousePos){
		glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
		glBegin(GL_TRIANGLES);
			glVertex2f( m_prevMouseX + 0.5f,        m_windowH-m_rulerSize );
			glVertex2f( m_prevMouseX + 0.5f - 3.0f, m_windowH-m_rulerSize/2 );
			glVertex2f( m_prevMouseX + 0.5f + 3.0f, m_windowH-m_rulerSize/2 );
		glEnd();
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);
	
	//draw text
	if(x_offset>=0){
		count = 0;
		x_start_offset = x_offset;
	}else{
		count = floor(-x_offset/(tickDist*10))*10;
		x_start_offset = fmod(x_offset, tickDist*10);
	}
	std::ostringstream ss;
	for(float x=x_start_offset;x<m_windowW;x+=tickDist*10){
		ss.str("");
		ss<<count*tickSpacing;
		m_fontgen.DrawText(ss.str().c_str(), x+3, m_windowH-10);
		count+=10;
	}
	glDisable(GL_SCISSOR_TEST);

	//VERTICAL RULER

	//draw ticks
	float y_start_offset;
	if(y_offset+m_imageH*m_scale<=m_windowH){
		count = 0;
		y_start_offset = y_offset+m_imageH*m_scale;
	}else{
		count = floor((y_offset+m_imageH*m_scale-m_windowH)/tickDist);
		y_start_offset = fmod(y_offset+m_imageH*m_scale-m_windowH, tickDist) + m_windowH;
	}
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable(GL_BLEND);
	glScissor(0, 0, m_rulerSize, m_windowH-m_rulerSize);
	glEnable(GL_SCISSOR_TEST);
	glLineWidth(1);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS);
	for(float y=y_start_offset;y>0;y-=tickDist){
		if( count%10==0 ){
			tick_length=m_rulerSize-1;
		}else if( count%5==0 ){
			tick_length=m_rulerSize/2-1;
		}else{
			tick_length=m_rulerSize/4-1;
		}
		count++;
		glVertex2f( m_rulerSize-tick_length, y - 1.0f );
		glVertex2f( m_rulerSize,             y - 1.0f );
		glVertex2f( m_rulerSize,             y - 0.0f );
		glVertex2f( m_rulerSize-tick_length, y - 0.0f );
	}
	glEnd();
	//draw mouse marker
	if(m_trackMousePos){
		glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
		glBegin(GL_TRIANGLES);
			glVertex2f( m_rulerSize,   m_windowH - m_prevMouseY - 0.5f );
			glVertex2f( m_rulerSize/2, m_windowH - m_prevMouseY - 0.5f + 3.0f );
			glVertex2f( m_rulerSize/2, m_windowH - m_prevMouseY - 0.5f - 3.0f );
		glEnd();
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);

	//draw text
	if(y_offset+m_imageH*m_scale<=m_windowH){
		count = 0;
		y_start_offset = y_offset+m_imageH*m_scale;
	}else{
		count = floor((y_offset+m_imageH*m_scale-m_windowH)/(tickDist*10))*10;
		y_start_offset = fmod(y_offset+m_imageH*m_scale-m_windowH, tickDist*10) + m_windowH;
	}

	for(float y=y_start_offset;y>0;y-=tickDist*10){
		ss.str("");
		ss<<count*tickSpacing;
		m_fontgen.DrawText(ss.str().c_str(), 10, y-2, true);
		count+=10;
	}
	glDisable(GL_SCISSOR_TEST);

	glPopMatrix();
}

LuxGLViewer::FontGenerator::FontGenerator(){
	isInitialized = false;
}

void LuxGLViewer::FontGenerator::Init(){
	if(isInitialized) return;
	m_texW=512;
	m_texH=16;
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, (GLuint*)&m_texName);
	glBindTexture(GL_TEXTURE_2D, m_texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_texW, m_texH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glDisable(GL_TEXTURE_2D);
	isInitialized = true;
}

LuxGLViewer::FontGenerator::~FontGenerator(){
}

void LuxGLViewer::FontGenerator::DrawText(const char* text, int x, int y, bool vertical){
	if(!isInitialized) return;

	cimg_library::CImg<unsigned char> img(1,1,0,1);
	unsigned char fg[] = {255};
	unsigned char bg[] = {0};
	img.draw_text(text,0,0,fg,bg); //default font is 7x11px
	int w = img.width;
	int h = img.height;
	if(w==0 || w>(int)m_texW || h==0 || h>(int)m_texH) return;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_texName);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelTransferf( GL_RED_BIAS,   1.0f );
	glPixelTransferf( GL_GREEN_BIAS, 1.0f );
	glPixelTransferf( GL_BLUE_BIAS,  1.0f );

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_ALPHA, GL_UNSIGNED_BYTE, img.ptr());

	glPixelTransferf( GL_RED_BIAS,   0.0f );
	glPixelTransferf( GL_GREEN_BIAS, 0.0f );
	glPixelTransferf( GL_BLUE_BIAS,  0.0f );

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
	if(!vertical){
		glTexCoord2f( 0.0f,          1.0f*h/m_texH );	glVertex2f( x,     y );
		glTexCoord2f( 1.0f*w/m_texW, 1.0f*h/m_texH );	glVertex2f( x + w, y );
		glTexCoord2f( 1.0f*w/m_texW, 0.0f );         	glVertex2f( x + w, y + h );
		glTexCoord2f( 0.0f,          0.0f );         	glVertex2f( x,     y + h );
	}else{
		glTexCoord2f( 0.0f,          1.0f*h/m_texH );	glVertex2f( x,     y - w );
		glTexCoord2f( 1.0f*w/m_texW, 1.0f*h/m_texH );	glVertex2f( x,     y );
		glTexCoord2f( 1.0f*w/m_texW, 0.0f );         	glVertex2f( x - h, y );
		glTexCoord2f( 0.0f,          0.0f) ;         	glVertex2f( x - h, y - w );
	}
	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

}

#endif // LUX_USE_OPENGL
