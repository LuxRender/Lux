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
#include <vector>

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
	void OnMouse(wxMouseEvent &event);
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
	void OnFocus(wxFocusEvent &event);
	void OnCheckBox(wxCommandEvent &event);
	void OnColourChanged(wxColourPickerEvent &event);

	void ChangeRenderState(LuxGuiRenderState state);
	void LoadImages();

	// Parsing and rendering threads
	void EngineThread(wxString filename);
	void UpdateThread();
	int m_numThreads;

	void UpdateStatistics();
	void ApplyTonemapping(bool withbloomcomputation = false);

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
	
	void UpdatedTonemapParam();
	void UpdateTonemapWidgetValues( void );
	void ResetToneMapping( void );
	void ResetToneMappingFromFilm( bool useDefaults=true );

	void UpdateLightGroupWidgetValues( void );
	void ResetLightGroups( void );
	void ResetLightGroupsFromFilm( bool useDefaults=true );

	void SetColorSpacePreset(int choice);
	void SetTonemapKernel(int choice);

	// Tonemapping/ToRGB variables
	bool m_auto_tonemap;
	int m_TM_kernel;

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
	double m_bloomradius, m_bloomweight;

	bool m_Vignetting_Enabled;
	double m_Vignetting_Scale;

	bool m_GREYC_enabled, m_GREYC_fast_approx;
	double m_GREYC_amplitude, m_GREYC_sharpness, m_GREYC_anisotropy,
		m_GREYC_alpha, m_GREYC_sigma, m_GREYC_gauss_prec, m_GREYC_dl, m_GREYC_da;
	double m_GREYC_nb_iter;
	int m_GREYC_interp;

	// Lightgroups
	class LuxLightGroupPanel : public LightGroupPanel {
	public:
		LuxLightGroupPanel( 
			LuxGui* gui,
			wxWindow* parent, wxWindowID id = wxID_ANY, 
			const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), 
			long style = wxTAB_TRAVERSAL );

		void SetIndex( int index );
		int GetIndex() const;
		
		void UpdateWidgetValues();
		void ResetValues();
		void ResetValuesFromFilm( bool useDefaults=true );

	protected:
		void SetWidgetsEnabled( bool enabled );

		void OnText(wxCommandEvent& event);
		void OnCheckBox(wxCommandEvent &event);
		void OnColourChanged(wxColourPickerEvent &event);
		void OnScroll(wxScrollEvent& event);
	private:
		LuxGui* const m_Gui;
		int m_Index;

		bool m_LG_enable;
		double m_LG_scale;
		double m_LG_temperature;
		double m_LG_scaleRed, m_LG_scaleGreen, m_LG_scaleBlue;
		double m_LG_scaleX, m_LG_scaleY;
	};

	std::vector<LuxLightGroupPanel*> m_LightGroupPanels;

	// ImageWindow - helper class for 24b image display
	class ImageWindow : public wxWindow {
		public:
			ImageWindow(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr);
			~ImageWindow();
			void SetImage(const wxImage& img);
		protected:
			void OnPaint(wxPaintEvent& event);
			void OnEraseBackground(wxEraseEvent& event);
		private:
			wxBitmap* m_bitmap;
	};

	// LuxHistogramWindow
	class LuxHistogramWindow : public ImageWindow {
		public:
			LuxHistogramWindow(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition);
			~LuxHistogramWindow();
			void Update();
			void SetOption(int option);
			void ClearOption(int option);
		protected:
			void OnSize(wxSizeEvent& event);
		private:
			int m_Options;
			bool m_Enabled;
	};

	LuxHistogramWindow *m_HistogramWindow;

};


/*** LuxGuiErrorHandler wrapper ***/

void LuxGuiErrorHandler(int code, int severity, const char *msg);

}//namespace lux

#endif // LUX_WXLUXGUI_H
