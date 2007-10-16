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

// luxgui.cpp*
#include "lux.h"
#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "direct.h"

#include "luxgui.h"

#include "icons.h"	// Include GUI icon data

// menu items
Fl_Menu_Item menu_1[] = {
 {"File", 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 11, 0},
 {"    Open scenefile...", 0x4006f, (Fl_Callback*)open_cb, 0, 128, FL_NORMAL_LABEL, 0, 11, 0},
 {"    Exit", 0x40071,  (Fl_Callback*)exit_cb, 0, 0, FL_NORMAL_LABEL, 0, 11, 0},
 {0,0,0,0,0,0,0,0,0},
 {"Help", 0,  0, 0, 65, FL_NORMAL_LABEL, 0, 11, 0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Menu_Item menu_[] = {
 {"+ Add Thread", 0,  (Fl_Callback*)addthread_cb, 0, 0, FL_NORMAL_LABEL, 0, 11, 0},
 {"- Remove Thread", 0,  (Fl_Callback*)removethread_cb, 0, 0, FL_NORMAL_LABEL, 0, 11, 0},
 {0,0,0,0,0,0,0,0,0}
};

// main window
Fl_Double_Window* make_MainWindow(int width, int height, Fl_RGB_Image* rgb_buffer)
{
  Fl_Color col_back = fl_rgb_color(212, 208, 200);
  Fl_Color col_activeback = fl_rgb_color(255, 170, 20);
  Fl_Color col_renderback = fl_rgb_color(128, 128, 128);

  Fl_Double_Window* w;
  { Fl_Double_Window* o = new Fl_Double_Window(width, height, "LuxRender");
    w = o;
    o->color(col_back);
    { Fl_Group* o = new Fl_Group(0, 20, width, height-20);
      o->color(col_back);
      o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
      { Fl_Tabs* o = new Fl_Tabs(0, 20, width, height-20);
        o->labelsize(12);
        { Fl_Group* o = new Fl_Group(0, 40, width, height-40, "Film");	// Film tab
          o->box(FL_FLAT_BOX);
          o->color(col_back);
          o->labelsize(11);
          o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
          { Fl_Group* o = new Fl_Group(0, 40, width, height-40, ""); // Renderdisplay
            o->box(FL_FLAT_BOX);
            o->color(col_renderback);
            //o->image(rgb_buffer);
            o->labelsize(10);
            o->align(FL_ALIGN_CENTER);
            o->end();
			renderview = o;
            Fl_Group::current()->resizable(o);
          } // Fl_Group* o
          o->end();
        } // Fl_Group* o
       /* { Fl_Group* o = new Fl_Group(0, 40, width, height-40, "Console"); // NOTE - radiance - disabled GUI console tab for now
          o->box(FL_FLAT_BOX);
          o->color((Fl_Color)24);
          o->labelsize(11);
          o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
          o->hide();
          o->deactivate();
          o->end();
          Fl_Group::current()->resizable(o);
        }*/ // Fl_Group* o
        o->end();
      } // Fl_Tabs* o
      { Fl_Group* o = new Fl_Group(90, 20, width-90, 20, "toolbar");
        o->box(FL_FLAT_BOX);
        o->color(col_back);
        o->labeltype(FL_NO_LABEL);
        { Fl_Group* o = new Fl_Group(310, 20, 260, 20, "statistics");
          o->box(FL_FLAT_BOX);
          o->color(col_back);
		  o->deactivate();
		  info_statistics_group = o;
          o->labeltype(FL_NO_LABEL);
          { Fl_Box* o = new Fl_Box(310, 20, 260, 20);
            o->box(FL_THIN_UP_BOX);
            o->image(image_clock);
            o->deimage(image_clock1);
            o->labelsize(11);
            o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
          } // Fl_Box* o
          { Fl_Group* o = new Fl_Group(330, 20, 240, 20, "");
            o->labelsize(11);
            o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
			info_statistics = o;
            o->end();
          } // Fl_Group* o
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(90, 20, 85, 20, "geometry");
          o->box(FL_FLAT_BOX);
          o->color(col_back);
          o->labeltype(FL_NO_LABEL);
          o->deactivate();
          { Fl_Box* o = new Fl_Box(90, 20, 85, 20);
            o->box(FL_THIN_UP_BOX);
            o->image(image_geom);
            o->deimage(image_geom1);
            o->labelsize(11);
            o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
          } // Fl_Box* o
          { Fl_Group* o = new Fl_Group(108, 20, 65, 20, "(0)");
            o->labelsize(11);
            o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
            o->end();
          } // Fl_Group* o
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(570, 20, 230, 20, "render");
          o->box(FL_FLAT_BOX);
          o->color(col_back);
          o->labeltype(FL_NO_LABEL);
		  o->deactivate();
		  info_render_group = o;
          o->align(FL_ALIGN_CENTER);
          { Fl_Menu_Button* o = new Fl_Menu_Button(570, 20, 170, 20);
            o->box(FL_THIN_UP_BOX);
            o->down_box(FL_THIN_DOWN_BOX);
            o->color(col_activeback);
            o->image(image_render);
            o->deimage(image_render1);
            o->labelsize(11);
            o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
            o->menu(menu_);
          } // Fl_Menu_Button* o
          { Fl_Group* o = new Fl_Group(590, 20, 65, 20, "(0)");
            o->labelsize(11);
            o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			info_render = o;
            o->end();
			info_render = o;
          } // Fl_Group* o
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(175, 20, 135, 20, "tonemap");
          o->box(FL_FLAT_BOX);
          o->color(col_back);
		  info_tonemap_group = o;
		  o->deactivate();
          o->labeltype(FL_NO_LABEL);
          { Fl_Menu_Button* o = new Fl_Menu_Button(175, 20, 135, 20);
            o->box(FL_THIN_UP_BOX);
            o->down_box(FL_THIN_DOWN_BOX);
            o->color(col_activeback);
            o->image(image_tonemap);
            o->deimage(image_tonemap1);
            o->labelsize(11);
            o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
          } // Fl_Menu_Button* o
          { Fl_Group* o = new Fl_Group(195, 20, 65, 20, "(0)");
            o->labelsize(11);
            o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			info_tonemap = o;
            o->end();
          } // Fl_Group* o
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(width-60, 20, 69, 20, "buttons");
          o->box(FL_FLAT_BOX);
          o->color(col_back);
          o->labeltype(FL_NO_LABEL);
          o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
          { Fl_Button* o = new Fl_Button(width-20, 20, 20, 20);	// pause button
            o->box(FL_THIN_UP_BOX);
            o->down_box(FL_THIN_DOWN_BOX);
            o->image(image_stop);
            o->deimage(image_stop1);
            o->callback((Fl_Callback*)stop_cb);
			o->deactivate();
			o->color(col_back);
			button_pause = o;
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(width-40, 20, 20, 20); // restart button
            o->box(FL_THIN_UP_BOX);
            o->down_box(FL_THIN_DOWN_BOX);
            o->image(image_rewind);
            o->deimage(image_rewind1);
            o->callback((Fl_Callback*)restart_cb);
			o->deactivate();
			o->color(col_back);
			button_restart = o;
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(width-60, 20, 20, 20); // play button
            o->box(FL_THIN_UP_BOX);
            o->down_box(FL_THIN_DOWN_BOX);
            o->image(image_play);
            o->deimage(image_play1);
            o->callback((Fl_Callback*)start_cb);
			o->deactivate();
			o->color(col_back);
			button_play = o;
          } // Fl_Button* o
          o->end();
        } // Fl_Group* o
        o->end();
      } // Fl_Group* o
      o->end();
      Fl_Group::current()->resizable(o);
    } // Fl_Group* o
    { Fl_Menu_Bar* o = new Fl_Menu_Bar(0, 0, 800, 20);
      o->box(FL_THIN_UP_BOX);
	  o->color(col_back);
      o->menu(menu_1);

    } // Fl_Menu_Bar* o
    o->end();
  } // Fl_Double_Window* o
  return w;
}

// Callback implementations
// 'File | Open' from main menu
void open_cb(Fl_Widget*, void*) {
	if( status_render == STATUS_RENDER_NONE ) {
		// Create the file chooser, and show it
		Fl_File_Chooser chooser(".",			// directory
					"LuxRender Scenes (*.lxs)\tPBRT Scenes (*.pbrt)",			// filter
					Fl_File_Chooser::SINGLE, 	// chooser type
					"Open Scenefile...");	// title
		chooser.preview(0);
		chooser.show();

		// Block until user picks something.
		while(chooser.shown())
			{ Fl::wait(); }

		// User hit cancel?
		if ( chooser.value() == NULL ) return;

		// set pwd and launch scene file render
		strcpy(gui_current_scenefile, chooser.value());

		// set PWD
		printf("GUI: Changing working directory to: %s\n", chooser.value());
		_chdir(chooser.directory());
		
		// update window filename string
		static char wintxt[512];
		sprintf(wintxt, "LuxRender: %s", chooser.value());
		window->label(wintxt);

		// start engine thread
		RenderScenefile();
	}
}

void exit_cb(Fl_Widget*, void*) {
	if(engine_thread) {
		luxExit();
		engine_thread->join();
	}
	exit(0);
}

void addthread_cb(Fl_Widget*, void*) { AddThread(); };
void removethread_cb(Fl_Widget*, void*) { RemoveThread(); };
void start_cb(Fl_Widget*, void*) { RenderStart(); }
void stop_cb(Fl_Widget*, void*) { RenderPause(); }
void restart_cb(Fl_Widget*, void*) {  }

void Engine_Thread()
{
	printf("GUI: Initializing Parser\n");
    luxInit();

	printf("GUI: Parsing scenefile '%s'...\n", gui_current_scenefile);
	ParseFile( gui_current_scenefile );
	printf("GUI: Finished parsing scenefile.\n");

	luxCleanup();
/*
#ifdef WIN32
	_endthread();
#else
	pthread_exit(0);
#endif
    return 0;*/
}

// GUI functions
void merge_FrameBuffer(void*) {
	static char ittxt[256];
	sprintf(ittxt, "(1) Tonemapping...");
	static const char *txttp = ittxt;
	printf("GUI: Updating framebuffer...\n");
	info_tonemap_group->activate();
	info_tonemap->label(txttp);
	Fl::redraw();

	Fl::lock();
	luxUpdateFramebuffer();
	printf("GUI: Done.\n");
	Fl::unlock();

    rgb_image->uncache();
	sprintf(ittxt, "(1) Idle.");
	info_tonemap->label(txttp);
    Fl::redraw();
	Fl::repeat_timeout(framebufferUpdate, merge_FrameBuffer);
}

int RenderScenefile()
{
	fflush(stdout);
    engine_thread = new boost::thread(&Engine_Thread);
    
	return 0;
}

void setInfo_render()
{
	static char irtxt[256];

	if(status_render == STATUS_RENDER_RENDER) {
		sprintf(irtxt, "(%i) Rendering...", gui_nrthreads);
		info_render_group->activate();
		info_tonemap_group->activate();
		button_play->value(1);
		button_play->deactivate();
		button_pause->activate();
		info_statistics_group->activate();
	} else if(status_render == STATUS_RENDER_IDLE) {
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
	
	static const char *txtp = irtxt;
	info_render->label(txtp);
	Fl::redraw();
}

void RenderStart()
{
	luxStart();
	status_render = STATUS_RENDER_RENDER;
	setInfo_render();
}

void RenderPause()
{
	luxPause();
	status_render = STATUS_RENDER_IDLE;
	setInfo_render();
}

void AddThread()
{
	gui_nrthreads++;
	setInfo_render();
	if(luxAddThread() == 1) {
		gui_nrthreads--;
		setInfo_render();
	}
}
void RemoveThread()
{
	if(gui_nrthreads > 1) {
		gui_nrthreads--;
		setInfo_render();
		luxRemoveThread();
	}
}

void bindFrameBuffer() {
	if(GuiSceneReady) {
		// fetch camera settings
		u_int xRes = luxFilmXres();
		u_int yRes = luxFilmYres();
		framebufferUpdate = luxDisplayInterval();

	    // bind frame- to rgb buffer
	    uchar* fbP = luxFramebuffer();
	    rgb_image = new Fl_RGB_Image( fbP , xRes, yRes, 3, 0); 

		// update display
		rgb_image->uncache();
		renderview->image(rgb_image);
		Fl::redraw();

        Fl::add_timeout(framebufferUpdate, merge_FrameBuffer);
	}
}

void update_Statistics(void*) {
	int samplessec = Floor2Int(luxStatistics("samplesSec"));
	int secelapsed = Floor2Int(luxStatistics("secElapsed"));
	double samplespx = luxStatistics("samplesPx");

	char t[] = "00:00";
    int secs = (secelapsed) % 60;
    int mins = (secelapsed / 60) % 60;
    int hours = (secelapsed / 3600);
	t[4] = (secs % 10) + '0';
    t[3] = ((secs / 10) % 10) + '0';
    t[1] = (mins % 10) + '0';
    t[0] = ((mins / 10) % 10) + '0';

	static char istxt[256];
	sprintf(istxt, "%i:%s - %i S/s - %.2f S/px", hours, t, samplessec, samplespx);
	static const char *txts = istxt;
	info_statistics->label(txts);
	Fl::redraw();
	Fl::repeat_timeout(2.0, update_Statistics);
}

void check_SceneReady(void*) {
	if(luxStatistics("sceneIsReady")) {
		GuiSceneReady = true;

		// add initial render thread
		status_render = STATUS_RENDER_RENDER;
		setInfo_render();
		Fl::add_timeout(2.0, update_Statistics);
		Fl::redraw();

		Fl::lock();
		//AddThread();
		//luxStart();

		// hook up the film framebuffer
		bindFrameBuffer();
	} else
		Fl::repeat_timeout(0.25, check_SceneReady);
}

// main program
int main(int argc, char *argv[]) {
	// Print welcome banner
	printf("Lux Renderer version %1.3f of %s at %s\n", LUX_VERSION, __DATE__, __TIME__);     
	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, covered by the GNU General Public License V3\n");
	printf("You are welcome to redistribute it under certain conditions,\nsee COPYING.TXT for details.\n");      

	GuiSceneReady = false;
	framebufferUpdate = 10.0f;

	status_render = STATUS_RENDER_NONE;

	// create render window
	int width = 800;
	int height = 600;
    window = make_MainWindow( width, height, rgb_image );
	setInfo_render();
	window->show();

	// set timeouts
    Fl::add_timeout(0.25, check_SceneReady);
    
    // run gui
    Fl::run();

	// TODO stop everything
	return 0;
}
