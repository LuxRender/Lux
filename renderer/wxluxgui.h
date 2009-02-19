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
#include "wxviewer.h"

namespace lux
{

#define ID_RENDERUPDATE	2000
#define ID_STATSUPDATE	2001
#define ID_LOADUPDATE	2002
#define ID_NETUPDATE	2003

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

class LuxOutputWin : public wxScrolledWindow, public wxViewerBase {
public:
	LuxOutputWin(wxWindow *parent);
	virtual wxWindow* GetWindow();
	virtual void Reload();

protected:
	DECLARE_EVENT_TABLE()
	void OnDraw(wxDC &dc);
};


/*** LuxGui ***/

enum LuxGuiRenderState
{
	WAITING,
	RENDERING,
	STOPPING,
	STOPPED,
	PAUSED,
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
	LuxGui(wxWindow* parent, bool opengl, bool copylog2console);
	~LuxGui();

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
	void OnCommand(wxCommandEvent &event);
	void OnIconize(wxIconizeEvent& event);
	void OnSelection(wxViewerEvent& event);
	void OnSpin( wxSpinEvent& event );
	void OnScroll( wxScrollEvent& event );
	void OnText(wxCommandEvent &event);
	void OnFocus( wxFocusEvent& event );

	void ChangeRenderState(LuxGuiRenderState state);
	void LoadImages();

	// Parsing and rendering threads
	void EngineThread(wxString filename);
	void UpdateThread();
	int m_numThreads;

	void UpdateStatistics();
	void ApplyTonemapping();

	boost::thread *m_engineThread, *m_updateThread;
	bool m_opengl;
	bool m_copyLog2Console;
	double m_samplesSec;
	LuxGuiRenderState m_guiRenderState;
	LuxGuiWindowState m_guiWindowState;

	wxProgressDialog* m_progDialog;

	wxViewerBase* m_renderOutput;

	wxTimer* m_loadTimer;
	wxTimer* m_renderTimer;
	wxTimer* m_statsTimer;
	wxTimer* m_netTimer;

	wxBitmap m_splashbmp;

	bool m_GLAcceleration; // false = no accel, true = opengl accerelation

	// CF
	class luxTreeData : public wxTreeItemData
	{
		public:

		wxString m_SlaveName;
		wxString m_SlaveFile;
		wxString m_SlavePort;
		wxString m_SlaveID;

		unsigned int 	m_secsSinceLastContact;
		double 			m_numberOfSamplesReceived;
	};

	wxString m_CurrentFile;

	void UpdateNetworkTree( void );

	void AddServer( void );
	void RemoveServer( void );

	void OnTreeSelChanged( wxTreeEvent& event );
	
	void UpdateTonemapWidgetValues( void );
	void ResetToneMapping( void );
	void ResetToneMappingFromFilm( void );

	void SetColorSpacePreset(int choice);
	void SetTonemapKernel(int choice);

	// Tonemapping/ToRGB variables

	bool m_auto_tonemap;
	int m_TM_kernel;

	bool m_TM_reinhard_autoywa;
	double m_TM_reinhard_ywa;
	double m_TM_reinhard_prescale;
	double m_TM_reinhard_postscale;
	double m_TM_reinhard_burn;

	double m_TM_linear_exposure;
	double m_TM_linear_sensitivity;
	double m_TM_linear_fstop;
	double m_TM_linear_gamma;

	double m_TM_contrast_ywa;

	double m_TORGB_xwhite, m_TORGB_ywhite;
	double m_TORGB_xred, m_TORGB_yred;
	double m_TORGB_xgreen, m_TORGB_ygreen;
	double m_TORGB_xblue, m_TORGB_yblue;

	double m_TORGB_gamma;


	class LuxOptions : public m_OptionsDialog {
		public:

		LuxOptions( LuxGui* parent );

		protected:

			void OnMenu( wxCommandEvent& event );
			void OnClose( wxCloseEvent& event );
			void OnSpin( wxSpinEvent& event );

		public:

			LuxGui *m_Parent;

			void UpdateSysOptions( void );
			void ApplySysOptions( void );

			unsigned int m_DisplayInterval;
			unsigned int m_WriteInterval;

			bool m_UseFlm;
			bool m_Write_TGA;
			bool m_Write_TM_EXR;
			bool m_Write_UTM_EXR;
			bool m_Write_TM_IGI;
			bool m_Write_UTM_IGI;
	};

	LuxOptions *m_LuxOptions;
};


/*** LuxGuiErrorHandler wrapper ***/

void LuxGuiErrorHandler(int code, int severity, const char *msg);

}//namespace lux

#endif // LUX_WXLUXGUI_H
