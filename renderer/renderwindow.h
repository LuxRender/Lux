/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Image.H>
#include <FL/fl_draw.H>

#ifdef LUX_USE_OPENGL
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#endif // LUX_USE_OPENGL


#ifdef LUX_USE_OPENGL

class GlWindow : public Fl_Gl_Window {
	int image_w, image_h;
	int tiles_x, tiles_y, tiles_nr;
	bool image_changed;
	const int texture_w;
	const int texture_h;
	int offset_x, offset_y, scale_xo2, scale_yo2, scale_xo, scale_yo, lastx, lasty;
	float scale;
	float scale_exp;
	Fl_RGB_Image *image_ptr;

 public:
	GlWindow(int x,int y,int w,int h,const char *lab=0);
	~GlWindow();
	void update_image();
	void set_image(Fl_RGB_Image *img);
	int handle(int event);
	void draw(void);
};

#else // LUX_USE_OPENGL

//dummy class
class GlWindow {
 public:
	 GlWindow(int x,int y,int w,int h,const char *lab=0){};
	~GlWindow(){};
	void update_image(){};
	void set_image(Fl_RGB_Image *img){};
	int handle(int event){};
	void draw(void){};
};

#endif // LUX_USE_OPENGL

class RenderWindow: public Fl_Group{
	GlWindow *glwin;
	Fl_Group *groupwin;
	Fl_RGB_Image *image_ptr;
	const bool opengl_enabled;

 public:
	RenderWindow(int x,int y,int w,int h,Fl_Color col_back,Fl_Color col_renderback,const char *lab=0,bool opengl_enabled=false);
	void update_image();
	void set_image(Fl_RGB_Image *img);
};

#endif // RENDER_WINDOW_H
