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

// luxgui.cpp*
#include "lux.h"
#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "error.h"
#include "luxgui.h"
#include "renderwindow.h"
#include "icons.h"		// Include GUI icon data

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <exception>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/bind.hpp>
#include <zlib.h>



#if defined(WIN32) && !defined(__CYGWIN__)
#include "direct.h"
#include "resource.h"
#define chdir _chdir
#endif


using namespace lux;
namespace po = boost::program_options;
static int threads;
bool parseError;
bool renderingDone;
void AddThread();

// menu items
Fl_Menu_Item menu_bar[] = {
	{ "&File", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 11, 0 },
		{"    &Open scenefile...", FL_CTRL + 'o', (Fl_Callback*) open_cb, 0, 128, FL_NORMAL_LABEL, 0, 11, 0 },
		{"    &Exit", FL_CTRL + 'q', (Fl_Callback *) exit_cb, 0, 0, FL_NORMAL_LABEL, 0, 11, 0 },
		{ 0 },
	{ "&Help", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 11, 0 },
		{"    &About...", 0, (Fl_Callback *) about_cb, 0, 0, FL_NORMAL_LABEL, 0, 11, 0 },
		{ 0 },
	{ 0 }
};

Fl_Menu_Item menu_threads[] = {
	{ "+ Add Thread", 0, (Fl_Callback *) addthread_cb, 0, 0, FL_NORMAL_LABEL, 0, 11, 0 },
	{ "- Remove Thread", 0, (Fl_Callback *) removethread_cb, 0, 0, FL_NORMAL_LABEL, 0, 11, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

void exit_cb(Fl_Widget *, void *) {
	if (fb_update_thread)
		fb_update_thread->join();

    //if we have a scene file
    if(gui_current_scenefile[0]!=0) {
        luxExit();

        if (engine_thread)
            engine_thread->join();

        luxError(LUX_NOERROR, LUX_INFO, "Freeing resources.");
        luxCleanup();
    }

    exit(0);
}

// main window
Fl_Double_Window * make_MainWindow(int width, int height,
		Fl_RGB_Image * rgb_buffer, bool opengl_enabled) {
	Fl_Color col_back = fl_rgb_color(212, 208, 200);
	Fl_Color col_activeback = fl_rgb_color(255, 170, 20);
	Fl_Color col_renderback = fl_rgb_color(128, 128, 128);

	Fl_Double_Window * w;
	{
		Fl_Double_Window * o = new Fl_Double_Window (width, height, "LuxRender");
		w = o;
		o->callback(exit_cb);
		o->color(col_back);
		{
			Fl_Group * o = new Fl_Group (0, 20, width, height - 20);
			o->color(col_back);
			o->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
			{
				Fl_Tabs * o = new Fl_Tabs (0, 20, width, height - 20);
				o->labelsize(12);
				{	//Film tab
					RenderWindow* o = new RenderWindow(0, 40, width, height-40, col_back, col_renderback, "Film", opengl_enabled);
					renderview = o;
				} // RenderWindow* o
				/* { Fl_Group* o = new Fl_Group(0, 40, width, height-40, "Console"); // NOTE - radiance - disabled GUI console tab for now
				 o->box(FL_FLAT_BOX);
				 o->color((Fl_Color)24);
				 o->labelsize(11);
				 o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
				 o->hide();
				 o->deactivate();
				 o->end();
				 Fl_Group::current()->resizable(o);
				 } */// Fl_Group* o
				o->end();
			} // Fl_Tabs* o
			{
				Fl_Group * o = new Fl_Group (90, 20, width - 90, 20, "toolbar");
				o->box(FL_FLAT_BOX);
				o->color(col_back);
				o->labeltype(FL_NO_LABEL);
				{
					Fl_Group * o = new Fl_Group (310, 20, 260, 20, "statistics");
					o->box(FL_FLAT_BOX);
					o->color(col_back);
					o->deactivate();
					info_statistics_group = o;
					o->labeltype(FL_NO_LABEL);
					{
						Fl_Box * o = new Fl_Box (310, 20, 260, 20);
						o->box(FL_THIN_UP_BOX);
						o->image(image_clock);
						o->deimage(image_clock1);
						o->labelsize(11);
						o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
					} // Fl_Box* o
					{
						Fl_Group * o = new Fl_Group (330, 20, 240, 20, "");
						o->labelsize(11);
						o->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
						info_statistics = o;
						o->end();
					} // Fl_Group* o
					o->end();
				} // Fl_Group* o
				{
					Fl_Group * o = new Fl_Group (90, 20, 85, 20, "geometry");
					o->box(FL_FLAT_BOX);
					o->color(col_back);
					o->labeltype(FL_NO_LABEL);
					o->deactivate();
					{
						Fl_Box * o = new Fl_Box (90, 20, 85, 20);
						o->box(FL_THIN_UP_BOX);
						o->image(image_geom);
						o->deimage(image_geom1);
						o->labelsize(11);
						o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
					} // Fl_Box* o
					{
						Fl_Group * o = new Fl_Group (108, 20, 65, 20, "(0)");
						o->labelsize(11);
						o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
						o->end();
					} // Fl_Group* o
					o->end();
				} // Fl_Group* o
				{
					Fl_Group * o = new Fl_Group (570, 20, 230, 20, "render");
					o->box(FL_FLAT_BOX);
					o->color(col_back);
					o->labeltype(FL_NO_LABEL);
					o->deactivate();
					info_render_group = o;
					o->align(FL_ALIGN_CENTER);
					{
						Fl_Menu_Button * o = new Fl_Menu_Button (570, 20, 170, 20);
						o->box(FL_THIN_UP_BOX);
						o->down_box(FL_THIN_DOWN_BOX);
						o->color(col_activeback);
						o->image(image_render);
						o->deimage(image_render1);
						o->labelsize(11);
						o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
						o->menu(menu_threads);
					} // Fl_Menu_Button* o
					{
						Fl_Group * o = new Fl_Group (590, 20, 65, 20, "(0)");
						o->labelsize(11);
						o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
						info_render = o;
						o->end();
						info_render = o;
					} // Fl_Group* o
					o->end();
				} // Fl_Group* o
				{
					Fl_Group * o = new Fl_Group (175, 20, 135, 20, "tonemap");
					o->box(FL_FLAT_BOX);
					o->color(col_back);
					info_tonemap_group = o;
					o->deactivate();
					o->labeltype(FL_NO_LABEL);
					{
						Fl_Menu_Button * o = new Fl_Menu_Button (175, 20, 135, 20);
						o->box(FL_THIN_UP_BOX);
						o->down_box(FL_THIN_DOWN_BOX);
						o->color(col_activeback);
						o->image(image_tonemap);
						o->deimage(image_tonemap1);
						o->labelsize(11);
						o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
					} // Fl_Menu_Button* o
					{
						Fl_Group * o = new Fl_Group (195, 20, 65, 20, "(0)");
						o->labelsize(11);
						o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
						info_tonemap = o;
						o->end();
					} // Fl_Group* o
					o->end();
				} // Fl_Group* o
				{
					Fl_Group * o = new Fl_Group (width - 60, 20, 69, 20, "buttons");
					o->box(FL_FLAT_BOX);
					o->color(col_back);
					o->labeltype(FL_NO_LABEL);
					o->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
					{
						Fl_Button * o = new Fl_Button (width - 20, 20, 20, 20); // pause button
						o->box(FL_THIN_UP_BOX);
						o->down_box(FL_THIN_DOWN_BOX);
						o->image(image_stop);
						o->deimage(image_stop1);
						o->callback((Fl_Callback *) stop_cb);
						o->deactivate();
						o->color(col_back);
						button_pause = o;
					} // Fl_Button* o
					{
						Fl_Button * o = new Fl_Button (width - 40, 20, 20, 20); // restart button
						o->box(FL_THIN_UP_BOX);
						o->down_box(FL_THIN_DOWN_BOX);
						o->image(image_rewind);
						o->deimage(image_rewind1);
						o->callback((Fl_Callback *) restart_cb);
						o->deactivate();
						o->color(col_back);
						button_restart = o;
					} // Fl_Button* o
					{
						Fl_Button * o = new Fl_Button (width - 60, 20, 20, 20); // play button
						o->box(FL_THIN_UP_BOX);
						o->down_box(FL_THIN_DOWN_BOX);
						o->image(image_play);
						o->deimage(image_play1);
						o->callback((Fl_Callback *) start_cb);
						o->deactivate();
						o->color(col_back);
						button_play = o;
					} // Fl_Button* o
					o->end();
				} // Fl_Group* o
				o->end();
			} // Fl_Group* o
			o->end();
			Fl_Group::current ()->resizable(o);
		} // Fl_Group* o
		{
			Fl_Menu_Bar * o = new Fl_Menu_Bar (0, 0, 800, 20);
			o->box(FL_THIN_UP_BOX);
			o->color(col_back);
			o->menu(menu_bar);

		} // Fl_Menu_Bar* o
		o->end();
	} // Fl_Double_Window* o
	return w;
}

// Callback implementations
// 'File | Open' from main menu
void open_cb(Fl_Widget *, void *) {
	if (status_render == STATUS_RENDER_NONE) {
		// Create the file chooser, and show it
		Fl_File_Chooser chooser(".", // directory
				"LuxRender Scenes (*.lxs)", // filter
				Fl_File_Chooser::SINGLE, // chooser type
				"Open Scenefile..."); // title
		chooser.preview(0);
		chooser.show();

		// Block until user picks something.
		while (chooser.shown()) {
			Fl::wait();
		}

		// User hit cancel?
		if (chooser.value() == NULL)
			return;

		// set pwd and launch scene file render
		strcpy(gui_current_scenefile, chooser.value());

		// set PWD
		printf("GUI: Changing working directory to: %s\n", chooser.value());
		chdir(chooser.directory());

		// update window filename string
		static char wintxt[512];
		sprintf(wintxt, "LuxRender: %s", chooser.value());
		window->label(wintxt);

		// start engine thread
		RenderScenefile();
	}
}

// NOTE - radiance - added simple about window for RC4 release, will need to add compression of image stored in splash.h
#include "splash.h"
void about_cb(Fl_Widget *, void *) {
	unsigned int about_window_w = 500;
	unsigned int about_window_h = 270;
	Fl_Window *about_window = new Fl_Double_Window(window->x()+window->w()/2-about_window_w/2,window->y()+window->h()/2-about_window_h/2,about_window_w, about_window_h, "About: LuxRender");
		{ Fl_Button* o = new Fl_Button(0, 0, 500, 270);
		  o->image(image_splash);
		  o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
		}
	about_window->end();
	about_window->set_modal();
	about_window->show();

	while(1){
		Fl_Widget *o = Fl::readqueue();
		if (!o) {
			Fl::wait();
			Fl::wait(0.1);
		}
		else	break;
	}

	about_window->hide();
	delete about_window;
}


/*

// NOTE - radiance - disabled zcott's horrid monstrosity 'about window' for PR reasons...

void about_cb(Fl_Widget *, void *) {
	unsigned long logo_size = lux_logo_w*lux_logo_h;
	unsigned long starting_color = 0;

	//unpack logo data to memory
	unsigned char *logo_data_bw = new unsigned char[logo_size];
	if(uncompress((Bytef *)logo_data_bw, &logo_size, (Bytef *)lux_logo_dataz, lux_logo_dataz_size) != Z_OK){
		delete logo_data_bw;
		return;
	}
	//make the logo colorful (base color: 212,127,0)
	unsigned char *logo_data = new unsigned char[logo_size*3];
	for(unsigned int i=0;i<logo_size;i++){
		//logo_data[i*3+0]=212*(255-logo_data_bw[i])/255;
		//logo_data[i*3+1]=127*(255-logo_data_bw[i])/255;
		//logo_data[i*3+2]=  0*(255-logo_data_bw[i])/255;
		logo_data[i*3+0]=212*(255-logo_data_bw[i])/255;
		logo_data[i*3+1]=127*(255-logo_data_bw[i])/255;
		logo_data[i*3+2]=  0*(255-logo_data_bw[i])/255;
	}
	delete logo_data_bw;
	//prepare the output image buffer
	unsigned char *logo_data_out = new unsigned char[logo_size*3];
	for(unsigned int i=0;i<logo_size*3;i++){
		logo_data_out[i]=starting_color;
	}
	Fl_RGB_Image *logo_image = new Fl_RGB_Image(logo_data_out, lux_logo_w, lux_logo_h, 3, 0);

	//create the window (top level, modal)
	unsigned int about_window_w = 10+lux_logo_w+10;
	unsigned int about_window_h = 10+lux_logo_h+140;
	Fl_Window *about_window = new Fl_Double_Window(window->x()+window->w()/2-about_window_w/2,window->y()+window->h()/2-about_window_h/2,about_window_w, about_window_h, "About: LuxRender");
	about_window->color(FL_BLACK);
	{	Fl_Box* o = new Fl_Box(10, 10, lux_logo_w, lux_logo_h);
		o->box(FL_NO_BOX);
		o->image(*logo_image);
	}
	{	Fl_Box *o = new Fl_Box(about_window_w/2-lux_logo_w/2, 10+lux_logo_h+10, lux_logo_w, 40);
		std::ostringstream oss;
		oss<<"LuxRender v"<<LUX_VERSION_STRING<<"\n(built on: "<<__DATE__<<")";
		o->copy_label(oss.str().c_str());
		o->labelfont(FL_HELVETICA);
		o->labelcolor(FL_LIGHT2);
		o->labelsize(14); 
		o->color(FL_WHITE);
	}
	{	Fl_Box *o = new Fl_Box(30, about_window_h-85, 125, 20, "Home page:");
		o->labelfont(FL_HELVETICA);
		o->labelcolor(FL_DARK1);
		o->labelsize(14);
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	}
	Fl_Widget *link1;
	{	Fl_Button *o = new Fl_Button(150, about_window_h-85, 130, 20, "www.luxrender2.org");
		o->color(FL_BLACK);
		o->labelcolor(FL_BLUE);
		o->labelsize(14);
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
		o->box(FL_NO_BOX);
		link1=o;
	}
	{	Fl_Box *o = new Fl_Box(30, about_window_h-65, 125, 20, "Based on PBRT:");
		o->labelfont(FL_HELVETICA);
		o->labelcolor(FL_DARK1);
		o->labelsize(14);
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	}
	Fl_Widget *link2;
	{	Fl_Button *o = new Fl_Button(150, about_window_h-65, 88, 20, "www.pbrt.org");
		o->color(FL_BLACK);
		o->labelcolor(FL_BLUE);
		o->labelsize(14);
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
		o->box(FL_NO_BOX);
		link2=o;
	}
	{	Fl_Return_Button *o = new Fl_Return_Button(about_window_w/2-100/2, about_window_h-35, 100, 25, "OK");
		o->color(FL_LIGHT2);
		o->labelcolor(FL_LIGHT2);
		o->box(FL_SHADOW_FRAME);
		o->take_focus();
	}
	about_window->end();
	about_window->set_modal();
	about_window->show();

	//the fancy fade-in routine
	const unsigned int r_vector_size=logo_size/4;
	float *r_vector = new float[r_vector_size];
	for(unsigned int i=0;i<r_vector_size;i++) r_vector[i]=0;
	int steps=0;
	while(Fl::readqueue());	//clear the queue
	while(1){
		Fl_Widget *o = Fl::readqueue();
		if (!o) {
			Fl::wait();
			Fl::wait(0.1);
			if(steps%2) for(unsigned int i=0;i<r_vector_size;i++) r_vector[i]+=(1-r_vector[i])* 1.0f/(rand()%255+1);
			for(unsigned int i=(steps%2)?0:1;i<logo_size;i+=2){
				float scale=r_vector[((steps%2)?((i/r_vector_size)%2):(1-(i/r_vector_size)%2))?(i%r_vector_size):(r_vector_size-1-i%r_vector_size)];
				logo_data_out[i*3+0]=starting_color+(logo_data[i*3+0]-starting_color)*scale;
				logo_data_out[i*3+1]=starting_color+(logo_data[i*3+1]-starting_color)*scale;
				logo_data_out[i*3+2]=starting_color+(logo_data[i*3+2]-starting_color)*scale;
			}
			logo_image->uncache();
			about_window->redraw();
			steps++;
		}
		else	if(o==link1) fl_open_uri("http://www.luxrender2.org");
		else	if(o==link2) fl_open_uri("http://www.pbrt.org");
		else	break;
	}
	//remove the window and clean up
	about_window->hide();
	delete r_vector;
	delete logo_data;
	delete logo_data_out;
	delete logo_image;
	delete about_window;
}
*/

void addthread_cb(Fl_Widget *, void *) {
	AddThread();
}

;
void removethread_cb(Fl_Widget *, void *) {
	RemoveThread();
}

;
void start_cb(Fl_Widget *, void *) {
	RenderStart();
}

void stop_cb(Fl_Widget *, void *) {
	RenderPause();
}

void restart_cb(Fl_Widget *, void *) {
}

void Engine_Thread() {
	//luxInit();
	std::stringstream ss;
	ss << "GUI: Parsing scenefile '" << gui_current_scenefile << "'";
	luxError(LUX_NOERROR, LUX_INFO, ss.str ().c_str());
	ParseFile(gui_current_scenefile);
	if (luxStatistics("sceneIsReady") == false)
		parseError = true;
    
    // Dade - wait for the end of the rendering
    
    luxWait();
    renderingDone = true;
    luxError(LUX_NOERROR, LUX_INFO, "Rendering done.");

    // Dade - avoid to call luxCleanup() in order to free resources used
    // merge_FrameBuffer_Thread

	/*
	 #ifdef WIN32
	 _endthread();
	 #else
	 pthread_exit(0);
	 #endif
	 return 0; */
}

void merge_FrameBuffer_Thread(bool *threadDone){
	luxUpdateFramebuffer();
	*threadDone = true;
}

// GUI functions
void merge_FrameBuffer(void *) {
	if (fb_update_thread)
        return; // update already in progress or rendering done

	static char ittxt[256];
	sprintf(ittxt, "(1) Tonemapping...");
	static const char * txttp = ittxt;
	luxError(LUX_NOERROR, LUX_INFO, "GUI: Updating framebuffer...");
	info_tonemap_group->activate();
	info_tonemap->label(txttp);
	Fl::redraw();

	bool threadDone = false;
	fb_update_thread = new boost::thread(boost::bind(&merge_FrameBuffer_Thread, &threadDone));
	while(!threadDone)	Fl::wait(0.5);
	fb_update_thread->join();
	delete fb_update_thread;
	fb_update_thread = NULL;
	renderview->update_image();

	luxError(LUX_NOERROR, LUX_INFO, "GUI: Framebuffer update done.");
	sprintf(ittxt, "(1) Idle.");
	info_tonemap->label(txttp);
	Fl::redraw();

    // Dade - continue to update the framebuffer only if the rendering is still
    // in progress
    if (!renderingDone)
        Fl::repeat_timeout(framebufferUpdate, merge_FrameBuffer);
}

int RenderScenefile() {
	//create and show an info window
	unsigned int parsing_window_w = 300;
	unsigned int parsing_window_h = 40;
	Fl_Window *parsing_window = new Fl_Window(window->x()+window->w()/2-parsing_window_w/2,window->y()+window->h()/2-parsing_window_h/2,parsing_window_w, parsing_window_h, "Processing...");
	{	Fl_Box *o = new Fl_Box(10, 10, parsing_window_w-20, parsing_window_h-20, "Parsing scene file, please wait...");
		o->labelsize(16);
	}
	parsing_window->end();
	parsing_window->set_modal();
	parsing_window->show();

	fflush(stdout);
	parseError = false;
    renderingDone = false;
	//create parsing thread
	engine_thread = new boost::thread (&Engine_Thread);

	//wait the scene parsing to finish
	while (!luxStatistics("sceneIsReady") && !parseError) {
/*		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec += 1;
		boost::thread::sleep(xt);
*/		
		Fl::wait(0.5);
	}

	if(parseError) {
		std::stringstream ss;
		ss<<"Skipping invalid scenefile '"<<gui_current_scenefile<<"'";
		luxError(LUX_BADFILE, LUX_SEVERE, ss.str ().c_str());

		//destroy the info window
		parsing_window->hide();
		delete parsing_window;
		//show an error message
		message_window("Error","Invalid scenefile!");

		//return 1;

		//note: something's wrong here, the next scene file won't load properly, have to quit! - zcott
		exit(1);
	}

	//add rendering threads
	int threadsToAdd = threads;
	while (--threadsToAdd) {
		AddThread();
	}

	//destroy the info window
	parsing_window->hide();
	delete parsing_window;

	return 0;
}

void setInfo_render() {
	static char irtxt[256];

	if (status_render == STATUS_RENDER_RENDER) {
		sprintf(irtxt, "(%i) Rendering...", gui_nrthreads);
		info_render_group->activate();
		info_tonemap_group->activate();
		button_play->value(1);
		button_play->deactivate();
		button_pause->activate();
		info_statistics_group->activate();
	} else if (status_render == STATUS_RENDER_IDLE) {
		sprintf(irtxt, "(%i) Idle.", gui_nrthreads);
		info_statistics_group->activate();
		info_render_group->activate();
		button_play->activate();
		button_play->value(0);
		button_pause->deactivate();
	} else {
		info_render_group->deactivate();
		info_tonemap_group->deactivate();
		button_play->deactivate();
		button_pause->deactivate();
		button_play->value(0);
		button_pause->value(0);
		info_statistics_group->deactivate();
		sprintf(irtxt, "(0)");
	}

	static const char * txtp = irtxt;
	info_render->label(txtp);
	Fl::redraw();
}

void RenderStart() {
	luxStart();
	status_render = STATUS_RENDER_RENDER;
	setInfo_render();
}

void RenderPause() {
	luxPause();
	status_render = STATUS_RENDER_IDLE;
	setInfo_render();
}

void AddThread() {
	gui_nrthreads++;
	setInfo_render();
	if (luxAddThread() == 1) {
		gui_nrthreads--;
		setInfo_render();
	}
}
void RemoveThread() {
	if (gui_nrthreads > 1) {
		gui_nrthreads--;
		setInfo_render();
		luxRemoveThread();
	}
}

void bindFrameBuffer() {
	if (GuiSceneReady) {
		// fetch camera settings
		u_int xRes = (u_int)luxStatistics("filmXres");//luxFilmXres();
		u_int yRes = (u_int)luxStatistics("filmYres");//luxFilmYres();
		framebufferUpdate = (float)luxStatistics("displayInterval");//luxDisplayInterval();

		// bind frame- to rgb buffer
		uchar * fbP = luxFramebuffer();
		rgb_image = new Fl_RGB_Image (fbP, xRes, yRes, 3, 0);

		// update display
		rgb_image->uncache();
		renderview->set_image(rgb_image);
		Fl::redraw();

		Fl::add_timeout(framebufferUpdate, merge_FrameBuffer);
	}
}

void update_Statistics(void *) {
	int samplessec = Floor2Int(luxStatistics("samplesSec"));
    int samplesTotSec = Floor2Int(luxStatistics("samplesTotSec"));
	int secelapsed = Floor2Int(luxStatistics("secElapsed"));
	double samplespx = luxStatistics("samplesPx");
	int efficiency = Floor2Int(luxStatistics("efficiency"));

	char t[] = "00:00";
	int secs = (secelapsed) % 60;
	int mins = (secelapsed / 60) % 60;
	int hours = (secelapsed / 3600);
	t[4] = (secs % 10) + '0';
	t[3] = ((secs / 10) % 10) + '0';
	t[1] = (mins % 10) + '0';
	t[0] = ((mins / 10) % 10) + '0';

	static char istxt[256];
	sprintf(istxt, "%i:%s - %i S/s - %i TotS/s - %.2f S/px - %i%% eff", hours, t, 
            samplessec, samplesTotSec,
			samplespx, efficiency);
	static const char * txts = istxt;
	info_statistics->label(txts);
	Fl::redraw();
	Fl::repeat_timeout(2.0, update_Statistics);
}

void check_SceneReady(void *) {
	if (luxStatistics("sceneIsReady")) {
		GuiSceneReady = true;

		// add initial render thread
		status_render = STATUS_RENDER_RENDER;
		setInfo_render();
		Fl::add_timeout(2.0, update_Statistics);
		Fl::redraw();

#ifndef __APPLE__
		Fl::lock();
#endif
		//AddThread();
		//luxStart();

		// hook up the film framebuffer
		bindFrameBuffer();
	} else
		Fl::repeat_timeout(0.25, check_SceneReady);
}

void message_window(const char *label, const char *msg){
	//very simple modal message window
	//todo: resize to fit text
	unsigned int message_window_w = 300;
	unsigned int message_window_h = 100;
	Fl_Window *message_window = new Fl_Window(window->x()+window->w()/2-message_window_w/2,window->y()+window->h()/2-message_window_h/2,message_window_w, message_window_h, label);
	if(msg!=NULL){
		Fl_Box *o = new Fl_Box(10, 10, message_window_w-20, message_window_h-10-45, msg);
		o->labelsize(16);
	}
	{	Fl_Return_Button *o = new Fl_Return_Button(message_window_w/2-100/2, message_window_h-35, 100, 25, "OK");
		o->take_focus();
	}
	message_window->end();
	message_window->set_modal();
	message_window->show();
	while(Fl::readqueue());
	while(1){
		Fl_Widget *o = Fl::readqueue();
		if (!o) Fl::wait();
		else break;
	}
	message_window->hide();
	delete message_window;
}

// main program
int main(int ac, char *av[]) {
    bool useServer = false;
	GuiSceneReady = false;
	framebufferUpdate = 10.0f;
	strcpy(gui_current_scenefile, "");
	status_render = STATUS_RENDER_NONE;
	bool opengl_enabled = true;
	engine_thread = NULL;
	fb_update_thread = NULL;

	//jromang : we have to call luxInit before luxStatistics (in check_SceneReady)
	luxInit();

	try
	{
		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic ("Generic options");
		generic.add_options ()
			("version,v", "Print version string")
			("help", "Produce help message")
            ("debug,d", "Enable debug mode")
            ;

		// Declare a group of options that will be
		// allowed both on command line and in
		// config file
		po::options_description config ("Configuration");
		config.add_options ()
			("threads,t", po::value < int >(), "Specify the number of threads that Lux will run in parallel.")
            ("useserver,u", po::value< std::vector<std::string> >()->composing(), "Specify the adress of a rendering server to use.")
            ("serverinterval,i", po::value < int >(), "Specify the number of seconds between requests to rendering servers.")
            ;

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden ("Hidden options");
		hidden.add_options ()
			("input-file", po::value < vector < string > >(), "input file");

		#ifdef LUX_USE_OPENGL
			generic.add_options ()
				("noopengl", "Disable OpenGL to display the image");
		#else
			hidden.add_options ()
				("noopengl", "Disable OpenGL to display the image");
		#endif // LUX_USE_OPENGL

		po::options_description cmdline_options;
		cmdline_options.add (generic).add (config).add (hidden);

		po::options_description config_file_options;
		config_file_options.add (config).add (hidden);

		po::options_description visible ("Allowed options");
		visible.add (generic).add (config);

		po::positional_options_description p;

		p.add ("input-file", -1);

		po::variables_map vm;
		store (po::command_line_parser (ac, av).
				options (cmdline_options).positional (p).run (), vm);

		std::ifstream ifs ("luxrender.cfg");
		store (parse_config_file (ifs, config_file_options), vm);
		notify (vm);

		if (vm.count ("help"))
		{
			std::cout << "Usage: luxrender [options] file...\n";
			std::cout << visible << "\n";
			return 0;
		}

		if (vm.count ("version"))
		{
			std::
			cout << "Lux version " << LUX_VERSION_STRING << " of " << __DATE__ <<
			" at " << __TIME__ << std::endl;
			return 0;
		}

		if (vm.count ("threads"))
		{
			threads = vm["threads"].as < int >();
		}
		else
		{
			threads = 1;;
		}

        if (vm.count("debug")) {
            luxError(LUX_NOERROR, LUX_INFO, "Debug mode enabled");
            luxEnableDebugMode();
        }

        int serverInterval;
        if (vm.count("serverinterval")) {
            serverInterval = vm["serverinterval"].as<int>();
            luxSetNetworkServerUpdateInterval(serverInterval);
        } else
            serverInterval = luxGetNetworkServerUpdateInterval();

        if (vm.count("useserver")) {
            std::stringstream ss;

            std::vector<std::string> names = vm["useserver"].as<std::vector<std::string> >();

            for (std::vector<std::string>::iterator i = names.begin(); i < names.end(); i++) {
                ss.str("");
                ss << "Connecting to server '" << (*i) << "'";
                luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

                //TODO jromang : try to connect to the server, and get version number. display message to see if it was successfull		
                luxAddServer((*i).c_str());
            }

            useServer = true;
            
            ss.str("");
            ss << "Server requests interval:  " << serverInterval << " secs";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
        }

		if (vm.count ("noopengl"))
		{
			opengl_enabled = false;
		}
		else
		{
			#ifdef LUX_USE_OPENGL
				opengl_enabled = true;
			#else
				opengl_enabled = false;
				luxError(LUX_NOERROR, LUX_INFO, "GUI: OpenGL support was not compiled in - will not be used.");
			#endif // LUX_USE_OPENGL
		}

		if (vm.count ("input-file"))
		{
			const std::vector < std::string > &v = vm["input-file"].as < vector < string > >();
			if (v.size() > 1)
			{
				luxError (LUX_SYSTEM, LUX_SEVERE,
						"More than one file passed on command line : rendering the first one.");
			}

			//change the working directory
			boost::filesystem::path fullPath (boost::filesystem::initial_path ());
			fullPath =
			boost::filesystem::system_complete (boost::filesystem::
					path (v[0],	boost::filesystem::native));
			if(!boost::filesystem::exists(fullPath))
			{
				std::stringstream ss;
				ss<<"Unable to open scenefile '"<<fullPath.string()<<"'";
				luxError(LUX_NOFILE, LUX_SEVERE, ss.str ().c_str());
			}
			else
			{
				strcpy (gui_current_scenefile, fullPath.leaf ().c_str ());
				chdir (fullPath.branch_path ().string ().c_str ());
			}
		}

		// create main window
		int width = 800;
		int height = 600;
		window = make_MainWindow(width, height, rgb_image, opengl_enabled);
		setInfo_render();
		#if defined(WIN32) && !defined(__CYGWIN__)
			//grab the icon resource and assign it to the window
			window->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON1)));
		#elif !defined(__APPLE__) && !defined(__CYGWIN__)
			//create an icon from the included bitmap (without transparency)
			fl_open_display();
			Pixmap icon_pixmap=XCreateBitmapFromData(fl_display, DefaultRootWindow(fl_display), (char*)lux_icon_bitmap, 32, 32);
			window->icon((char *)icon_pixmap);
		#endif
		//set basic colors so they don't get overridden by system defaults
		Fl::foreground(0,0,0);
		Fl::background(200,200,200);
		Fl::background2(255,255,255);

		window->show(1,av);
		#if !defined(WIN32) && !defined(__APPLE__)
			//(FLTK workaround) to make the icon transparent we have to tell X directly about the mask
			XWMHints *hints  = XGetWMHints(fl_display, fl_xid(window));
			hints->icon_mask = XCreateBitmapFromData(fl_display, fl_xid(window), (char*)lux_icon_mask, 32, 32);
			hints->flags    |= IconMaskHint;
			XSetWMHints(fl_display, fl_xid(window), hints);
			XFree(hints);
		#endif
		
		if(gui_current_scenefile[0]!=0) //if we have a scene file
			RenderScenefile();

		// set timeouts
		Fl::add_timeout(0.25, check_SceneReady);

		// run gui
		Fl::run ();
	}
	catch (std::exception & e)
	{
		std::cout << e.what () << std::endl; return 1;
	}

	// TODO stop everything
	return 0;
}
