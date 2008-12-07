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
#include <wx/toolbar.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/treectrl.h>
#include <wx/aui/auibook.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/checklst.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

namespace lux
{
	#define ID_RESUMEITEM 1000
	#define ID_PAUSEITEM 1001
	#define ID_STOPITEM 1002
	#define ID_TOOL_BAR 1003
	#define ID_STATUS_BAR 1004
	#define ID_OPTIONS 1005
	#define ID_PAN_MODE 1006
	#define ID_ZOOM_MODE 1007
	#define ID_RENDER_COPY 1008
	#define ID_CLEAR_LOG 1009
	#define ID_FULL_SCREEN 1010
	#define ID_RESUMETOOL 1011
	#define ID_PAUSETOOL 1012
	#define ID_STOPTOOL 1013
	#define ID_NUM_THREADS 1014
	#define ID_ADD_THREAD 1015
	#define ID_REMOVE_THREAD 1016
	#define ID_PANTOOL 1017
	#define ID_ZOOMTOOL 1018
	#define ID_REFINETOOL 1019
	#define ID_ADD_SERVER 1020
	#define ID_REMOVE_SERVER 1021
	#define ID_SERVER_UPDATE_INT 1022
	#define ID_NETWORK_TREE 1023
	#define ID_TM_CHOICE 1024
	#define ID_RH_PRESCALE 1025
	#define ID_RH_PRESCALE_TEXT 1026
	#define ID_RH_POSTSCALE 1027
	#define ID_RH_POSTSCALE_TEXT 1028
	#define ID_RH_BURN 1029
	#define ID_RH_BURN_TEXT 1030
	#define ID_TM_RESET 1031
	#define ID_RENDER_REFRESH 1032
	#define ID_SYS_DISPLAY_INT 1033
	#define ID_SYS_WRITE_INT 1034
	#define ID_WRITE_OPTIONS 1035
	#define ID_SYS_APPLY 1036
	
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
			wxMenu* m_view;
			wxMenu* m_help;
			wxAuiNotebook* m_auinotebook;
			wxPanel* m_renderPage;
			wxToolBar* m_renderToolBar;
			wxStaticText* m_ThreadText;
			wxToolBar* m_viewerToolBar;
			wxPanel* m_logPage;
			wxTextCtrl* m_logTextCtrl;
			wxPanel* m_networkPage;
			wxToolBar* m_networkToolBab;
			wxStaticText* m_serverStaticText;
			wxTextCtrl* m_serverTextCtrl;
			wxBitmapButton* m_addServerButton;
			wxBitmapButton* m_removeServerButton;
			wxStaticText* m_updateStaticText;
			wxSpinCtrl* m_serverUpdateSpin;
			wxTreeCtrl* m_networkTreeCtrl;
			wxStatusBar* m_statusBar;
			
			// Virtual event handlers, overide them in your derived class
			virtual void OnExit( wxCloseEvent& event ){ event.Skip(); }
			virtual void OnOpen( wxCommandEvent& event ){ event.Skip(); }
			virtual void OnMenu( wxCommandEvent& event ){ event.Skip(); }
			virtual void OnSpin( wxSpinEvent& event ){ event.Skip(); }
			virtual void OnTreeSelChanged( wxTreeEvent& event ){ event.Skip(); }
			
		
		public:
			LuxMainFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("LuxRender"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1024,768 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
			~LuxMainFrame();
		
	};
	
	///////////////////////////////////////////////////////////////////////////////
	/// Class m_OptionsDialog
	///////////////////////////////////////////////////////////////////////////////
	class m_OptionsDialog : public wxDialog 
	{
		private:
		
		protected:
			wxNotebook* m_Options_notebook;
			wxPanel* m_ToneMappingPanel;
			wxChoice* m_TM_choice;
			wxStaticText* m_staticText4;
			
			wxSlider* m_RH_prescaleSlider;
			wxTextCtrl* m_RH_preText;
			wxStaticText* m_staticText5;
			
			wxSlider* m_RH_postscaleSlider;
			wxTextCtrl* m_RH_postText;
			wxStaticText* m_staticText6;
			
			wxSlider* m_RH_burnSlider;
			wxTextCtrl* m_RH_burnText;
			wxButton* m_TM_resetButton;
			
			wxButton* m_refreshButton;
			wxPanel* m_systemPanel;
			
			wxSpinCtrl* m_Display_spinCtrl;
			wxStaticText* m_staticText2;
			
			wxSpinCtrl* m_Write_spinCtrl;
			wxCheckListBox* m_writeOptions;
			
			wxButton* m_SysApplyButton;
			
			// Virtual event handlers, overide them in your derived class
			virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
			virtual void OnMenu( wxCommandEvent& event ){ event.Skip(); }
			virtual void OnScroll( wxScrollEvent& event ){ event.Skip(); }
			virtual void OnText( wxCommandEvent& event ){ event.Skip(); }
			virtual void OnSpin( wxSpinEvent& event ){ event.Skip(); }
			
		
		public:
			m_OptionsDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Luxrender Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxCLOSE_BOX );
			~m_OptionsDialog();
		
	};
	
} // namespace lux

#endif //__wxluxframe__
