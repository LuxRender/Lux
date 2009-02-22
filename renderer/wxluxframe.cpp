///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
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
	
	wxMenuItem* m_options;
	m_options = new wxMenuItem( m_view, ID_OPTIONS, wxString( wxT("O&ptions") ) + wxT('\t') + wxT("CTRL+P"), wxT("Program Options and additional controls"), wxITEM_CHECK );
	m_view->Append( m_options );
	
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
	bSizer33211 = new wxBoxSizer( wxHORIZONTAL );
	
	m_tonemapBitmap = new wxStaticBitmap( m_TonemapOptionsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer33211->Add( m_tonemapBitmap, 0, wxALL, 5 );
	
	m_tonemappingkernelStaticText = new wxStaticText( m_TonemapOptionsPanel, wxID_ANY, wxT("Tonemapping Kernel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tonemappingkernelStaticText->Wrap( -1 );
	bSizer33211->Add( m_tonemappingkernelStaticText, 0, wxALL, 5 );
	
	wxString m_TM_kernelChoiceChoices[] = { wxT("Reinhard / non-Linear"), wxT("Linear"), wxT("Contrast"), wxT("MaxWhite") };
	int m_TM_kernelChoiceNChoices = sizeof( m_TM_kernelChoiceChoices ) / sizeof( wxString );
	m_TM_kernelChoice = new wxChoice( m_TonemapOptionsPanel, ID_TM_KERNELCHOICE, wxDefaultPosition, wxDefaultSize, m_TM_kernelChoiceNChoices, m_TM_kernelChoiceChoices, 0 );
	m_TM_kernelChoice->SetSelection( 0 );
	m_TM_kernelChoice->SetToolTip( wxT("Select type of Tone Mapping") );
	
	bSizer33211->Add( m_TM_kernelChoice, 1, wxALL, 2 );
	
	m_TonemapOptionsPanel->SetSizer( bSizer33211 );
	m_TonemapOptionsPanel->Layout();
	bSizer33211->Fit( m_TonemapOptionsPanel );
	bTonemapSizer->Add( m_TonemapOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_TonemapReinhardOptionsPanel = new wxPanel( m_Tonemap, ID_TONEMAPREINHARDOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer33;
	bSizer33 = new wxBoxSizer( wxVERTICAL );
	
	m_TM_Reinhard_YwaStaticText = new wxStaticText( m_TonemapReinhardOptionsPanel, wxID_ANY, wxT("Ywa (Display/World Adaption Luminance)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Reinhard_YwaStaticText->Wrap( -1 );
	bSizer33->Add( m_TM_Reinhard_YwaStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer104;
	bSizer104 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Reinhard_autoywaCheckbox = new wxCheckBox( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_AUTOYWA, wxT("auto"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TM_Reinhard_autoywaCheckbox->SetValue(true);
	
	bSizer104->Add( m_TM_Reinhard_autoywaCheckbox, 0, wxALIGN_CENTER|wxALL, 5 );
	
	m_TM_Reinhard_ywaSlider = new wxSlider( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_YWA, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Reinhard_ywaSlider->Enable( false );
	m_TM_Reinhard_ywaSlider->SetToolTip( wxT("Reinhard Prescale") );
	
	bSizer104->Add( m_TM_Reinhard_ywaSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Reinhard_ywaText = new wxTextCtrl( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_YWA_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Reinhard_ywaText->SetMaxLength( 5 ); 
	m_TM_Reinhard_ywaText->Enable( false );
	m_TM_Reinhard_ywaText->SetToolTip( wxT("Please enter a new Reinhard Prescale value and press enter.") );
	
	bSizer104->Add( m_TM_Reinhard_ywaText, 0, wxALIGN_CENTER, 0 );
	
	bSizer33->Add( bSizer104, 0, wxEXPAND, 5 );
	
	m_TM_Reinhard_prescaleStaticText = new wxStaticText( m_TonemapReinhardOptionsPanel, wxID_ANY, wxT("Prescale "), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Reinhard_prescaleStaticText->Wrap( -1 );
	bSizer33->Add( m_TM_Reinhard_prescaleStaticText, 0, wxALL|wxFIXED_MINSIZE, 2 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Reinhard_prescaleSlider = new wxSlider( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_PRESCALE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Reinhard_prescaleSlider->SetToolTip( wxT("Reinhard Prescale") );
	
	bSizer10->Add( m_TM_Reinhard_prescaleSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Reinhard_prescaleText = new wxTextCtrl( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_PRESCALE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Reinhard_prescaleText->SetMaxLength( 5 ); 
	m_TM_Reinhard_prescaleText->SetToolTip( wxT("Please enter a new Reinhard Prescale value and press enter.") );
	
	bSizer10->Add( m_TM_Reinhard_prescaleText, 0, wxALIGN_CENTER, 0 );
	
	bSizer33->Add( bSizer10, 0, wxEXPAND, 5 );
	
	m_TM_Reinhard_postscaleStaticText = new wxStaticText( m_TonemapReinhardOptionsPanel, wxID_ANY, wxT("Postscale"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Reinhard_postscaleStaticText->Wrap( -1 );
	bSizer33->Add( m_TM_Reinhard_postscaleStaticText, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 2 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Reinhard_postscaleSlider = new wxSlider( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_POSTSCALE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TM_Reinhard_postscaleSlider->SetToolTip( wxT("Reinhard Postscale") );
	
	bSizer12->Add( m_TM_Reinhard_postscaleSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TM_Reinhard_postscaleText = new wxTextCtrl( m_TonemapReinhardOptionsPanel, ID_TM_REINHARD_POSTSCALE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TM_Reinhard_postscaleText->SetMaxLength( 5 ); 
	m_TM_Reinhard_postscaleText->SetToolTip( wxT("Please enter a new Reinhard Postscale value and press enter.") );
	
	bSizer12->Add( m_TM_Reinhard_postscaleText, 0, wxALIGN_CENTER, 0 );
	
	bSizer33->Add( bSizer12, 0, wxEXPAND, 5 );
	
	m_TM_Reinhard_burnStaticText = new wxStaticText( m_TonemapReinhardOptionsPanel, wxID_ANY, wxT("Burn"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Reinhard_burnStaticText->Wrap( -1 );
	bSizer33->Add( m_TM_Reinhard_burnStaticText, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 2 );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
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
	bTonemapSizer->Add( m_TonemapReinhardOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_TonemapLinearOptionsPanel = new wxPanel( m_Tonemap, ID_TONEMAPLINEAROPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	m_TonemapLinearOptionsPanel->Hide();
	
	wxBoxSizer* bSizer331;
	bSizer331 = new wxBoxSizer( wxVERTICAL );
	
	m_TM_Linear_sensitivityStaticText = new wxStaticText( m_TonemapLinearOptionsPanel, wxID_ANY, wxT("Sensitivity"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_Linear_sensitivityStaticText->Wrap( -1 );
	bSizer331->Add( m_TM_Linear_sensitivityStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1041;
	bSizer1041 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_Linear_sensitivitySlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_SENSITIVITY, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
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
	
	m_TM_Linear_exposureSlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_EXPOSURE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
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
	
	m_TM_Linear_fstopSlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_FSTOP, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
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
	
	m_TM_Linear_gammaSlider = new wxSlider( m_TonemapLinearOptionsPanel, ID_TM_LINEAR_GAMMA, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
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
	bTonemapSizer->Add( m_TonemapLinearOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_TonemapContrastOptionsPanel = new wxPanel( m_Tonemap, ID_TONEMAPCONTRASTOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	m_TonemapContrastOptionsPanel->Hide();
	
	wxBoxSizer* bSizer332;
	bSizer332 = new wxBoxSizer( wxVERTICAL );
	
	m_TM_contrast_YwaStaticText = new wxStaticText( m_TonemapContrastOptionsPanel, wxID_ANY, wxT("Ywa (Display/World Adaption Luminance)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TM_contrast_YwaStaticText->Wrap( -1 );
	bSizer332->Add( m_TM_contrast_YwaStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1042;
	bSizer1042 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TM_contrast_ywaSlider = new wxSlider( m_TonemapContrastOptionsPanel, ID_TM_CONTRAST_YWA, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL|wxSL_SELRANGE );
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
	bTonemapSizer->Add( m_TonemapContrastOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_ColorSpaceOptionsPanel = new wxPanel( m_Tonemap, ID_COLORSPACEOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3321;
	bSizer3321 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer332111;
	bSizer332111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_colorspaceBitmap = new wxStaticBitmap( m_ColorSpaceOptionsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer332111->Add( m_colorspaceBitmap, 0, wxALL, 5 );
	
	m_TORGB_colorspacepresetsStaticText = new wxStaticText( m_ColorSpaceOptionsPanel, wxID_ANY, wxT("ColorSpace"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_colorspacepresetsStaticText->Wrap( -1 );
	bSizer332111->Add( m_TORGB_colorspacepresetsStaticText, 0, wxALL, 5 );
	
	wxString m_TORGB_colorspaceChoiceChoices[] = { wxT("sRGB - HDTV (ITU-R BT.709-5)"), wxT("ROMM RGB"), wxT("Adobe RGB 98"), wxT("Apple RGB"), wxT("NTSC (FCC 1953)"), wxT("NTSC (1979) (SMPTE C/-RP 145)"), wxT("PAL/SECAM (EBU 3213)"), wxT("CIE (1931) E") };
	int m_TORGB_colorspaceChoiceNChoices = sizeof( m_TORGB_colorspaceChoiceChoices ) / sizeof( wxString );
	m_TORGB_colorspaceChoice = new wxChoice( m_ColorSpaceOptionsPanel, ID_TORGB_COLORSPACECHOICE, wxDefaultPosition, wxDefaultSize, m_TORGB_colorspaceChoiceNChoices, m_TORGB_colorspaceChoiceChoices, 0 );
	m_TORGB_colorspaceChoice->SetSelection( 0 );
	m_TORGB_colorspaceChoice->SetToolTip( wxT("Select Color Space") );
	
	bSizer332111->Add( m_TORGB_colorspaceChoice, 1, wxALL, 2 );
	
	bSizer3321->Add( bSizer332111, 0, wxEXPAND, 5 );
	
	m_TORGB_whitexyStaticText = new wxStaticText( m_ColorSpaceOptionsPanel, wxID_ANY, wxT("White XY"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_whitexyStaticText->Wrap( -1 );
	bSizer3321->Add( m_TORGB_whitexyStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer212;
	bSizer212 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer1012;
	bSizer1012 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xwhiteSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_XWHITE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xwhiteSlider->SetToolTip( wxT("White X") );
	
	bSizer1012->Add( m_TORGB_xwhiteSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xwhiteText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_XWHITE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xwhiteText->SetMaxLength( 5 ); 
	m_TORGB_xwhiteText->SetToolTip( wxT("White X") );
	
	bSizer1012->Add( m_TORGB_xwhiteText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer212->Add( bSizer1012, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer1022;
	bSizer1022 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_ywhiteSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_YWHITE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_ywhiteSlider->SetToolTip( wxT("White Y") );
	
	bSizer1022->Add( m_TORGB_ywhiteSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_ywhiteText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_YWHITE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_ywhiteText->SetMaxLength( 5 ); 
	m_TORGB_ywhiteText->SetToolTip( wxT("White Y") );
	
	bSizer1022->Add( m_TORGB_ywhiteText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer212->Add( bSizer1022, 1, wxEXPAND, 5 );
	
	bSizer3321->Add( bSizer212, 0, wxEXPAND, 5 );
	
	m_TORGB_rgbxyStaticText = new wxStaticText( m_ColorSpaceOptionsPanel, wxID_ANY, wxT("Red/Green/Blue XY"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_rgbxyStaticText->Wrap( -1 );
	bSizer3321->Add( m_TORGB_rgbxyStaticText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer213;
	bSizer213 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer1013;
	bSizer1013 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xredSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_XRED, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xredSlider->SetToolTip( wxT("Red X") );
	
	bSizer1013->Add( m_TORGB_xredSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xredText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_XRED_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xredText->SetMaxLength( 5 ); 
	m_TORGB_xredText->SetToolTip( wxT("Red X") );
	
	bSizer1013->Add( m_TORGB_xredText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer213->Add( bSizer1013, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer1023;
	bSizer1023 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_yredSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_YRED, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_yredSlider->SetToolTip( wxT("Red Y") );
	
	bSizer1023->Add( m_TORGB_yredSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_yredText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_YRED_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_yredText->SetMaxLength( 5 ); 
	m_TORGB_yredText->SetToolTip( wxT("Red Y") );
	
	bSizer1023->Add( m_TORGB_yredText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer213->Add( bSizer1023, 1, wxEXPAND, 5 );
	
	bSizer3321->Add( bSizer213, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer2131;
	bSizer2131 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer10131;
	bSizer10131 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xgreenSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_XGREEN, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xgreenSlider->SetToolTip( wxT("Green X") );
	
	bSizer10131->Add( m_TORGB_xgreenSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xgreenText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_XGREEN_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xgreenText->SetMaxLength( 5 ); 
	m_TORGB_xgreenText->SetToolTip( wxT("Green X") );
	
	bSizer10131->Add( m_TORGB_xgreenText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2131->Add( bSizer10131, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10231;
	bSizer10231 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_ygreenSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_YGREEN, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_ygreenSlider->SetToolTip( wxT("Green Y") );
	
	bSizer10231->Add( m_TORGB_ygreenSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_ygreenText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_YGREEN_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_ygreenText->SetMaxLength( 5 ); 
	m_TORGB_ygreenText->SetToolTip( wxT("Green Y") );
	
	bSizer10231->Add( m_TORGB_ygreenText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2131->Add( bSizer10231, 1, wxEXPAND, 5 );
	
	bSizer3321->Add( bSizer2131, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer2132;
	bSizer2132 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer10132;
	bSizer10132 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_xblueSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_XBLUE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_xblueSlider->SetToolTip( wxT("Blue X") );
	
	bSizer10132->Add( m_TORGB_xblueSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_xblueText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_XBLUE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_xblueText->SetMaxLength( 5 ); 
	m_TORGB_xblueText->SetToolTip( wxT("Blue X") );
	
	bSizer10132->Add( m_TORGB_xblueText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2132->Add( bSizer10132, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10232;
	bSizer10232 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_yblueSlider = new wxSlider( m_ColorSpaceOptionsPanel, ID_TORGB_YBLUE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_yblueSlider->SetToolTip( wxT("Blue Y") );
	
	bSizer10232->Add( m_TORGB_yblueSlider, 1, wxALL|wxEXPAND, 1 );
	
	m_TORGB_yblueText = new wxTextCtrl( m_ColorSpaceOptionsPanel, ID_TORGB_YBLUE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_yblueText->SetMaxLength( 5 ); 
	m_TORGB_yblueText->SetToolTip( wxT("Blue Y") );
	
	bSizer10232->Add( m_TORGB_yblueText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer2132->Add( bSizer10232, 1, wxEXPAND, 5 );
	
	bSizer3321->Add( bSizer2132, 1, wxEXPAND, 5 );
	
	m_ColorSpaceOptionsPanel->SetSizer( bSizer3321 );
	m_ColorSpaceOptionsPanel->Layout();
	bSizer3321->Fit( m_ColorSpaceOptionsPanel );
	bTonemapSizer->Add( m_ColorSpaceOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
	m_GammaOptionsPanel = new wxPanel( m_Tonemap, ID_GAMMAOPTIONSPANEL, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxRAISED_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3322;
	bSizer3322 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer1031;
	bSizer1031 = new wxBoxSizer( wxHORIZONTAL );
	
	m_gammaBitmap = new wxStaticBitmap( m_GammaOptionsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1031->Add( m_gammaBitmap, 0, wxALL, 5 );
	
	m_TORGB_gammaStaticText = new wxStaticText( m_GammaOptionsPanel, wxID_ANY, wxT("Output Gamma"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TORGB_gammaStaticText->Wrap( -1 );
	bSizer1031->Add( m_TORGB_gammaStaticText, 0, wxALL, 5 );
	
	bSizer3322->Add( bSizer1031, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer103;
	bSizer103 = new wxBoxSizer( wxHORIZONTAL );
	
	m_TORGB_gammaSlider = new wxSlider( m_GammaOptionsPanel, ID_TORGB_GAMMA, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	m_TORGB_gammaSlider->SetToolTip( wxT("Gamma Value") );
	
	bSizer103->Add( m_TORGB_gammaSlider, 1, wxALL|wxEXPAND, 2 );
	
	m_TORGB_gammaText = new wxTextCtrl( m_GammaOptionsPanel, ID_TORGB_GAMMA_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_TORGB_gammaText->SetMaxLength( 5 ); 
	m_TORGB_gammaText->SetToolTip( wxT("Gamma Value") );
	
	bSizer103->Add( m_TORGB_gammaText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 0 );
	
	bSizer3322->Add( bSizer103, 0, wxEXPAND, 5 );
	
	m_GammaOptionsPanel->SetSizer( bSizer3322 );
	m_GammaOptionsPanel->Layout();
	bSizer3322->Fit( m_GammaOptionsPanel );
	bTonemapSizer->Add( m_GammaOptionsPanel, 0, wxEXPAND | wxALL, 1 );
	
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
	m_outputNotebook->AddPage( m_Tonemap, wxT("Tonemap"), true, wxNullBitmap );
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
	this->Connect( m_options->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
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
	m_TM_kernelChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_ywaText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_ywaText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
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
	m_TM_resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
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
	m_TM_kernelChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( LuxMainFrame::OnScroll ), NULL, this );
	m_TM_Reinhard_ywaText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( LuxMainFrame::OnFocus ), NULL, this );
	m_TM_Reinhard_ywaText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
	m_TM_Reinhard_ywaText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( LuxMainFrame::OnText ), NULL, this );
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
	m_TM_resetButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	m_TM_applyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ), NULL, this );
	this->Disconnect( ID_ADD_SERVER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_REMOVE_SERVER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	m_serverUpdateSpin->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( LuxMainFrame::OnSpin ), NULL, this );
	m_networkTreeCtrl->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( LuxMainFrame::OnTreeSelChanged ), NULL, this );
}

m_OptionsDialog::m_OptionsDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_Options_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_systemPanel = new wxPanel( m_Options_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText1;
	m_staticText1 = new wxStaticText( m_systemPanel, wxID_ANY, wxT("Display Interval: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer15->Add( m_staticText1, 0, wxALL, 5 );
	
	
	bSizer15->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_Display_spinCtrl = new wxSpinCtrl( m_systemPanel, ID_SYS_DISPLAY_INT, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 10000000, 12 );
	bSizer15->Add( m_Display_spinCtrl, 0, wxEXPAND, 5 );
	
	bSizer14->Add( bSizer15, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText2 = new wxStaticText( m_systemPanel, wxID_ANY, wxT("Write Interval: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer16->Add( m_staticText2, 0, wxALL, 5 );
	
	
	bSizer16->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_Write_spinCtrl = new wxSpinCtrl( m_systemPanel, ID_SYS_WRITE_INT, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 10000000, 120 );
	bSizer16->Add( m_Write_spinCtrl, 0, wxEXPAND, 5 );
	
	bSizer14->Add( bSizer16, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_writeOptionsChoices[] = { wxT("Write and Use FLM"), wxT("Tonemapped TGA"), wxT("Tonemapped EXR"), wxT("Untonemapped EXR"), wxT("Tonemapped IGI"), wxT("Untonemapped IGI") };
	int m_writeOptionsNChoices = sizeof( m_writeOptionsChoices ) / sizeof( wxString );
	m_writeOptions = new wxCheckListBox( m_systemPanel, ID_WRITE_OPTIONS, wxDefaultPosition, wxSize( -1,-1 ), m_writeOptionsNChoices, m_writeOptionsChoices, wxLB_NEEDED_SB );
	m_writeOptions->SetToolTip( wxT("Save Options") );
	
	bSizer17->Add( m_writeOptions, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer17->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_SysApplyButton = new wxButton( m_systemPanel, ID_SYS_APPLY, wxT("Apply Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_SysApplyButton, 0, wxALL|wxSHAPED, 5 );
	
	bSizer14->Add( bSizer17, 0, wxEXPAND, 5 );
	
	m_systemPanel->SetSizer( bSizer14 );
	m_systemPanel->Layout();
	bSizer14->Fit( m_systemPanel );
	m_Options_notebook->AddPage( m_systemPanel, wxT("System"), false );
	
	bSizer7->Add( m_Options_notebook, 0, wxALL, 5 );
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( m_OptionsDialog::OnClose ) );
	m_Display_spinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( m_OptionsDialog::OnSpin ), NULL, this );
	m_Write_spinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( m_OptionsDialog::OnSpin ), NULL, this );
	m_writeOptions->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_writeOptions->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_writeOptions->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_SysApplyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
}

m_OptionsDialog::~m_OptionsDialog()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( m_OptionsDialog::OnClose ) );
	m_Display_spinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( m_OptionsDialog::OnSpin ), NULL, this );
	m_Write_spinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( m_OptionsDialog::OnSpin ), NULL, this );
	m_writeOptions->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_writeOptions->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_writeOptions->Disconnect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_SysApplyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
}

LightGroupPanel::LightGroupPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer183;
	bSizer183 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer3321121;
	bSizer3321121 = new wxBoxSizer( wxHORIZONTAL );
	
	m_lightgroupBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3321121->Add( m_lightgroupBitmap, 0, wxALL, 5 );
	
	m_LG_nameLabel = new wxStaticText( this, wxID_ANY, wxT("Group:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LG_nameLabel->Wrap( -1 );
	m_LG_nameLabel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer3321121->Add( m_LG_nameLabel, 0, wxALIGN_CENTER|wxALL, 2 );
	
	m_LG_name = new wxStaticText( this, ID_LG_NAME, wxT("default"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LG_name->Wrap( -1 );
	m_LG_name->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer3321121->Add( m_LG_name, 0, wxALIGN_CENTER|wxALL, 2 );
	
	m_LG_enableCheckbox = new wxCheckBox( this, ID_LG_ENABLE, wxT("Enabled"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_LG_enableCheckbox->SetValue(true);
	
	bSizer3321121->Add( m_LG_enableCheckbox, 1, wxALL, 5 );
	
	bSizer183->Add( bSizer3321121, 0, wxEXPAND, 5 );
	
	m_notebook4 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT );
	m_LG_basicPanel = new wxPanel( m_notebook4, ID_LG_BASICPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
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
	
	m_LG_rgbLabel = new wxStaticText( m_LG_basicPanel, wxID_ANY, wxT("RGB Colour / Blackbody Temperature"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_LG_rgbLabel->Wrap( -1 );
	bSizer159111->Add( m_LG_rgbLabel, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer101111;
	bSizer101111 = new wxBoxSizer( wxHORIZONTAL );
	
	m_LG_rgbPicker = new wxColourPickerCtrl( m_LG_basicPanel, ID_LG_RGBCOLOR, wxColour( 255, 255, 255 ), wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	bSizer101111->Add( m_LG_rgbPicker, 0, wxALL, 5 );
	
	m_LG_temperatureSlider = new wxSlider( m_LG_basicPanel, ID_LG_TEMPERATURE, 50, 0, 512, wxDefaultPosition, wxSize( -1,-1 ), wxSL_HORIZONTAL );
	bSizer101111->Add( m_LG_temperatureSlider, 1, wxALL|wxEXPAND, 5 );
	
	m_LG_temperatureText = new wxTextCtrl( m_LG_basicPanel, ID_LG_TEMPERATURE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 36,-1 ), wxTE_PROCESS_ENTER );
	m_LG_temperatureText->SetMaxLength( 5 ); 
	bSizer101111->Add( m_LG_temperatureText, 0, wxALL|wxFIXED_MINSIZE, 2 );
	
	bSizer159111->Add( bSizer101111, 1, wxEXPAND, 5 );
	
	bSizer189->Add( bSizer159111, 1, wxEXPAND, 5 );
	
	m_LG_basicPanel->SetSizer( bSizer189 );
	m_LG_basicPanel->Layout();
	bSizer189->Fit( m_LG_basicPanel );
	m_notebook4->AddPage( m_LG_basicPanel, wxT("basic"), true );
	m_LG_advancedPanel = new wxPanel( m_notebook4, ID_LG_ADVANCEDPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_notebook4->AddPage( m_LG_advancedPanel, wxT("advanced"), false );
	
	bSizer183->Add( m_notebook4, 1, wxEXPAND | wxALL, 2 );
	
	this->SetSizer( bSizer183 );
	this->Layout();
	bSizer183->Fit( this );
	
	// Connect Events
	m_LG_enableCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LightGroupPanel::OnCheckBox ), NULL, this );
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
	m_LG_enableCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( LightGroupPanel::OnCheckBox ), NULL, this );
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
