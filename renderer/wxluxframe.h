///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
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
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/clrpicker.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/scrolwin.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/aui/auibook.h>
#include <wx/spinctrl.h>
#include <wx/treectrl.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/checklst.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

namespace lux
{
	#define ID_RESUMEITEM 1000
	#define ID_PAUSEITEM 1001
	#define ID_STOPITEM 1002
	#define ID_TOOL_BAR 1003
	#define ID_STATUS_BAR 1004
	#define ID_SIDE_PANE 1005
	#define ID_OPTIONS 1006
	#define ID_PAN_MODE 1007
	#define ID_ZOOM_MODE 1008
	#define ID_RENDER_COPY 1009
	#define ID_CLEAR_LOG 1010
	#define ID_FULL_SCREEN 1011
	#define ID_RESUMETOOL 1012
	#define ID_PAUSETOOL 1013
	#define ID_STOPTOOL 1014
	#define ID_NUM_THREADS 1015
	#define ID_ADD_THREAD 1016
	#define ID_REMOVE_THREAD 1017
	#define ID_PANTOOL 1018
	#define ID_ZOOMTOOL 1019
	#define ID_REFINETOOL 1020
	#define ID_LIGHTGROUPS 1021
	#define ID_RH_PRESCALE 1022
	#define ID_RH_PRESCALE_TEXT 1023
	#define ID_TONEMAP 1024
	#define ID_TONEMAPOPTIONSPANEL 1025
	#define ID_TM_KERNELCHOICE 1026
	#define ID_TONEMAPREINHARDOPTIONSPANEL 1027
	#define ID_TM_REINHARD_AUTOYWA 1028
	#define ID_TM_REINHARD_YWA 1029
	#define ID_TM_REINHARD_YWA_TEXT 1030
	#define ID_TM_REINHARD_PRESCALE 1031
	#define ID_TM_REINHARD_PRESCALE_TEXT 1032
	#define ID_TM_REINHARD_POSTSCALE 1033
	#define ID_TM_REINHARD_POSTSCALE_TEXT 1034
	#define ID_TM_REINHARD_BURN 1035
	#define ID_TM_REINHARD_BURN_TEXT 1036
	#define ID_TONEMAPLINEAROPTIONSPANEL 1037
	#define ID_TM_LINEAR_SENSITIVITY 1038
	#define ID_TM_LINEAR_SENSITIVITY_TEXT 1039
	#define ID_TM_LINEAR_EXPOSURE 1040
	#define ID_TM_LINEAR_EXPOSURE_TEXT 1041
	#define ID_TM_LINEAR_FSTOP 1042
	#define ID_TM_LINEAR_FSTOP_TEXT 1043
	#define ID_TM_LINEAR_GAMMA 1044
	#define ID_TM_LINEAR_GAMMA_TEXT 1045
	#define ID_TONEMAPCONTRASTOPTIONSPANEL 1046
	#define ID_TM_CONTRAST_YWA 1047
	#define ID_TM_CONTRAST_YWA_TEXT 1048
	#define ID_COLORSPACEOPTIONSPANEL 1049
	#define ID_TORGB_COLORSPACECHOICE 1050
	#define ID_TORGB_XWHITE 1051
	#define ID_TORGB_XWHITE_TEXT 1052
	#define ID_TORGB_YWHITE 1053
	#define ID_TORGB_YWHITE_TEXT 1054
	#define ID_TORGB_XRED 1055
	#define ID_TORGB_XRED_TEXT 1056
	#define ID_TORGB_YRED 1057
	#define ID_TORGB_YRED_TEXT 1058
	#define ID_TORGB_XGREEN 1059
	#define ID_TORGB_XGREEN_TEXT 1060
	#define ID_TORGB_YGREEN 1061
	#define ID_TORGB_YGREEN_TEXT 1062
	#define ID_TORGB_XBLUE 1063
	#define ID_TORGB_XBLUE_TEXT 1064
	#define ID_TORGB_YBLUE 1065
	#define ID_TORGB_YBLUE_TEXT 1066
	#define ID_GAMMAOPTIONSPANEL 1067
	#define ID_TORGB_GAMMA 1068
	#define ID_TORGB_GAMMA_TEXT 1069
	#define ID_TM_RESET 1070
	#define ID_AUTO_TONEMAP 1071
	#define ID_TM_APPLY 1072
	#define ID_SYSTEMOPTIONS 1073
	#define ID_ADD_SERVER 1074
	#define ID_REMOVE_SERVER 1075
	#define ID_SERVER_UPDATE_INT 1076
	#define ID_NETWORK_TREE 1077
	#define ID_SYS_DISPLAY_INT 1078
	#define ID_SYS_WRITE_INT 1079
	#define ID_WRITE_OPTIONS 1080
	#define ID_SYS_APPLY 1081
	
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
			wxAuiNotebook* m_outputNotebook;
			wxScrolledWindow* m_LightGroups;
			wxPanel* m_panel40;
			wxStaticBitmap* m_lightgroupBitmap;
			wxStaticText* m_tonemappingkernelText111;
			wxStaticText* m_tonemappingkernelText11;
			wxCheckBox* m_checkBox151;
			wxNotebook* m_notebook4;
			wxPanel* m_panel37;
			wxStaticText* m_prescaleText311;
			wxSlider* m_RH_prescaleSlider111;
			wxTextCtrl* m_RH_preText111;
			wxStaticText* m_prescaleText3111;
			wxColourPickerCtrl* m_colourPicker3;
			wxSlider* m_RH_prescaleSlider1111;
			wxTextCtrl* m_RH_preText1111;
			wxPanel* m_panel14;
			wxScrolledWindow* m_Tonemap;
			wxPanel* m_TonemapOptionsPanel;
			wxStaticBitmap* m_tonemapBitmap;
			wxStaticText* m_tonemappingkernelStaticText;
			wxChoice* m_TM_kernelChoice;
			wxPanel* m_TonemapReinhardOptionsPanel;
			wxStaticText* m_TM_Reinhard_YwaStaticText;
			wxCheckBox* m_TM_Reinhard_autoywaCheckbox;
			wxSlider* m_TM_Reinhard_ywaSlider;
			wxTextCtrl* m_TM_Reinhard_ywaText;
			wxStaticText* m_TM_Reinhard_prescaleStaticText;
			wxSlider* m_TM_Reinhard_prescaleSlider;
			wxTextCtrl* m_TM_Reinhard_prescaleText;
			wxStaticText* m_TM_Reinhard_postscaleStaticText;
			wxSlider* m_TM_Reinhard_postscaleSlider;
			wxTextCtrl* m_TM_Reinhard_postscaleText;
			wxStaticText* m_TM_Reinhard_burnStaticText;
			wxSlider* m_TM_Reinhard_burnSlider;
			wxTextCtrl* m_TM_Reinhard_burnText;
			wxPanel* m_TonemapLinearOptionsPanel;
			wxStaticText* m_TM_Linear_sensitivityStaticText;
			wxSlider* m_TM_Linear_sensitivitySlider;
			wxTextCtrl* m_TM_Linear_sensitivityText;
			wxStaticText* m_TM_Linear_exposureStaticText;
			wxSlider* m_TM_Linear_exposureSlider;
			wxTextCtrl* m_TM_Linear_exposureText;
			wxStaticText* m_TM_Linear_fstopStaticText;
			wxSlider* m_TM_Linear_fstopSlider;
			wxTextCtrl* m_TM_Linear_fstopText;
			wxStaticText* m_TM_Linear_gammaStaticText;
			wxSlider* m_TM_Linear_gammaSlider;
			wxTextCtrl* m_TM_Linear_gammaText;
			wxPanel* m_TonemapContrastOptionsPanel;
			wxStaticText* m_TM_contrast_YwaStaticText;
			wxSlider* m_TM_contrast_ywaSlider;
			wxTextCtrl* m_TM_contrast_ywaText;
			wxPanel* m_ColorSpaceOptionsPanel;
			wxStaticBitmap* m_colorspaceBitmap;
			wxStaticText* m_TORGB_colorspacepresetsStaticText;
			wxChoice* m_TORGB_colorspaceChoice;
			wxStaticText* m_TORGB_whitexyStaticText;
			wxSlider* m_TORGB_xwhiteSlider;
			wxTextCtrl* m_TORGB_xwhiteText;
			wxSlider* m_TORGB_ywhiteSlider;
			wxTextCtrl* m_TORGB_ywhiteText;
			wxStaticText* m_TORGB_rgbxyStaticText;
			wxSlider* m_TORGB_xredSlider;
			wxTextCtrl* m_TORGB_xredText;
			wxSlider* m_TORGB_yredSlider;
			wxTextCtrl* m_TORGB_yredText;
			wxSlider* m_TORGB_xgreenSlider;
			wxTextCtrl* m_TORGB_xgreenText;
			wxSlider* m_TORGB_ygreenSlider;
			wxTextCtrl* m_TORGB_ygreenText;
			wxSlider* m_TORGB_xblueSlider;
			wxTextCtrl* m_TORGB_xblueText;
			wxSlider* m_TORGB_yblueSlider;
			wxTextCtrl* m_TORGB_yblueText;
			wxPanel* m_GammaOptionsPanel;
			wxStaticBitmap* m_gammaBitmap;
			wxStaticText* m_TORGB_gammaStaticText;
			wxSlider* m_TORGB_gammaSlider;
			wxTextCtrl* m_TORGB_gammaText;
			wxButton* m_TM_resetButton;
			wxCheckBox* m_auto_tonemapCheckBox;
			wxButton* m_TM_applyButton;
			wxScrolledWindow* m_SystemOptions;
			wxCheckBox* m_checkBox1;
			wxStaticLine* m_staticline1;
			wxStaticText* m_staticText9;
			wxChoice* m_choice2;
			wxNotebook* m_outputNotebook2;
			wxPanel* m_logPage;
			wxTextCtrl* m_logTextCtrl;
			wxPanel* m_networkPage;
			wxToolBar* m_networkToolBar;
			wxStaticText* m_serverStaticText;
			wxTextCtrl* m_serverTextCtrl;
			wxStaticText* m_updateStaticText;
			wxSpinCtrl* m_serverUpdateSpin;
			wxTreeCtrl* m_networkTreeCtrl;
			wxStatusBar* m_statusBar;
			
			// Virtual event handlers, overide them in your derived class
			virtual void OnExit( wxCloseEvent& event ){ event.Skip(); }
			virtual void OnOpen( wxCommandEvent& event ){ event.Skip(); }
			virtual void OnMenu( wxCommandEvent& event ){ event.Skip(); }
			virtual void OnScroll( wxScrollEvent& event ){ event.Skip(); }
			virtual void OnText( wxCommandEvent& event ){ event.Skip(); }
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
			wxPanel* m_systemPanel;
			
			wxSpinCtrl* m_Display_spinCtrl;
			wxStaticText* m_staticText2;
			
			wxSpinCtrl* m_Write_spinCtrl;
			wxCheckListBox* m_writeOptions;
			
			wxButton* m_SysApplyButton;
			
			// Virtual event handlers, overide them in your derived class
			virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
			virtual void OnSpin( wxSpinEvent& event ){ event.Skip(); }
			virtual void OnMenu( wxCommandEvent& event ){ event.Skip(); }
			
		
		public:
			m_OptionsDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Luxrender Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxCLOSE_BOX );
			~m_OptionsDialog();
		
	};
	
} // namespace lux

#endif //__wxluxframe__
