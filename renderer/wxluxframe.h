///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __wxluxframe__
#define __wxluxframe__

#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/toolbar.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/aui/auibook.h>
#include <wx/statusbr.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

namespace lux
{
	#define ID_RESUMEITEM 1000
	#define ID_PAUSEITEM 1001
	#define ID_STOPITEM 1002
	#define ID_RESUMETOOL 1003
	#define ID_PAUSETOOL 1004
	#define ID_STOPTOOL 1005
	
	///////////////////////////////////////////////////////////////////////////////
	/// Class LuxMainFrame
	///////////////////////////////////////////////////////////////////////////////
	class LuxMainFrame : public wxFrame 
	{
		private:
		
		protected:
			wxMenuBar* m_menubar;
			wxMenu* m_file;
			wxMenu* m_render;
			wxMenu* m_help;
			wxAuiNotebook* m_auinotebook;
			wxPanel* m_renderPage;
			wxToolBar* m_renderToolBar;
			wxSpinCtrl* m_threadSpinCtrl;
			wxPanel* m_logPage;
			wxTextCtrl* m_logTextCtrl;
			wxStatusBar* m_statusBar;
			
			// Virtual event handlers, overide them in your derived class
			virtual void OnExit( wxCloseEvent& event ){ event.Skip(); }
			virtual void OnOpen( wxCommandEvent& event ){ event.Skip(); }
			virtual void OnMenu( wxCommandEvent& event ){ event.Skip(); }
			
		
		public:
			LuxMainFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("LuxRender"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1024,768 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
			~LuxMainFrame();
		
	};
	
} // namespace lux

#endif //__wxluxframe__
