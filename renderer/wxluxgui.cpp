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

#include <boost/bind.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <boost/cast.hpp>

#include <sstream>
#include <clocale>

#include "lux.h"
#include "api.h"
#include "error.h"

#include "wx/app.h"
#include "wx/filedlg.h"
#include "wx/filename.h"
#include "wx/dcbuffer.h"
#include "wx/splash.h"
#include "wx/clipbrd.h" // CF
#include <boost/date_time/posix_time/posix_time.hpp>

#include "wxluxgui.h"
#include "wxglviewer.h"
#include "wximages.h"

using namespace lux;

/*** LuxGui ***/

//////////////////////////////////////////////////////////////////////////////////////////////////

#define FLOAT_SLIDER_RES 512.f

#define TM_REINHARD_YWA_RANGE 1.0f
#define TM_REINHARD_PRESCALE_RANGE 8.0f
#define TM_REINHARD_POSTSCALE_RANGE 8.0f
#define TM_REINHARD_BURN_RANGE 12.0f
#define TM_LINEAR_EXPOSURE_RANGE 60.0f
#define TM_LINEAR_SENSITIVITY_RANGE 10000.0f
#define TM_LINEAR_FSTOP_RANGE 64.0f
#define TM_LINEAR_GAMMA_RANGE 5.0f
#define TM_CONTRAST_YWA_RANGE 1.0f
#define TORGB_XWHITE_RANGE 1.0f
#define TORGB_YWHITE_RANGE 1.0f
#define TORGB_XRED_RANGE 1.0f
#define TORGB_YRED_RANGE 1.0f
#define TORGB_XGREEN_RANGE 1.0f
#define TORGB_YGREEN_RANGE 1.0f
#define TORGB_XBLUE_RANGE 1.0f
#define TORGB_YBLUE_RANGE 1.0f
#define TORGB_GAMMA_RANGE 5.0f
#define LG_SCALE_RANGE 100.f
#define LG_TEMPERATURE_RANGE 20000.f

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(lux::wxEVT_LUX_ERROR)
#if defined(__WXOSX_COCOA__) // wx 2.9 and later //
wxDEFINE_EVENT(EVT_LUX_PARSEERROR, wxCommandEvent);
wxDEFINE_EVENT(EVT_LUX_FINISHED, wxCommandEvent);
wxDEFINE_EVENT(EVT_LUX_TONEMAPPED, wxCommandEvent);
#else
DEFINE_EVENT_TYPE(lux::wxEVT_LUX_PARSEERROR)
DEFINE_EVENT_TYPE(lux::wxEVT_LUX_FINISHED)
DEFINE_EVENT_TYPE(lux::wxEVT_LUX_TONEMAPPED)
#endif

BEGIN_EVENT_TABLE(LuxGui, wxFrame)
	EVT_LUX_ERROR (wxID_ANY, LuxGui::OnError)
	EVT_LUX_VIEWER_SELECTION (wxID_ANY, LuxGui::OnSelection)
	EVT_TIMER     (wxID_ANY, LuxGui::OnTimer)
#if defined(__WXOSX_COCOA__)
	EVT_COMMAND   (wxID_ANY, EVT_LUX_TONEMAPPED, LuxGui::OnCommand)
	EVT_COMMAND   (wxID_ANY, EVT_LUX_PARSEERROR, LuxGui::OnCommand)
	EVT_COMMAND   (wxID_ANY, EVT_LUX_FINISHED, LuxGui::OnCommand)
#else
	EVT_COMMAND   (wxID_ANY, lux::wxEVT_LUX_TONEMAPPED, LuxGui::OnCommand)
	EVT_COMMAND   (wxID_ANY, lux::wxEVT_LUX_PARSEERROR, LuxGui::OnCommand)
	EVT_COMMAND   (wxID_ANY, lux::wxEVT_LUX_FINISHED, LuxGui::OnCommand)
#endif

#if defined (__WXMSW__) ||  defined (__WXGTK__)
	EVT_ICONIZE   (LuxGui::OnIconize)
#endif
END_EVENT_TABLE()

#if defined(__WXOSX_COCOA__) // wx 2.9 and later //
#define wxEVT_LUX_TONEMAPPED EVT_LUX_TONEMAPPED
#define wxEVT_LUX_PARSEERROR EVT_LUX_PARSEERROR
#define wxEVT_LUX_FINISHED EVT_LUX_FINISHED
#endif

// Dade - global variable used by LuxGuiErrorHandler()
bool copyLog2Console = false;

LuxGui::LuxGui(wxWindow* parent, bool opengl, bool copylog2console) :
	LuxMainFrame(parent), m_opengl(opengl), m_copyLog2Console(copylog2console) {
	// Load images and icons from header.
	LoadImages();

	// Add custom output viewer window
	if(m_opengl)
		m_renderOutput = new LuxGLViewer(m_renderPage);
	else
		m_renderOutput = new LuxOutputWin(m_renderPage);

	m_renderPage->GetSizer()->GetItem( 1 )->GetSizer()->GetItem( 1 )->GetSizer()->Add( m_renderOutput->GetWindow(), 1, wxALL | wxEXPAND, 5 );
	m_renderPage->Layout();

	// Trick to generate resize event and show output window
	// http://lists.wxwidgets.org/pipermail/wx-users/2007-February/097829.html
	SetSize(GetSize());
	m_renderOutput->GetWindow()->Update();

	// Create render output update timer
	m_renderTimer = new wxTimer(this, ID_RENDERUPDATE);
	m_statsTimer = new wxTimer(this, ID_STATSUPDATE);
	m_loadTimer = new wxTimer(this, ID_LOADUPDATE);
	m_netTimer = new wxTimer(this, ID_NETUPDATE);

	m_numThreads = 0;
	m_engineThread = NULL;
	m_updateThread = NULL;

	// Dade - I should use boost bind to avoid global variable
	copyLog2Console = m_copyLog2Console;
	luxErrorHandler(&LuxGuiErrorHandler);

	ChangeRenderState(WAITING);
	m_guiWindowState = SHOWN;

	// CF
	m_LuxOptions = new LuxOptions( this );

	m_serverUpdateSpin->SetValue( luxGetNetworkServerUpdateInterval() );

	m_auinotebook->SetSelection( 0 );

	ResetToneMapping();
	m_auto_tonemap = true;

	wxTextValidator vt( wxFILTER_NUMERIC );

	m_TM_Reinhard_ywaText->SetValidator( vt );
	m_TM_Reinhard_prescaleText->SetValidator( vt );
	m_TM_Reinhard_postscaleText->SetValidator( vt );
	m_TM_Reinhard_burnText->SetValidator( vt );
	m_TM_Linear_sensitivityText->SetValidator( vt );
	m_TM_Linear_exposureText->SetValidator( vt );
	m_TM_Linear_fstopText->SetValidator( vt );
	m_TM_Linear_gammaText->SetValidator( vt );
	m_TM_contrast_ywaText->SetValidator( vt );
	m_TORGB_xwhiteText->SetValidator( vt );
	m_TORGB_ywhiteText->SetValidator( vt );
	m_TORGB_xredText->SetValidator( vt );
	m_TORGB_yredText->SetValidator( vt );
	m_TORGB_xgreenText->SetValidator( vt );
	m_TORGB_ygreenText->SetValidator( vt );
	m_TORGB_xblueText->SetValidator( vt );
	m_TORGB_yblueText->SetValidator( vt );
	m_TORGB_gammaText->SetValidator( vt );

	m_GLAcceleration = false;
}

LuxGui::~LuxGui() {
	delete m_renderTimer;
	delete m_statsTimer;
	delete m_loadTimer;
	delete m_netTimer;

	for( std::vector<LuxLightGroupPanel*>::iterator it = m_LightGroupPanels.begin(); it != m_LightGroupPanels.end(); it++) {
		LuxLightGroupPanel *currPanel = *it;
		m_LightGroupsSizer->Detach(currPanel);
		delete currPanel;
	}
	m_LightGroupPanels.clear();
}

void LuxGui::ChangeRenderState(LuxGuiRenderState state) {
	switch(state) {
		case WAITING:
			// Waiting for input file. Most controls disabled.
			m_render->Enable(ID_RESUMEITEM, false);
			m_render->Enable(ID_PAUSEITEM, false);
			m_render->Enable(ID_STOPITEM, false);
			m_view->Enable(ID_RENDER_COPY, false);
			m_renderToolBar->EnableTool(ID_RESUMETOOL, false);
			m_renderToolBar->EnableTool(ID_PAUSETOOL, false);
			m_renderToolBar->EnableTool(ID_STOPTOOL, false);
			m_renderToolBar->EnableTool(ID_RENDER_COPY, false);
			m_viewerToolBar->Disable();
			break;
		case RENDERING:
			// Rendering is in progress.
			m_render->Enable(ID_RESUMEITEM, false);
			m_render->Enable(ID_PAUSEITEM, true);
			m_render->Enable(ID_STOPITEM, true);
			m_view->Enable(ID_RENDER_COPY, true);
			m_renderToolBar->EnableTool(ID_RESUMETOOL, false);
			m_renderToolBar->EnableTool(ID_PAUSETOOL, true);
			m_renderToolBar->EnableTool(ID_STOPTOOL, true);
			m_renderToolBar->EnableTool(ID_RENDER_COPY, true);
			m_viewerToolBar->Enable();
			break;
		case STOPPING:
			// Rendering is being stopped.
			m_render->Enable(ID_RESUMEITEM, false);
			m_render->Enable(ID_PAUSEITEM, false);
			m_render->Enable(ID_STOPITEM, false);
			m_view->Enable(ID_RENDER_COPY, true);
			m_renderToolBar->EnableTool(ID_RESUMETOOL, false);
			m_renderToolBar->EnableTool(ID_PAUSETOOL, false);
			m_renderToolBar->EnableTool(ID_STOPTOOL, false);
			m_renderToolBar->EnableTool(ID_RENDER_COPY, true);
			break;
		case STOPPED:
			// Rendering is stopped.
			m_render->Enable(ID_RESUMEITEM, true);
			m_render->Enable(ID_PAUSEITEM, false);
			m_render->Enable(ID_STOPITEM, false);
			m_view->Enable(ID_RENDER_COPY, true);
			m_renderToolBar->EnableTool(ID_RESUMETOOL, true);
			m_renderToolBar->EnableTool(ID_PAUSETOOL, false);
			m_renderToolBar->EnableTool(ID_STOPTOOL, false);
			m_renderToolBar->EnableTool(ID_RENDER_COPY, true);
			break;
		case PAUSED:
			// Rendering is paused.
			m_render->Enable(ID_RESUMEITEM, true);
			m_render->Enable(ID_PAUSEITEM, false);
			m_render->Enable(ID_STOPITEM, true);
			m_view->Enable(ID_RENDER_COPY, true);
			m_renderToolBar->EnableTool(ID_RESUMETOOL, true);
			m_renderToolBar->EnableTool(ID_PAUSETOOL, false);
			m_renderToolBar->EnableTool(ID_STOPTOOL, true);
			m_renderToolBar->EnableTool(ID_RENDER_COPY, true);
			break;
		case FINISHED:
			// Rendering is finished.
			m_render->Enable(ID_RESUMEITEM, false);
			m_render->Enable(ID_PAUSEITEM, false);
			m_render->Enable(ID_STOPITEM, false);
			m_view->Enable(ID_RENDER_COPY, true);
			m_renderToolBar->EnableTool(ID_RESUMETOOL, false);
			m_renderToolBar->EnableTool(ID_PAUSETOOL, false);
			m_renderToolBar->EnableTool(ID_STOPTOOL, false);
			m_renderToolBar->EnableTool(ID_RENDER_COPY, true);
			break;
	}
	m_guiRenderState = state;
}

void LuxGui::LoadImages() {
	wxImage::AddHandler(new wxPNGHandler());

	// App icon - only set on non-windows platforms
#ifndef __WXMSW__
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxMEMORY_BITMAP(luxicon_png));
	SetIcon(appIcon);
#else
	SetIcon( wxIcon("WXDEFAULT_FRAME", wxBITMAP_TYPE_ICO_RESOURCE ));
#endif

	// wxMac has problems changing an existing tool's icon, so we remove and add then again...
	// Resume toolbar tool
	wxToolBarToolBase *rendertool = m_renderToolBar->RemoveTool(ID_RESUMETOOL);
	rendertool->SetNormalBitmap(wxMEMORY_BITMAP(resume_png));
	m_renderToolBar->InsertTool(0, rendertool);
	m_renderToolBar->Realize();

	// Pause toolbar tool
	wxToolBarToolBase *pausetool = m_renderToolBar->RemoveTool(ID_PAUSETOOL);
	pausetool->SetNormalBitmap(wxMEMORY_BITMAP(pause_png));
	m_renderToolBar->InsertTool(1, pausetool);
	m_renderToolBar->Realize();

	// Stop toolbar tool
	wxToolBarToolBase *stoptool = m_renderToolBar->RemoveTool(ID_STOPTOOL);
	stoptool->SetNormalBitmap(wxMEMORY_BITMAP(stop_png));
	m_renderToolBar->InsertTool(2, stoptool);
	m_renderToolBar->Realize();

	// Add Thread toolbar tool
	wxToolBarToolBase *addtheadtool = m_renderToolBar->RemoveTool(ID_ADD_THREAD);
	addtheadtool->SetNormalBitmap(wxMEMORY_BITMAP(plus_png));
	m_renderToolBar->InsertTool(5, addtheadtool);
	m_renderToolBar->Realize();

	// Add Thread toolbar tool
	wxToolBarToolBase *remtheadtool = m_renderToolBar->RemoveTool(ID_REMOVE_THREAD);
	remtheadtool->SetNormalBitmap(wxMEMORY_BITMAP(minus_png));
	m_renderToolBar->InsertTool(6, remtheadtool);
	m_renderToolBar->Realize();

	// Copy toolbar tool
	wxToolBarToolBase *copytool = m_renderToolBar->RemoveTool(ID_RENDER_COPY);
	copytool->SetNormalBitmap(wxMEMORY_BITMAP(edit_copy_png));
	m_renderToolBar->InsertTool(8, copytool);
	m_renderToolBar->Realize();

	///////////////////////////////////////////////////////////////////////////////

	// NOTE - Ratow - Network toolbar is now a real toolbar component
	wxToolBarToolBase *addServertool = m_networkToolBar->RemoveTool(ID_ADD_SERVER);
	addServertool->SetNormalBitmap(wxMEMORY_BITMAP(plus_png));
	m_networkToolBar->InsertTool(2, addServertool);
	wxToolBarToolBase *removeServertool = m_networkToolBar->RemoveTool(ID_REMOVE_SERVER);
	removeServertool->SetNormalBitmap(wxMEMORY_BITMAP(minus_png));
	m_networkToolBar->InsertTool(3, removeServertool);
	m_networkToolBar->Realize();

	///////////////////////////////////////////////////////////////////////////////

	// Pan toolbar tool
	wxToolBarToolBase *pantool = m_viewerToolBar->RemoveTool(ID_PANTOOL);
	pantool->SetNormalBitmap(wxMEMORY_BITMAP(pan_png));
	m_viewerToolBar->InsertTool(0, pantool);
	m_viewerToolBar->Realize();

	// Zoom toolbar tool
	wxToolBarToolBase *zoomtool = m_viewerToolBar->RemoveTool(ID_ZOOMTOOL);
	zoomtool->SetNormalBitmap(wxMEMORY_BITMAP(zoom_png));
	m_viewerToolBar->InsertTool(1, zoomtool);
	m_viewerToolBar->Realize();

	// Refine toolbar tool
	wxToolBarToolBase *refinetool = m_viewerToolBar->RemoveTool(ID_REFINETOOL);
	refinetool->SetNormalBitmap(wxMEMORY_BITMAP(radiofocus_png));
	m_viewerToolBar->InsertTool(2, refinetool);
	m_viewerToolBar->Realize();

	wxMenuItem *renderitem = m_render->Remove(ID_RESUMEITEM);
	renderitem->SetBitmap(wxMEMORY_BITMAP(resume_png));
	m_render->Insert(0,renderitem);
	// Pause menu item
	wxMenuItem *pauseitem = m_render->Remove(ID_PAUSEITEM);
	pauseitem->SetBitmap(wxMEMORY_BITMAP(pause_png));
	m_render->Insert(1,pauseitem);
	// stop menu item
	wxMenuItem *stopitem = m_render->Remove(ID_STOPITEM);
	stopitem->SetBitmap(wxMEMORY_BITMAP(stop_png));
	m_render->Insert(2,stopitem);

	m_auinotebook->SetPageBitmap(0, wxMEMORY_BITMAP(render_png));
	m_auinotebook->SetPageBitmap(1, wxMEMORY_BITMAP(info_png));
	m_auinotebook->SetPageBitmap(2, wxMEMORY_BITMAP(network_png));
	m_auinotebook->SetPageBitmap(3, wxMEMORY_BITMAP(output_png));

	m_outputNotebook->SetPageBitmap(0, wxMEMORY_BITMAP(n_lightgroup_png));
	m_outputNotebook->SetPageBitmap(1, wxMEMORY_BITMAP(n_tonemap_png));
	m_outputNotebook->SetPageBitmap(2, wxMEMORY_BITMAP(n_system_png));

	m_tonemapBitmap->SetBitmap(wxMEMORY_BITMAP(n_tonemap_png));
	m_colorspaceBitmap->SetBitmap(wxMEMORY_BITMAP(n_color_png));
	m_gammaBitmap->SetBitmap(wxMEMORY_BITMAP(n_gamma_png));

	m_splashbmp = wxMEMORY_BITMAP(splash_png);
}


void LuxGui::OnMenu(wxCommandEvent& event) {
	switch (event.GetId()) {
		case ID_RESUMEITEM:
		case ID_RESUMETOOL:
			if(m_guiRenderState != RENDERING) {
				// CF
				m_LuxOptions->UpdateSysOptions();
				UpdateNetworkTree();

				// Start display update timer
				m_renderOutput->Reload();
				m_renderTimer->Start(1000*luxStatistics("displayInterval"), wxTIMER_CONTINUOUS);
				m_statsTimer->Start(1000, wxTIMER_CONTINUOUS);
				m_netTimer->Start(1000, wxTIMER_CONTINUOUS);
				if(m_guiRenderState == PAUSED || m_guiRenderState == STOPPED) // Only re-start if we were previously stopped
					luxStart();
				if(m_guiRenderState == STOPPED)
					luxSetHaltSamplePerPixel(-1, false, false);
				ChangeRenderState(RENDERING);
			}
			break;
		case ID_PAUSEITEM:
		case ID_PAUSETOOL:
			if(m_guiRenderState != PAUSED) {
				// Stop display update timer
				m_renderTimer->Stop();
				m_statsTimer->Stop();
				if(m_guiRenderState == RENDERING)
					luxPause();
				ChangeRenderState(PAUSED);
			}
			break;
		case ID_STOPITEM:
		case ID_STOPTOOL:
			if ((m_guiRenderState == RENDERING) || (m_guiRenderState == PAUSED)) {
				// Dade - we can set the enough sample per pixel condition
				m_renderTimer->Stop();
				// Leave stats timer running so we know when threads stopped
				luxSetHaltSamplePerPixel(1, true, true);
				m_statusBar->SetStatusText(wxT("Waiting for render threads to stop."), 0);
				ChangeRenderState(STOPPING);
			}
			break;
		case ID_FULL_SCREEN: // CF
			if ( !IsFullScreen() )
			{
				m_renderToolBar->Show( false );
				m_viewerToolBar->Show( false );
				m_outputNotebook->Show( false );
				ShowFullScreen( true );
			}
			else
			{
				ShowFullScreen( false );

				if (m_view->IsChecked( ID_TOOL_BAR ) )
				{
					m_renderToolBar->Show( true );
					m_viewerToolBar->Show( true );
				}
				if (m_view->IsChecked( ID_SIDE_PANE ) )
				{
					m_outputNotebook->Show( true );
				}
			}
			Layout();
			m_renderPage->Layout();
			break;
		case ID_TOOL_BAR: // CF
			if ( m_renderToolBar->IsShown() )
			{
				m_renderToolBar->Show( false );
				m_viewerToolBar->Show( false );
			}
			else
			{
				m_renderToolBar->Show( true );
				m_viewerToolBar->Show( true );
			}
			m_renderPage->Layout();
			break;
		case ID_STATUS_BAR: // CF
			if ( m_statusBar->IsShown() )
			{
				m_statusBar->Show( false );
			}
			else
			{
				m_statusBar->Show( true );
			}
			Layout();
			break;
		case ID_SIDE_PANE: // CF
			if ( m_outputNotebook->IsShown() )
			{
				m_outputNotebook->Show( false );
				m_view->Check( ID_SIDE_PANE, false );
			}
			else
			{
				m_outputNotebook->Show( true );
				m_view->Check( ID_SIDE_PANE, true );
			}
			m_renderPage->Layout();
			break;
		case ID_RENDER_COPY: // CF
			if ( m_guiRenderState != WAITING &&
				 wxTheClipboard->Open() )
			{
				m_statusBar->SetStatusText(wxT("Copying..."), 0);

				wxTheClipboard->SetData( new wxBitmapDataObject( *(new wxBitmap( *(new wxImage( luxStatistics("filmXres"), luxStatistics("filmYres"), luxFramebuffer()))))));
				wxTheClipboard->Close();

				m_statusBar->SetStatusText(wxT(""), 0);
			}
			break;
		case ID_CLEAR_LOG: // CF
			m_logTextCtrl->Clear();
			break;
		case ID_OPTIONS: // CF
			if ( m_LuxOptions->IsShown() )
			{
				m_LuxOptions->Show( false );
			}
			else
			{
				m_LuxOptions->Show( true );
			}
			break;
		case ID_ADD_THREAD: // CF
			if ( m_numThreads < 16 ) SetRenderThreads( m_numThreads + 1 );
			break;
		case ID_REMOVE_THREAD: // CF
			if ( m_numThreads > 1 ) SetRenderThreads( m_numThreads - 1 );
			break;
		case ID_ADD_SERVER: // CF
			AddServer();
			break;
		case ID_REMOVE_SERVER: // CF
			RemoveServer();
			break;
		case ID_TM_RESET:
			ResetToneMapping();
			break;
		case wxID_ABOUT:
			new wxSplashScreen(m_splashbmp, wxSPLASH_CENTRE_ON_PARENT, 0, this, -1);
			break;
		case wxID_EXIT:
			Close(false);
			break;
		case ID_PAN_MODE: // CF
			m_viewerToolBar->ToggleTool( ID_PANTOOL, true );
			m_renderOutput->SetMode(PANZOOM);
			break;
		case ID_PANTOOL:
			m_renderOutput->SetMode(PANZOOM);
			break;
		case ID_ZOOM_MODE: // CF
			m_viewerToolBar->ToggleTool( ID_ZOOMTOOL, true );
			m_renderOutput->SetMode(SELECTION);
			break;
		case ID_ZOOMTOOL:
		case ID_REFINETOOL:
			m_renderOutput->SetMode(SELECTION);
			break;
		case ID_TM_APPLY:
			{
				// Apply tonemapping
				ApplyTonemapping();
			}
			break;
		case ID_TM_KERNELCHOICE:
			{
				// Change tonemapping kernel
				int choice = event.GetInt();
				SetTonemapKernel(choice);
			}
			break;
		case ID_TORGB_COLORSPACECHOICE:
			{
				// Change colorspace preset
				int choice = event.GetInt();
				SetColorSpacePreset(choice);
			}
			break;
		default:
			break;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////

void LuxGui::ApplyTonemapping() {
	if(m_updateThread == NULL && luxStatistics("sceneIsReady") &&
    (m_guiWindowState == SHOWN || m_guiRenderState == FINISHED)) {
		luxError(LUX_NOERROR, LUX_INFO, "GUI: Updating framebuffer...");
		m_statusBar->SetStatusText(wxT("Tonemapping..."), 0);
		m_updateThread = new boost::thread(boost::bind(&LuxGui::UpdateThread, this));
	}
}

void UpdateParam(luxComponent comp, luxComponentParameters param, double value, int index = 0) {
	if(luxStatistics("sceneIsReady")) {
	// Update OpenGL viewer
	// m_renderOutput->SetComponentParameter(comp, param, value);
	// Update lux's film
	luxSetParameterValue(comp, param, value, index);
	}
}

void UpdateParam(luxComponent comp, luxComponentParameters param, const char* value, int index = 0) {
	if(luxStatistics("sceneIsReady")) {
	// Update OpenGL viewer
	// m_renderOutput->SetComponentParameter(comp, param, value);
	// Update lux's film
	luxSetStringParameterValue(comp, param, value, index);
	}
}

void LuxGui::OnText(wxCommandEvent& event) {
	if ( event.GetEventType() != wxEVT_COMMAND_TEXT_ENTER ) return;

	switch (event.GetId()) {
		// Reinhard tonemapper options
		case ID_TM_REINHARD_PRESCALE_TEXT:
			if( m_TM_Reinhard_prescaleText->IsModified() )
			{
				wxString st = m_TM_Reinhard_prescaleText->GetValue();
				st.ToDouble( &m_TM_reinhard_prescale );

				if ( m_TM_reinhard_prescale > TM_REINHARD_PRESCALE_RANGE ) m_TM_reinhard_prescale = TM_REINHARD_PRESCALE_RANGE;
				else if ( m_TM_reinhard_prescale < 0.f ) m_TM_reinhard_prescale = 0.f;

				st = wxString::Format( _("%.02f"), m_TM_reinhard_prescale );
				m_TM_Reinhard_prescaleText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_REINHARD_PRESCALE_RANGE ) * (m_TM_reinhard_prescale));
				m_TM_Reinhard_prescaleSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_REINHARD_PRESCALE, m_TM_reinhard_prescale);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_REINHARD_POSTSCALE_TEXT:
			if ( m_TM_Reinhard_postscaleText->IsModified() )
			{
				wxString st = m_TM_Reinhard_postscaleText->GetValue();
				st.ToDouble( &m_TM_reinhard_postscale );

				if ( m_TM_reinhard_postscale > TM_REINHARD_POSTSCALE_RANGE ) m_TM_reinhard_postscale = TM_REINHARD_POSTSCALE_RANGE;
				else if ( m_TM_reinhard_postscale < 0.f ) m_TM_reinhard_postscale = 0.f;

				st = wxString::Format( _("%.02f"), m_TM_reinhard_postscale );
				m_TM_Reinhard_postscaleText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_REINHARD_POSTSCALE_RANGE ) * (m_TM_reinhard_postscale));
				m_TM_Reinhard_postscaleSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_REINHARD_POSTSCALE, m_TM_reinhard_postscale);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_REINHARD_BURN_TEXT:
			if ( m_TM_Reinhard_burnText->IsModified() )
			{
				wxString st = m_TM_Reinhard_burnText->GetValue();
				st.ToDouble( &m_TM_reinhard_burn );

				if ( m_TM_reinhard_burn > TM_REINHARD_BURN_RANGE ) m_TM_reinhard_burn = TM_REINHARD_BURN_RANGE;
				else if ( m_TM_reinhard_burn < 0 ) m_TM_reinhard_burn = 0;

				st = wxString::Format( _("%.02f"), m_TM_reinhard_burn );
				m_TM_Reinhard_burnText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_REINHARD_BURN_RANGE ) * (m_TM_reinhard_burn));
				m_TM_Reinhard_burnSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_REINHARD_BURN, m_TM_reinhard_burn);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;

		// Linear tonemapper options
		case ID_TM_LINEAR_SENSITIVITY_TEXT:
			if ( m_TM_Linear_sensitivityText->IsModified() )
			{
				wxString st = m_TM_Linear_sensitivityText->GetValue();
				st.ToDouble( &m_TM_linear_sensitivity );

				if ( m_TM_linear_sensitivity > TM_LINEAR_SENSITIVITY_RANGE ) m_TM_linear_sensitivity = TM_LINEAR_SENSITIVITY_RANGE;
				else if ( m_TM_linear_sensitivity < 0 ) m_TM_linear_sensitivity = 0;

				st = wxString::Format( _("%.02f"), m_TM_linear_sensitivity );
				m_TM_Linear_sensitivityText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_LINEAR_SENSITIVITY_RANGE ) * (m_TM_linear_sensitivity));
				m_TM_Linear_sensitivitySlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_SENSITIVITY, m_TM_linear_sensitivity);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_LINEAR_EXPOSURE_TEXT:
			if ( m_TM_Linear_exposureText->IsModified() )
			{
				wxString st = m_TM_Linear_exposureText->GetValue();
				st.ToDouble( &m_TM_linear_exposure );

				if ( m_TM_linear_exposure > TM_LINEAR_EXPOSURE_RANGE ) m_TM_linear_exposure = TM_LINEAR_EXPOSURE_RANGE;
				else if ( m_TM_linear_exposure < 0 ) m_TM_linear_exposure = 0;

				st = wxString::Format( _("%.02f"), m_TM_linear_exposure );
				m_TM_Linear_exposureText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_LINEAR_EXPOSURE_RANGE ) * (m_TM_linear_exposure));
				m_TM_Linear_exposureSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_EXPOSURE, m_TM_linear_exposure);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_LINEAR_FSTOP_TEXT:
			if ( m_TM_Linear_fstopText->IsModified() )
			{
				wxString st = m_TM_Linear_fstopText->GetValue();
				st.ToDouble( &m_TM_linear_fstop );

				if ( m_TM_linear_fstop > TM_LINEAR_FSTOP_RANGE ) m_TM_linear_fstop = TM_LINEAR_FSTOP_RANGE;
				else if ( m_TM_linear_fstop < 0 ) m_TM_linear_fstop = 0;

				st = wxString::Format( _("%.02f"), m_TM_linear_fstop );
				m_TM_Linear_fstopText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_LINEAR_FSTOP_RANGE ) * (m_TM_linear_fstop));
				m_TM_Linear_fstopSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_FSTOP, m_TM_linear_fstop);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_LINEAR_GAMMA_TEXT:
			if ( m_TM_Linear_gammaText->IsModified() )
			{
				wxString st = m_TM_Linear_gammaText->GetValue();
				st.ToDouble( &m_TM_linear_gamma );

				if ( m_TM_linear_gamma > TM_LINEAR_GAMMA_RANGE ) m_TM_linear_gamma = TM_LINEAR_GAMMA_RANGE;
				else if ( m_TM_linear_gamma < 0 ) m_TM_linear_gamma = 0;

				st = wxString::Format( _("%.02f"), m_TM_linear_gamma );
				m_TM_Linear_gammaText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_LINEAR_GAMMA_RANGE ) * (m_TM_linear_gamma));
				m_TM_Linear_gammaSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_GAMMA, m_TM_linear_gamma);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;

		// Contrast tonemapper options
		case ID_TM_CONTRAST_YWA_TEXT:
			if ( m_TM_contrast_ywaText->IsModified() )
			{
				wxString st = m_TM_contrast_ywaText->GetValue();
				st.ToDouble( &m_TM_contrast_ywa );

				if ( m_TM_contrast_ywa > TM_CONTRAST_YWA_RANGE ) m_TM_contrast_ywa = TM_CONTRAST_YWA_RANGE;
				else if ( m_TM_contrast_ywa < 0 ) m_TM_contrast_ywa = 0;

				st = wxString::Format( _("%.02f"), m_TM_contrast_ywa );
				m_TM_contrast_ywaText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TM_CONTRAST_YWA_RANGE ) * (m_TM_contrast_ywa));
				m_TM_contrast_ywaSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TM_CONTRAST_YWA, m_TM_contrast_ywa);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;


		// Colorspace options
		case ID_TORGB_XWHITE_TEXT:
			if ( m_TORGB_xwhiteText->IsModified() )
			{
				wxString st = m_TORGB_xwhiteText->GetValue();
				st.ToDouble( &m_TORGB_xwhite );

				if ( m_TORGB_xwhite > TORGB_XWHITE_RANGE ) m_TORGB_xwhite = TORGB_XWHITE_RANGE;
				else if ( m_TORGB_xwhite < 0 ) m_TORGB_xwhite = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_xwhite );
				m_TORGB_xwhiteText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE ) * (m_TORGB_xwhite));
				m_TORGB_xwhiteSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YWHITE_TEXT:
			if ( m_TORGB_ywhiteText->IsModified() )
			{
				wxString st = m_TORGB_ywhiteText->GetValue();
				st.ToDouble( &m_TORGB_ywhite );

				if ( m_TORGB_ywhite > TORGB_YWHITE_RANGE ) m_TORGB_ywhite = TORGB_YWHITE_RANGE;
				else if ( m_TORGB_ywhite < 0 ) m_TORGB_ywhite = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_ywhite );
				m_TORGB_ywhiteText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE ) * (m_TORGB_ywhite));
				m_TORGB_ywhiteSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_XRED_TEXT:
			if ( m_TORGB_xredText->IsModified() )
			{
				wxString st = m_TORGB_xredText->GetValue();
				st.ToDouble( &m_TORGB_xred );

				if ( m_TORGB_xred > TORGB_XRED_RANGE ) m_TORGB_xred = TORGB_XRED_RANGE;
				else if ( m_TORGB_xred < 0 ) m_TORGB_xred = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_xred );
				m_TORGB_xredText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_XRED_RANGE ) * (m_TORGB_xred));
				m_TORGB_xredSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YRED_TEXT:
			if ( m_TORGB_yredText->IsModified() )
			{
				wxString st = m_TORGB_yredText->GetValue();
				st.ToDouble( &m_TORGB_yred );

				if ( m_TORGB_yred > TORGB_YRED_RANGE ) m_TORGB_yred = TORGB_YRED_RANGE;
				else if ( m_TORGB_yred < 0 ) m_TORGB_yred = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_yred );
				m_TORGB_yredText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_YRED_RANGE ) * (m_TORGB_yred));
				m_TORGB_yredSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_XGREEN_TEXT:
			if ( m_TORGB_xgreenText->IsModified() )
			{
				wxString st = m_TORGB_xgreenText->GetValue();
				st.ToDouble( &m_TORGB_xgreen );

				if ( m_TORGB_xgreen > TORGB_XGREEN_RANGE ) m_TORGB_xgreen = TORGB_XGREEN_RANGE;
				else if ( m_TORGB_xgreen < 0 ) m_TORGB_xgreen = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_xgreen );
				m_TORGB_xgreenText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE ) * (m_TORGB_xgreen));
				m_TORGB_xgreenSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YGREEN_TEXT:
			if ( m_TORGB_ygreenText->IsModified() )
			{
				wxString st = m_TORGB_ygreenText->GetValue();
				st.ToDouble( &m_TORGB_ygreen );

				if ( m_TORGB_ygreen > TORGB_YGREEN_RANGE ) m_TORGB_ygreen = TORGB_YGREEN_RANGE;
				else if ( m_TORGB_ygreen < 0 ) m_TORGB_ygreen = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_ygreen );
				m_TORGB_ygreenText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE ) * (m_TORGB_ygreen));
				m_TORGB_ygreenSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_XBLUE_TEXT:
			if ( m_TORGB_xblueText->IsModified() )
			{
				wxString st = m_TORGB_xblueText->GetValue();
				st.ToDouble( &m_TORGB_xblue );

				if ( m_TORGB_xblue > TORGB_XBLUE_RANGE ) m_TORGB_xblue = TORGB_XBLUE_RANGE;
				else if ( m_TORGB_xblue < 0 ) m_TORGB_xblue = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_xblue );
				m_TORGB_xblueText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE ) * (m_TORGB_xblue));
				m_TORGB_xblueSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YBLUE_TEXT:
			if ( m_TORGB_yblueText->IsModified() )
			{
				wxString st = m_TORGB_yblueText->GetValue();
				st.ToDouble( &m_TORGB_yblue );

				if ( m_TORGB_yblue > TORGB_YBLUE_RANGE ) m_TORGB_yblue = TORGB_YBLUE_RANGE;
				else if ( m_TORGB_yblue < 0 ) m_TORGB_yblue = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_yblue );
				m_TORGB_yblueText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE ) * (m_TORGB_yblue));
				m_TORGB_yblueSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_GAMMA_TEXT:
			if ( m_TORGB_gammaText->IsModified() )
			{
				wxString st = m_TORGB_gammaText->GetValue();
				st.ToDouble( &m_TORGB_gamma );

				if ( m_TORGB_gamma > TORGB_GAMMA_RANGE ) m_TORGB_gamma = TORGB_GAMMA_RANGE;
				else if ( m_TORGB_gamma < 0 ) m_TORGB_gamma = 0;

				st = wxString::Format( _("%.02f"), m_TORGB_gamma );
				m_TORGB_gammaText->SetValue( st );
				int val = (int)(( FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE ) * (m_TORGB_gamma));
				m_TORGB_gammaSlider->SetValue( val );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		default:
			break;
	}
}

void LuxGui::OnCheckBox(wxCommandEvent& event)
{
}

void LuxGui::OnColourChanged(wxColourPickerEvent &event)
{
}

void LuxGui::OnScroll( wxScrollEvent& event ){
	switch (event.GetId()) {
		// Reinhard tonemapper options
		case ID_TM_REINHARD_PRESCALE:
			{
				m_TM_reinhard_prescale = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_REINHARD_PRESCALE_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_reinhard_prescale );
				m_TM_Reinhard_prescaleText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_REINHARD_PRESCALE, m_TM_reinhard_prescale);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_REINHARD_POSTSCALE:
			{
				m_TM_reinhard_postscale = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_REINHARD_POSTSCALE_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_reinhard_postscale );
				m_TM_Reinhard_postscaleText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_REINHARD_POSTSCALE, m_TM_reinhard_postscale);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_REINHARD_BURN:
			{
				m_TM_reinhard_burn = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_REINHARD_BURN_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_reinhard_burn );
				m_TM_Reinhard_burnText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_REINHARD_BURN, m_TM_reinhard_burn);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;

		// Linear tonemapper options
		case ID_TM_LINEAR_SENSITIVITY:
			{
				m_TM_linear_sensitivity = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_LINEAR_SENSITIVITY_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_linear_sensitivity );
				m_TM_Linear_sensitivityText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_SENSITIVITY, m_TM_linear_sensitivity);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_LINEAR_EXPOSURE:
			{
				m_TM_linear_exposure = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_LINEAR_EXPOSURE_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_linear_exposure );
				m_TM_Linear_exposureText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_EXPOSURE, m_TM_linear_exposure);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_LINEAR_FSTOP:
			{
				m_TM_linear_fstop = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_LINEAR_FSTOP_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_linear_fstop );
				m_TM_Linear_fstopText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_FSTOP, m_TM_linear_fstop);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TM_LINEAR_GAMMA:
			{
				m_TM_linear_gamma = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_LINEAR_GAMMA_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_linear_gamma );
				m_TM_Linear_gammaText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_LINEAR_GAMMA, m_TM_linear_gamma);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;

		// Contrast tonemapper options
		case ID_TM_CONTRAST_YWA:
			{
				m_TM_contrast_ywa = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TM_CONTRAST_YWA_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TM_contrast_ywa );
				m_TM_contrast_ywaText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TM_CONTRAST_YWA, m_TM_contrast_ywa);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;

		// Colorspace options
		case ID_TORGB_XWHITE:
			{
				m_TORGB_xwhite = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_xwhite );
				m_TORGB_xwhiteText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YWHITE:
			{
				m_TORGB_ywhite = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_ywhite );
				m_TORGB_ywhiteText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_XRED:
			{
				m_TORGB_xred = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_XRED_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_xred );
				m_TORGB_xredText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YRED:
			{
				m_TORGB_yred = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_YRED_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_yred );
				m_TORGB_yredText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_XGREEN:
			{
				m_TORGB_xgreen = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_xgreen );
				m_TORGB_xgreenText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YGREEN:
			{
				m_TORGB_ygreen = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_ygreen );
				m_TORGB_ygreenText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_XBLUE:
			{
				m_TORGB_xblue = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_xblue );
				m_TORGB_xblueText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_YBLUE:
			{
				m_TORGB_yblue = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_yblue );
				m_TORGB_yblueText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		case ID_TORGB_GAMMA:
			{
				m_TORGB_gamma = (double)event.GetPosition() / ( FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE );
				wxString st = wxString::Format( _("%.02f"), m_TORGB_gamma );
				m_TORGB_gammaText->SetValue( st );
				UpdateParam(LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);
				if(m_auto_tonemap) ApplyTonemapping();
			}
			break;
		default:
			break;
	}
}

void LuxGui::OnFocus( wxFocusEvent& event ){
	//set text control's contents to current value when losing focus
	if( event.GetEventType() != wxEVT_KILL_FOCUS ) return;

	switch (event.GetId()) {
		// Reinhard tonemapper options
		case ID_TM_REINHARD_PRESCALE_TEXT:
			m_TM_Reinhard_prescaleText->SetValue( wxString::Format( _("%.02f"), m_TM_reinhard_prescale ) );
			break;
		case ID_TM_REINHARD_POSTSCALE_TEXT:
			m_TM_Reinhard_postscaleText->SetValue( wxString::Format( _("%.02f"), m_TM_reinhard_postscale ) );
			break;
		case ID_TM_REINHARD_BURN_TEXT:
			m_TM_Reinhard_burnText->SetValue( wxString::Format( _("%.02f"), m_TM_reinhard_burn ) );
			break;
		// Linear tonemapper options
		case ID_TM_LINEAR_SENSITIVITY_TEXT:
			m_TM_Linear_sensitivityText->SetValue( wxString::Format( _("%.02f"), m_TM_linear_sensitivity ) );
			break;
		case ID_TM_LINEAR_EXPOSURE_TEXT:
			m_TM_Linear_exposureText->SetValue( wxString::Format( _("%.02f"), m_TM_linear_exposure ) );
			break;
		case ID_TM_LINEAR_FSTOP_TEXT:
			m_TM_Linear_fstopText->SetValue( wxString::Format( _("%.02f"), m_TM_linear_fstop ) );
			break;
		case ID_TM_LINEAR_GAMMA_TEXT:
			m_TM_Linear_gammaText->SetValue( wxString::Format( _("%.02f"), m_TM_linear_gamma ) );
			break;
		// Contrast tonemapper options
		case ID_TM_CONTRAST_YWA_TEXT:
			m_TM_contrast_ywaText->SetValue( wxString::Format( _("%.02f"), m_TM_contrast_ywa ) );
			break;
		// Colorspace options
		case ID_TORGB_XWHITE_TEXT:
			m_TORGB_xwhiteText->SetValue( wxString::Format( _("%.02f"), m_TORGB_xwhite ) );
			break;
		case ID_TORGB_YWHITE_TEXT:
			m_TORGB_ywhiteText->SetValue( wxString::Format( _("%.02f"), m_TORGB_ywhite ) );
			break;
		case ID_TORGB_XRED_TEXT:
			m_TORGB_xredText->SetValue( wxString::Format( _("%.02f"), m_TORGB_xred ) );
			break;
		case ID_TORGB_YRED_TEXT:
			m_TORGB_yredText->SetValue( wxString::Format( _("%.02f"), m_TORGB_yred ) );
			break;
		case ID_TORGB_XGREEN_TEXT:
			m_TORGB_xgreenText->SetValue( wxString::Format( _("%.02f"), m_TORGB_xgreen ) );
			break;
		case ID_TORGB_YGREEN_TEXT:
			m_TORGB_ygreenText->SetValue( wxString::Format( _("%.02f"), m_TORGB_ygreen ) );
			break;
		case ID_TORGB_XBLUE_TEXT:
			m_TORGB_xblueText->SetValue( wxString::Format( _("%.02f"), m_TORGB_xblue ) );
			break;
		case ID_TORGB_YBLUE_TEXT:
			m_TORGB_yblueText->SetValue( wxString::Format( _("%.02f"), m_TORGB_yblue ) );
			break;
		case ID_TORGB_GAMMA_TEXT:
			m_TORGB_gammaText->SetValue( wxString::Format( _("%.02f"), m_TORGB_gamma ) );
			break;
		default:
			break;
	}

}

void LuxGui::SetTonemapKernel(int choice) {
	m_TM_kernelChoice->SetSelection(choice);
	UpdateParam(LUX_FILM, LUX_FILM_TM_TONEMAPKERNEL, choice);
	m_TM_kernel = choice;
	switch (choice) {
		case 0: {
				// Reinhard
				m_TonemapReinhardOptionsPanel->Show();
				m_TonemapLinearOptionsPanel->Hide();
				m_TonemapContrastOptionsPanel->Hide(); }
			break;
		case 1: {
				// Linear
				m_TonemapReinhardOptionsPanel->Hide();
				m_TonemapLinearOptionsPanel->Show();
				m_TonemapContrastOptionsPanel->Hide(); }
			break;
		case 2: {
				// Contrast
				m_TonemapReinhardOptionsPanel->Hide();
				m_TonemapLinearOptionsPanel->Hide();
				m_TonemapContrastOptionsPanel->Show(); }
			break;
		case 3: {
				// MaxWhite
				m_TonemapReinhardOptionsPanel->Hide();
				m_TonemapLinearOptionsPanel->Hide();
				m_TonemapContrastOptionsPanel->Hide(); }
			break;
		default:
			break;
	}
	m_Tonemap->GetSizer()->Layout();
	Refresh();
	if(m_auto_tonemap) ApplyTonemapping();
}

void LuxGui::SetColorSpacePreset(int choice) {
	m_TORGB_colorspaceChoice->SetSelection(choice);
	switch (choice) {
		case 0: {
				// sRGB - HDTV (ITU-R BT.709-5)
				m_TORGB_xwhite = 0.314275f; m_TORGB_ywhite = 0.329411f;
				m_TORGB_xred = 0.63f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.31f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 1: {
				// ROMM RGB
				m_TORGB_xwhite = 0.346f; m_TORGB_ywhite = 0.359f;
				m_TORGB_xred = 0.7347f; m_TORGB_yred = 0.2653f;
				m_TORGB_xgreen = 0.1596f; m_TORGB_ygreen = 0.8404f;
				m_TORGB_xblue = 0.0366f; m_TORGB_yblue = 0.0001f; }
			break;
		case 2: {
				// Adobe RGB 98
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.64f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.21f; m_TORGB_ygreen = 0.71f;
				m_TORGB_xblue = 0.15f; m_TORGB_yblue = 0.06f; }
			break;
		case 3: {
				// Apple RGB
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.625f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.28f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 4: {
				// NTSC (FCC 1953, ITU-R BT.470-2 System M)
				m_TORGB_xwhite = 0.310f; m_TORGB_ywhite = 0.316f;
				m_TORGB_xred = 0.67f; m_TORGB_yred = 0.33f;
				m_TORGB_xgreen = 0.21f; m_TORGB_ygreen = 0.71f;
				m_TORGB_xblue = 0.14f; m_TORGB_yblue = 0.08f; }
			break;
		case 5: {
				// NTSC (1979) (SMPTE C, SMPTE-RP 145)
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.63f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.31f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 6: {
				// PAL/SECAM (EBU 3213, ITU-R BT.470-6)
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.64f; m_TORGB_yred = 0.33f;
				m_TORGB_xgreen = 0.29f; m_TORGB_ygreen = 0.60f;
				m_TORGB_xblue = 0.15f; m_TORGB_yblue = 0.06f; }
			break;
		case 7: {
				// CIE (1931) E
				m_TORGB_xwhite = 0.333f; m_TORGB_ywhite = 0.333f;
				m_TORGB_xred = 0.7347f; m_TORGB_yred = 0.2653f;
				m_TORGB_xgreen = 0.2738f; m_TORGB_ygreen = 0.7174f;
				m_TORGB_xblue = 0.1666f; m_TORGB_yblue = 0.0089f; }
			break;
		default:
			break;
	}

	// Update values in film trough API
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
	UpdateParam(LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);

	UpdateTonemapWidgetValues();
	Refresh();
	if(m_auto_tonemap) ApplyTonemapping();
}

LuxGui::LuxOptions::LuxOptions( LuxGui *parent ) : m_OptionsDialog( parent ) {
	m_Parent = parent;
	UpdateSysOptions();
}

void LuxGui::LuxOptions::UpdateSysOptions( void ) {
	m_DisplayInterval = luxStatistics("displayInterval");
	m_Display_spinCtrl->SetValue( m_DisplayInterval );
	m_WriteInterval = 120;
	m_UseFlm = false;
	m_Write_TGA = false;
	m_Write_TM_EXR = false;
	m_Write_UTM_EXR = false;
	m_Write_TM_IGI = false;
	m_Write_UTM_IGI = false;
}

void LuxGui::LuxOptions::ApplySysOptions( void ){
// TODO
}

void LuxGui::LuxOptions::OnClose( wxCloseEvent& event ){

	Show( false );
	m_Parent->m_view->Check( ID_OPTIONS, false );
}

void LuxGui::ResetToneMapping(){
	if(luxStatistics("sceneIsReady")) {
		ResetToneMappingFromFilm();
		return;
	}

	m_TM_kernel = 0; // *

	m_TM_reinhard_autoywa = true; // *
	m_TM_reinhard_ywa = 1.0;
	m_TM_reinhard_prescale = 1.0;
	m_TM_reinhard_postscale = 1.0;
	m_TM_reinhard_burn = 6.0;

	m_TM_linear_exposure = 50.0f;
	m_TM_linear_sensitivity = 1.f / 1000.f;
	m_TM_linear_fstop = 2.8;
	m_TM_linear_gamma = 1.0;

	m_TM_contrast_ywa = 1.0;

	m_TORGB_xwhite = 0.314275f;
	m_TORGB_ywhite = 0.329411f;
	m_TORGB_xred = 0.63f;
	m_TORGB_yred = 0.34f;
	m_TORGB_xgreen = 0.31f;
	m_TORGB_ygreen = 0.595f;
	m_TORGB_xblue = 0.155f;
	m_TORGB_yblue = 0.07f;
	m_TORGB_gamma = 2.2f;

	ResetLightGroups();

	UpdateTonemapWidgetValues();
	m_outputNotebook->Enable( false );
	Refresh();
}

void LuxGui::UpdatedTonemapParam() {
	if(m_auto_tonemap) ApplyTonemapping();
}

void LuxGui::UpdateTonemapWidgetValues() {
	// Tonemap kernel selection
	SetTonemapKernel(m_TM_kernel);

	// Reinhard widgets
	m_TM_Reinhard_ywaSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_REINHARD_YWA_RANGE) * m_TM_reinhard_ywa) );
	wxString st = wxString::Format( _("%.02f"), m_TM_reinhard_ywa );
	m_TM_Reinhard_ywaText->SetValue(st);

	m_TM_Reinhard_prescaleSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_REINHARD_PRESCALE_RANGE) * m_TM_reinhard_prescale) );
	st = wxString::Format( _("%.02f"), m_TM_reinhard_prescale );
	m_TM_Reinhard_prescaleText->SetValue(st);

	m_TM_Reinhard_postscaleSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_REINHARD_POSTSCALE_RANGE) * (m_TM_reinhard_postscale)));
	st = wxString::Format( _("%.02f"), m_TM_reinhard_postscale );
	m_TM_Reinhard_postscaleText->SetValue(st);

	m_TM_Reinhard_burnSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_REINHARD_BURN_RANGE) * m_TM_reinhard_burn));
	st = wxString::Format( _("%.02f"), m_TM_reinhard_burn );
	m_TM_Reinhard_burnText->SetValue(st);

	// Linear widgets
	m_TM_Linear_exposureSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_LINEAR_EXPOSURE_RANGE) * m_TM_linear_exposure) );
	st = wxString::Format( _("%.02f"), m_TM_linear_exposure );
	m_TM_Linear_exposureText->SetValue(st);

	m_TM_Linear_sensitivitySlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_LINEAR_SENSITIVITY_RANGE) * m_TM_linear_sensitivity) );
	st = wxString::Format( _("%.02f"), m_TM_linear_sensitivity );
	m_TM_Linear_sensitivityText->SetValue(st);

	m_TM_Linear_fstopSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_LINEAR_FSTOP_RANGE) * (m_TM_linear_fstop)));
	st = wxString::Format( _("%.02f"), m_TM_linear_fstop );
	m_TM_Linear_fstopText->SetValue(st);

	m_TM_Linear_gammaSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_LINEAR_GAMMA_RANGE) * m_TM_linear_gamma));
	st = wxString::Format( _("%.02f"), m_TM_linear_gamma );
	m_TM_Linear_gammaText->SetValue(st);

	// Contrast widgets
	m_TM_contrast_ywaSlider->SetValue( (int)((FLOAT_SLIDER_RES / TM_CONTRAST_YWA_RANGE) * m_TM_contrast_ywa) );
	st = wxString::Format( _("%.02f"), m_TM_contrast_ywa );
	m_TM_contrast_ywaText->SetValue(st);

	// Colorspace widgets
	m_TORGB_xwhiteSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE) * m_TORGB_xwhite));
	st = wxString::Format( _("%.02f"), m_TORGB_xwhite );
	m_TORGB_xwhiteText->SetValue(st);
	m_TORGB_ywhiteSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE) * m_TORGB_ywhite));
	st = wxString::Format( _("%.02f"), m_TORGB_ywhite );
	m_TORGB_ywhiteText->SetValue(st);

	m_TORGB_xredSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_XRED_RANGE) * m_TORGB_xred));
	st = wxString::Format( _("%.02f"), m_TORGB_xred );
	m_TORGB_xredText->SetValue(st);
	m_TORGB_yredSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_YRED_RANGE) * m_TORGB_yred));
	st = wxString::Format( _("%.02f"), m_TORGB_yred );
	m_TORGB_yredText->SetValue(st);

	m_TORGB_xgreenSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE) * m_TORGB_xgreen));
	st = wxString::Format( _("%.02f"), m_TORGB_xgreen );
	m_TORGB_xgreenText->SetValue(st);
	m_TORGB_ygreenSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE) * m_TORGB_ygreen));
	st = wxString::Format( _("%.02f"), m_TORGB_ygreen );
	m_TORGB_ygreenText->SetValue(st);

	m_TORGB_xblueSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE) * m_TORGB_xblue));
	st = wxString::Format( _("%.02f"), m_TORGB_xblue );
	m_TORGB_xblueText->SetValue(st);
	m_TORGB_yblueSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE) * m_TORGB_yblue));
	st = wxString::Format( _("%.02f"), m_TORGB_yblue );
	m_TORGB_yblueText->SetValue(st);

	m_TORGB_gammaSlider->SetValue( (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * m_TORGB_gamma));
	st = wxString::Format( _("%.02f"), m_TORGB_gamma );
	m_TORGB_gammaText->SetValue(st);

	// Light groups widgets
	UpdateLightGroupWidgetValues();

	Refresh();
}

void LuxGui::ResetToneMappingFromFilm(){
	m_TM_kernel = (int) luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_TONEMAPKERNEL);

	m_TM_reinhard_autoywa = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_AUTOYWA) != 0.0;
	m_TM_reinhard_ywa = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_YWA);
	m_TM_reinhard_prescale = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_PRESCALE);
	m_TM_reinhard_postscale = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_POSTSCALE);
	m_TM_reinhard_burn = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_BURN);

	m_TM_linear_exposure = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_SENSITIVITY);
	m_TM_linear_sensitivity = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_EXPOSURE);
	m_TM_linear_fstop = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_FSTOP);
	m_TM_linear_gamma = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_GAMMA);

	m_TM_contrast_ywa = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TM_CONTRAST_YWA);

	m_TORGB_xwhite = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_X_WHITE);
	m_TORGB_ywhite = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_WHITE);
	m_TORGB_xred = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_X_RED);
	m_TORGB_yred = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_RED);
	m_TORGB_xgreen = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_X_GREEN);
	m_TORGB_ygreen = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_GREEN);
	m_TORGB_xblue = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_X_BLUE);
	m_TORGB_yblue = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_BLUE);
	m_TORGB_gamma = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_TORGB_GAMMA);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_TONEMAPKERNEL, m_TM_kernel);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_AUTOYWA, m_TM_reinhard_autoywa);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_YWA, m_TM_reinhard_ywa);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_PRESCALE, m_TM_reinhard_prescale);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_POSTSCALE, m_TM_reinhard_postscale);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_BURN, m_TM_reinhard_burn);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_SENSITIVITY, m_TM_linear_exposure);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_EXPOSURE, m_TM_linear_sensitivity);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_FSTOP, m_TM_linear_fstop);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_GAMMA, m_TM_linear_gamma);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_CONTRAST_YWA, m_TM_contrast_ywa);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);

	ResetLightGroupsFromFilm();

	UpdateTonemapWidgetValues();
	if(m_auto_tonemap) ApplyTonemapping();
}

void LuxGui::UpdateLightGroupWidgetValues() {
	for( std::vector<LuxLightGroupPanel*>::iterator it = m_LightGroupPanels.begin(); it != m_LightGroupPanels.end(); it++) {
		(*it)->UpdateWidgetValues();
	}
}
void LuxGui::ResetLightGroups( void ) {
	for( std::vector<LuxLightGroupPanel*>::iterator it = m_LightGroupPanels.begin(); it != m_LightGroupPanels.end(); it++) {
		LuxLightGroupPanel *currPanel = *it;
		m_LightGroupsSizer->Detach(currPanel);
		delete currPanel;
	}
	m_LightGroupPanels.clear();
	m_LightGroups->Layout();
}
void LuxGui::ResetLightGroupsFromFilm( void ) {
	int numLightGroups = (int)luxGetParameterValue(LUX_FILM, LUX_FILM_LG_COUNT);
	for(int i = 0; i < numLightGroups; i++) {
		LuxLightGroupPanel *currPanel = new LuxLightGroupPanel(
			this, m_LightGroups
		);
		currPanel->SetIndex(i);
		currPanel->ResetValuesFromFilm();
		m_LightGroupsSizer->Add(currPanel, 0, wxEXPAND | wxALL, 1);
		m_LightGroupPanels.push_back(currPanel);
	}
	m_LightGroups->Layout();
}

void LuxGui::LuxOptions::OnMenu(wxCommandEvent& event) {
	switch (event.GetId()) {
		case ID_SYS_APPLY:
			ApplySysOptions();
			break;
		case ID_WRITE_OPTIONS:
			{
				int nSel = event.GetSelection();

				if ( nSel == 0 ) m_UseFlm = event.IsChecked();
				else if ( nSel == 1 ) m_Write_TGA = event.IsChecked();
				else if ( nSel == 2 ) m_Write_TM_EXR = event.IsChecked();
				else if ( nSel == 3 ) m_Write_UTM_EXR = event.IsChecked();
				else if ( nSel == 4 ) m_Write_TM_IGI = event.IsChecked();
				else if ( nSel == 5 ) m_Write_UTM_IGI = event.IsChecked();
			}
			break;
		default:
			break;
	}
}

void LuxGui::LuxOptions::OnSpin( wxSpinEvent& event ) {
	switch (event.GetId()) {
		case ID_SYS_DISPLAY_INT:
			m_DisplayInterval = event.GetPosition();
			break;
		case ID_SYS_WRITE_INT:
			m_WriteInterval = event.GetPosition();
			break;
		default:
			break;
	}
}

LuxGui::LuxLightGroupPanel::LuxLightGroupPanel(
	LuxGui *gui,
	wxWindow* parent, 
	wxWindowID id, 
	const wxPoint& pos, 
	const wxSize& size, 
	long style )
	: LightGroupPanel(parent, id, pos, size, style), m_Gui(gui)
{
	m_lightgroupBitmap->SetBitmap(wxMEMORY_BITMAP(n_lightgroup_png));
	m_Index = -1;
}

int LuxGui::LuxLightGroupPanel::GetIndex() const {
	return m_Index;
}

void LuxGui::LuxLightGroupPanel::SetIndex( int index ) {
	m_Index = index;
}

void LuxGui::LuxLightGroupPanel::UpdateWidgetValues() {
	wxString st;
	m_LG_scaleSlider->SetValue(m_LG_scale / LG_SCALE_RANGE * FLOAT_SLIDER_RES);
	st = wxString::Format(_("%.02f"), m_LG_scale);
	m_LG_scaleText->SetValue(st);
	m_LG_temperatureSlider->SetValue(m_LG_temperature / LG_TEMPERATURE_RANGE * FLOAT_SLIDER_RES);
	st = wxString::Format(_("%.02f"), m_LG_temperature);
	m_LG_temperatureText->SetValue(st);
}
void LuxGui::LuxLightGroupPanel::ResetValues() {
	m_LG_enable = true;
	m_LG_scale = 1.f;
	m_LG_temperature = 6500.f;
	m_LG_scaleRed = 1.f;
	m_LG_scaleGreen = 1.f;
	m_LG_scaleBlue = 1.f;
	m_LG_scaleX = 1.f;
	m_LG_scaleY = 1.f;
}
void LuxGui::LuxLightGroupPanel::ResetValuesFromFilm() {
	char tmpStr[256];
	luxGetStringParameterValue(LUX_FILM, LUX_FILM_LG_NAME, &tmpStr[0], 256, m_Index);
	m_LG_name->SetLabel(wxString::FromAscii(tmpStr));
	m_LG_enable = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, m_Index) != 0.f;
	m_LG_scale = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_LG_SCALE, m_Index);
	m_LG_temperature = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_LG_TEMPERATURE, m_Index);
	m_LG_scaleRed = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_LG_SCALE_RED, m_Index);
	m_LG_scaleGreen = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_LG_SCALE_GREEN, m_Index);
	m_LG_scaleBlue = luxGetDefaultParameterValue(LUX_FILM, LUX_FILM_LG_SCALE_BLUE, m_Index);

	luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, m_LG_enable, m_Index);
	luxSetParameterValue(LUX_FILM, LUX_FILM_LG_SCALE, m_LG_scale, m_Index);
	luxSetParameterValue(LUX_FILM, LUX_FILM_LG_SCALE_RED, m_LG_scaleRed, m_Index);
	luxSetParameterValue(LUX_FILM, LUX_FILM_LG_SCALE_GREEN, m_LG_scaleGreen, m_Index);
	luxSetParameterValue(LUX_FILM, LUX_FILM_LG_SCALE_BLUE, m_LG_scaleBlue, m_Index);
}

void LuxGui::LuxLightGroupPanel::OnText(wxCommandEvent& event) {
	if ( event.GetEventType() != wxEVT_COMMAND_TEXT_ENTER ) return;

	switch(event.GetId()) {
		case ID_LG_SCALE_TEXT:
			if (m_LG_scaleText->IsModified()) {
				wxString st = m_LG_scaleText->GetValue();
				st.ToDouble(&m_LG_scale);
				if (m_LG_scale > LG_SCALE_RANGE)
					m_LG_scale = LG_SCALE_RANGE;
				else if (m_LG_scale < 0.f)
					m_LG_scale = 0.f;
				st = wxString::Format(_("%.02f"), m_LG_scale);
				m_LG_scaleText->SetValue(st);
				const int val = static_cast<int>(m_LG_scale / LG_SCALE_RANGE * FLOAT_SLIDER_RES);
				m_LG_scaleSlider->SetValue(val);
				UpdateParam(LUX_FILM, LUX_FILM_LG_SCALE, m_LG_scale, m_Index);
				m_Gui->UpdatedTonemapParam();
			}
			break;
		case ID_LG_TEMPERATURE_TEXT:
			if (m_LG_temperatureText->IsModified()) {
				wxString st = m_LG_temperatureText->GetValue();
				st.ToDouble(&m_LG_temperature);
				if (m_LG_temperature > LG_TEMPERATURE_RANGE)
					m_LG_temperature = LG_TEMPERATURE_RANGE;
				else if (m_LG_temperature < 0.f)
					m_LG_temperature = 0.f;
				st = wxString::Format(_("%.02f"), m_LG_temperature);
				m_LG_temperatureText->SetValue(st);
				const int val = static_cast<int>(m_LG_temperature / LG_TEMPERATURE_RANGE * FLOAT_SLIDER_RES);
				m_LG_temperatureSlider->SetValue(val);
				UpdateParam(LUX_FILM, LUX_FILM_LG_TEMPERATURE, m_LG_temperature, m_Index);
				m_Gui->UpdatedTonemapParam();
			}
			break;
		default:
			break;
	}
}

void LuxGui::LuxLightGroupPanel::OnCheckBox(wxCommandEvent& event)
{
	if (event.GetEventType() != wxEVT_COMMAND_CHECKBOX_CLICKED)
		return;

	switch (event.GetId()) {
		case ID_LG_ENABLE:
			m_LG_enable = m_LG_enableCheckbox->GetValue();
			m_LG_scaleSlider->Enable(m_LG_enable);
			m_LG_scaleText->Enable(m_LG_enable);
			m_LG_rgbPicker->Enable(m_LG_enable);
			m_LG_temperatureSlider->Enable(m_LG_enable);
			m_LG_temperatureText->Enable(m_LG_enable);
			UpdateParam(LUX_FILM, LUX_FILM_LG_ENABLE, m_LG_enable, m_Index);
			m_Gui->UpdatedTonemapParam();
			break;
		default:
			break;
	}
}

void LuxGui::LuxLightGroupPanel::OnColourChanged(wxColourPickerEvent &event)
{
	switch (event.GetId()) {
		case ID_LG_RGBCOLOR:
			m_LG_scaleRed = event.GetColour().Red();
			m_LG_scaleGreen = event.GetColour().Green();
			m_LG_scaleBlue = event.GetColour().Blue();
			UpdateParam(LUX_FILM, LUX_FILM_LG_SCALE_RED, m_LG_scaleRed / 255., m_Index);
			UpdateParam(LUX_FILM, LUX_FILM_LG_SCALE_GREEN, m_LG_scaleGreen / 255., m_Index);
			UpdateParam(LUX_FILM, LUX_FILM_LG_SCALE_BLUE, m_LG_scaleBlue / 255., m_Index);
			m_Gui->UpdatedTonemapParam();
			break;
		default:
			break;
	}
}

void LuxGui::LuxLightGroupPanel::OnScroll(wxScrollEvent& event) {
	switch(event.GetId()) {
		case ID_LG_SCALE:
		{
			m_LG_scale = event.GetPosition() * LG_SCALE_RANGE / FLOAT_SLIDER_RES;
			wxString st = wxString::Format(_("%.02f"), m_LG_scale);
			m_LG_scaleText->SetValue(st);
			UpdateParam(LUX_FILM, LUX_FILM_LG_SCALE, m_LG_scale, m_Index);
			m_Gui->UpdatedTonemapParam();
			break;
		}
		case ID_LG_TEMPERATURE:
		{
			m_LG_temperature = event.GetPosition() * LG_TEMPERATURE_RANGE / FLOAT_SLIDER_RES;
			wxString st = wxString::Format(_("%.02f"), m_LG_temperature);
			m_LG_temperatureText->SetValue(st);
			UpdateParam(LUX_FILM, LUX_FILM_LG_TEMPERATURE, m_LG_temperature, m_Index);
			m_Gui->UpdatedTonemapParam();
			break;
		}
		default:
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Network panel...

void LuxGui::UpdateNetworkTree( void )
{
	m_serverUpdateSpin->SetValue( luxGetNetworkServerUpdateInterval() );

	m_networkTreeCtrl->DeleteAllItems();

	wxTreeItemId idRootNode = m_networkTreeCtrl->AddRoot( _("Master") );

	int nServers = luxGetServerCount();
	RenderingServerInfo *pInfoList = new RenderingServerInfo[nServers];
	nServers = luxGetRenderingServersStatus( pInfoList, nServers );

	wxString sTemp;
	wxTreeItemId idTempNode;

	double sampDiv = luxStatistics("filmXres") * luxStatistics("filmYres");

	for( int n1 = 0; n1 < nServers; n1++ )
	{
		luxTreeData *pTempData = new luxTreeData;

		pTempData->m_SlaveFile 	= m_CurrentFile;

		pTempData->m_SlaveName 	= wxString::FromUTF8(pInfoList[n1].name);
		pTempData->m_SlavePort 	= wxString::FromUTF8(pInfoList[n1].port);
		pTempData->m_SlaveID 	= wxString::FromUTF8(pInfoList[n1].sid);

		pTempData->m_secsSinceLastContact 		= pInfoList[n1].secsSinceLastContact;
		pTempData->m_numberOfSamplesReceived 	= pInfoList[n1].numberOfSamplesReceived;

		sTemp = wxString::Format( _("Slave: %s - Port: %s - ID: %s - Samples Per Pixel: %lf"),
									pTempData->m_SlaveName.c_str(),
									pTempData->m_SlavePort.c_str(),
									pTempData->m_SlaveID.c_str(),
									( pInfoList[n1].numberOfSamplesReceived / sampDiv ) );

		idTempNode = m_networkTreeCtrl->AppendItem( idRootNode, sTemp, -1, -1, pTempData );
						m_networkTreeCtrl->AppendItem( idTempNode, m_CurrentFile );
 	}

	m_networkTreeCtrl->ExpandAll();
}

void LuxGui::OnTreeSelChanged( wxTreeEvent& event )
{
	wxTreeItemId idTreeNode = event.GetItem();

	luxTreeData *pNodeData = (luxTreeData *)m_networkTreeCtrl->GetItemData( idTreeNode );

	if ( pNodeData != NULL )
	{
		m_serverTextCtrl->SetValue( wxString::Format(_("%s:%s"), pNodeData->m_SlaveName.c_str(), pNodeData->m_SlavePort.c_str()));
	}
}

void LuxGui::AddServer( void )
{
	wxString sServer( m_serverTextCtrl->GetValue() );

	if ( !sServer.empty() )
	{
		luxAddServer( sServer.char_str() );

		UpdateNetworkTree();
	}
}

void LuxGui::RemoveServer( void )
{
	wxString sServer( m_serverTextCtrl->GetValue() );

	if ( !sServer.empty() )
	{
		luxRemoveServer( sServer.char_str() );

		UpdateNetworkTree();
	}
}


void LuxGui::OnSpin( wxSpinEvent& event )
{
	if ( event.GetId() == ID_SERVER_UPDATE_INT )
	{
		luxSetNetworkServerUpdateInterval( event.GetPosition() );
	}
}

// CF
//////////////////////////////////////////////////////////////////////////////////////////////////

void LuxGui::OnOpen(wxCommandEvent& event) {
	if(m_guiRenderState == RENDERING) {
		// Give warning that current rendering is not stoped
		if(wxMessageBox(wxT("Current file is still rendering. Do you want to continue?"), wxT(""), wxYES_NO | wxICON_QUESTION, this) == wxNO)
			return;
	}

	wxFileDialog filedlg(this,
	                     _("Choose a file to open"),
											 wxEmptyString,
											 wxEmptyString,
											 _("LuxRender scene files (*.lxs)|*.lxs|All files (*.*)|*.*"),
											 wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if(filedlg.ShowModal() == wxID_OK) {
		m_statusBar->SetStatusText(wxT(""), 0);
		// Clean up if this is not the first rendering
		if(m_guiRenderState != WAITING) {
			if(m_guiRenderState != FINISHED) {
				if(m_updateThread)
					m_updateThread->join();
				luxExit();
				if(m_engineThread)
					m_engineThread->join();
			}
			luxError(LUX_NOERROR, LUX_INFO, "Freeing resources.");
			luxCleanup();
			ChangeRenderState(WAITING);
			m_renderOutput->Reset();
		}

		RenderScenefile(filedlg.GetPath());
	}
}

void LuxGui::OnExit(wxCloseEvent& event) {
	m_renderTimer->Stop();
	m_statsTimer->Stop();;
	m_loadTimer->Stop();;
	m_netTimer->Stop();;

	//if we have a scene file
  if(m_guiRenderState != WAITING) {
		if(m_updateThread)
			m_updateThread->join();

		luxExit();

		if(m_engineThread)
			m_engineThread->join();

		luxError(LUX_NOERROR, LUX_INFO, "Freeing resources.");
		luxCleanup();
	}

	Destroy();
}

void LuxGui::OnError(wxLuxErrorEvent &event) {
	std::stringstream ss("");
	ss << boost::posix_time::second_clock::local_time() << ' ';
	switch(event.GetError()->GetSeverity()) {
		case LUX_DEBUG:
			ss << "Debug: "; break;
		case LUX_INFO:
			ss << "Info: ";	break;
		case LUX_WARNING:
			ss << "Warning: ";	break;
		case LUX_ERROR:
			ss << "Error: ";	break;
		case LUX_SEVERE:
			ss << "Severe error: ";	break;
	}
	ss << "(" << event.GetError()->GetCode() << ") ";
	ss << event.GetError()->GetMessage() << std::endl;
	m_logTextCtrl->AppendText(wxString::FromAscii(ss.str().c_str()));
	m_logTextCtrl->ShowPosition(m_logTextCtrl->GetLastPosition());
}

void LuxGui::OnTimer(wxTimerEvent& event) {
	switch (event.GetId()) {
		case ID_RENDERUPDATE:
			if(m_updateThread == NULL && luxStatistics("sceneIsReady") &&
			    (m_guiWindowState == SHOWN || m_guiRenderState == FINISHED)) {
				luxError(LUX_NOERROR, LUX_INFO, "GUI: Updating framebuffer...");
				m_statusBar->SetStatusText(wxT("Tonemapping..."), 0);
				m_updateThread = new boost::thread(boost::bind(&LuxGui::UpdateThread, this));
			}
			break;
		case ID_STATSUPDATE:
			if(luxStatistics("sceneIsReady")) {
				UpdateStatistics();
				if(m_guiRenderState == STOPPING && m_samplesSec == 0.0) {
					// Render threads stopped, do one last render update
					luxError(LUX_NOERROR, LUX_INFO, "GUI: Updating framebuffer...");
					m_statusBar->SetStatusText(wxT("Tonemapping..."), 0);
					m_updateThread = new boost::thread(boost::bind(&LuxGui::UpdateThread, this));
					m_statsTimer->Stop();
					luxPause();
					luxError(LUX_NOERROR, LUX_INFO, "Rendering stopped by user.");
					ChangeRenderState(STOPPED);
				}
			}
			break;
		case ID_LOADUPDATE:
			m_progDialog->Pulse();
			if(luxStatistics("sceneIsReady") || m_guiRenderState == FINISHED) {
				m_progDialog->Destroy();
				m_loadTimer->Stop();

				if(luxStatistics("sceneIsReady")) {
					// Scene file loaded
					// Add other render threads if necessary
					int curThreads = 1;
					while(curThreads < m_numThreads) {
						luxAddThread();
						curThreads++;
					}

					// Start updating the display by faking a resume menu item click.
					wxCommandEvent startEvent(wxEVT_COMMAND_MENU_SELECTED, ID_RESUMEITEM);
					GetEventHandler()->AddPendingEvent(startEvent);

					// Enable tonemapping options and reset from values trough API
					m_outputNotebook->Enable( true );
					ResetToneMapping();
					Refresh();
				}
			}
			break;
		case ID_NETUPDATE:
			UpdateNetworkTree();
			break;
	}
}

void LuxGui::OnCommand(wxCommandEvent &event) {
	if(event.GetEventType() == wxEVT_LUX_TONEMAPPED) {
		// Make sure the update thread has ended so we can start another one later.
		m_updateThread->join();
		delete m_updateThread;
		m_updateThread = NULL;
		m_statusBar->SetStatusText(wxT(""), 0);
		m_renderOutput->Reload();

	} else if(event.GetEventType() == wxEVT_LUX_PARSEERROR) {
		wxMessageBox(wxT("Scene file parse error.\nSee log for details."), wxT("Error"), wxOK | wxICON_ERROR, this);
		ChangeRenderState(FINISHED);
	} else if(event.GetEventType() == wxEVT_LUX_FINISHED && m_guiRenderState == RENDERING) {
		// Ignoring finished events if another file is being opened (state != RENDERING)
		//wxMessageBox(wxT("Rendering is finished."), wxT("LuxRender"), wxOK | wxICON_INFORMATION, this);
		ChangeRenderState(FINISHED);
		// Stop timers and update output one last time.
		m_renderTimer->Stop();
#ifdef __WXOSX_COCOA__
		wxTimerEvent rendUpdEvent(*m_renderTimer);
		rendUpdEvent.SetId(ID_RENDERUPDATE);
#else
		wxTimerEvent rendUpdEvent(ID_RENDERUPDATE, GetId());
#endif
		GetEventHandler()->AddPendingEvent(rendUpdEvent);
		m_statsTimer->Stop();
#ifdef __WXOSX_COCOA__
		wxTimerEvent statUpdEvent(*m_renderTimer);
		rendUpdEvent.SetId(ID_RENDERUPDATE);
#else
		wxTimerEvent statUpdEvent(ID_STATSUPDATE, GetId());
#endif
		GetEventHandler()->AddPendingEvent(statUpdEvent);
	}
}

#if defined (__WXMSW__) ||  defined (__WXGTK__)
void lux::LuxGui::OnIconize( wxIconizeEvent& event )
{
	if(!event.Iconized())
		m_guiWindowState = SHOWN;
	else
		m_guiWindowState = HIDDEN;
}
#endif

void LuxGui::RenderScenefile(wxString filename) {
	wxFileName fn(filename);
	SetTitle(wxT("LuxRender - ")+fn.GetName());

	// CF
	m_CurrentFile = filename;

	// Start main render thread
	m_engineThread = new boost::thread(boost::bind(&LuxGui::EngineThread, this, filename));

	m_progDialog = new wxProgressDialog(wxT("Loading..."), wxT(""), 100, NULL, wxSTAY_ON_TOP);
	m_progDialog->Pulse();
	m_loadTimer->Start(1000, wxTIMER_CONTINUOUS);
}

void LuxGui::OnSelection(wxViewerEvent& event) {
	if(m_viewerToolBar->GetToolState(ID_ZOOMTOOL) == true) {
		// Zoom in and de-select anything
		m_renderOutput->SetZoom(event.GetSelection().get());
		m_renderOutput->SetSelection(NULL);
	} else if(m_viewerToolBar->GetToolState(ID_REFINETOOL) == true) {
		// Highlight current selection
		m_renderOutput->SetHighlight(event.GetSelection().get());
		m_renderOutput->SetSelection(NULL);
		// TODO: Pass selection to the core for actual refinement
	}
}

void LuxGui::EngineThread(wxString filename) {
	boost::filesystem::path fullPath(boost::filesystem::initial_path());
	fullPath = boost::filesystem::system_complete(boost::filesystem::path(filename.fn_str(), boost::filesystem::native));

	chdir(fullPath.branch_path().string().c_str());

	ParseFile(fullPath.leaf().c_str());

	if(!luxStatistics("sceneIsReady")) {
		wxCommandEvent errorEvent(wxEVT_LUX_PARSEERROR, GetId());
		GetEventHandler()->AddPendingEvent(errorEvent);

		luxWait();
	} else {
		luxWait();

	  luxError(LUX_NOERROR, LUX_INFO, "Rendering done.");
		wxCommandEvent endEvent(wxEVT_LUX_FINISHED, GetId());
		GetEventHandler()->AddPendingEvent(endEvent);
	}
}

void LuxGui::UpdateThread() {
	if(m_GLAcceleration == false)
		luxUpdateFramebuffer();
	else
		luxUpdateHDRFramebuffer();

	wxCommandEvent endEvent(wxEVT_LUX_TONEMAPPED, GetId());
	GetEventHandler()->AddPendingEvent(endEvent);
}

void LuxGui::SetRenderThreads(int num) {
	if(luxStatistics("sceneIsReady")) {
		if(num > m_numThreads) {
			for(; num > m_numThreads; m_numThreads++)
				luxAddThread();
		} else {
			for(; num < m_numThreads; m_numThreads--)
				luxRemoveThread();
		}
	} else {
		m_numThreads = num;
	}

	m_ThreadText->SetLabel( wxString::Format( _("Threads: %02d  "), m_numThreads ) );
}

void LuxGui::UpdateStatistics() {
	m_samplesSec = luxStatistics("samplesSec");
	int samplesSec = Floor2Int(m_samplesSec);
	int samplesTotSec = Floor2Int(luxStatistics("samplesTotSec"));
	int secElapsed = Floor2Int(luxStatistics("secElapsed"));
	double samplesPx = luxStatistics("samplesPx");
	int efficiency = Floor2Int(luxStatistics("efficiency"));
	int EV = luxStatistics("filmEV");

	int secs = (secElapsed) % 60;
	int mins = (secElapsed / 60) % 60;
	int hours = (secElapsed / 3600);

	wxString stats;
	stats.Printf(wxT("%02d:%02d:%02d - %d S/s - %d TotS/s - %.2f S/px - %i%% eff - EV = %d"),
	             hours, mins, secs, samplesSec, samplesTotSec, samplesPx, efficiency, EV);
	m_statusBar->SetStatusText(stats, 1);
}

/*** LuxOutputWin ***/

BEGIN_EVENT_TABLE(LuxOutputWin, wxWindow)
#ifndef __WXOSX_COCOA__
    EVT_PAINT (LuxOutputWin::OnPaint)
#endif
END_EVENT_TABLE()

LuxOutputWin::LuxOutputWin(wxWindow *parent)
      : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, -1)), wxViewerBase() {
}

wxWindow* LuxOutputWin::GetWindow() {
	return this;
}

void LuxOutputWin::Reload() {
	Refresh();
}

void LuxOutputWin::OnDraw(wxDC &dc) {
	if (luxStatistics("sceneIsReady")) {
		int w = luxStatistics("filmXres"), h = luxStatistics("filmYres");
		SetVirtualSize(w, h);
		SetScrollRate(1,1);
		unsigned char* fb = luxFramebuffer();
		dc.DrawBitmap(wxBitmap(wxImage(w, h, fb, true)), 0, 0, false);
	}
}

/*** LuxGuiErrorHandler wrapper ***/

void lux::LuxGuiErrorHandler(int code, int severity, const char *msg) {
	// Dade - console print enabled by command line option
	if (copyLog2Console)
		luxErrorPrint(code, severity, msg);

	boost::shared_ptr<LuxError> error(new LuxError(code, severity, msg));
	wxLuxErrorEvent errorEvent(error, wxEVT_LUX_ERROR);
	wxTheApp->GetTopWindow()->GetEventHandler()->AddPendingEvent(errorEvent);
}
