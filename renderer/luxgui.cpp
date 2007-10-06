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

bool GuiSceneReady = false;
float framebufferUpdate;
Fl_RGB_Image* rgb_image;
Fl_Window* window;

Fl_Thread e_thr;

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

#include "luxwindow.h"	// radiance: fltk window code temporarily in luxwindow.h (exported from fluid)

void merge_FrameBuffer(void*) {
	printf("GUI: Updating framebuffer...\n");
	Fl::lock();
	luxUpdateFramebuffer();
	printf("GUI: Done.\n");
	Fl::unlock();

    rgb_image->uncache();

    Fl::redraw();
	Fl::repeat_timeout(framebufferUpdate, merge_FrameBuffer);
}

void* Engine_Thread( void* p )
{
	printf("GUI: Initializing Parser\n");
    luxInit();

	printf("GUI: Parsing scenefile '%s'...\n", gui_current_scenefile);
	ParseFile( gui_current_scenefile );
	printf("GUI: Finished parsing scenefile.\n");

	luxCleanup();

#ifdef WIN32
	_endthread();
#else
	pthread_exit(0);
#endif
    return 0;
}

int RenderScenefile()
{
	fflush(stdout);
    //fl_create_thread((Fl_Thread&)e_thr, Engine_Thread, 0 );
    fl_create_thread(e_thr, Engine_Thread, 0 );
	return 0;
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

void check_SceneReady(void*) {
	if(luxStatistics("sceneIsReady")) {
		GuiSceneReady = true;

		// add initial render thread
		Fl::lock();
		luxAddThread();

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

	// create render window
	u_int width = 800;
	u_int height = 600;
    window = make_window( width, height, rgb_image );
	window->show();

	// set timeouts
    Fl::add_timeout(0.25, check_SceneReady);
    
    // run gui
    Fl::run();

	// TODO stop everything
	return 0;
}
