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

#ifndef LUX_GUI_H
#define LUX_GUI_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Image.H>
#include <FL/x.H>
#include <FL/filename.H>

#include "renderwindow.h"

bool GuiSceneReady = false;
float framebufferUpdate;
Fl_RGB_Image* rgb_image;
Fl_Window* window;
//Fl_Thread e_thr;

RenderWindow *renderview;
Fl_Group *info_render;
Fl_Group *info_render_group;
Fl_Group *info_tonemap;
Fl_Group *info_tonemap_group;
Fl_Group *info_statistics;
Fl_Group *info_statistics_group;

Fl_Button *button_play;
Fl_Button *button_pause;
Fl_Button *button_restart;

boost::thread *engine_thread;
int gui_nrthreads = 1;
char gui_current_scenefile[256];

#define STATUS_RENDER_NONE 0
#define STATUS_RENDER_IDLE 1
#define STATUS_RENDER_RENDER 2

int status_render = STATUS_RENDER_NONE;

// functions
void AddThread();
void RemoveThread();
void RenderStart();
void RenderPause();
int RenderScenefile();
void message_window(const char *label, const char *msg);

// callbacks
void open_cb(Fl_Widget*, void*);
void exit_cb(Fl_Widget*, void*);
void about_cb(Fl_Widget*, void*);
void addthread_cb(Fl_Widget*, void*);
void removethread_cb(Fl_Widget*, void*);
void start_cb(Fl_Widget*, void*);
void stop_cb(Fl_Widget*, void*);
void restart_cb(Fl_Widget*, void*);

#endif // LUX_GUI_H
