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

// lux.cpp*
#include "lux.h"
#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "luxgui.h"

int RenderScenefile();

Scene* GuiScenePtr;
float framebufferUpdate;
Fl_RGB_Image* rgb_image;
Fl_Window* window;

Fl_Thread* e_thr;

char gui_current_scenefile[256];

// Callback: when use picks 'File | Open' from main menu
void open_cb(Fl_Widget*, void*) {

    // Create the file chooser, and show it
    Fl_File_Chooser chooser(".",			// directory
			    "Lux Files (*.lux)\tPBRT Files (*.pbrt)",			// filter
			    Fl_File_Chooser::SINGLE, 	// chooser type
			    "Open Scenefile...");	// title
    chooser.show();

    // Block until user picks something.
    while(chooser.shown())
        { Fl::wait(); }

    // User hit cancel?
    if ( chooser.value() == NULL ) return;

    // dir is in: chooser.directory()
	//gui_current_scenefile = chooser.value();
	strcpy(gui_current_scenefile, chooser.value());
	RenderScenefile();
}

Fl_Menu_Item menu_menu[] = {
 {"File", 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 12, 0},
 {"Open Scenefile...", 0x4006f,  (Fl_Callback*)open_cb, 0, 128, FL_NORMAL_LABEL, 0, 12, 0},
 {"Exit", 0x40071,  0, 0, 0, FL_NORMAL_LABEL, 0, 12, 0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Button* image_button;

Fl_Window* Render_Window(unsigned int& width, unsigned int& height, Fl_RGB_Image* rgb_image) {
   Fl_Double_Window* w;
  { Fl_Double_Window* o = new Fl_Double_Window(1024, 768, "Lux Render");
    w = o;
    o->color((Fl_Color)1);
    { Fl_Group* o = new Fl_Group(0, 0, 1024, 768);
      o->box(FL_FLAT_BOX);
      o->color((Fl_Color)3);
      o->align(FL_ALIGN_CENTER);
      { Fl_Scroll* o = new Fl_Scroll(0, 23, 1024, 722, "scroll");
        o->box(FL_FLAT_BOX);
        o->color(FL_INACTIVE_COLOR);
        o->labeltype(FL_NO_LABEL);
        o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
        { Fl_Button* o = new Fl_Button(1024 / 2 - width / 2, 768 / 2 - height / 2, width, height);
          o->box(FL_NO_BOX);
          //o->image(rgb_image); TODO set buttonsize
          o->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
		  Fl_Group::current()->resizable(o);
		  image_button = o;
        } // Fl_Button* o
        o->end();
        Fl_Group::current()->resizable(o);
      } // Fl_Scroll* o
      { Fl_Menu_Bar* o = new Fl_Menu_Bar(0, 0, 1024, 23);
	    o->labeltype(FL_NO_LABEL);
        o->menu(menu_menu);
      } // Fl_Menu_Bar* o
      { Fl_Group* o = new Fl_Group(0, 745, 1024, 23, "status_bar");
        o->box(FL_UP_BOX);
        o->labeltype(FL_NO_LABEL);
        o->end();
      } // Fl_Group* o
      o->end();
      Fl_Group::current()->resizable(o);
    } // Fl_Group* o
    o->size_range(400, 300);
    o->end();
  } // Fl_Double_Window* o
  return w;
}

void merge_FrameBuffer(void*) {
	printf("Updating frambuffer...\n");
	Fl::lock();
	GuiScenePtr->camera->film->updateFrameBuffer();
	printf("Done.\n");
	Fl::unlock();

    rgb_image->uncache();

    Fl::redraw();
	Fl::repeat_timeout(framebufferUpdate, merge_FrameBuffer);
}

void* Engine_Thread( void* p )
{
	printf("Initializing Parser\n");
    luxInit();

	printf("Parsing scenefile '%s'...\n", gui_current_scenefile);
	ParseFile( gui_current_scenefile );
	printf("Finished parsing scenefile.\n");

	luxCleanup();

	_endthread();
    return 0;
}

int RenderScenefile()
{
	fflush(stdout);
    fl_create_thread((Fl_Thread&)e_thr, Engine_Thread, 0 );
	return 0;
}

void bindFrameBuffer() {
	if(GuiScenePtr) {
		// fetch camera settings
		Film* cFilm = GuiScenePtr->camera->film;
		u_int xRes = cFilm->xResolution;
		u_int yRes = cFilm->yResolution;
		framebufferUpdate = cFilm->getldrDisplayInterval();

	    // bind frame- to rgb buffer
	    uchar* fbP = GuiScenePtr->camera->film->getFrameBuffer();
	    rgb_image = new Fl_RGB_Image( fbP , xRes, yRes, 3, 0); 

		// update display
		rgb_image->uncache();
		image_button->image(rgb_image);
		Fl::redraw();

        Fl::add_timeout(framebufferUpdate, merge_FrameBuffer);
	}
}

void check_scenePtr(void*) {
	if(GuiScenePtr) {
		bindFrameBuffer();
	} else
		Fl::repeat_timeout(0.25, check_scenePtr);
}

// main program
int main(int argc, char *argv[]) {
	// Print welcome banner
	printf("Lux Renderer version %1.3f of %s at %s\n", LUX_VERSION, __DATE__, __TIME__);     
	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, covered by the GNU General Public License V3\n");
	printf("You are welcome to redistribute it under certain conditions,\nsee COPYING.TXT for details.\n");      

	GuiScenePtr = NULL;
	framebufferUpdate = 10.0f;

	// create render window
	u_int width = 800;
	u_int height = 600;
    window = Render_Window( width, height, rgb_image );
	window->show();

	// set timeouts
    Fl::add_timeout(0.25, check_scenePtr);
    
    // run gui
    Fl::run();

	// TODO stop everything
	return 0;
}
