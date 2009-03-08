///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 19 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wxluxframe.h"

#include "blank.xpm"

///////////////////////////////////////////////////////////////////////////
using namespace lux;

LuxMainFrame::LuxMainFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_menubar = new wxMenuBar( 0 );
	m_file = new wxMenu();
	wxMenuItem* m_open;
	m_open = new wxMenuItem( m_file, wxID_OPEN, wxString( wxT("&Open") ) + wxT('\t') + wxT("CTRL+O"), wxT("Open a scene"), wxITEM_NORMAL );
	m_file->Append( m_open );
	
	m_file->AppendSeparator();
	
	wxMenuItem* m_exit;
	m_exit = new wxMenuItem( m_file, wxID_EXIT, wxString( wxT("&Exit") ) + wxT('\t') + wxT("ALT+F4"), wxT("Exit LuxRender"), wxITEM_NORMAL );
	m_file->Append( m_exit );
	
	m_menubar->Append( m_file, wxT("&File") );
	
	m_render = new wxMenu();
	wxMenuItem* m_resume;
	m_resume = new wxMenuItem( m_render, ID_RESUMEITEM, wxString( wxT("&Resume") ) + wxT('\t') + wxT("CTRL+S"), wxT("Resume rendering"), wxITEM_NORMAL );
	m_render->Append( m_resume );
	
	wxMenuItem* m_pause;
	m_pause = new wxMenuItem( m_render, ID_PAUSEITEM, wxString( wxT("&Pause") ) + wxT('\t') + wxT("CTRL+T"), wxT("Pause rendering threads"), wxITEM_NORMAL );
	m_render->Append( m_pause );
	
	wxMenuItem* m_stop;
	m_stop = new wxMenuItem( m_render, ID_STOPITEM, wxString( wxT("S&top") ) + wxT('\t') + wxT("CTRL+C"), wxT("Stop current rendering at next valid point"), wxITEM_NORMAL );
	m_render->Append( m_stop );
	
	m_menubar->Append( m_render, wxT("&Render") );
	
	m_view = new wxMenu();
	wxMenuItem* m_toolBar;
	m_toolBar = new wxMenuItem( m_view, ID_TOOL_BAR, wxString( wxT("&Tool Bar") ) , wxT("Toggle the toolbar display"), wxITEM_CHECK );
	m_view->Append( m_toolBar );
	m_toolBar->Check( true );
	
	wxMenuItem* m_statusBarMenu;
	m_statusBarMenu = new wxMenuItem( m_view, ID_STATUS_BAR, wxString( wxT("&Status Bar") ) , wxT("Toggle the status bar display"), wxITEM_CHECK );
	m_view->Append( m_statusBarMenu );
	m_statusBarMenu->Check( true );
	
	wxMenuItem* m_sidePane;
	m_sidePane = new wxMenuItem( m_view, ID_SIDE_PANE, wxString( wxT("S&ide Pane") ) + wxT('\t') + wxT("CTRL+I"), wxT("Toggle the side pane display"), wxITEM_CHECK );
	m_view->Append( m_sidePane );
	m_sidePane->Check( true );
	
	m_view->AppendSeparator();
	
	wxMenuItem* m_panMode;
	m_panMode = new wxMenuItem( m_view, ID_PAN_MODE, wxString( wxT("&Pan Mode") ) + wxT('\t') + wxT("CTRL+P"), wxT("Pan rendering with the mouse "), wxITEM_RADIO );
	m_view->Append( m_panMode );
	
	wxMenuItem* m_zoomMode;
	m_zoomMode = new wxMenuItem( m_view, ID_ZOOM_MODE, wxString( wxT("&Zoom Mode") ) + wxT('\t') + wxT("CTRL+Z"), wxT("Zoom rendering by selecting a area with the mouse"), wxITEM_RADIO );
	m_view->Append( m_zoomMode );
	
	m_view->AppendSeparator();
	
	wxMenuItem* m_copy;
	m_copy = new wxMenuItem( m_view, ID_RENDER_COPY, wxString( wxT("&Copy") ) , wxT("Copy rendering image to the clipboard"), wxITEM_NORMAL );
	m_view->Append( m_copy );
	
	wxMenuItem* m_clearLog;
	m_clearLog = new wxMenuItem( m_view, ID_CLEAR_LOG, wxString( wxT("C&lear Log") ) , wxT("Clear the log window."), wxITEM_NORMAL );
	m_view->Append( m_clearLog );
	
	m_view->AppendSeparator();
	
	wxMenuItem* m_fullScreen;
	m_fullScreen = new wxMenuItem( m_view, ID_FULL_SCREEN, wxString( wxT("&Full Screen") ) + wxT('\t') + wxT("CTRL+F"), wxT("Switch to full screen rendering"), wxITEM_NORMAL );
	m_view->Append( m_fullScreen );
	
	m_menubar->Append( m_view, wxT("&View") );
	
	m_help = new wxMenu();
	wxMenuItem* m_about;
	m_about = new wxMenuItem( m_help, wxID_ABOUT, wxString( wxT("&About") ) + wxT('\t') + wxT("F1"), wxT("Show about dialog"), wxITEM_NORMAL );
	m_help->Append( m_about );
	
	m_menubar->Append( m_help, wxT("Help") );
	
	this->SetMenuBar( m_menubar );
	
	wxBoxSizer* bSizer;
	bSizer = new wxBoxSizer( wxVERTICAL );
	
	m_auinotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_MOVE|wxAUI_NB_TAB_SPLIT|wxAUI_NB_WINDOWLIST_BUTTON );
	m_renderPage = new wxPanel( m_auinotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bRenderSizer;
	bRenderSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bRenderToolSizer;
	bRenderToolSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_renderToolBar = new wxToolBar( m_renderPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL );
	m_renderToolBar->SetToolBitmapSize( wxSize( 16,16 ) );
	m_renderToolBar->AddTool( ID_RESUMETOOL, wxT("Resume"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Resume rendering"), wxT("Resume rendering") );
	m_renderToolBar->AddTool( ID_PAUSETOOL, wxT("Pause"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Pause current rendering"), wxT("Pause current rendering") );
	m_renderToolBar->AddTool( ID_STOPTOOL, wxT("Stop"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Stop current rendering"), wxT("Stop current rendering") );
	m_renderToolBar->AddSeparator();
	m_ThreadText = new wxStaticText( m_renderToolBar, ID_NUM_THREADS, wxT("Threads: 01  "), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT|wxNO_BORDER|wxTRANSPARENT_WINDOW );
	m_ThreadText->Wrap( -1 );
	m_ThreadText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	m_renderToolBar->AddControl( m_ThreadText );
	m_renderToolBar->AddSeparator();
	m_renderToolBar->AddTool( ID_ADD_THREAD, wxT("Add Thread"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Add a rendering thread"), wxT("Add a rendering thread") );
	m_renderToolBar->AddTool( ID_REMOVE_THREAD, wxT("Remove Thread"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Remove rendering thread"), wxT("Remove rendering thread") );
	m_renderToolBar->AddSeparator();
	m_renderToolBar->AddTool( ID_RENDER_COPY, wxT("Copy"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Copy rendering image to clipboard."), wxT("Copy rendering image to clipboard.") );
	m_renderToolBar->Realize();
	
	bRenderToolSizer->Add( m_renderToolBar, 1, wxEXPAND, 5 );
	
	m_viewerToolBar = new wxToolBar( m_renderPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL );
	m_viewerToolBar->SetToolBitmapSize( wxSize( 16,16 ) );
	m_viewerToolBar->AddTool( ID_PANTOOL, wxT("Pan"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_RADIO, wxT("Pan and Zoom"), wxT("Pan and Zoom") );
	m_viewerToolBar->AddTool( ID_ZOOMTOOL, wxT("Zoom"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_RADIO, wxT("Rectangle Zoom"), wxT("Rectangle Zoom") );
	m_viewerToolBar->AddTool( ID_REFINETOOL, wxT("Refine"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_RADIO, wxT("Refine Area (NOT FUNCTIONAL)"), wxT("Refine Area (NOT FUNCTIONAL)") );
	m_viewerToolBar->Realize();
	
	bRenderToolSizer->Add( m_viewerToolBar, 0, wxEXPAND, 5 );
	
	bRenderSizer->Add( bRenderToolSizer, 0, wxEXPAND, 0 );
	
	wxBoxSizer* bOutputDisplaySizer;
	bOutputDisplaySizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_outputNotebook = new wxAuiNotebook( m_renderPage, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_MOVE|wxAUI_NB_TAB_SPLIT|wxAUI_NB_WINDOWLIST_BUTTON );
	m_outputNotebook->SetMinSize( wxSize( 380,-1 ) );
	
	m_LightGroups = new wxScrolledWindow( m_outputNotebook, ID_LIGHTGROUPS, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxVSCROLL );
	m_LightGroups->SetScrollRate( 5, 5 );
	m_LightGroupsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_LightGroups->SetSizer( m_LightGroupsSizer );
	m_LightGroups->Layout();
	m_LightGroupsSizer->Fit( m_LightGroups );
	m_outputNotebook->AddPage( m_LightGroups, wxT("LightGroups"), false, wxNullBitmap );
	m_Tonemap = new wxScrolledWindow( m_outputNotebook, ID_TONEMAP, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxVSCROLL );
	m_Tonemap->SetScrollRate( 5, 5 );
	wxBoxSizer* bTonemapSizer;
	bTonemapSizer = new wxBoxSizer( wxVERTICAL );
	
	m_TonemapOptionsPanel = new wxPanel( m_Tonemap, ID_TONEMAPOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer33211;
	bSizer33211 = new wxBoxSizer( wxVERTICAL );
	
	m_Tab_ToneMapPanel = new wxPanel( m_TonemapOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_Tab_ToneMapPanel->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	
	wxBoxSizer* bSizer1031114;
	bSizer1031114 = new wxBoxSizer( wxHORIZONTAL );
	
	m_tonemapBitmap = new wxStaticBitmap( m_Tab_ToneMapPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1031114->Add( m_tonemapBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	m_ToneMapStaticText = new wxStaticText( m_Tab_ToneMapPanel, wxID_ANY, wxT("Tonemap"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ToneMapStaticText->Wrap( -1 );
	m_ToneMapStaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_ToneMapStaticText->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer1031114->Add( m_ToneMapStaticText, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer1031114->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer884;
	bSizer884 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_ToneMapToggleIcon = new wxStaticBitmap( m_Tab_ToneMapPanel, ID_TAB_TONEMAP_TOGGLE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer884->Add( m_Tab_ToneMapToggleIcon, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 1 );
	
	m_Tab_ToneMapIcon = new wxStaticBitmap( m_Tab_ToneMapPanel, ID_TAB_TONEMAP, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer884->Add( m_Tab_ToneMapIcon, 0, wxALL, 1 );
	
	bSizer1031114->Add( bSizer884, 0, wxEXPAND, 5 );
	
	m_Tab_ToneMapPanel->SetSizer( bSizer1031114 );
	m_Tab_ToneMapPanel->Layout();
	bSizer1031114->Fit( m_Tab_ToneMapPanel );
	bSizer33211->Add( m_Tab_ToneMapPanel, 0, wxEXPAND | wxALL, 2 );
	
	m_Tab_Control_ToneMapPanel = new wxPanel( m_TonemapOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
	wxBoxSizer* bSizer126;
	bSizer126 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer120;
	bSizer120 = new wxBoxSizer( wxHORIZONTAL );
	
	m_ToneMapKernelStaticText = new wxStaticText( m_Tab_Control_ToneMapPanel, wxID_ANY, wxT("Kernel"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ToneMapKernelStaticText->Wrap( -1 );
	m_ToneMapKernelStaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer120->Add( m_ToneMapKernelStaticText, 0, wxALL, 5 );
	
	wxString m_TM_kernelChoiceChoices[] = { wxT("Reinhard / non-Linear"), wxT("Linear"), wxT("Contrast"), wxT("MaxWhite") };
	int m_TM_kernelChoiceNChoices = sizeof( m_TM_kernelChoiceChoices ) / sizeof( wxString );
	m_TM_kernelChoice = new wxChoice( m_Tab_Control_ToneMapPanel, ID_TM_KERNELCHOICE, wxDefaultPosition, wxDefaultSize, m_TM_kernelChoiceNChoices, m_TM_kernelChoiceChoices, 0 );
	m_TM_kernelChoice->SetSelection( 0 );
	m_TM_kernelChoice->SetToolTip( wxT("Select type of Tone Mapping") );
	
	bSizer120->Add( m_TM_kernelChoice, 1, wxALL, 2 );
	
	bSizer126->Add( bSizer120, 1, wxEXPAND, 2 );
	
	m_TonemapReinhardOptionsPanel = new wxPanel( m_Tab_Control_ToneMapPanel, ID_TONEMAPREINHARDOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer33;
	bSizer33 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Reinhard_prescaleStaticText = new wxStaticText( m_TonemapReinhardOptionsPanel, wxID_ANY, wxT("Prescale "), wxDefaultPosition, wxSize( 50,-1 ), wxALIGN_LEFT );
	m_TM_Reinhard_prescaleStaticText->Wrap( -1 );
	bSizer10->Add( m_TM_Reinhard_prescaleStaticText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 2 );
	
	m_TM_Reinhard_prescaleSlider = new wxSlider( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_PRESCALE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Reinhard_prescaleSlider->SetToolTip( wxT("Reinhard Prescale") );
	
	bSizer10->Add( m_TM_Reinhard_prescaleSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Reinhard_prescaleText = new wxTextCtrl( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_PRESCALE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Reinhard_prescaleText->SetMaxLength( 5 ); 
	m_TM_Reinhard_prescaleText->SetToolTip( wxT("Please enter a new Reinhard Prescale value and press enter.") );
	
	bSizer10->Add( m_TM_Reinhard_prescaleText, 0, wxALIGN_CENTER, 0 );
	
	bSizer33->Add( bSizer10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Reinhard_postscaleStaticText = new wxStaticText( m_TonemapReinhardOptionsPanel, wxID_ANY, wxT("Postscale"), wxDefaultPosition, wxSize( 50,-1 ), wxALIGN_LEFT );
	m_TM_Reinhard_postscaleStaticText->Wrap( -1 );
	bSizer12->Add( m_TM_Reinhard_postscaleStaticText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 2 );
	
	m_TM_Reinhard_postscaleSlider = new wxSlider( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_POSTSCALE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Reinhard_postscaleSlider->SetToolTip( wxT("Reinhard Postscale") );
	
	bSizer12->Add( m_TM_Reinhard_postscaleSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Reinhard_postscaleText = new wxTextCtrl( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_POSTSCALE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Reinhard_postscaleText->SetMaxLength( 5 ); 
	m_TM_Reinhard_postscaleText->SetToolTip( wxT("Please enter a new Reinhard Postscale value and press enter.") );
	
	bSizer12->Add( m_TM_Reinhard_postscaleText, 0, wxALIGN_CENTER, 0 );
	
	bSizer33->Add( bSizer12, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Reinhard_burnStaticText = new wxStaticText( m_TonemapReinhardOptionsPanel, wxID_ANY, wxT("Burn"), wxDefaultPosition, wxSize( 50,-1 ), wxALIGN_LEFT );
	m_TM_Reinhard_burnStaticText->Wrap( -1 );
	bSizer13->Add( m_TM_Reinhard_burnStaticText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 2 );
	
	m_TM_Reinhard_burnSlider = new wxSlider( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_BURN, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Reinhard_burnSlider->SetToolTip( wxT("Reinhard Burn") );
	
	bSizer13->Add( m_TM_Reinhard_burnSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Reinhard_burnText = new wxTextCtrl( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_BURN_TEXT, wxT("6.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Reinhard_burnText->SetMaxLength( 5 ); 
	m_TM_Reinhard_burnText->SetToolTip( wxT("Please enter a new Reinhard Burn value and press enter.") );
	
	bSizer13->Add( m_TM_Reinhard_burnText, 0, wxALIGN_CENTER, 0 );
	
	bSizer33->Add( bSizer13, 0, wxEXPAND, 5 );
	
	m_TonemapReinhardOptionsPanel->SetSizer( bSizer33 );
	m_TonemapReinhardOptionsPanel->Layout();
	bSizer33->Fit( m_TonemapReinhardOptionsPanel );
	bSizer126->Add( m_TonemapReinhardOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_TonemapLinearOptionsPanel = new wxPanel( m_Tab_Control_ToneMapPanel, ID_TONEMAPLINEAROPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	m_TonemapLinearOptionsPanel->Hide();
	
	wxBoxSizer* bSizer331;
	bSizer331 = new wxBoxSizer( wxVERTICAL );
	
	m_TM_Linear_sensitivityStaticText = new wxStaticText( m_TonemapLinearOptionsPanel, wxID_ANY, wxT("Sensitivity"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Linear_sensitivityStaticText->Wrap( -1 );
	bSizer331->Add( m_TM_Linear_sensitivityStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1041;
	bSizer1041 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Linear_sensitivitySlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_SENSITIVITY, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Linear_sensitivitySlider->SetToolTip( wxT("Reinhard Prescale") );
	
	bSizer1041->Add( m_TM_Linear_sensitivitySlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Linear_sensitivityText = new wxTextCtrl( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_SENSITIVITY_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Linear_sensitivityText->SetMaxLength( 5 ); 
	m_TM_Linear_sensitivityText->SetToolTip( wxT("Please enter a new Reinhard Prescale value and press enter.") );
	
	bSizer1041->Add( m_TM_Linear_sensitivityText, 0, wxALIGN_CENTER, 0 );
	
	bSizer331->Add( bSizer1041, 0, wxEXPAND, 5 );
	
	m_TM_Linear_exposureStaticText = new wxStaticText( m_TonemapLinearOptionsPanel, wxID_ANY, wxT("Exposure"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Linear_exposureStaticText->Wrap( -1 );
	bSizer331->Add( m_TM_Linear_exposureStaticText, 0, wxALL|wxFIXED_MINSIZE, 5 );
	
	wxBoxSizer* bSizer105;
	bSizer105 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Linear_exposureSlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_EXPOSURE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Linear_exposureSlider->SetToolTip( wxT("Reinhard Prescale") );
	
	bSizer105->Add( m_TM_Linear_exposureSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Linear_exposureText = new wxTextCtrl( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_EXPOSURE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Linear_exposureText->SetMaxLength( 5 ); 
	m_TM_Linear_exposureText->SetToolTip( wxT("Please enter a new Reinhard Prescale value and press enter.") );
	
	bSizer105->Add( m_TM_Linear_exposureText, 0, wxALIGN_CENTER, 0 );
	
	bSizer331->Add( bSizer105, 0, wxEXPAND, 5 );
	
	m_TM_Linear_fstopStaticText = new wxStaticText( m_TonemapLinearOptionsPanel, wxID_ANY, wxT("FStop"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Linear_fstopStaticText->Wrap( -1 );
	bSizer331->Add( m_TM_Linear_fstopStaticText, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5 );
	
	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Linear_fstopSlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_FSTOP, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Linear_fstopSlider->SetToolTip( wxT("Reinhard Postscale") );
	
	bSizer121->Add( m_TM_Linear_fstopSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Linear_fstopText = new wxTextCtrl( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_FSTOP_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Linear_fstopText->SetMaxLength( 5 ); 
	m_TM_Linear_fstopText->SetToolTip( wxT("Please enter a new Reinhard Postscale value and press enter.") );
	
	bSizer121->Add( m_TM_Linear_fstopText, 0, wxALIGN_CENTER, 0 );
	
	bSizer331->Add( bSizer121, 0, wxEXPAND, 5 );
	
	m_TM_Linear_gammaStaticText = new wxStaticText( m_TonemapLinearOptionsPanel, wxID_ANY, wxT("Gamma"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Linear_gammaStaticText->Wrap( -1 );
	bSizer331->Add( m_TM_Linear_gammaStaticText, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5 );
	
	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Linear_gammaSlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_GAMMA, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Linear_gammaSlider->SetToolTip( wxT("Reinhard Burn") );
	
	bSizer131->Add( m_TM_Linear_gammaSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Linear_gammaText = new wxTextCtrl( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_GAMMA_TEXT, wxT("6.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Linear_gammaText->SetMaxLength( 5 ); 
	m_TM_Linear_gammaText->SetToolTip( wxT("Please enter a new Reinhard Burn value and press enter.") );
	
	bSizer131->Add( m_TM_Linear_gammaText, 0, wxALIGN_CENTER, 0 );
	
	bSizer331->Add( bSizer131, 0, wxEXPAND, 5 );
	
	m_TonemapLinearOptionsPanel->SetSizer( bSizer331 );
	m_TonemapLinearOptionsPanel->Layout();
	bSizer331->Fit( m_TonemapLinearOptionsPanel );
	bSizer126->Add( m_TonemapLinearOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_TonemapContrastOptionsPanel = new wxPanel( m_Tab_Control_ToneMapPanel, ID_TONEMAPCONTRASTOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	m_TonemapContrastOptionsPanel->Hide();
	
	wxBoxSizer* bSizer332;
	bSizer332 = new wxBoxSizer( wxVERTICAL );
	
	m_TM_contrast_YwaStaticText = new wxStaticText( m_TonemapContrastOptionsPanel, wxID_ANY, wxT("Ywa (Display/World Adaption Luminance)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_contrast_YwaStaticText->Wrap( -1 );
	bSizer332->Add( m_TM_contrast_YwaStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1042;
	bSizer1042 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_contrast_ywaSlider = new wxSlider( m_TonemapContrastOptionsPanel, ID_TM_CONTRAST_YWA, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_contrast_ywaSlider->SetToolTip( wxT("Reinhard Prescale") );
	
	bSizer1042->Add( m_TM_contrast_ywaSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_contrast_ywaText = new wxTextCtrl( m_TonemapContrastOptionsPanel, ID_TM_CONTRAST_YWA_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_contrast_ywaText->SetMaxLength( 5 ); 
	m_TM_contrast_ywaText->SetToolTip( wxT("Please enter a new Reinhard Prescale value and press enter.") );
	
	bSizer1042->Add( m_TM_contrast_ywaText, 0, wxALIGN_CENTER, 0 );
	
	bSizer332->Add( bSizer1042, 0, wxEXPAND, 5 );
	
	m_TonemapContrastOptionsPanel->SetSizer( bSizer332 );
	m_TonemapContrastOptionsPanel->Layout();
	bSizer332->Fit( m_TonemapContrastOptionsPanel );
	bSizer126->Add( m_TonemapContrastOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_Tab_Control_ToneMapPanel->SetSizer( bSizer126 );
	m_Tab_Control_ToneMapPanel->Layout();
	bSizer126->Fit( m_Tab_Control_ToneMapPanel );
	bSizer33211->Add( m_Tab_Control_ToneMapPanel, 1, wxEXPAND | wxALL, 0 );
	
	m_TonemapOptionsPanel->SetSizer( bSizer33211 );
	m_TonemapOptionsPanel->Layout();
	bSizer33211->Fit( m_TonemapOptionsPanel );
	bTonemapSizer->Add( m_TonemapOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_BloomOptionsPanel = new wxPanel( m_Tonemap, ID_BLOOMOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer33221;
	bSizer33221 = new wxBoxSizer( wxVERTICAL );
	
	m_Tab_LensEffectsPanel = new wxPanel( m_BloomOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_Tab_LensEffectsPanel->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	
	wxBoxSizer* bSizer103111;
	bSizer103111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bloomBitmap = new wxStaticBitmap( m_Tab_LensEffectsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer103111->Add( m_bloomBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	m_TORGB_lensfxStaticText = new wxStaticText( m_Tab_LensEffectsPanel, wxID_ANY, wxT("Lens Effects"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_lensfxStaticText->Wrap( -1 );
	m_TORGB_lensfxStaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_TORGB_lensfxStaticText->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer103111->Add( m_TORGB_lensfxStaticText, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer103111->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer88;
	bSizer88 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_LensEffectsToggleIcon = new wxStaticBitmap( m_Tab_LensEffectsPanel, ID_TAB_LENSEFFECTS_TOGGLE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_Tab_LensEffectsToggleIcon, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 1 );
	
	m_Tab_LensEffectsIcon = new wxStaticBitmap( m_Tab_LensEffectsPanel, ID_TAB_LENSEFFECTS, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_Tab_LensEffectsIcon, 0, wxALL, 1 );
	
	bSizer103111->Add( bSizer88, 0, wxEXPAND, 5 );
	
	m_Tab_LensEffectsPanel->SetSizer( bSizer103111 );
	m_Tab_LensEffectsPanel->Layout();
	bSizer103111->Fit( m_Tab_LensEffectsPanel );
	bSizer33221->Add( m_Tab_LensEffectsPanel, 0, wxEXPAND | wxALL, 2 );
	
	m_Tab_Control_LensEffectsPanel = new wxPanel( m_BloomOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
	wxBoxSizer* bSizer127;
	bSizer127 = new wxBoxSizer( wxVERTICAL );
	
	m_LensEffectsAuiNotebook = new wxAuiNotebook( m_Tab_Control_LensEffectsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_WINDOWLIST_BUTTON|wxDOUBLE_BORDER );
	m_LensEffectsAuiNotebook->SetMinSize( wxSize( -1,100 ) );
	
	m_bloomPanel = new wxPanel( m_LensEffectsAuiNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer74;
	bSizer74 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer1032;
	bSizer1032 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_bloomweightStaticText11 = new wxStaticText( m_bloomPanel, wxID_ANY, wxT("Radius"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_bloomweightStaticText11->Wrap( -1 );
	bSizer1032->Add( m_TORGB_bloomweightStaticText11, 0, wxALL, 5 );
	
	m_TORGB_bloomradiusSlider = new wxSlider( m_bloomPanel, ID_TORGB_BLOOMRADIUS, 35, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_bloomradiusSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer1032->Add( m_TORGB_bloomradiusSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TORGB_bloomradiusText = new wxTextCtrl( m_bloomPanel, ID_TORGB_BLOOMRADIUS_TEXT, wxT("0.07"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_bloomradiusText->SetMaxLength( 5 ); 
	m_TORGB_bloomradiusText->SetToolTip( wxT("Gamma Value") );
	
	bSizer1032->Add( m_TORGB_bloomradiusText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	m_computebloomlayer = new wxButton( m_bloomPanel, ID_COMPUTEBLOOMLAYER, wxT("Compute Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1032->Add( m_computebloomlayer, 0, wxALL, 5 );
	
	bSizer74->Add( bSizer1032, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10321;
	bSizer10321 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_bloomweightStaticText1 = new wxStaticText( m_bloomPanel, wxID_ANY, wxT("Amount"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_bloomweightStaticText1->Wrap( -1 );
	bSizer10321->Add( m_TORGB_bloomweightStaticText1, 0, wxALL, 5 );
	
	m_TORGB_bloomweightSlider = new wxSlider( m_bloomPanel, ID_TORGB_BLOOMWEIGHT, 128, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_bloomweightSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer10321->Add( m_TORGB_bloomweightSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TORGB_bloomweightText = new wxTextCtrl( m_bloomPanel, ID_TORGB_BLOOMWEIGHT_TEXT, wxT("0.25"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_bloomweightText->SetMaxLength( 5 ); 
	m_TORGB_bloomweightText->SetToolTip( wxT("Gamma Value") );
	
	bSizer10321->Add( m_TORGB_bloomweightText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer74->Add( bSizer10321, 1, wxEXPAND, 5 );
	
	m_bloomPanel->SetSizer( bSizer74 );
	m_bloomPanel->Layout();
	bSizer74->Fit( m_bloomPanel );
	m_LensEffectsAuiNotebook->AddPage( m_bloomPanel, wxT("Gaussian Bloom"), true, wxNullBitmap );
	m_vignettingPanel = new wxPanel( m_LensEffectsAuiNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer741;
	bSizer741 = new wxBoxSizer( wxVERTICAL );
	
	m_vignettingenabledCheckBox = new wxCheckBox( m_vignettingPanel, ID_VIGNETTING_ENABLED, wxT("Enabled"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer741->Add( m_vignettingenabledCheckBox, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1036;
	bSizer1036 = new wxBoxSizer( wxHORIZONTAL );
	
	m_vignettingamountStaticText = new wxStaticText( m_vignettingPanel, wxID_ANY, wxT("Amount"), wxDefaultPosition, wxSize( 50,-1 ), wxALIGN_LEFT );
	m_vignettingamountStaticText->Wrap( -1 );
	bSizer1036->Add( m_vignettingamountStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer79;
	bSizer79 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer80;
	bSizer80 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText39 = new wxStaticText( m_vignettingPanel, wxID_ANY, wxT("-1.0"), wxDefaultPosition, wxSize( 50,-1 ), wxALIGN_LEFT );
	m_staticText39->Wrap( -1 );
	m_staticText39->SetMinSize( wxSize( 50,-1 ) );
	
	bSizer80->Add( m_staticText39, 0, wxALL, 5 );
	
	m_staticText40 = new wxStaticText( m_vignettingPanel, wxID_ANY, wxT("0.0"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText40->Wrap( -1 );
	bSizer80->Add( m_staticText40, 1, wxALL, 5 );
	
	m_staticText41 = new wxStaticText( m_vignettingPanel, wxID_ANY, wxT("+1.0"), wxDefaultPosition, wxSize( 50,-1 ), wxALIGN_RIGHT );
	m_staticText41->Wrap( -1 );
	m_staticText41->SetMinSize( wxSize( 50,-1 ) );
	
	bSizer80->Add( m_staticText41, 0, wxALL, 5 );
	
	bSizer79->Add( bSizer80, 0, wxEXPAND, 5 );
	
	m_vignettingamountSlider = new wxSlider( m_vignettingPanel, ID_VIGNETTINGAMOUNT, 358, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_TOP );
	m_vignettingamountSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer79->Add( m_vignettingamountSlider, 1, wxALL|wxEXPAND, 2 );
	
	bSizer1036->Add( bSizer79, 1, wxEXPAND, 5 );
	
	m_vignettingamountText = new wxTextCtrl( m_vignettingPanel, ID_VIGNETTINGAMOUNT_TEXT, wxT("0.4"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_vignettingamountText->SetMaxLength( 5 ); 
	m_vignettingamountText->SetToolTip( wxT("Gamma Value") );
	
	bSizer1036->Add( m_vignettingamountText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer741->Add( bSizer1036, 1, wxEXPAND, 5 );
	
	m_vignettingPanel->SetSizer( bSizer741 );
	m_vignettingPanel->Layout();
	bSizer741->Fit( m_vignettingPanel );
	m_LensEffectsAuiNotebook->AddPage( m_vignettingPanel, wxT("Vignetting"), false, wxNullBitmap );
	
	bSizer127->Add( m_LensEffectsAuiNotebook, 1, wxEXPAND | wxALL, 2 );
	
	m_Tab_Control_LensEffectsPanel->SetSizer( bSizer127 );
	m_Tab_Control_LensEffectsPanel->Layout();
	bSizer127->Fit( m_Tab_Control_LensEffectsPanel );
	bSizer33221->Add( m_Tab_Control_LensEffectsPanel, 1, wxEXPAND | wxALL, 0 );
	
	m_BloomOptionsPanel->SetSizer( bSizer33221 );
	m_BloomOptionsPanel->Layout();
	bSizer33221->Fit( m_BloomOptionsPanel );
	bTonemapSizer->Add( m_BloomOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_ColorSpaceOptionsPanel = new wxPanel( m_Tonemap, ID_COLORSPACEOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3321;
	bSizer3321 = new wxBoxSizer( wxVERTICAL );
	
	m_Tab_ColorSpacePanel = new wxPanel( m_ColorSpaceOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_Tab_ColorSpacePanel->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	
	wxBoxSizer* bSizer1031113;
	bSizer1031113 = new wxBoxSizer( wxHORIZONTAL );
	
	m_colorspaceBitmap = new wxStaticBitmap( m_Tab_ColorSpacePanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1031113->Add( m_colorspaceBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	m_TORGB_colorspaceStaticText = new wxStaticText( m_Tab_ColorSpacePanel, wxID_ANY, wxT("Colorspace"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_colorspaceStaticText->Wrap( -1 );
	m_TORGB_colorspaceStaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_TORGB_colorspaceStaticText->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer1031113->Add( m_TORGB_colorspaceStaticText, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer1031113->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer883;
	bSizer883 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_ColorSpaceToggleIcon = new wxStaticBitmap( m_Tab_ColorSpacePanel, ID_TAB_COLORSPACE_TOGGLE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer883->Add( m_Tab_ColorSpaceToggleIcon, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 1 );
	
	m_Tab_ColorSpaceIcon = new wxStaticBitmap( m_Tab_ColorSpacePanel, ID_TAB_COLORSPACE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer883->Add( m_Tab_ColorSpaceIcon, 0, wxALL, 1 );
	
	bSizer1031113->Add( bSizer883, 0, wxEXPAND, 5 );
	
	m_Tab_ColorSpacePanel->SetSizer( bSizer1031113 );
	m_Tab_ColorSpacePanel->Layout();
	bSizer1031113->Fit( m_Tab_ColorSpacePanel );
	bSizer3321->Add( m_Tab_ColorSpacePanel, 0, wxEXPAND | wxALL, 2 );
	
	m_Tab_Control_ColorSpacePanel = new wxPanel( m_ColorSpaceOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
	wxBoxSizer* bSizer128;
	bSizer128 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer332111;
	bSizer332111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_colorspacepresetsStaticText = new wxStaticText( m_Tab_Control_ColorSpacePanel, wxID_ANY, wxT("Preset"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_colorspacepresetsStaticText->Wrap( -1 );
	bSizer332111->Add( m_TORGB_colorspacepresetsStaticText, 0, wxALL, 5 );
	
	wxString m_TORGB_colorspaceChoiceChoices[] = { wxT("sRGB - HDTV (ITU-R BT.709-5)"), wxT("ROMM RGB"), wxT("Adobe RGB 98"), wxT("Apple RGB"), wxT("NTSC (FCC 1953)"), wxT("NTSC (1979) (SMPTE C/-RP 145)"), wxT("PAL/SECAM (EBU 3213)"), wxT("CIE (1931) E") };
	int m_TORGB_colorspaceChoiceNChoices = sizeof( m_TORGB_colorspaceChoiceChoices ) / sizeof( wxString );
	m_TORGB_colorspaceChoice = new wxChoice( m_Tab_Control_ColorSpacePanel, ID_TORGB_COLORSPACECHOICE, wxDefaultPosition, wxDefaultSize, m_TORGB_colorspaceChoiceNChoices, m_TORGB_colorspaceChoiceChoices, 0 );
	m_TORGB_colorspaceChoice->SetSelection( 0 );
	m_TORGB_colorspaceChoice->SetToolTip( wxT("Select Color Space") );
	
	bSizer332111->Add( m_TORGB_colorspaceChoice, 1, wxALL, 2 );
	
	bSizer128->Add( bSizer332111, 0, wxEXPAND, 5 );
	
	m_ColorSpaceAuiNotebook = new wxAuiNotebook( m_Tab_Control_ColorSpacePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_WINDOWLIST_BUTTON|wxDOUBLE_BORDER );
	m_ColorSpaceAuiNotebook->SetMinSize( wxSize( -1,122 ) );
	
	m_ColorSpaceWhitepointPanel = new wxPanel( m_ColorSpaceAuiNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer212;
	bSizer212 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_whitexyStaticText = new wxStaticText( m_ColorSpaceWhitepointPanel, wxID_ANY, wxT("White XY"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_whitexyStaticText->Wrap( -1 );
	bSizer212->Add( m_TORGB_whitexyStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1012;
	bSizer1012 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xwhiteSlider = new wxSlider( m_ColorSpaceWhitepointPanel, ID_TORGB_XWHITE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xwhiteSlider->SetToolTip( wxT("White X") );
	
	bSizer1012->Add( m_TORGB_xwhiteSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xwhiteText = new wxTextCtrl( m_ColorSpaceWhitepointPanel, ID_TORGB_XWHITE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xwhiteText->SetMaxLength( 5 ); 
	m_TORGB_xwhiteText->SetToolTip( wxT("White X") );
	
	bSizer1012->Add( m_TORGB_xwhiteText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer212->Add( bSizer1012, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer1022;
	bSizer1022 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_ywhiteSlider = new wxSlider( m_ColorSpaceWhitepointPanel, ID_TORGB_YWHITE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_ywhiteSlider->SetToolTip( wxT("White Y") );
	
	bSizer1022->Add( m_TORGB_ywhiteSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_ywhiteText = new wxTextCtrl( m_ColorSpaceWhitepointPanel, ID_TORGB_YWHITE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_ywhiteText->SetMaxLength( 5 ); 
	m_TORGB_ywhiteText->SetToolTip( wxT("White Y") );
	
	bSizer1022->Add( m_TORGB_ywhiteText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer212->Add( bSizer1022, 1, wxEXPAND, 5 );
	
	m_ColorSpaceWhitepointPanel->SetSizer( bSizer212 );
	m_ColorSpaceWhitepointPanel->Layout();
	bSizer212->Fit( m_ColorSpaceWhitepointPanel );
	m_ColorSpaceAuiNotebook->AddPage( m_ColorSpaceWhitepointPanel, wxT("Whitepoint"), true, wxNullBitmap );
	m_ColorSpaceRGBPanel = new wxPanel( m_ColorSpaceAuiNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer114;
	bSizer114 = new wxBoxSizer( wxVERTICAL );
	
	m_TORGB_rgbxyStaticText = new wxStaticText( m_ColorSpaceRGBPanel, wxID_ANY, wxT("Red/Green/Blue XY"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_rgbxyStaticText->Wrap( -1 );
	bSizer114->Add( m_TORGB_rgbxyStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer213;
	bSizer213 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer1013;
	bSizer1013 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xredSlider = new wxSlider( m_ColorSpaceRGBPanel, ID_TORGB_XRED, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xredSlider->SetToolTip( wxT("Red X") );
	
	bSizer1013->Add( m_TORGB_xredSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xredText = new wxTextCtrl( m_ColorSpaceRGBPanel, ID_TORGB_XRED_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xredText->SetMaxLength( 5 ); 
	m_TORGB_xredText->SetToolTip( wxT("Red X") );
	
	bSizer1013->Add( m_TORGB_xredText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer213->Add( bSizer1013, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer1023;
	bSizer1023 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_yredSlider = new wxSlider( m_ColorSpaceRGBPanel, ID_TORGB_YRED, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_yredSlider->SetToolTip( wxT("Red Y") );
	
	bSizer1023->Add( m_TORGB_yredSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_yredText = new wxTextCtrl( m_ColorSpaceRGBPanel, ID_TORGB_YRED_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_yredText->SetMaxLength( 5 ); 
	m_TORGB_yredText->SetToolTip( wxT("Red Y") );
	
	bSizer1023->Add( m_TORGB_yredText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer213->Add( bSizer1023, 1, wxEXPAND, 5 );
	
	bSizer114->Add( bSizer213, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer2131;
	bSizer2131 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer10131;
	bSizer10131 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xgreenSlider = new wxSlider( m_ColorSpaceRGBPanel, ID_TORGB_XGREEN, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xgreenSlider->SetToolTip( wxT("Green X") );
	
	bSizer10131->Add( m_TORGB_xgreenSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xgreenText = new wxTextCtrl( m_ColorSpaceRGBPanel, ID_TORGB_XGREEN_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xgreenText->SetMaxLength( 5 ); 
	m_TORGB_xgreenText->SetToolTip( wxT("Green X") );
	
	bSizer10131->Add( m_TORGB_xgreenText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2131->Add( bSizer10131, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10231;
	bSizer10231 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_ygreenSlider = new wxSlider( m_ColorSpaceRGBPanel, ID_TORGB_YGREEN, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_ygreenSlider->SetToolTip( wxT("Green Y") );
	
	bSizer10231->Add( m_TORGB_ygreenSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_ygreenText = new wxTextCtrl( m_ColorSpaceRGBPanel, ID_TORGB_YGREEN_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_ygreenText->SetMaxLength( 5 ); 
	m_TORGB_ygreenText->SetToolTip( wxT("Green Y") );
	
	bSizer10231->Add( m_TORGB_ygreenText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2131->Add( bSizer10231, 1, wxEXPAND, 5 );
	
	bSizer114->Add( bSizer2131, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer2132;
	bSizer2132 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer10132;
	bSizer10132 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xblueSlider = new wxSlider( m_ColorSpaceRGBPanel, ID_TORGB_XBLUE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xblueSlider->SetToolTip( wxT("Blue X") );
	
	bSizer10132->Add( m_TORGB_xblueSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xblueText = new wxTextCtrl( m_ColorSpaceRGBPanel, ID_TORGB_XBLUE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xblueText->SetMaxLength( 5 ); 
	m_TORGB_xblueText->SetToolTip( wxT("Blue X") );
	
	bSizer10132->Add( m_TORGB_xblueText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2132->Add( bSizer10132, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10232;
	bSizer10232 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_yblueSlider = new wxSlider( m_ColorSpaceRGBPanel, ID_TORGB_YBLUE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_yblueSlider->SetToolTip( wxT("Blue Y") );
	
	bSizer10232->Add( m_TORGB_yblueSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_yblueText = new wxTextCtrl( m_ColorSpaceRGBPanel, ID_TORGB_YBLUE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_yblueText->SetMaxLength( 5 ); 
	m_TORGB_yblueText->SetToolTip( wxT("Blue Y") );
	
	bSizer10232->Add( m_TORGB_yblueText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2132->Add( bSizer10232, 1, wxEXPAND, 5 );
	
	bSizer114->Add( bSizer2132, 1, wxEXPAND, 5 );
	
	m_ColorSpaceRGBPanel->SetSizer( bSizer114 );
	m_ColorSpaceRGBPanel->Layout();
	bSizer114->Fit( m_ColorSpaceRGBPanel );
	m_ColorSpaceAuiNotebook->AddPage( m_ColorSpaceRGBPanel, wxT("RGB"), false, wxNullBitmap );
	
	bSizer128->Add( m_ColorSpaceAuiNotebook, 0, wxALL|wxEXPAND, 2 );
	
	m_Tab_Control_ColorSpacePanel->SetSizer( bSizer128 );
	m_Tab_Control_ColorSpacePanel->Layout();
	bSizer128->Fit( m_Tab_Control_ColorSpacePanel );
	bSizer3321->Add( m_Tab_Control_ColorSpacePanel, 1, wxEXPAND | wxALL, 0 );
	
	m_ColorSpaceOptionsPanel->SetSizer( bSizer3321 );
	m_ColorSpaceOptionsPanel->Layout();
	bSizer3321->Fit( m_ColorSpaceOptionsPanel );
	bTonemapSizer->Add( m_ColorSpaceOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_GammaOptionsPanel = new wxPanel( m_Tonemap, ID_GAMMAOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3322;
	bSizer3322 = new wxBoxSizer( wxVERTICAL );
	
	m_Tab_GammaPanel = new wxPanel( m_GammaOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_Tab_GammaPanel->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	
	wxBoxSizer* bSizer1031112;
	bSizer1031112 = new wxBoxSizer( wxHORIZONTAL );
	
	m_gammaBitmap = new wxStaticBitmap( m_Tab_GammaPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1031112->Add( m_gammaBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	m_GammaStaticText = new wxStaticText( m_Tab_GammaPanel, wxID_ANY, wxT("Gamma"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GammaStaticText->Wrap( -1 );
	m_GammaStaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_GammaStaticText->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer1031112->Add( m_GammaStaticText, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer1031112->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer882;
	bSizer882 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_GammaToggleIcon = new wxStaticBitmap( m_Tab_GammaPanel, ID_TAB_GAMMA_TOGGLE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer882->Add( m_Tab_GammaToggleIcon, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 1 );
	
	m_Tab_GammaIcon = new wxStaticBitmap( m_Tab_GammaPanel, ID_TAB_GAMMA, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer882->Add( m_Tab_GammaIcon, 0, wxALL, 1 );
	
	bSizer1031112->Add( bSizer882, 0, wxEXPAND, 5 );
	
	m_Tab_GammaPanel->SetSizer( bSizer1031112 );
	m_Tab_GammaPanel->Layout();
	bSizer1031112->Fit( m_Tab_GammaPanel );
	bSizer3322->Add( m_Tab_GammaPanel, 0, wxEXPAND | wxALL, 2 );
	
	m_Tab_Control_GammaPanel = new wxPanel( m_GammaOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
	wxBoxSizer* bSizer103;
	bSizer103 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_gammaSlider = new wxSlider( m_Tab_Control_GammaPanel, ID_TORGB_GAMMA, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_gammaSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer103->Add( m_TORGB_gammaSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TORGB_gammaText = new wxTextCtrl( m_Tab_Control_GammaPanel, ID_TORGB_GAMMA_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_gammaText->SetMaxLength( 5 ); 
	m_TORGB_gammaText->SetToolTip( wxT("Gamma Value") );
	
	bSizer103->Add( m_TORGB_gammaText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	m_Tab_Control_GammaPanel->SetSizer( bSizer103 );
	m_Tab_Control_GammaPanel->Layout();
	bSizer103->Fit( m_Tab_Control_GammaPanel );
	bSizer3322->Add( m_Tab_Control_GammaPanel, 1, wxEXPAND | wxALL, 0 );
	
	m_GammaOptionsPanel->SetSizer( bSizer3322 );
	m_GammaOptionsPanel->Layout();
	bSizer3322->Fit( m_GammaOptionsPanel );
	bTonemapSizer->Add( m_GammaOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_HistogramPanel = new wxPanel( m_Tonemap, ID_HISTOGRAMPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer83;
	bSizer83 = new wxBoxSizer( wxVERTICAL );
	
	m_Tab_HistogramPanel = new wxPanel( m_HistogramPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_Tab_HistogramPanel->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	
	wxBoxSizer* bSizer1031115;
	bSizer1031115 = new wxBoxSizer( wxHORIZONTAL );
	
	m_histogramBitmap = new wxStaticBitmap( m_Tab_HistogramPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1031115->Add( m_histogramBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	m_HistogramstaticText = new wxStaticText( m_Tab_HistogramPanel, wxID_ANY, wxT("HDR Histogram"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_HistogramstaticText->Wrap( -1 );
	m_HistogramstaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_HistogramstaticText->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer1031115->Add( m_HistogramstaticText, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer1031115->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer885;
	bSizer885 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_HistogramToggleIcon = new wxStaticBitmap( m_Tab_HistogramPanel, ID_TAB_HISTOGRAM_TOGGLE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer885->Add( m_Tab_HistogramToggleIcon, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 1 );
	
	m_Tab_HistogramIcon = new wxStaticBitmap( m_Tab_HistogramPanel, ID_TAB_HISTOGRAM, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer885->Add( m_Tab_HistogramIcon, 0, wxALL, 1 );
	
	bSizer1031115->Add( bSizer885, 0, wxEXPAND, 5 );
	
	m_Tab_HistogramPanel->SetSizer( bSizer1031115 );
	m_Tab_HistogramPanel->Layout();
	bSizer1031115->Fit( m_Tab_HistogramPanel );
	bSizer83->Add( m_Tab_HistogramPanel, 0, wxEXPAND | wxALL, 2 );
	
	m_Tab_Control_HistogramPanel = new wxPanel( m_HistogramPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
	wxBoxSizer* bSizer125;
	bSizer125 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer95;
	bSizer95 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText43 = new wxStaticText( m_Tab_Control_HistogramPanel, wxID_ANY, wxT("Input channel:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText43->Wrap( -1 );
	bSizer95->Add( m_staticText43, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	wxString m_Histogram_ChoiceChoices[] = { wxT("RGB ADD"), wxT("RGB"), wxT("Red"), wxT("Green"), wxT("Blue"), wxT("Value") };
	int m_Histogram_ChoiceNChoices = sizeof( m_Histogram_ChoiceChoices ) / sizeof( wxString );
	m_Histogram_Choice = new wxChoice( m_Tab_Control_HistogramPanel, ID_HISTOGRAM_CHANNEL, wxDefaultPosition, wxDefaultSize, m_Histogram_ChoiceNChoices, m_Histogram_ChoiceChoices, 0 );
	m_Histogram_Choice->SetSelection( 0 );
	bSizer95->Add( m_Histogram_Choice, 1, wxALL, 2 );
	
	
	bSizer95->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText431 = new wxStaticText( m_Tab_Control_HistogramPanel, wxID_ANY, wxT("Output:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText431->Wrap( -1 );
	bSizer95->Add( m_staticText431, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_HistogramLogCheckBox = new wxCheckBox( m_Tab_Control_HistogramPanel, ID_HISTOGRAM_LOG, wxT("Log"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer95->Add( m_HistogramLogCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	bSizer125->Add( bSizer95, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer96;
	bSizer96 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizer125->Add( bSizer96, 1, wxEXPAND, 5 );
	
	m_Tab_Control_HistogramPanel->SetSizer( bSizer125 );
	m_Tab_Control_HistogramPanel->Layout();
	bSizer125->Fit( m_Tab_Control_HistogramPanel );
	bSizer83->Add( m_Tab_Control_HistogramPanel, 1, wxEXPAND | wxALL, 0 );
	
	m_HistogramPanel->SetSizer( bSizer83 );
	m_HistogramPanel->Layout();
	bSizer83->Fit( m_HistogramPanel );
	bTonemapSizer->Add( m_HistogramPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_NoiseOptionsPanel = new wxPanel( m_Tonemap, ID_NOISEOPTIONSPANEL, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer33222;
	bSizer33222 = new wxBoxSizer( wxVERTICAL );
	
	m_Tab_LensEffectsPanel1 = new wxPanel( m_NoiseOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_Tab_LensEffectsPanel1->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	
	wxBoxSizer* bSizer1031111;
	bSizer1031111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_NoiseReductionBitmap = new wxStaticBitmap( m_Tab_LensEffectsPanel1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1031111->Add( m_NoiseReductionBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	m_NoiseReductionStaticText = new wxStaticText( m_Tab_LensEffectsPanel1, wxID_ANY, wxT("Noise Reduction"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_NoiseReductionStaticText->Wrap( -1 );
	m_NoiseReductionStaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_NoiseReductionStaticText->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer1031111->Add( m_NoiseReductionStaticText, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer1031111->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer881;
	bSizer881 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_NoiseReductionToggleIcon = new wxStaticBitmap( m_Tab_LensEffectsPanel1, ID_TAB_NOISEREDUCTION_TOGGLE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer881->Add( m_Tab_NoiseReductionToggleIcon, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 1 );
	
	m_Tab_NoiseReductionIcon = new wxStaticBitmap( m_Tab_LensEffectsPanel1, ID_TAB_NOISEREDUCTION, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer881->Add( m_Tab_NoiseReductionIcon, 0, wxALL, 1 );
	
	bSizer1031111->Add( bSizer881, 0, wxEXPAND, 5 );
	
	m_Tab_LensEffectsPanel1->SetSizer( bSizer1031111 );
	m_Tab_LensEffectsPanel1->Layout();
	bSizer1031111->Fit( m_Tab_LensEffectsPanel1 );
	bSizer33222->Add( m_Tab_LensEffectsPanel1, 0, wxEXPAND | wxALL, 2 );
	
	m_Tab_Control_NoiseReductionPanel = new wxPanel( m_NoiseOptionsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
	wxBoxSizer* bSizer130;
	bSizer130 = new wxBoxSizer( wxVERTICAL );
	
	m_NoiseReductionAuiNotebook = new wxAuiNotebook( m_Tab_Control_NoiseReductionPanel, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxAUI_NB_WINDOWLIST_BUTTON|wxDOUBLE_BORDER );
	m_NoiseReductionAuiNotebook->SetMinSize( wxSize( -1,150 ) );
	
	m_GREYCPanel = new wxPanel( m_NoiseReductionAuiNotebook, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer65;
	bSizer65 = new wxBoxSizer( wxVERTICAL );
	
	m_GREYCNotebook = new wxNotebook( m_GREYCPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT );
	m_GREYCRegPanel = new wxPanel( m_GREYCNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer66;
	bSizer66 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer10342;
	bSizer10342 = new wxBoxSizer( wxHORIZONTAL );
	
	m_greyc_EnabledCheckBox = new wxCheckBox( m_GREYCRegPanel, ID_GREYC_ENABLED, wxT("Enabled"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer10342->Add( m_greyc_EnabledCheckBox, 0, wxALL, 5 );
	
	m_greyc_fastapproxCheckBox = new wxCheckBox( m_GREYCRegPanel, ID_GREYC_FASTAPPROX, wxT("Fast approximation"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_greyc_fastapproxCheckBox->SetValue(true);
	
	bSizer10342->Add( m_greyc_fastapproxCheckBox, 1, wxALL, 5 );
	
	bSizer66->Add( bSizer10342, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer1034;
	bSizer1034 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCIterationsStaticText = new wxStaticText( m_GREYCRegPanel, wxID_ANY, wxT("Iterations"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCIterationsStaticText->Wrap( -1 );
	bSizer1034->Add( m_GREYCIterationsStaticText, 0, wxALL, 5 );
	
	m_greyc_iterationsSlider = new wxSlider( m_GREYCRegPanel, ID_GREYC_ITERATIONS, 1, 1, 16, wxDefaultPosition, wxSize( -1,-1 ), wxSL_AUTOTICKS|wxSL_HORIZONTAL );
	m_greyc_iterationsSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer1034->Add( m_greyc_iterationsSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_iterationsText = new wxTextCtrl( m_GREYCRegPanel, ID_GREYC_ITERATIONS_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_iterationsText->SetMaxLength( 5 ); 
	m_greyc_iterationsText->SetToolTip( wxT("Gamma Value") );
	
	bSizer1034->Add( m_greyc_iterationsText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer66->Add( bSizer1034, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer1035;
	bSizer1035 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCamplitureStaticText = new wxStaticText( m_GREYCRegPanel, wxID_ANY, wxT("Amplitude"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCamplitureStaticText->Wrap( -1 );
	bSizer1035->Add( m_GREYCamplitureStaticText, 0, wxALL, 5 );
	
	m_greyc_amplitudeSlider = new wxSlider( m_GREYCRegPanel, ID_GREYC_AMPLITUDE, 102, 1, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_amplitudeSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer1035->Add( m_greyc_amplitudeSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_amplitudeText = new wxTextCtrl( m_GREYCRegPanel, ID_GREYC_AMPLITUDE_TEXT, wxT("40.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_amplitudeText->SetMaxLength( 5 ); 
	m_greyc_amplitudeText->SetToolTip( wxT("Gamma Value") );
	
	bSizer1035->Add( m_greyc_amplitudeText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer66->Add( bSizer1035, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10352;
	bSizer10352 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCgaussprecStaticText = new wxStaticText( m_GREYCRegPanel, wxID_ANY, wxT("Gaussian Precision"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCgaussprecStaticText->Wrap( -1 );
	bSizer10352->Add( m_GREYCgaussprecStaticText, 0, wxALL, 5 );
	
	m_greyc_gausprecSlider = new wxSlider( m_GREYCRegPanel, ID_GREYC_GAUSSPREC, 85, 1, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_gausprecSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer10352->Add( m_greyc_gausprecSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_gaussprecText = new wxTextCtrl( m_GREYCRegPanel, ID_GREYC_GAUSSPREC_TEXT, wxT("2.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_gaussprecText->SetMaxLength( 5 ); 
	m_greyc_gaussprecText->SetToolTip( wxT("Gamma Value") );
	
	bSizer10352->Add( m_greyc_gaussprecText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer66->Add( bSizer10352, 0, wxEXPAND, 5 );
	
	m_GREYCRegPanel->SetSizer( bSizer66 );
	m_GREYCRegPanel->Layout();
	bSizer66->Fit( m_GREYCRegPanel );
	m_GREYCNotebook->AddPage( m_GREYCRegPanel, wxT("regularization"), true );
	m_GREYCFilterPanel = new wxPanel( m_GREYCNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer661;
	bSizer661 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer10341;
	bSizer10341 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCAlphaStaticText = new wxStaticText( m_GREYCFilterPanel, wxID_ANY, wxT("Alpha"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCAlphaStaticText->Wrap( -1 );
	bSizer10341->Add( m_GREYCAlphaStaticText, 0, wxALL, 5 );
	
	m_greyc_alphaSlider = new wxSlider( m_GREYCFilterPanel, ID_GREYC_ALPHA, 34, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_alphaSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer10341->Add( m_greyc_alphaSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_alphaText = new wxTextCtrl( m_GREYCFilterPanel, ID_GREYC_ALPHA_TEXT, wxT("0.8"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_alphaText->SetMaxLength( 5 ); 
	m_greyc_alphaText->SetToolTip( wxT("Gamma Value") );
	
	bSizer10341->Add( m_greyc_alphaText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer661->Add( bSizer10341, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10351;
	bSizer10351 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCSigmaStaticText = new wxStaticText( m_GREYCFilterPanel, wxID_ANY, wxT("Sigma"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCSigmaStaticText->Wrap( -1 );
	bSizer10351->Add( m_GREYCSigmaStaticText, 0, wxALL, 5 );
	
	m_greyc_sigmaSlider = new wxSlider( m_GREYCFilterPanel, ID_GREYC_SIGMA, 47, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_sigmaSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer10351->Add( m_greyc_sigmaSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_sigmaText = new wxTextCtrl( m_GREYCFilterPanel, ID_GREYC_SIGMA_TEXT, wxT("1.1"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_sigmaText->SetMaxLength( 5 ); 
	m_greyc_sigmaText->SetToolTip( wxT("Gamma Value") );
	
	bSizer10351->Add( m_greyc_sigmaText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer661->Add( bSizer10351, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer103411;
	bSizer103411 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCsharpnessStaticText = new wxStaticText( m_GREYCFilterPanel, wxID_ANY, wxT("Sharpness"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCsharpnessStaticText->Wrap( -1 );
	bSizer103411->Add( m_GREYCsharpnessStaticText, 0, wxALL, 5 );
	
	m_greyc_sharpnessSlider = new wxSlider( m_GREYCFilterPanel, ID_GREYC_SHARPNESS, 205, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_sharpnessSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer103411->Add( m_greyc_sharpnessSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_sharpnessText = new wxTextCtrl( m_GREYCFilterPanel, ID_GREYC_SHARPNESS_TEXT, wxT("0.8"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_sharpnessText->SetMaxLength( 5 ); 
	m_greyc_sharpnessText->SetToolTip( wxT("Gamma Value") );
	
	bSizer103411->Add( m_greyc_sharpnessText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer661->Add( bSizer103411, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer103412;
	bSizer103412 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCAnisoStaticText = new wxStaticText( m_GREYCFilterPanel, wxID_ANY, wxT("Aniso"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCAnisoStaticText->Wrap( -1 );
	bSizer103412->Add( m_GREYCAnisoStaticText, 0, wxALL, 5 );
	
	m_greyc_anisoSlider = new wxSlider( m_GREYCFilterPanel, ID_GREYC_ANISO, 102, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_anisoSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer103412->Add( m_greyc_anisoSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_anisoText = new wxTextCtrl( m_GREYCFilterPanel, ID_GREYC_ANISO_TEXT, wxT("0.2"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_anisoText->SetMaxLength( 5 ); 
	m_greyc_anisoText->SetToolTip( wxT("Gamma Value") );
	
	bSizer103412->Add( m_greyc_anisoText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer661->Add( bSizer103412, 0, wxEXPAND, 5 );
	
	m_GREYCFilterPanel->SetSizer( bSizer661 );
	m_GREYCFilterPanel->Layout();
	bSizer661->Fit( m_GREYCFilterPanel );
	m_GREYCNotebook->AddPage( m_GREYCFilterPanel, wxT("filter"), false );
	m_GREYCAdvancedPanel = new wxPanel( m_GREYCNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6611;
	bSizer6611 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer103413;
	bSizer103413 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCspatialStaticText = new wxStaticText( m_GREYCAdvancedPanel, wxID_ANY, wxT("Spatial Integration"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCspatialStaticText->Wrap( -1 );
	bSizer103413->Add( m_GREYCspatialStaticText, 0, wxALL, 5 );
	
	m_greyc_spatialSlider = new wxSlider( m_GREYCAdvancedPanel, ID_GREYC_SPATIAL, 410, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_spatialSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer103413->Add( m_greyc_spatialSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_spatialText = new wxTextCtrl( m_GREYCAdvancedPanel, ID_GREYC_SPATIAL_TEXT, wxT("0.8"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_spatialText->SetMaxLength( 5 ); 
	m_greyc_spatialText->SetToolTip( wxT("Gamma Value") );
	
	bSizer103413->Add( m_greyc_spatialText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer6611->Add( bSizer103413, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer103511;
	bSizer103511 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCangularStaticText = new wxStaticText( m_GREYCAdvancedPanel, wxID_ANY, wxT("Angular Integration"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCangularStaticText->Wrap( -1 );
	bSizer103511->Add( m_GREYCangularStaticText, 0, wxALL, 5 );
	
	m_greyc_angularSlider = new wxSlider( m_GREYCAdvancedPanel, ID_GREYC_ANGULAR, 171, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_greyc_angularSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer103511->Add( m_greyc_angularSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_greyc_angularText = new wxTextCtrl( m_GREYCAdvancedPanel, ID_GREYC_ANGULAR_TEXT, wxT("30.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_greyc_angularText->SetMaxLength( 5 ); 
	m_greyc_angularText->SetToolTip( wxT("Gamma Value") );
	
	bSizer103511->Add( m_greyc_angularText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer6611->Add( bSizer103511, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer1034111;
	bSizer1034111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_GREYCinterpolationStaticText = new wxStaticText( m_GREYCAdvancedPanel, wxID_ANY, wxT("Interpolation Type"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_GREYCinterpolationStaticText->Wrap( -1 );
	bSizer1034111->Add( m_GREYCinterpolationStaticText, 0, wxALL, 5 );
	
	wxString m_GREYCinterpolationChoiceChoices[] = { wxT("Nearest-Neighbour"), wxT("Linear"), wxT("Runge-Kutta") };
	int m_GREYCinterpolationChoiceNChoices = sizeof( m_GREYCinterpolationChoiceChoices ) / sizeof( wxString );
	m_GREYCinterpolationChoice = new wxChoice( m_GREYCAdvancedPanel, ID_GREYC_INTERPOLATIONCHOICE, wxDefaultPosition, wxDefaultSize, m_GREYCinterpolationChoiceNChoices, m_GREYCinterpolationChoiceChoices, 0 );
	m_GREYCinterpolationChoice->SetSelection( 0 );
	bSizer1034111->Add( m_GREYCinterpolationChoice, 1, wxALL, 2 );
	
	bSizer6611->Add( bSizer1034111, 0, wxEXPAND, 5 );
	
	m_GREYCAdvancedPanel->SetSizer( bSizer6611 );
	m_GREYCAdvancedPanel->Layout();
	bSizer6611->Fit( m_GREYCAdvancedPanel );
	m_GREYCNotebook->AddPage( m_GREYCAdvancedPanel, wxT("adv"), false );
	
	bSizer65->Add( m_GREYCNotebook, 1, wxEXPAND | wxALL, 1 );
	
	m_GREYCPanel->SetSizer( bSizer65 );
	m_GREYCPanel->Layout();
	bSizer65->Fit( m_GREYCPanel );
	m_NoiseReductionAuiNotebook->AddPage( m_GREYCPanel, wxT("GREYCStoration"), true, wxNullBitmap );
	m_ChiuPanel = new wxPanel( m_NoiseReductionAuiNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_ChiuPanel->Hide();
	
	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxHORIZONTAL );
	
	m_enableChiuCheckBox = new wxCheckBox( m_ChiuPanel, wxID_ANY, wxT("Enabled"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer61->Add( m_enableChiuCheckBox, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1033;
	bSizer1033 = new wxBoxSizer( wxHORIZONTAL );
	
	m_ChiuRadiusSlider = new wxSlider( m_ChiuPanel, ID_CHIU_RADIUS, 3, 1, 9, wxDefaultPosition, wxSize( -1,-1 ), wxSL_AUTOTICKS|wxSL_HORIZONTAL );
	m_ChiuRadiusSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer1033->Add( m_ChiuRadiusSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TORGB_gammaText1 = new wxTextCtrl( m_ChiuPanel, ID_TORGB_GAMMA_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_gammaText1->SetMaxLength( 5 ); 
	m_TORGB_gammaText1->SetToolTip( wxT("Gamma Value") );
	
	bSizer1033->Add( m_TORGB_gammaText1, 0, wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer61->Add( bSizer1033, 1, wxEXPAND, 5 );
	
	m_checkBox6 = new wxCheckBox( m_ChiuPanel, wxID_ANY, wxT("Include Center"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer61->Add( m_checkBox6, 0, wxALL, 5 );
	
	m_ChiuPanel->SetSizer( bSizer61 );
	m_ChiuPanel->Layout();
	bSizer61->Fit( m_ChiuPanel );
	m_NoiseReductionAuiNotebook->AddPage( m_ChiuPanel, wxT("Chiu"), false, wxNullBitmap );
	
	bSizer130->Add( m_NoiseReductionAuiNotebook, 1, wxALL|wxEXPAND, 2 );
	
	m_Tab_Control_NoiseReductionPanel->SetSizer( bSizer130 );
	m_Tab_Control_NoiseReductionPanel->Layout();
	bSizer130->Fit( m_Tab_Control_NoiseReductionPanel );
	bSizer33222->Add( m_Tab_Control_NoiseReductionPanel, 1, wxEXPAND | wxALL, 0 );
	
	m_NoiseOptionsPanel->SetSizer( bSizer33222 );
	m_NoiseOptionsPanel->Layout();
	bSizer33222->Fit( m_NoiseOptionsPanel );
	bTonemapSizer->Add( m_NoiseOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	wxBoxSizer* bTonemapButtonsSizer;
	bTonemapButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bTonemapButtonsFillSizer;
	bTonemapButtonsFillSizer = new wxBoxSizer( wxVERTICAL );
	
	bTonemapButtonsSizer->Add( bTonemapButtonsFillSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer55;
	bSizer55 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_resetButton = new wxButton( m_Tonemap, ID_TM_RESET, wxT("Reset "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_TM_resetButton->SetToolTip( wxT("Reset Tone Mapping to default values") );
	
	bSizer55->Add( m_TM_resetButton, 0, wxALL, 5 );
	
	m_auto_tonemapCheckBox = new wxCheckBox( m_Tonemap, ID_AUTO_TONEMAP, wxT("auto"), wxDefaultPosition, wxDefaultSize, 0 );
	m_auto_tonemapCheckBox->SetValue(true);
	
	bSizer55->Add( m_auto_tonemapCheckBox, 0, wxALIGN_CENTER|wxALL, 5 );
	
	m_TM_applyButton = new wxButton( m_Tonemap, ID_TM_APPLY, wxT("Apply"), wxDefaultPosition, wxSize( -1,-1 ), wxBU_EXACTFIT );
	m_TM_applyButton->SetToolTip( wxT("Apply changes") );
	
	bSizer55->Add( m_TM_applyButton, 1, wxALL, 5 );
	
	bTonemapButtonsSizer->Add( bSizer55, 0, wxEXPAND, 1 );
	
	bTonemapSizer->Add( bTonemapButtonsSizer, 1, wxEXPAND, 1 );
	
	m_Tonemap->SetSizer( bTonemapSizer );
	m_Tonemap->Layout();
	bTonemapSizer->Fit( m_Tonemap );
	m_outputNotebook->AddPage( m_Tonemap, wxT("Imaging"), true, wxNullBitmap );
	m_SystemOptions = new wxScrolledWindow( m_outputNotebook, ID_SYSTEMOPTIONS, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxVSCROLL );
	m_SystemOptions->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBox1 = new wxCheckBox( m_SystemOptions, wxID_ANY, wxT("OpenGL 2.0 Acceleration"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox1->SetValue(true);
	
	m_checkBox1->Enable( false );
	
	bSizer20->Add( m_checkBox1, 0, wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( m_SystemOptions, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer20->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_staticText9 = new wxStaticText( m_SystemOptions, wxID_ANY, wxT("Precision"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_staticText9->Enable( false );
	
	bSizer20->Add( m_staticText9, 0, wxALL, 5 );
	
	wxString m_choice2Choices[] = { wxT("16bits"), wxT("32bits float (Geforce 8+/Radeon HD+)") };
	int m_choice2NChoices = sizeof( m_choice2Choices ) / sizeof( wxString );
	m_choice2 = new wxChoice( m_SystemOptions, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choice2NChoices, m_choice2Choices, 0 );
	m_choice2->SetSelection( 1 );
	m_choice2->Enable( false );
	
	bSizer20->Add( m_choice2, 0, wxALL, 5 );
	
	m_SystemOptions->SetSizer( bSizer20 );
	m_SystemOptions->Layout();
	bSizer20->Fit( m_SystemOptions );
	m_outputNotebook->AddPage( m_SystemOptions, wxT("System"), false, wxNullBitmap );
	
	bOutputDisplaySizer->Add( m_outputNotebook, 0, wxALL|wxEXPAND, 1 );
	
	wxBoxSizer* bOutputPreviewSizer;
	bOutputPreviewSizer = new wxBoxSizer( wxVERTICAL );
	
	bOutputDisplaySizer->Add( bOutputPreviewSizer, 2, wxEXPAND, 0 );
	
	m_outputNotebook2 = new wxNotebook( m_renderPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT );
	m_outputNotebook2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_outputNotebook2->Hide();
	
	
	bOutputDisplaySizer->Add( m_outputNotebook2, 0, wxEXPAND | wxALL, 1 );
	
	bRenderSizer->Add( bOutputDisplaySizer, 1, wxEXPAND, 0 );
	
	m_renderPage->SetSizer( bRenderSizer );
	m_renderPage->Layout();
	bRenderSizer->Fit( m_renderPage );
	m_auinotebook->AddPage( m_renderPage, wxT("Render"), true, wxNullBitmap );
	m_logPage = new wxPanel( m_auinotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bLogSizer;
	bLogSizer = new wxBoxSizer( wxVERTICAL );
	
	m_logTextCtrl = new wxTextCtrl( m_logPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_WORDWRAP );
	bLogSizer->Add( m_logTextCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_logPage->SetSizer( bLogSizer );
	m_logPage->Layout();
	bLogSizer->Fit( m_logPage );
	m_auinotebook->AddPage( m_logPage, wxT("Log"), false, wxNullBitmap );
	m_networkPage = new wxPanel( m_auinotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bNetworkSizer;
	bNetworkSizer = new wxBoxSizer( wxVERTICAL );
	
	m_networkToolBar = new wxToolBar( m_networkPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 
	m_serverStaticText = new wxStaticText( m_networkToolBar, wxID_ANY, wxT("Server: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_serverStaticText->Wrap( -1 );
	m_networkToolBar->AddControl( m_serverStaticText );
	m_serverTextCtrl = new wxTextCtrl( m_networkToolBar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), wxTE_PROCESS_ENTER );
	m_serverTextCtrl->SetToolTip( wxT("Type the address of a network server") );
	
	m_networkToolBar->AddControl( m_serverTextCtrl );
	m_networkToolBar->AddTool( ID_ADD_SERVER, wxT("Add Server"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Add Server"), wxT("Add Server") );
	m_networkToolBar->AddTool( ID_REMOVE_SERVER, wxT("Remove Server"), wxBitmap( blank_xpm ), wxNullBitmap, wxITEM_NORMAL, wxT("Remove Server"), wxT("Remove Server") );
	m_networkToolBar->AddSeparator();
	m_updateStaticText = new wxStaticText( m_networkToolBar, wxID_ANY, wxT("Update interval: "), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_updateStaticText->Wrap( -1 );
	m_networkToolBar->AddControl( m_updateStaticText );
	m_serverUpdateSpin = new wxSpinCtrl( m_networkToolBar, ID_SERVER_UPDATE_INT, wxT("240"), wxDefaultPosition, wxSize( 70,-1 ), wxSP_ARROW_KEYS, 0, 10000, 0 );
	m_serverUpdateSpin->SetToolTip( wxT("Enter the number of seconds between server updates") );
	
	m_networkToolBar->AddControl( m_serverUpdateSpin );
	m_networkToolBar->Realize();
	
	bNetworkSizer->Add( m_networkToolBar, 0, wxEXPAND, 5 );
	
	m_networkTreeCtrl = new wxTreeCtrl( m_networkPage, ID_NETWORK_TREE, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE );
	bNetworkSizer->Add( m_networkTreeCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_networkPage->SetSizer( bNetworkSizer );
	m_networkPage->Layout();
	bNetworkSizer->Fit( m_networkPage );
	m_auinotebook->AddPage( m_networkPage, wxT("Network"), false, wxNullBitmap );
	
	bSizer->Add( m_auinotebook, 1, wxEXPAND | wxALL, 5 );
	
	this->SetSizer( bSizer );
	this->Layout();
	m_statusBar = this->CreateStatusBar( 2, wxST_SIZEGRIP, wxID_ANY );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( LuxMainFrame::OnExit ) );
	this->Connect( m_open->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnOpen ) );
	this->Connect( m_exit->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_resume->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_pause->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_stop->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_toolBar->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_statusBarMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_sidePane->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_panMode->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_zoomMode->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_copy->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_clearLog->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_fullScreen->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_about->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_RESUMETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_PAUSETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_STOPTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_ADD_THREAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_REMOVE_THREAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_RENDER_COPY, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_PANTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_ZOOMTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_REFINETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	m_Tab_ToneMapToggleIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Tab_ToneMapIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TM_kernelChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_prescaleText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_prescaleText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_postscaleText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_postscaleText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_burnText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_burnText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivityText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_sensitivityText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_sensitivityText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_exposureText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_exposureText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_fstopText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_fstopText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_gammaText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_gammaText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_contrast_ywaText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_contrast_ywaText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_LensEffectsIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_bloomradiusText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_bloomradiusText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_computebloomlayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_bloomweightText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_bloomweightText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_vignettingenabledCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_vignettingamountText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_vignettingamountText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_ColorSpaceToggleIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Tab_ColorSpaceIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TORGB_colorspaceChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xwhiteText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xwhiteText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_ywhiteText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ywhiteText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xredText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xredText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_yredText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yredText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xgreenText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xgreenText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_ygreenText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ygreenText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xblueText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xblueText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_yblueText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yblueText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_GammaIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_gammaText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_gammaText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_HistogramToggleIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Tab_HistogramIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Histogram_Choice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_HistogramLogCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_Tab_NoiseReductionIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_greyc_EnabledCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_greyc_fastapproxCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_iterationsText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_iterationsText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_amplitudeText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_amplitudeText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gaussprecText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_gaussprecText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_gaussprecText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_alphaText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_alphaText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_sigmaText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sigmaText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_sharpnessText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sharpnessText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_anisoText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_anisoText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_spatialText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_spatialText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_angularText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_angularText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_GREYCinterpolationChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaText1->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_gammaText1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_gammaText1->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_auto_tonemapCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TM_applyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	this->Connect( ID_ADD_SERVER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( ID_REMOVE_SERVER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	m_serverUpdateSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( LuxMainFrame::OnSpin ), NULL, this );
	m_networkTreeCtrl->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( LuxMainFrame::OnTreeSelChanged ), NULL, this );
}

LuxMainFrame::~LuxMainFrame()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( LuxMainFrame::OnExit ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnOpen ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_RESUMETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_PAUSETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_STOPTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_ADD_THREAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_REMOVE_THREAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_RENDER_COPY, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_PANTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_ZOOMTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_REFINETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	m_Tab_ToneMapToggleIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Tab_ToneMapIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TM_kernelChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_prescaleText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_prescaleText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_prescaleText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_postscaleText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_postscaleText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_postscaleText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_burnText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_burnText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_burnText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivitySlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_sensitivityText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_sensitivityText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_sensitivityText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_exposureText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_exposureText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_exposureText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_fstopText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_fstopText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_fstopText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Linear_gammaText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Linear_gammaText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Linear_gammaText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_contrast_ywaText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_contrast_ywaText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_contrast_ywaText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_LensEffectsIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomradiusText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_bloomradiusText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_bloomradiusText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_computebloomlayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_bloomweightText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_bloomweightText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_bloomweightText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_vignettingenabledCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_vignettingamountText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_vignettingamountText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_vignettingamountText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_ColorSpaceToggleIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Tab_ColorSpaceIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TORGB_colorspaceChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xwhiteText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xwhiteText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xwhiteText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ywhiteText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_ywhiteText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ywhiteText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xredText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xredText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xredText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yredText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_yredText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yredText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xgreenText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xgreenText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xgreenText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_ygreenText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_ygreenText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_ygreenText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_xblueText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_xblueText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_xblueText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_yblueText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_yblueText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_yblueText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_GammaIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_gammaText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_gammaText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_Tab_HistogramToggleIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Tab_HistogramIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_Histogram_Choice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_HistogramLogCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_Tab_NoiseReductionIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LuxMainFrame::OnMouse ), NULL, this );
	m_greyc_EnabledCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_greyc_fastapproxCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_iterationsText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_iterationsText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_iterationsText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_amplitudeText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_amplitudeText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_amplitudeText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gausprecSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_gaussprecText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_gaussprecText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_gaussprecText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_alphaText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_alphaText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_alphaText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sigmaText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_sigmaText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sigmaText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_sharpnessText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_sharpnessText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_sharpnessText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_anisoText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_anisoText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_anisoText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_spatialText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_spatialText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_spatialText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_greyc_angularText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_greyc_angularText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_greyc_angularText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_GREYCinterpolationChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_ChiuRadiusSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TORGB_gammaText1->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TORGB_gammaText1->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TORGB_gammaText1->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_resetButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_auto_tonemapCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TM_applyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	this->Disconnect( ID_ADD_SERVER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_REMOVE_SERVER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	m_serverUpdateSpin->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( LuxMainFrame::OnSpin ), NULL, this );
	m_networkTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( LuxMainFrame::OnTreeSelChanged ), NULL, this );
}

LightGroupPanel::LightGroupPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	m_LG_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_LG_MainPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxRAISED_BORDER|wxTAB_TRAVERSAL );
	m_LG_SubSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer3321121;
	bSizer3321121 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_LightGroupPanel = new wxPanel( m_LG_MainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	m_Tab_LightGroupPanel->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	
	wxBoxSizer* bSizer103111;
	bSizer103111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_lightgroupBitmap = new wxStaticBitmap( m_Tab_LightGroupPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer103111->Add( m_lightgroupBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	m_lightgroupStaticText = new wxStaticText( m_Tab_LightGroupPanel, wxID_ANY, wxT("Group:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_lightgroupStaticText->Wrap( -1 );
	m_lightgroupStaticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_lightgroupStaticText->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer103111->Add( m_lightgroupStaticText, 0, wxALIGN_CENTER|wxALL, 3 );
	
	m_LG_name = new wxStaticText( m_Tab_LightGroupPanel, ID_LG_NAME, wxT("default"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LG_name->Wrap( -1 );
	m_LG_name->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_LG_name->SetForegroundColour( wxColour( 255, 255, 255 ) );
	
	bSizer103111->Add( m_LG_name, 0, wxALL, 5 );
	
	
	bSizer103111->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer88;
	bSizer88 = new wxBoxSizer( wxHORIZONTAL );
	
	m_Tab_LightGroupToggleIcon = new wxStaticBitmap( m_Tab_LightGroupPanel, ID_TAB_LG_TOGGLE, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_Tab_LightGroupToggleIcon, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 1 );
	
	m_Tab_LightGroupIcon = new wxStaticBitmap( m_Tab_LightGroupPanel, ID_TAB_LG, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_Tab_LightGroupIcon, 0, wxALL, 1 );
	
	bSizer103111->Add( bSizer88, 0, wxEXPAND, 5 );
	
	m_Tab_LightGroupPanel->SetSizer( bSizer103111 );
	m_Tab_LightGroupPanel->Layout();
	bSizer103111->Fit( m_Tab_LightGroupPanel );
	bSizer3321121->Add( m_Tab_LightGroupPanel, 1, wxEXPAND | wxALL, 2 );
	
	m_LG_SubSizer->Add( bSizer3321121, 0, wxEXPAND, 5 );
	
	m_TabNoteBook = new wxNotebook( m_LG_MainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT );
	m_LG_basicPanel = new wxPanel( m_TabNoteBook, ID_LG_BASICPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer189;
	bSizer189 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer15911;
	bSizer15911 = new wxBoxSizer( wxVERTICAL );
	
	m_LG_scaleLabel = new wxStaticText( m_LG_basicPanel, wxID_ANY, wxT("Scale/Intensity"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_LG_scaleLabel->Wrap( -1 );
	bSizer15911->Add( m_LG_scaleLabel, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer10111;
	bSizer10111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_LG_scaleSlider = new wxSlider( m_LG_basicPanel, ID_LG_SCALE, 5, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	bSizer10111->Add( m_LG_scaleSlider, 1, wxALL|wxEXPAND, 5 );
	
	m_LG_scaleText = new wxTextCtrl( m_LG_basicPanel, ID_LG_SCALE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_LG_scaleText->SetMaxLength( 5 ); 
	bSizer10111->Add( m_LG_scaleText, 0, wxALL|wxFIXED_MINSIZE, 2 );
	
	bSizer15911->Add( bSizer10111, 1, wxEXPAND, 5 );
	
	bSizer189->Add( bSizer15911, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer159111;
	bSizer159111 = new wxBoxSizer( wxVERTICAL );
	
	m_LG_rgbLabel = new wxStaticText( m_LG_basicPanel, wxID_ANY, wxT("Colour"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_LG_rgbLabel->Wrap( -1 );
	bSizer159111->Add( m_LG_rgbLabel, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer101111;
	bSizer101111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_LG_rgbPicker = new wxColourPickerCtrl( m_LG_basicPanel, ID_LG_RGBCOLOR, wxColour( 255, 255, 255 ), wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	bSizer101111->Add( m_LG_rgbPicker, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer102;
	bSizer102 = new wxBoxSizer( wxVERTICAL );
	
	bSizer102->SetMinSize( wxSize( 220,-1 ) ); 
	m_BarBlackBodyStaticBitmap = new wxStaticBitmap( m_LG_basicPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 200,17 ), 0 );
	m_BarBlackBodyStaticBitmap->SetMinSize( wxSize( 200,17 ) );
	m_BarBlackBodyStaticBitmap->SetMaxSize( wxSize( 200,17 ) );
	
	bSizer102->Add( m_BarBlackBodyStaticBitmap, 0, wxALIGN_CENTER, 0 );
	
	m_LG_temperatureSlider = new wxSlider( m_LG_basicPanel, ID_LG_TEMPERATURE, 333, 0, 512, wxPoint( -1,-1 ), wxSize( 220,25 ), wxSL_HORIZONTAL|wxSL_TOP );
	m_LG_temperatureSlider->SetMinSize( wxSize( 220,25 ) );
	m_LG_temperatureSlider->SetMaxSize( wxSize( 220,25 ) );
	
	bSizer102->Add( m_LG_temperatureSlider, 0, wxALIGN_CENTER, 0 );
	
	bSizer101111->Add( bSizer102, 1, wxEXPAND, 5 );
	
	m_LG_temperatureText = new wxTextCtrl( m_LG_basicPanel, ID_LG_TEMPERATURE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_LG_temperatureText->SetMaxLength( 5 ); 
	bSizer101111->Add( m_LG_temperatureText, 0, wxALL|wxFIXED_MINSIZE, 2 );
	
	bSizer159111->Add( bSizer101111, 1, wxEXPAND, 5 );
	
	bSizer189->Add( bSizer159111, 1, wxEXPAND, 5 );
	
	m_LG_basicPanel->SetSizer( bSizer189 );
	m_LG_basicPanel->Layout();
	bSizer189->Fit( m_LG_basicPanel );
	m_TabNoteBook->AddPage( m_LG_basicPanel, wxT("basic"), true );
	m_LG_advancedPanel = new wxPanel( m_TabNoteBook, ID_LG_ADVANCEDPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_TabNoteBook->AddPage( m_LG_advancedPanel, wxT("advanced"), false );
	
	m_LG_SubSizer->Add( m_TabNoteBook, 1, wxEXPAND | wxALL, 2 );
	
	m_LG_MainPanel->SetSizer( m_LG_SubSizer );
	m_LG_MainPanel->Layout();
	m_LG_SubSizer->Fit( m_LG_MainPanel );
	m_LG_MainSizer->Add( m_LG_MainPanel, 0, wxALL|wxEXPAND, 0 );
	
	this->SetSizer( m_LG_MainSizer );
	this->Layout();
	m_LG_MainSizer->Fit( this );
	
	// Connect Events
	m_Tab_LightGroupToggleIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LightGroupPanel::OnMouse ), NULL, this );
	m_Tab_LightGroupIcon->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( LightGroupPanel::OnMouse ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LightGroupPanel::OnFocus ), NULL, this );
	m_LG_scaleText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
	m_LG_scaleText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
	m_LG_rgbPicker->Connect( wxEVT_COMMAND_COLOURPICKER_CHANGED, wxColourPickerEventHandler( LightGroupPanel::OnColourChanged ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LightGroupPanel::OnFocus ), NULL, this );
	m_LG_temperatureText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
	m_LG_temperatureText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
}

LightGroupPanel::~LightGroupPanel()
{
	// Disconnect Events
	m_Tab_LightGroupToggleIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LightGroupPanel::OnMouse ), NULL, this );
	m_Tab_LightGroupIcon->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( LightGroupPanel::OnMouse ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_scaleText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LightGroupPanel::OnFocus ), NULL, this );
	m_LG_scaleText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
	m_LG_scaleText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
	m_LG_rgbPicker->Disconnect( wxEVT_COMMAND_COLOURPICKER_CHANGED, wxColourPickerEventHandler( LightGroupPanel::OnColourChanged ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LightGroupPanel::OnScroll ), NULL, this );
	m_LG_temperatureText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LightGroupPanel::OnFocus ), NULL, this );
	m_LG_temperatureText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
	m_LG_temperatureText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LightGroupPanel::OnText ), NULL, this );
}
