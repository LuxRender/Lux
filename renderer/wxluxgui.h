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

#ifndef LUX_WXLUXGUI_H
#define LUX_WXLUXGUI_H

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>

#include <wx/scrolwin.h>
#include <wx/progdlg.h>

#include "wxluxframe.h"

namespace lux
{

#define ID_RENDERUPDATE	2000
#define ID_STATSUPDATE	2001
#define ID_LOADUPDATE		2002

/*** LuxError and wxLuxErrorEvent ***/

class LuxError {
public:
	LuxError(int code, int severity, const char *msg): m_code(code), m_severity(severity), m_msg(msg) {}

	int GetCode() { return m_code; }
	int GetSeverity() { return m_severity; }
	const std::string& GetMessage() { return m_msg; }

protected:
	int m_code;
	int m_severity;
	std::string m_msg;
};

class wxLuxErrorEvent : public wxEvent {
public:
    wxLuxErrorEvent(const boost::shared_ptr<LuxError> error, wxEventType eventType = wxEVT_NULL, int id = 0): wxEvent(id, eventType), m_error(error) {}

    boost::shared_ptr<LuxError> GetError() { return m_error; }

    // required for sending with wxPostEvent()
    wxEvent* Clone(void) const { return new wxLuxErrorEvent(*this); }

protected:
    boost::shared_ptr<LuxError> m_error;
};

DECLARE_EVENT_TYPE(wxEVT_LUX_ERROR, -1)
DECLARE_EVENT_TYPE(wxEVT_LUX_PARSEERROR, -1)
DECLARE_EVENT_TYPE(wxEVT_LUX_FINISHED, -1)
DECLARE_EVENT_TYPE(wxEVT_LUX_TONEMAPPED, -1)

typedef void (wxEvtHandler::*wxLuxErrorEventFunction)(wxLuxErrorEvent&);

#define EVT_LUX_ERROR(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_LUX_ERROR, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) (wxNotifyEventFunction) \
    wxStaticCastEvent( wxLuxErrorEventFunction, & fn ), (wxObject *) NULL ),

/*** LuxOutputWin ***/

class LuxOutputWin : public wxScrolledWindow {
public:
	LuxOutputWin(wxWindow *parent);

protected:
	DECLARE_EVENT_TABLE()
	void OnDraw(wxDC &dc);
};


/*** LuxGui ***/

enum LuxGuiRenderState
{
	WAITING,
	RENDERING,
	IDLE,
	FINISHED
};
enum LuxGuiWindowState
{
	SHOWN,
	HIDDEN
};

class LuxGui : public LuxMainFrame {
public:
	/** Constructor */
	LuxGui(wxWindow* parent, bool opengl);

	void RenderScenefile(wxString filename);
	void SetRenderThreads(int num);

protected:
	DECLARE_EVENT_TABLE()
	// Handlers for LuxMainFrame events.
	void OnMenu(wxCommandEvent &event);
	void OnOpen(wxCommandEvent &event);
	void OnExit(wxCloseEvent &event);
	void OnError(wxLuxErrorEvent &event);
	void OnTimer(wxTimerEvent& event);
	void OnSpin(wxSpinEvent& event);
	void OnCommand(wxCommandEvent &event);
	void OnIconize(wxIconizeEvent& event);

	void ChangeRenderState(LuxGuiRenderState state);
	void LoadImages();

	// Parsing and rendering threads
	void EngineThread(wxString filename);
	void UpdateThread();
	int m_numThreads;

	void UpdateStatistics();

	boost::thread *m_engineThread, *m_updateThread;
	bool m_opengl;
	LuxGuiRenderState m_guiRenderState;
	LuxGuiWindowState m_guiWindowState;

	wxProgressDialog* m_progDialog;
	wxWindow* m_renderOutput;
	wxTimer* m_loadTimer;
	wxTimer* m_renderTimer;
	wxTimer* m_statsTimer;

	wxBitmap m_splashbmp;
};


/*** LuxGuiErrorHandler wrapper ***/

void LuxGuiErrorHandler(int code, int severity, const char *msg);

}//namespace lux

#endif // LUX_WXLUXGUI_H
