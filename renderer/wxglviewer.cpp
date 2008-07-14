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
#ifdef __WXMAC__
#include "OpenGL/glu.h"
#include "OpenGL/gl.h"
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

#include <cmath>

#include "lux.h"
#include "api.h"

using namespace lux;

BEGIN_EVENT_TABLE(LuxGLViewer, wxWindow)
    EVT_PAINT            (LuxGLViewer::OnPaint)
		EVT_SIZE             (LuxGLViewer::OnSize)
		EVT_MOUSE_EVENTS     (LuxGLViewer::OnMouse)
		EVT_ERASE_BACKGROUND (LuxGLViewer::OnEraseBackground)
END_EVENT_TABLE()

int glAttribList[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0};

LuxGLViewer::LuxGLViewer(wxWindow *parent, int textureW, int textureH)
      : wxGLCanvas(parent, wxID_ANY, glAttribList), m_glContext(this), m_textureW(textureW), m_textureH(textureH) {
	m_firstDraw = true;
	m_offsetX = m_offsetY = 0;
	m_scale = 1.0f;
	m_scaleExp=0;
	m_scaleXo = m_scaleYo = 0;
	m_scaleXo2 = m_scaleYo2 = 0;
	m_lastX = m_lastY = 0;
	SetCursor(wxCURSOR_CROSS);
}

void LuxGLViewer::OnPaint(wxPaintEvent& event) {
	SetCurrent(m_glContext);
	wxPaintDC(this);
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glViewport(0, 0, (GLint)GetSize().x, (GLint)GetSize().y);
	glLoadIdentity();
	glOrtho(0, GetSize().x, 0, GetSize().y, -1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if(luxStatistics("sceneIsReady")) {
		if(m_firstDraw || m_imageW != luxStatistics("filmXres") || m_imageH != luxStatistics("filmYres")) {
			m_firstDraw = false;
			m_imageW = luxStatistics("filmXres");
			m_imageH = luxStatistics("filmYres");

			// NOTE - Ratow - using ceiling function so that a minimum number of tiles are created in the case of m_imageW % 256 == 0
			m_tilesX = ceil((float)m_imageW/m_textureW);
			m_tilesY = ceil((float)m_imageH/m_textureH);
			m_tilesNr = m_tilesX*m_tilesY;
			for(int i = 0; i < m_tilesNr; i++){
				glBindTexture(GL_TEXTURE_2D, i+1);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, m_imageW);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_LINEAR); //'linear' causes seams to show on my ati card - zcott
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_textureW, m_textureH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); //warning: the texture isn't initialized, don't display before uploading
			}
			m_imageChanged = true;
			//move to center of window
			m_offsetX = m_offsetY = 0;
			m_scale = 1.0f;
			m_scaleExp=0;
			m_scaleXo = m_scaleYo = 0;
			m_lastX = m_lastY = 0;
			m_scaleXo2 = -m_imageW/2 + GetSize().x/2;
			m_scaleYo2 = -m_imageH/2 + GetSize().y/2;
		}

		glPushMatrix();
		//transform and roll out!
		glTranslatef(m_offsetX, m_offsetY, 0.f);
		glTranslatef(m_scaleXo2, m_scaleYo2, 0.f);
		glScalef(m_scale, m_scale, 1.f);
		glTranslatef(-m_scaleXo, -m_scaleYo, 0.f);

		glEnable (GL_TEXTURE_2D);
		//draw the texture tiles
		for(int y = 0; y < m_tilesY; y++){
			for(int x = 0; x < m_tilesX; x++){
				int offX = x*m_textureW;
				int offY = y*m_textureH;
				int tileW = min(m_textureW, m_imageW - offX);
				int tileH = min(m_textureH, m_imageH - offY);
				glBindTexture (GL_TEXTURE_2D, y*m_tilesX+x+1);
				if(m_imageChanged)	{ //upload the textures only when needed (takes long...)
					// NOTE - Ratow - loading texture tile in one pass
					glPixelStorei(GL_UNPACK_SKIP_PIXELS, offX);
					glPixelStorei(GL_UNPACK_SKIP_ROWS, offY);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tileW, tileH, GL_RGB, GL_UNSIGNED_BYTE, luxFramebuffer());
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
			}
		}
		glDisable(GL_TEXTURE_2D);
		glPopMatrix();
	}

	glFlush();
	SwapBuffers();
	m_imageChanged=false;
}

void LuxGLViewer::OnEraseBackground(wxEraseEvent &event) {
	// Method overridden for less flicker:
	// http://wiki.wxwidgets.org/WxGLCanvas#Animation_flicker:_wxGLCanvas-as-member_vs_subclass-as-member
	return;
}

void LuxGLViewer::OnSize(wxSizeEvent &event) {
	wxGLCanvas::OnSize(event);

	int W, H;
	W = event.GetSize().x;
	H = event.GetSize().y;

	if(!m_firstDraw) {
		//calculate new offset
		m_offsetX += (W-m_lastW)/2;
		m_offsetY += (H-m_lastH)/2;
		//make sure new offset is in bounds
		if((m_imageW-m_scaleXo)*m_scale+m_scaleXo2+m_offsetX-10<0)
			m_offsetX = (int)(     10-m_scaleXo2-(m_imageW-m_scaleXo)*m_scale );
		if((0-m_scaleXo)*m_scale+m_scaleXo2+m_offsetX+10>W-1)
			m_offsetX = (int)( W-1-10-m_scaleXo2-(0-m_scaleXo)*m_scale );
		if((m_imageH-m_scaleYo)*m_scale+m_scaleYo2+m_offsetY-10<0)
			m_offsetY = (int)(     10-m_scaleYo2-(m_imageH-m_scaleYo)*m_scale );
		if((0-m_scaleYo)*m_scale+m_scaleYo2+m_offsetY+10>H-1)
			m_offsetY = (int)( H-1-10-m_scaleYo2-(0-m_scaleYo)*m_scale );
	}
	m_lastW = W;
	m_lastH = H;

	Update();
}

void LuxGLViewer::OnMouse(wxMouseEvent &event) {
	int W, H;
	GetClientSize(&W, &H);

	// Skip events if we have nothing to draw
	if(m_firstDraw) {
		event.Skip();
	} else if(event.GetEventType() == wxEVT_LEFT_DOWN) {
		SetCursor(wxCURSOR_HAND);
		event.Skip();
	} else if(event.GetEventType() == wxEVT_LEFT_UP) {
		SetCursor(wxCURSOR_CROSS);
	} else if(event.GetEventType() == wxEVT_MIDDLE_DOWN) {
		m_scale = min(W/(float)m_imageW, H/(float)m_imageH);
		m_scaleExp = floor(log((float)m_scale)/log(2.0f)*2+0.5f)/2;
		m_scaleXo2 = W/2;
		m_scaleYo2 = H/2;
		m_scaleXo = m_imageW/2;
		m_scaleYo = m_imageH/2;
		m_offsetX = 0;
		m_offsetY = 0;
		wxGLCanvas::Refresh();
	} else if(event.GetEventType() == wxEVT_RIGHT_DOWN) {
		//calculate the scaling point in image space
		m_scaleXo = (int)(     event.GetX() /m_scale-m_scaleXo2/m_scale-m_offsetX/m_scale+m_scaleXo);
		m_scaleYo = (int)((H-1-event.GetY())/m_scale-m_scaleYo2/m_scale-m_offsetY/m_scale+m_scaleYo);
		//make sure the scaling point is in bounds
		if(m_scaleXo<0) m_scaleXo = 0;
		if(m_scaleXo>m_imageW-1) m_scaleXo = m_imageW-1;
		if(m_scaleYo<0) m_scaleYo = 0;
		if(m_scaleYo>m_imageH-1) m_scaleYo = m_imageH-1;
		//new scale
		m_scaleExp = 0;
		m_scale = 1.0f;
		//get the scaling point in window space
		m_scaleXo2 =     event.GetX();
		m_scaleYo2 = H-1-event.GetY();
		m_offsetX = m_offsetY = 0;
		wxGLCanvas::Refresh();
	} else if(event.GetEventType() == wxEVT_MOTION) {
		if(event.Dragging() && event.m_leftDown) {
			//calculate new offset
			int offsetXNew = m_offsetX + (event.GetX()-m_lastX);
			int offsetYNew = m_offsetY - (event.GetY()-m_lastY);
			//check if new offset in bounds
			if((m_imageW-m_scaleXo)*m_scale+m_scaleXo2+offsetXNew-10>=0 && (0-m_scaleXo)*m_scale+m_scaleXo2+offsetXNew+10<=W-1)
				m_offsetX = offsetXNew;
			if((m_imageH-m_scaleYo)*m_scale+m_scaleYo2+offsetYNew-10>=0 && (0-m_scaleYo)*m_scale+m_scaleYo2+offsetYNew+10<=H-1)
				m_offsetY = offsetYNew;
			wxGLCanvas::Refresh();
		}
	} else if(event.GetEventType() == wxEVT_MOUSEWHEEL) {
		if((event.GetWheelRotation() > 0 && m_scale < 4) || //zoom in up to 4x
			  (event.GetWheelRotation() < 0 && (m_scale > 1 || m_scale*m_imageW > W*0.5f || m_scale*m_imageH > H*0.5f))) { //zoom out up to 50% of window size
			//calculate the scaling point in image space
			m_scaleXo = (int)(     event.GetX() /m_scale-m_scaleXo2/m_scale-m_offsetX/m_scale+m_scaleXo);
			m_scaleYo = (int)((H-1-event.GetY())/m_scale-m_scaleYo2/m_scale-m_offsetY/m_scale+m_scaleYo);
			//make sure the scaling point is in bounds
			if(m_scaleXo < 0) m_scaleXo = 0;
			if(m_scaleXo > m_imageW-1) m_scaleXo = m_imageW-1;
			if(m_scaleYo < 0) m_scaleYo = 0;
			if(m_scaleYo > m_imageH-1) m_scaleYo = m_imageH-1;
			//calculate new scale
			m_scaleExp += (event.GetWheelRotation()>0?1:-1)*0.5f;
			m_scale = pow(2.0f, m_scaleExp);
			//get the scaling point in window space
			m_scaleXo2 = event.GetX();
			m_scaleYo2 = H-1-event.GetY();
			m_offsetX = m_offsetY = 0;
			wxGLCanvas::Refresh();
		}
	} else {
		event.Skip();
	}
	m_lastX = event.GetX();
	m_lastY = event.GetY();
}

void LuxGLViewer::Refresh(bool eraseBackground, const wxRect* rect) {
	m_imageChanged=true;
	wxGLCanvas::Refresh(eraseBackground, rect);
}

#endif // LUX_USE_OPENGL
