///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
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
	
	m_auinotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_MOVE|wxAUI_NB_TAB_SPLIT );
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
	m_ThreadText = new wxTextCtrl( m_renderToolBar, wxID_ANY, wxT("Threads: 0"), wxDefaultPosition, wxSize( 86,-1 ), wxTE_READONLY|wxNO_BORDER );
	m_ThreadText->SetMaxLength( 12 ); 
	m_ThreadText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	m_ThreadText->SetToolTip( wxT("Number of rendering threads") );
	
	m_renderToolBar->AddControl( m_ThreadText );
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
	this->Connect( m_options->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_panMode->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_zoomMode->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Connect( m_copy->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
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
	this->Disconnect( ID_RESUMETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_PAUSETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_STOPTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_ADD_THREAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_REMOVE_THREAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_RENDER_COPY, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_PANTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_ZOOMTOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
	this->Disconnect( ID_REFINETOOL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( LuxMainFrame::OnMenu ) );
}

m_OptionsDialog::m_OptionsDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_Options_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_ToneMappingPanel = new wxPanel( m_Options_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_TM_choiceChoices[] = { wxT("Reinhard") };
	int m_TM_choiceNChoices = sizeof( m_TM_choiceChoices ) / sizeof( wxString );
	m_TM_choice = new wxChoice( m_ToneMappingPanel, ID_TM_CHOICE, wxDefaultPosition, wxDefaultSize, m_TM_choiceNChoices, m_TM_choiceChoices, 0 );
	m_TM_choice->SetSelection( 0 );
	m_TM_choice->SetToolTip( wxT("Select type of Tone Mapping") );
	
	bSizer8->Add( m_TM_choice, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText4 = new wxStaticText( m_ToneMappingPanel, wxID_ANY, wxT("Prescale "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer10->Add( m_staticText4, 0, wxALL, 5 );
	
	
	bSizer10->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_RH_prescaleSlider = new wxSlider( m_ToneMappingPanel, ID_RH_PRESCALE, 50, 0, 100, wxDefaultPosition, wxSize( 128,-1 ), wxSL_HORIZONTAL );
	m_RH_prescaleSlider->SetToolTip( wxT("Reinhard Prescale") );
	
	bSizer10->Add( m_RH_prescaleSlider, 0, wxALL, 5 );
	
	m_RH_preText = new wxTextCtrl( m_ToneMappingPanel, ID_RH_PRESCALE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 48,-1 ), wxTE_PROCESS_ENTER|wxTAB_TRAVERSAL );
	m_RH_preText->SetMaxLength( 5 ); 
	m_RH_preText->SetToolTip( wxT("Please enter a new Reinhard Prescale value and press enter.") );
	
	bSizer10->Add( m_RH_preText, 0, 0, 5 );
	
	bSizer8->Add( bSizer10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText5 = new wxStaticText( m_ToneMappingPanel, wxID_ANY, wxT("Postscale"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer12->Add( m_staticText5, 0, wxALL, 5 );
	
	
	bSizer12->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_RH_postscaleSlider = new wxSlider( m_ToneMappingPanel, ID_RH_POSTSCALE, 50, 0, 100, wxDefaultPosition, wxSize( 128,-1 ), wxSL_HORIZONTAL );
	m_RH_postscaleSlider->SetToolTip( wxT("Reinhard Postscale") );
	
	bSizer12->Add( m_RH_postscaleSlider, 0, wxALL, 5 );
	
	m_RH_postText = new wxTextCtrl( m_ToneMappingPanel, ID_RH_POSTSCALE_TEXT, wxT("1.0"), wxDefaultPosition, wxSize( 48,-1 ), wxTE_PROCESS_ENTER|wxTAB_TRAVERSAL );
	m_RH_postText->SetMaxLength( 5 ); 
	m_RH_postText->SetToolTip( wxT("Please enter a new Reinhard Postscale value and press enter.") );
	
	bSizer12->Add( m_RH_postText, 0, 0, 5 );
	
	bSizer8->Add( bSizer12, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText6 = new wxStaticText( m_ToneMappingPanel, wxID_ANY, wxT("Burn"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizer13->Add( m_staticText6, 0, wxALL, 5 );
	
	
	bSizer13->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_RH_burnSlider = new wxSlider( m_ToneMappingPanel, ID_RH_BURN, 50, 0, 100, wxDefaultPosition, wxSize( 128,-1 ), wxSL_HORIZONTAL );
	m_RH_burnSlider->SetToolTip( wxT("Reinhard Burn") );
	
	bSizer13->Add( m_RH_burnSlider, 0, wxALL, 5 );
	
	m_RH_burnText = new wxTextCtrl( m_ToneMappingPanel, ID_RH_BURN_TEXT, wxT("6.0"), wxDefaultPosition, wxSize( 48,-1 ), wxTE_PROCESS_ENTER|wxTAB_TRAVERSAL );
	m_RH_burnText->SetMaxLength( 5 ); 
	m_RH_burnText->SetToolTip( wxT("Please enter a new Reinhard Burn value and press enter.") );
	
	bSizer13->Add( m_RH_burnText, 0, 0, 5 );
	
	bSizer8->Add( bSizer13, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_TM_resetButton = new wxButton( m_ToneMappingPanel, ID_TM_RESET, wxT("Reset "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_TM_resetButton->SetToolTip( wxT("Reset Tone Mapping to default values") );
	
	bSizer11->Add( m_TM_resetButton, 0, wxALL, 5 );
	
	
	bSizer11->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_refreshButton = new wxButton( m_ToneMappingPanel, ID_RENDER_REFRESH, wxT("Refresh "), wxDefaultPosition, wxDefaultSize, 0 );
	m_refreshButton->SetToolTip( wxT("Refresh the current rendering image") );
	
	bSizer11->Add( m_refreshButton, 0, wxALL, 5 );
	
	bSizer8->Add( bSizer11, 0, wxEXPAND, 5 );
	
	m_ToneMappingPanel->SetSizer( bSizer8 );
	m_ToneMappingPanel->Layout();
	bSizer8->Fit( m_ToneMappingPanel );
	m_Options_notebook->AddPage( m_ToneMappingPanel, wxT("Tone Mapping"), true );
	m_systemPanel = new wxPanel( m_Options_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 1, 2, 2, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxStaticText* m_staticText1;
	m_staticText1 = new wxStaticText( m_systemPanel, wxID_ANY, wxT("Display Interval: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALL, 5 );
	
	m_Display_spinCtrl = new wxSpinCtrl( m_systemPanel, ID_SYS_DISPLAY_INT, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 10000000, 12 );
	fgSizer1->Add( m_Display_spinCtrl, 0, 0, 5 );
	
	m_staticText2 = new wxStaticText( m_systemPanel, wxID_ANY, wxT("Write Interval: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALL, 5 );
	
	m_Write_spinCtrl = new wxSpinCtrl( m_systemPanel, ID_SYS_WRITE_INT, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 10000000, 120 );
	fgSizer1->Add( m_Write_spinCtrl, 0, 0, 5 );
	
	wxString m_writeOptionsChoices[] = { wxT("Write and Use FLM"), wxT("Tonemapped TGA"), wxT("Tonemapped EXR"), wxT("Untonemapped EXR"), wxT("Tonemapped IGI"), wxT("Untonemapped IGI") };
	int m_writeOptionsNChoices = sizeof( m_writeOptionsChoices ) / sizeof( wxString );
	m_writeOptions = new wxCheckListBox( m_systemPanel, ID_WRITE_OPTIONS, wxDefaultPosition, wxSize( -1,-1 ), m_writeOptionsNChoices, m_writeOptionsChoices, wxLB_NEEDED_SB );
	m_writeOptions->SetToolTip( wxT("Save Options") );
	
	fgSizer1->Add( m_writeOptions, 0, wxALL, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_SysApplyButton = new wxButton( m_systemPanel, ID_SYS_APPLY, wxT("Apply Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_SysApplyButton, 0, wxALL|wxSHAPED, 5 );
	
	m_systemPanel->SetSizer( fgSizer1 );
	m_systemPanel->Layout();
	fgSizer1->Fit( m_systemPanel );
	m_Options_notebook->AddPage( m_systemPanel, wxT("System"), false );
	
	bSizer7->Add( m_Options_notebook, 1, wxEXPAND | wxALL, 5 );
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( m_OptionsDialog::OnClose ) );
	m_TM_choice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_preText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_preText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_postText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_burnText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_TM_resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_refreshButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
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
	m_TM_choice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_prescaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_preText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_preText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postscaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_postText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_postText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( m_OptionsDialog::OnScroll ), NULL, this );
	m_RH_burnText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_RH_burnText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( m_OptionsDialog::OnText ), NULL, this );
	m_TM_resetButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_refreshButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_Display_spinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( m_OptionsDialog::OnSpin ), NULL, this );
	m_Write_spinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( m_OptionsDialog::OnSpin ), NULL, this );
	m_writeOptions->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_writeOptions->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_writeOptions->Disconnect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
	m_SysApplyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( m_OptionsDialog::OnMenu ), NULL, this );
}
