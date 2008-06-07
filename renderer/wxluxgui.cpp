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
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

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

#include "wxluxgui.h"
#include "wximages.h"

using namespace lux;

/*** LuxGui ***/

DEFINE_EVENT_TYPE(lux::wxEVT_LUX_ERROR)

BEGIN_EVENT_TABLE(LuxGui, wxFrame)
		EVT_LUX_ERROR (wxID_ANY, LuxGui::OnError)
    EVT_TIMER			(wxID_ANY, LuxGui::OnTimer)
		EVT_SPINCTRL	(wxID_ANY, LuxGui::OnSpin)
END_EVENT_TABLE()

LuxGui::LuxGui(wxWindow* parent):LuxMainFrame(parent) {
	// Load images and icons from header.
	LoadImages();

	// Add custom output viewer window
	m_renderOutput = new LuxOutputWin(m_renderPage);
	m_renderPage->GetSizer()->Add(m_renderOutput, 1, wxALL | wxEXPAND, 5);

	// Create render output update timer
	m_renderTimer = new wxTimer(this, ID_RENDERUPDATE);
	m_statsTimer = new wxTimer(this, ID_STATSUPDATE);

	m_numThreads = 0;

	luxErrorHandler(&LuxGuiErrorHandler);
}

void LuxGui::LoadImages() {
	wxImage::AddHandler(new wxPNGHandler());

	// App icon
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxMEMORY_BITMAP(luxicon_png));
	SetIcon(appIcon);

	// Resume toolbar tool
	m_renderToolBar->SetToolNormalBitmap(ID_RESUMETOOL, wxMEMORY_BITMAP(resume_png));
	// Stop toolbar tool
	m_renderToolBar->SetToolNormalBitmap(ID_STOPTOOL, wxMEMORY_BITMAP(stop_png));

	// wxGTK has problems changing an existing menu item's icon, so we remove and add then again...
	// Resume menu item
	wxMenuItem *renderitem = m_render->Remove(ID_RESUMEITEM);
	renderitem->SetBitmap(wxMEMORY_BITMAP(resume_png));
	m_render->Insert(0,renderitem);
	// Stop menu item
	wxMenuItem *stopitem = m_render->Remove(ID_STOPITEM);
	stopitem->SetBitmap(wxMEMORY_BITMAP(stop_png));
	m_render->Insert(1,stopitem);

	m_auinotebook->SetPageBitmap(0, wxMEMORY_BITMAP(render_png));
	m_auinotebook->SetPageBitmap(1, wxMEMORY_BITMAP(info_png));
	m_auinotebook->SetPageBitmap(2, wxMEMORY_BITMAP(output_png));

	m_splashbmp = wxMEMORY_BITMAP(splash_png);
}

void LuxGui::OnMenu(wxCommandEvent& event) {
	switch (event.GetId()) {
		case ID_RESUMEITEM:
		case ID_RESUMETOOL:
			// Start display update timer
			if(luxStatistics("sceneIsReady")) {
				m_renderOutput->Refresh();
				m_renderTimer->Start(1000*luxStatistics("displayInterval"), wxTIMER_CONTINUOUS);
				m_statsTimer->Start(1000, wxTIMER_CONTINUOUS);
			}
			break;
		case ID_STOPITEM:
		case ID_STOPTOOL:
			// Stop display update timer
			m_renderTimer->Stop();
			m_statsTimer->Stop();
			break;
		case wxID_ABOUT:
			new wxSplashScreen(m_splashbmp, wxSPLASH_CENTRE_ON_PARENT, 0, this, -1);
			break;
		default:
			break;
	}
}

void LuxGui::OnOpen(wxCommandEvent& event) {
	wxFileDialog filedlg(this,
	                     _("Choose a file to open"),
											 wxEmptyString,
											 wxEmptyString,
											 _("LuxRender scene files (*.lxs)|*.lxs|All files (*.*)|*.*"),
											 wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (filedlg.ShowModal() == wxID_OK)
		RenderScenefile(filedlg.GetPath());
}

void LuxGui::OnExit(wxCommandEvent& event) {
	Close(false);
}

void LuxGui::OnError(wxLuxErrorEvent &event) {
	std::stringstream ss("");
	switch(event.GetError()->GetSeverity()) {
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
}

void LuxGui::OnTimer(wxTimerEvent& event) {
	switch (event.GetId()) {
		case ID_RENDERUPDATE:
			if (luxStatistics("sceneIsReady")) {
				LuxError(LUX_NOERROR, LUX_INFO, "GUI: Updating framebuffer...");
				luxUpdateFramebuffer();
				m_renderOutput->Refresh();
			}
			break;
		case ID_STATSUPDATE:
			if (luxStatistics("sceneIsReady"))
				UpdateStatistics();
			break;
	}
}

void LuxGui::OnSpin(wxSpinEvent& event) {
	SetRenderThreads(event.GetPosition());
}

void LuxGui::RenderScenefile(wxString filename) {
	boost::thread engine(boost::bind(&LuxGui::EngineThread, this, filename));
	m_numThreads = 1;
	m_threadSpinCtrl->SetValue(m_numThreads);

	wxFileName fn(filename);
	SetTitle(wxT("LuxRender - ")+fn.GetName());

	//TODO: Show "loading" dialog

	while(!luxStatistics("sceneIsReady"))
		wxSleep(1);

	// Start updating the display by faking a resume menu item click.
	m_renderOutput->SetVirtualSize(luxStatistics("filmXres"), luxStatistics("filmYres"));
	m_renderOutput->SetScrollRate(1,1);
	wxCommandEvent startEvent(wxEVT_COMMAND_MENU_SELECTED, ID_RESUMEITEM);
	GetEventHandler()->AddPendingEvent(startEvent);
}

void LuxGui::EngineThread(wxString filename) {
	boost::filesystem::path fullPath(boost::filesystem::initial_path());
	fullPath = boost::filesystem::system_complete(boost::filesystem::path(filename.fn_str(), boost::filesystem::native));

	chdir(fullPath.branch_path().string().c_str());

	ParseFile(fullPath.leaf().c_str());
}

void LuxGui::SetRenderThreads(int num) {
	if(num > m_numThreads) {
		for(; num > m_numThreads; m_numThreads++)
			luxAddThread();
	} else {
		for(; num < m_numThreads; m_numThreads--)
			luxRemoveThread();
	}
	m_threadSpinCtrl->SetValue(m_numThreads);
}

void LuxGui::UpdateStatistics() {
	int samplesSec = Floor2Int(luxStatistics("samplesSec"));
	int samplesTotSec = Floor2Int(luxStatistics("samplesTotSec"));
	int secElapsed = Floor2Int(luxStatistics("secElapsed"));
	double samplesPx = luxStatistics("samplesPx");
	int efficiency = Floor2Int(luxStatistics("efficiency"));

	int secs = (secElapsed) % 60;
	int mins = (secElapsed / 60) % 60;
	int hours = (secElapsed / 3600);

	wxString stats;
	stats.Printf(wxT("%02d:%02d:%02d - %d S/s - %d TotS/s - %.2f S/px - %i%% eff"),
	             hours, mins, secs, samplesSec, samplesTotSec, samplesPx, efficiency);
	m_statusBar->SetStatusText(stats, 1);
}

/*** LuxOutputWin ***/

BEGIN_EVENT_TABLE(LuxOutputWin, wxWindow)
    EVT_PAINT (LuxOutputWin::OnPaint)
END_EVENT_TABLE()

LuxOutputWin::LuxOutputWin(wxWindow *parent)
      : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, -1)) {
}

void LuxOutputWin::OnDraw(wxDC &dc) {
	if (luxStatistics("sceneIsReady")) {
		unsigned char* fb = luxFramebuffer();
		dc.DrawBitmap(wxBitmap(wxImage(luxStatistics("filmXres"), luxStatistics("filmYres"), fb, true)), 0, 0, false);
	}
}

/*** LuxGuiErrorHandler wrapper ***/

void lux::LuxGuiErrorHandler(int code, int severity, const char *msg) {
	boost::shared_ptr<LuxError> error(new LuxError(code, severity, msg));
	wxLuxErrorEvent errorEvent(error, wxEVT_LUX_ERROR);
	wxTheApp->GetTopWindow()->GetEventHandler()->AddPendingEvent(errorEvent);
}
