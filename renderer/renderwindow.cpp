/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
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

// renderwindow.cpp*
#include "lux.h"
#include "renderwindow.h"


#ifdef LUX_USE_OPENGL
//--------------------GlWindow--------------------

GlWindow::GlWindow(int x,int y,int w,int h,const char *lab): Fl_Gl_Window(x,y,w,h,lab), texture_w(256), texture_h(256){
	offset_x=offset_y=scale_xo2=scale_yo2=0;
	lastx=lasty=0;
	image_w=image_h=0;
	tiles_x=tiles_y=tiles_nr=0;
	scale=1.0f;
	scale_exp=0;
	scale_xo=scale_yo=0;
	image_ptr = NULL;
	image_changed = false;
	mode(FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);
	end();
}

GlWindow::~GlWindow(){
}

void GlWindow::update_image(){
	image_changed = true;
}

void GlWindow::set_image(Fl_RGB_Image *img){
	image_ptr = img;
}

void GlWindow::resize(int X,int Y,int W,int H){
	//calculate new offset
	offset_x+=(W-w())/2;
	offset_y+=(H-h())/2;
	//make sure new offset is in bounds
	if((image_w-scale_xo)*scale+scale_xo2+offset_x-10<0)
		offset_x=10-scale_xo2-(image_w-scale_xo)*scale;	
	if((0-scale_xo)*scale+scale_xo2+offset_x+10>W-1)
		offset_x=W-1-10-scale_xo2-(0-scale_xo)*scale;
	if((image_h-scale_yo)*scale+scale_yo2+offset_y-10<0)
		offset_y=10-scale_yo2-(image_h-scale_yo)*scale;
	if((0-scale_yo)*scale+scale_yo2+offset_y+10>H-1)
		offset_y=H-1-10-scale_yo2-(0-scale_yo)*scale;
	Fl_Gl_Window::resize(X,Y,W,H);
}

int GlWindow::handle(int event){
	if(event==FL_ENTER){
		fl_cursor(FL_CURSOR_CROSS);
		return 1;
	}
	if(event==FL_LEAVE){
		fl_cursor(FL_CURSOR_DEFAULT);
		return 1;
	}
	if(event==FL_PUSH){
		if(image_ptr==NULL) return 1;
		lastx=Fl::event_x();
		lasty=Fl::event_y();
		int button=Fl::event_button();
		if(button==3){	//set scale to 100%
			//calculate the scaling point in image space
			scale_xo=       Fl::event_x() /scale-scale_xo2/scale-offset_x/scale+scale_xo;
			scale_yo=(h()-1-Fl::event_y())/scale-scale_yo2/scale-offset_y/scale+scale_yo;
			//make sure the scaling point is in bounds
			if(scale_xo<0) scale_xo=0;
			if(scale_xo>image_w-1) scale_xo=image_w-1;
			if(scale_yo<0) scale_yo=0;
			if(scale_yo>image_h-1) scale_yo=image_h-1;
			//new scale
			scale_exp=0;
			scale=1.0f;
			//get the scaling point in window space
			scale_xo2=Fl::event_x();
			scale_yo2=h()-1-Fl::event_y();
			offset_x=offset_y=0;
			redraw();
		}
		if(button==2){	//fit image to window
			scale=min(w()/(float)image_w, h()/(float)image_h);
			scale_exp=floor(log((float)scale)/log(2.0f)*2+0.5f)/2;
			scale_xo2=w()/2;
			scale_yo2=h()/2;
			scale_xo=image_w/2;
			scale_yo=image_h/2;
			offset_x=0;
			offset_y=0;
			redraw();
		}
		return 1;
	}
	if(event==FL_DRAG){
		if(image_ptr==NULL || !Fl::event_state(FL_BUTTON1)) return 1;
		fl_cursor(FL_CURSOR_MOVE);
		//calculate new offset
		int offset_x_new=offset_x+Fl::event_x()-lastx;
		int offset_y_new=offset_y-(Fl::event_y()-lasty);
		//check if new offset in bounds
		if((image_w-scale_xo)*scale+scale_xo2+offset_x_new-10>=0 && (0-scale_xo)*scale+scale_xo2+offset_x_new+10<=w()-1)
			offset_x=offset_x_new;
		if((image_h-scale_yo)*scale+scale_yo2+offset_y_new-10>=0 && (0-scale_yo)*scale+scale_yo2+offset_y_new+10<=h()-1)
			offset_y=offset_y_new;
		lastx=Fl::event_x();
		lasty=Fl::event_y();
		redraw();
		return 1;
	}
	//if(event==FL_RELEASE){}
	if(event==FL_MOUSEWHEEL){
		if(image_ptr==NULL) return 1;
		if(Fl::event_dy()<0){ //zoom in up to 4x
			if(scale>=4)
				return 1;
		}else{ //zoom out up to 50% of window size
			if(scale<=1 && scale*image_w<w()*0.5f && scale*image_h<h()*0.5f)
				return 1;
		}
		//calculate the scaling point in image space
		scale_xo=       Fl::event_x() /scale-scale_xo2/scale-offset_x/scale+scale_xo;
		scale_yo=(h()-1-Fl::event_y())/scale-scale_yo2/scale-offset_y/scale+scale_yo;
		//make sure the scaling point is in bounds
		if(scale_xo<0) scale_xo=0;
		if(scale_xo>image_w-1) scale_xo=image_w-1;
		if(scale_yo<0) scale_yo=0;
		if(scale_yo>image_h-1) scale_yo=image_h-1;
		//calculate new scale
		scale_exp+=-(Fl::event_dy()>0?1:-1)*0.5f;
		scale=pow(2.0f,scale_exp);
		//get the scaling point in window space
		scale_xo2=Fl::event_x();
		scale_yo2=h()-1-Fl::event_y();
		offset_x=offset_y=0;
		redraw();
		return 1;
	}
	return Fl_Gl_Window::handle(event);
}

void GlWindow::draw(void){
    if (!valid()) { //first execution, init viewport
        valid(1);
        glLoadIdentity();
        glViewport(0,0,w(),h());
        glOrtho(0,w(),0,h(),-1,1);
		glClearColor(0.5,0.5,0.5,1.0); //gray background
	}

    glClear(GL_COLOR_BUFFER_BIT);
	
	static bool firstDraw = true;
	if(firstDraw && image_ptr){	//allocate textures on first draw
		firstDraw=false;
		image_w=image_ptr->w();
		image_h=image_ptr->h();
		tiles_x=image_w/texture_w+1;
		tiles_y=image_h/texture_h+1;
		tiles_nr=tiles_x*tiles_y;
		for(int i=0; i<tiles_nr; i++){
			glBindTexture (GL_TEXTURE_2D, i+1);
			glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);//GL_LINEAR); //'linear' causes seams to show on my ati card - zcott
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, texture_w, texture_h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); //warning: the texture isn't initialized, don't display before uploading
		}
		image_changed=true;
		//move to center of window
		scale_xo2=-image_w/2+w()/2;
		scale_yo2=-image_h/2+h()/2;
	}

	if(image_ptr){	//show the image
		glPushMatrix();
		//transform and roll out!
		glTranslatef(offset_x,offset_y,0);
		glTranslatef(scale_xo2,scale_yo2,0);
		glScalef(scale,scale,1);
		glTranslatef(-scale_xo,-scale_yo,0);

		glEnable (GL_TEXTURE_2D);
		//draw the texture tiles
		for(int i=0; i<tiles_nr; i++){
			int tile_w = i%tiles_x==tiles_x-1?image_w%texture_w:texture_w;
			int tile_h = i/tiles_x==tiles_y-1?image_h%texture_h:texture_h;
			glBindTexture (GL_TEXTURE_2D, i+1);
			if(image_changed)	//upload the textures only when needed (takes long...)
				for(int j=0; j<tile_h; j++) //line by line
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, j, tile_w, 1, GL_RGB, GL_UNSIGNED_BYTE, *image_ptr->data()+(((i%tiles_x)*texture_w)*3 + ((tiles_y-1-i/tiles_x)*texture_h+(texture_h-1-j)-(tiles_y*texture_h-image_h))*image_w*3));

			glBegin (GL_QUADS);
			glTexCoord2f (                           0.0f,                           0.0f );
			glVertex3f (   (i%tiles_x)*texture_w +   0.0f, (i/tiles_x)*texture_h +   0.0f, 0.0f );
			glTexCoord2f (          1.0f*tile_w/texture_w,                           0.0f );
			glVertex3f (   (i%tiles_x)*texture_w + tile_w, (i/tiles_x)*texture_h +   0.0f, 0.0f );
			glTexCoord2f (          1.0f*tile_w/texture_w,          1.0f*tile_h/texture_h );
			glVertex3f (   (i%tiles_x)*texture_w + tile_w, (i/tiles_x)*texture_h + tile_h, 0.0f );
			glTexCoord2f (                           0.0f,          1.0f*tile_h/texture_h );
			glVertex3f (   (i%tiles_x)*texture_w +   0.0f, (i/tiles_x)*texture_h + tile_h, 0.0f );
			glEnd ();
		}
		glDisable (GL_TEXTURE_2D);
		glPopMatrix();
		image_changed=false;
	}else{	// no image, draw an 'X'
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_LINE_STRIP); glVertex2f(w(), h()); glVertex2f(0, 0); glEnd();
		glBegin(GL_LINE_STRIP); glVertex2f(w(), 0); glVertex2f(0, h()); glEnd();
	}
	
	glFlush();
}

#endif // LUX_USE_OPENGL

//--------------------RenderWindow--------------------

RenderWindow::RenderWindow(int x,int y,int w,int h,Fl_Color col_back,Fl_Color col_renderback,const char *lab,bool opengl_enabled)
	: Fl_Group(x,y,w,h,lab), opengl_enabled(opengl_enabled){
	box(FL_FLAT_BOX);
	color(col_back);
	labelsize(11);
	align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
	image_ptr = NULL;
	#ifndef LUX_USE_OPENGL
		opengl_enabled = false;
	#endif // LUX_USE_OPENGL
	if(opengl_enabled){
		groupwin = NULL;
		glwin = new GlWindow(x,y,w,h,"");
	}else{
		glwin = NULL;
		groupwin = new Fl_Group(x,y,w,h,"");
		groupwin->box(FL_FLAT_BOX);
		groupwin->color(col_renderback);
		groupwin->labelsize(1);
		groupwin->align(FL_ALIGN_CENTER);
		groupwin->end();
	}
	end();
}
void RenderWindow::update_image(){
	image_ptr->uncache();
	if(opengl_enabled)
		glwin->update_image();
}
void RenderWindow::set_image(Fl_RGB_Image *img){
	if(img==NULL) return;
	image_ptr=img;
	if(opengl_enabled)
		glwin->set_image(img);
	else
		groupwin->image(img);
}

