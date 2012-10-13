/***************************************************************************
 *   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 *
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

#define TAB_ID_RENDER  1
#define TAB_ID_QUEUE   2
#define TAB_ID_NETWORK 3
#define TAB_ID_LOG     4

#define OUTPUT_TAB_ID_IMAGING		0
#define OUTPUT_TAB_ID_LIGHTGROUP	1
#define OUTPUT_TAB_ID_REFINE		2
#define OUTPUT_TAB_ID_ADVANCED		3

#include <cmath>
#include <ctime>

#include <QProgressDialog>
#include <QInputDialog>

#include <QList>

#include <QDateTime>
#include <QTextStream>

#include <QTextLayout>
#include <QFontMetrics>

#include <QStringListModel>

#include "error.h"

#include "mainwindow.hxx"
#include "ui_luxrender.h"
#include "aboutdialog.hxx"
#include "batchprocessdialog.hxx"
#include "openexroptionsdialog.hxx"
#include "guiutil.h"

inline int Floor2Int(float val) {
	return static_cast<int>(std::floor(val));
}

bool copyLog2Console = false;
int EVT_LUX_PARSEERROR = QEvent::registerEventType();
int EVT_LUX_FINISHED = QEvent::registerEventType();
int EVT_LUX_TONEMAPPED = QEvent::registerEventType();
int EVT_LUX_FLMLOADERROR = QEvent::registerEventType();
int EVT_LUX_SAVEDFLM = QEvent::registerEventType();
int EVT_LUX_LOGEVENT = QEvent::registerEventType();
int EVT_LUX_BATCHEVENT = QEvent::registerEventType();
int EVT_LUX_NETWORKUPDATETREEEVENT = QEvent::registerEventType();

LuxLogEvent::LuxLogEvent(int code,int severity,QString message) : QEvent((QEvent::Type)EVT_LUX_LOGEVENT), _message(message), _severity(severity), _code(code)
{
	setAccepted(false);
}

BatchEvent::BatchEvent(const QString &currentFile, const int &numCompleted, const int &total)
	: QEvent((QEvent::Type)EVT_LUX_BATCHEVENT), _currentFile(currentFile), _numCompleted(numCompleted), _total(total)
{
	setAccepted(false);
}

NetworkUpdateTreeEvent::NetworkUpdateTreeEvent() : QEvent((QEvent::Type)EVT_LUX_NETWORKUPDATETREEEVENT)
{
	setAccepted(false);
}


void updateWidgetValue(QSlider *slider, int value)
{
	slider->blockSignals (true);
	slider->setValue (value);
	slider->blockSignals (false);
}

void updateWidgetValue(QDoubleSpinBox *spinbox, double value)
{
	spinbox->blockSignals (true);
	spinbox->setValue (value);
	spinbox->blockSignals (false);
}

void updateWidgetValue(QSpinBox *spinbox, int value)
{
	spinbox->blockSignals (true);
	spinbox->setValue (value);
	spinbox->blockSignals (false);
}

void updateWidgetValue(QCheckBox *checkbox, bool checked)
{
	checkbox->blockSignals (true);
	checkbox->setChecked(checked);
	checkbox->blockSignals (false);
}

void updateParam(luxComponent comp, luxComponentParameters param, double value, int index)
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		// Update lux's film
		luxSetParameterValue(comp, param, value, index);
	}
}

void updateParam(luxComponent comp, luxComponentParameters param, const char* value, int index)
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		// Update lux's film
		luxSetStringParameterValue(comp, param, value, index);
	}
}

double retrieveParam(bool useDefault, luxComponent comp, luxComponentParameters param, int index)
{
	if (useDefault)
		return luxGetDefaultParameterValue(comp, param, index);
	else
		return luxGetParameterValue(comp, param, index);
}

QWidget *MainWindow::instance;
MainWindow::MainWindow(QWidget *parent, bool copylog2console) : QMainWindow(parent), ui(new Ui::MainWindow), m_copyLog2Console(copylog2console)
{
	MainWindow::instance = this;

	luxErrorHandler(&MainWindow::LuxGuiErrorHandler);

	ui->setupUi(this);

#if defined(__APPLE__)
	ui->menubar->setNativeMenuBar(true);
#endif

	createActions();

	// File menu slots
	connect(ui->action_openFile, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(ui->action_resumeFLM, SIGNAL(triggered()), this, SLOT(resumeFLM()));
	connect(ui->action_loadFLM, SIGNAL(triggered()), this, SLOT(loadFLM()));
	connect(ui->action_saveFLM, SIGNAL(triggered()), this, SLOT(saveFLM()));
	connect(ui->action_exitAppSave, SIGNAL(triggered()), this, SLOT(exitAppSave()));
	connect(ui->action_exitApp, SIGNAL(triggered()), this, SLOT(exitApp()));
	connect(ui->action_Save_Panel_Settings, SIGNAL(triggered()), this, SLOT(SavePanelSettings()));
	connect(ui->action_Load_Panel_Settings, SIGNAL(triggered()), this, SLOT(LoadPanelSettings()));

	// Export to Image sub-menu slots
	connect(ui->action_outputTonemapped, SIGNAL(triggered()), this, SLOT(outputTonemapped()));
	connect(ui->action_outputHDR, SIGNAL(triggered()), this, SLOT(outputHDR()));
	connect(ui->action_outputBufferGroupsTonemapped, SIGNAL(triggered()), this, SLOT(outputBufferGroupsTonemapped()));
	connect(ui->action_outputBufferGroupsHDR, SIGNAL(triggered()), this, SLOT(outputBufferGroupsHDR()));
	connect(ui->action_batchProcess, SIGNAL(triggered()), this, SLOT(batchProcess()));

	// Render menu slots
	connect(ui->action_resumeRender, SIGNAL(triggered()), this, SLOT(resumeRender()));
	connect(ui->button_resume, SIGNAL(clicked()), this, SLOT(resumeRender()));
	connect(ui->action_pauseRender, SIGNAL(triggered()), this, SLOT(pauseRender()));
	connect(ui->button_pause, SIGNAL(clicked()), this, SLOT(pauseRender()));
	connect(ui->action_stopRender, SIGNAL(triggered()), this, SLOT(stopRender()));
	connect(ui->button_stop, SIGNAL(clicked()), this, SLOT(stopRender()));
	connect(ui->action_endRender, SIGNAL(triggered()), this, SLOT(endRender()));

	// View menu slots
	connect(ui->action_copyLog, SIGNAL(triggered()), this, SLOT(copyLog()));
	connect(ui->action_clearLog, SIGNAL(triggered()), this, SLOT(clearLog()));
	connect(ui->action_fullScreen, SIGNAL(triggered()), this, SLOT(fullScreen()));
	connect(ui->action_normalScreen, SIGNAL(triggered()), this, SLOT(normalScreen()));
	connect(ui->action_showAlphaView, SIGNAL(triggered(bool)), this, SLOT(showAlphaChanged(bool)));
	connect(ui->action_overlayStatsView, SIGNAL(triggered(bool)), this, SLOT(overlayStatsChanged(bool)));
	connect(ui->action_showUserSamplingMapView, SIGNAL(triggered(bool)), this, SLOT(showUserSamplingMapChanged(bool)));
	connect(ui->action_Show_Side_Panel, SIGNAL(triggered(bool)), this, SLOT(ShowSidePanel(bool)));

	// Help menu slots
	connect(ui->action_aboutDialog, SIGNAL(triggered()), this, SLOT(aboutDialog()));
	connect(ui->action_documentation, SIGNAL(triggered()), this, SLOT(openDocumentation()));
	connect(ui->action_forums, SIGNAL(triggered()), this, SLOT(openForums()));
	connect(ui->action_gallery, SIGNAL(triggered()), this, SLOT(openGallery()));
	connect(ui->action_bugtracker, SIGNAL(triggered()), this, SLOT(openBugTracker()));

	connect(ui->checkBox_imagingAuto, SIGNAL(stateChanged(int)), this, SLOT(autoEnabledChanged(int)));
	connect(ui->spinBox_overrideDisplayInterval, SIGNAL(valueChanged(int)), this, SLOT(overrideDisplayIntervalChanged(int)));

	// Panes
	panes[0] = new PaneWidget(ui->panesAreaContents, "Tone Mapping", ":/icons/tonemapicon.png");
	panes[1] = new PaneWidget(ui->panesAreaContents, "Color Space", ":/icons/colorspaceicon.png");
	panes[2] = new PaneWidget(ui->panesAreaContents, "Gamma + Film Response", ":/icons/gammaicon.png", true);
	panes[3] = new PaneWidget(ui->panesAreaContents, "HDR Histogram", ":/icons/histogramicon.png");
	panes[4] = new PaneWidget(ui->panesAreaContents, "Lens Effects", ":/icons/lenseffectsicon.png", true);
	panes[5] = new PaneWidget(ui->panesAreaContents, "Noise Reduction", ":/icons/noisereductionicon.png", true);

	advpanes[0] = new PaneWidget(ui->advancedAreaContents, "Scene information", ":/icons/logtabicon.png");

#if defined(__APPLE__) // cosmetical work to have the tabs less crowded
	ui->outputTabs->setFont(QFont  ("Lucida Grande", 10));
	QSize iconSize(12, 12);
	ui->outputTabs->setIconSize(iconSize);
#endif

	// Tonemap page
	tonemapwidget = new ToneMapWidget(panes[0]);
	panes[0]->setWidget(tonemapwidget);
	ui->panesLayout->addWidget(panes[0]);
	panes[0]->expand();
	connect(tonemapwidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Colorspace
	colorspacewidget = new ColorSpaceWidget(panes[1]);
	panes[1]->setWidget(colorspacewidget);
	ui->panesLayout->addWidget(panes[1]);
	connect(colorspacewidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Gamma
	gammawidget = new GammaWidget(panes[2]);
	panes[2]->setWidget(gammawidget);
	ui->panesLayout->addWidget(panes[2]);
	connect(gammawidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Histogram
	histogramwidget = new HistogramWidget(panes[3]);
	panes[3]->setWidget(histogramwidget);
	ui->panesLayout->addWidget(panes[3]);
	panes[3]->expand();

	// Lens effects page
	lenseffectswidget = new LensEffectsWidget(panes[4]);
	panes[4]->setWidget(lenseffectswidget);
	ui->panesLayout->addWidget(panes[4]);
	connect(lenseffectswidget, SIGNAL(forceUpdate()), this, SLOT(forceToneMapUpdate()));
	connect(lenseffectswidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Noise reduction
	noisereductionwidget = new NoiseReductionWidget(panes[5]);
	panes[5]->setWidget(noisereductionwidget);
	ui->panesLayout->addWidget(panes[5]);
	connect(noisereductionwidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	ui->panesLayout->setAlignment(Qt::AlignTop);
	ui->panesLayout->addItem(new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

	// Advanced info
	advancedinfowidget = new AdvancedInfoWidget(advpanes[0]);
	advpanes[0]->setWidget(advancedinfowidget);
	ui->advancedLayout->addWidget(advpanes[0]);
	advpanes[0]->expand();
	//connect(noisereductionwidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	ui->advancedLayout->setAlignment(Qt::AlignTop);
	ui->advancedLayout->addItem(new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

	ui->lightGroupsLayout->setAlignment(Qt::AlignTop);
	spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);

	// Network tab
	connect(ui->button_addServer, SIGNAL(clicked()), this, SLOT(addServer()));
	connect(ui->lineEdit_server, SIGNAL(returnPressed()), this, SLOT(addServer()));
	connect(ui->button_removeServer, SIGNAL(clicked()), this, SLOT(removeServer()));
	connect(ui->button_resetServer, SIGNAL(clicked()), this, SLOT(resetServer()));
	connect(ui->comboBox_updateInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(updateIntervalChanged(int)));
	connect(ui->comboBox_updateInterval->lineEdit(), SIGNAL(editingFinished()), this, SLOT(updateIntervalChanged()));
	connect(ui->table_servers, SIGNAL(itemSelectionChanged()), this, SLOT(networknodeSelectionChanged()));

	m_recentServersModel = new QStringMRUListModel(MaxRecentServers, this);
	ui->lineEdit_server->setCompleter(new QCompleter(m_recentServersModel, this));
	ui->lineEdit_server->completer()->setCompletionRole(Qt::DisplayRole);
	ui->comboBox_updateInterval->setValidator(new QIntValidator(1, 86400, this));

	setServerUpdateInterval(luxGetIntAttribute("render_farm", "pollingInterval"));

	// Queue tab
	ui->table_queue->setModel(&renderQueueData);
	addQueueHeaders();
	connect(ui->button_addQueueFiles, SIGNAL(clicked()), this, SLOT(addQueueFiles()));
	connect(ui->button_removeQueueFiles, SIGNAL(clicked()), this, SLOT(removeQueueFiles()));
	connect(ui->spinBox_overrideHaltSpp, SIGNAL(valueChanged(int)), this, SLOT(overrideHaltSppChanged(int)));
	connect(ui->spinBox_overrideHaltTime, SIGNAL(valueChanged(int)), this, SLOT(overrideHaltTimeChanged(int)));
	connect(ui->checkBox_loopQueue, SIGNAL(stateChanged(int)), this, SLOT(loopQueueChanged(int)));
	connect(ui->checkBox_overrideWriteFlm, SIGNAL(toggled(bool)), this, SLOT(overrideWriteFlmChanged(bool)));
	ui->table_queue->setColumnHidden(2, true);
	ui->table_queue->setColumnHidden(3, true);
	
	// Log tab
	connect(ui->comboBox_verbosity, SIGNAL(currentIndexChanged(int)), this, SLOT(setVerbosity(int)));

	// Buttons
	connect(ui->button_imagingApply, SIGNAL(clicked()), this, SLOT(applyTonemapping()));
	connect(ui->button_imagingReset, SIGNAL(clicked()), this, SLOT(resetToneMapping()));

	connect(ui->tabs_main, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
	connect(ui->outputTabs, SIGNAL(currentChanged(int)), this, SLOT(outputTabChanged(int)));

	// User driven sampling tab
	connect(ui->button_usAddPenButton, SIGNAL(clicked()), this, SLOT(userSamplingAddPen()));
	connect(ui->button_usSubPenButton, SIGNAL(clicked()), this, SLOT(userSamplingSubPen()));
	connect(ui->button_usApplyButton, SIGNAL(clicked()), this, SLOT(userSamplingApply()));
	connect(ui->button_usUndoButton, SIGNAL(clicked()), this, SLOT(userSamplingUndo()));
	connect(ui->button_usResetButton, SIGNAL(clicked()), this, SLOT(userSamplingReset()));
	connect(ui->slider_usPenSize, SIGNAL(valueChanged(int)), this, SLOT(userSamplingPenSize(int)));
	connect(ui->slider_usOpacity, SIGNAL(valueChanged(int)), this, SLOT(userSamplingMapOpacity(int)));
	connect(ui->slider_usPenStrength, SIGNAL(valueChanged(int)), this, SLOT(userSamplingPenStrength(int)));

	// Render threads
	connect(ui->spinBox_Threads, SIGNAL(valueChanged(int)), this, SLOT(ThreadChanged(int)));

	// Clipboard
	connect(ui->button_copyToClipboard, SIGNAL(clicked()), this, SLOT(copyToClipboard()));

	// Statusbar
	activityLabel = new QLabel(tr("  Status:"));
	activityMessage = new QLabel();

	statusLabel = new QLabel(tr(" Activity:"));
	statusMessage = new QLabel();
	statusProgress = new QProgressBar();

	statsLabel = new QLabel(tr(" Statistics:"));
	statsBox = new QFrame();
	statsBoxLayout = new QHBoxLayout(statsBox);

	activityLabel->setMaximumWidth(60);
	activityMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	activityMessage->setMaximumWidth(140);

	statusLabel->setMaximumWidth(60);
	statusMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statusMessage->setMaximumWidth(320);
	statusMessage->setMinimumWidth(100);
	statusProgress->setMaximumWidth(100);
	statusProgress->setTextVisible(false);
	statusProgress->setRange(0, 100);

	statsLabel->setMaximumWidth(70);
	statsBox->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statsBoxLayout->setSpacing(0);
	statsBoxLayout->setContentsMargins(3, 0, 3, 0);
	statsBoxLayout->addStretch(-1);

	ui->statusbar->addPermanentWidget(activityLabel, 1);
	ui->statusbar->addPermanentWidget(activityMessage, 1);

	ui->statusbar->addPermanentWidget(statusLabel, 1);
	ui->statusbar->addPermanentWidget(statusMessage, 1);
	ui->statusbar->addPermanentWidget(statusProgress, 1);

	ui->statusbar->addPermanentWidget(statsLabel, 1);
	ui->statusbar->addPermanentWidget(statsBox, 1);

	// Update timers
	m_renderTimer = new QTimer();
	connect(m_renderTimer, SIGNAL(timeout()), SLOT(renderTimeout()));

	m_statsTimer = new QTimer();
	connect(m_statsTimer, SIGNAL(timeout()), SLOT(statsTimeout()));

	m_loadTimer = new QTimer();
	connect(m_loadTimer, SIGNAL(timeout()), SLOT(loadTimeout()));

	m_saveTimer = new QTimer();
	connect(m_saveTimer, SIGNAL(timeout()), SLOT(saveTimeout()));

	m_netTimer = new QTimer();
	connect(m_netTimer, SIGNAL(timeout()), SLOT(netTimeout()));

	m_blinkTimer = new QTimer();
	connect(m_blinkTimer, SIGNAL(timeout()), SLOT(blinkTrigger()));

	// Init render area
	renderView = new RenderView(ui->frame_render);
	ui->renderLayout->addWidget(renderView, 0, 0, 1, 1);
	connect(renderView, SIGNAL(viewChanged()), this, SLOT(viewportChanged()));
	renderView->show ();

	ui->outputTabs->setEnabled (false);

	// Initialize threads
	m_numThreads = 0;

	m_engineThread = NULL;
	m_updateThread = NULL;
	m_flmloadThread = NULL;
	m_flmsaveThread = NULL;

	m_batchProcessThread = NULL;
	batchProgress = NULL;

	m_networkAddRemoveSlavesThread = NULL;

	openExrHalfFloats = true;
	openExrDepthBuffer = false;
	openExrCompressionType = 1;

	// used in ResetToneMapping
	m_auto_tonemap = false;
	resetToneMapping();
	m_auto_tonemap = true;

	// hack for "tonemapping lag"
	m_bTonemapPending = false;

	copyLog2Console = m_copyLog2Console;
	luxErrorHandler(&LuxGuiErrorHandler);

	//m_renderTimer->Start(1000*luxStatistics("displayInterval"));

	// Set default splitter sizes
	QList<int> sizes;
	sizes << 500 << 700;
	ui->splitter->setSizes(sizes);

	changeRenderState(WAITING);

	ReadSettings();
}

MainWindow::~MainWindow()
{
	if (m_guiRenderState != WAITING)
		endRenderingSession();

	for (QVector<PaneWidget*>::iterator it = m_LightGroupPanes.begin(); it != m_LightGroupPanes.end(); it++) {
		PaneWidget *currWidget = *it;
		ui->lightGroupsLayout->removeWidget(currWidget);
		delete currWidget;
	}
	m_LightGroupPanes.clear();

	// TODO - figure out why core has been reset on OSX but not Windows
	updateIntervalChanged();
	WriteSettings();

	delete ui;
	delete statsBoxLayout;
	delete statsBox;
	delete m_engineThread;
	delete m_updateThread;
	delete m_flmloadThread;
	delete m_flmsaveThread;
	delete m_renderTimer;
	delete m_batchProcessThread;
	delete renderView;
	delete tonemapwidget;
	delete lenseffectswidget;
	delete colorspacewidget;
	delete noisereductionwidget;
	delete histogramwidget;
	delete batchProgress;

	for (int i = 0; i < NumPanes; i++)
		delete panes[i];
}

void MainWindow::createActions()
{
	for (int i = 0; i < MaxRecentFiles; ++i) {
		m_recentFileActions[i] = new QAction(this);
		m_recentFileActions[i]->setVisible(false);
		connect(m_recentFileActions[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}

	for (int i = 0; i < MaxRecentFiles; ++i) {
		ui->menuOpen_Recent->addAction(m_recentFileActions[i]);
	}
}

int MainWindow::getTabIndex(int tabID) 
{
	switch(tabID) {
		case TAB_ID_LOG:
			return ui->tabs_main->indexOf(ui->tab_log);
		case TAB_ID_NETWORK:
			return ui->tabs_main->indexOf(ui->tab_network);
		case TAB_ID_RENDER:
			return ui->tabs_main->indexOf(ui->tab_render);
		case TAB_ID_QUEUE:
			return ui->tabs_main->indexOf(ui->tab_queue);
		default:
			return -1;
	}
}

void MainWindow::ReadSettings()
{
	QSettings settings("luxrender.net", "LuxRender GUI");

	settings.beginGroup("MainWindow");
	restoreGeometry(settings.value("geometry").toByteArray());
	ui->splitter->restoreState(settings.value("splittersizes").toByteArray());
	{
		QStringList recentFilesList = settings.value("recentFiles").toStringList();
		QStringListIterator i(recentFilesList);
		while (i.hasNext())
			m_recentFiles.append(QFileInfo(i.next()));
	}
	m_lastOpendir = settings.value("lastOpenDir","").toString();
	ui->action_overlayStats->setChecked(settings.value("overlayStatistics").toBool());
	ui->action_HDR_tonemapped->setChecked(settings.value("tonemappedHDR").toBool());
	ui->action_useAlpha->setChecked(settings.value("outputUseAlpha").toBool());
	ui->action_useAlphaHDR->setChecked(settings.value("outputUseAlphaHDR").toBool());
	m_recentServersModel->setList(settings.value("recentServers").toStringList());
	setServerUpdateInterval(settings.value("serverUpdateInterval", luxGetIntAttribute("render_farm", "pollingInterval")).toInt());
	ui->action_Show_Side_Panel->setChecked(settings.value("outputTabs", 1 ).toBool());
	ui->outputTabs->setVisible(ui->action_Show_Side_Panel->isChecked());
	settings.endGroup();

	updateRecentFileActions();
}

void MainWindow::WriteSettings()
{
	QSettings settings("luxrender.net", "LuxRender GUI");

	settings.beginGroup("MainWindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("splittersizes", ui->splitter->saveState());
	{
		QListIterator<QFileInfo> i(m_recentFiles);
		QStringList recentFilesList;
		while (i.hasNext())
			recentFilesList.append(i.next().absoluteFilePath());
		settings.setValue("recentFiles", recentFilesList);
	}
	settings.setValue("lastOpenDir", m_lastOpendir);
	settings.setValue("overlayStatistics", ui->action_overlayStats->isChecked());
	settings.setValue("tonemappedHDR", ui->action_HDR_tonemapped->isChecked());
	settings.setValue("outputUseAlpha", ui->action_useAlpha->isChecked());
	settings.setValue("outputUseAlphaHDR", ui->action_useAlphaHDR->isChecked());
	settings.setValue("outputTabs", ui->action_Show_Side_Panel->isChecked());
	settings.setValue("recentServers", QStringList(m_recentServersModel->list()));
	settings.setValue("serverUpdateInterval", luxGetIntAttribute("render_farm", "pollingInterval"));
	settings.endGroup();
}

void MainWindow::ShowTabLogIcon ( int tabID, const QIcon & icon )
{
	int tab_index = getTabIndex(tabID);

	if (tab_index != -1) {
		ui->tabs_main->setTabIcon(tab_index, icon);			
	}
}

void MainWindow::toneMapParamsChanged()
{
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::indicateActivity(bool active)
{
	if (active) {
		statusProgress->setRange(0, 0);
	} else {
		statusProgress->setRange(0, 100);
	}
}

void MainWindow::forceToneMapUpdate()
{
	applyTonemapping(true);
}

void MainWindow::openDocumentation ()
{
	QDesktopServices::openUrl(QUrl("http://www.luxrender.net/wiki/index.php?title=Main_Page"));
}

void MainWindow::openForums ()
{
	QDesktopServices::openUrl(QUrl("http://www.luxrender.net/forum/"));
}

void MainWindow::openGallery ()
{
	QDesktopServices::openUrl(QUrl("http://www.luxrender.net/gallery"));
}

void MainWindow::openBugTracker ()
{
	QDesktopServices::openUrl(QUrl("http://www.luxrender.net/mantis"));
}

void MainWindow::ThreadChanged(int value)
{
	SetRenderThreads (value);
}

void MainWindow::copyToClipboard ()
{
	renderView->copyToClipboard();
}

void MainWindow::autoEnabledChanged (int value)
{
	if (value == Qt::Checked)
		m_auto_tonemap = true;
	else
		m_auto_tonemap = false;

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::overrideDisplayIntervalChanged(int value)
{
	if (m_guiRenderState != RENDERING)
		return;

	if (value > 0)
		luxSetIntAttribute("film", "displayInterval", value);
	else
		luxSetIntAttribute("film", "displayInterval", luxGetIntAttributeDefault("film", "displayInterval"));

	if (m_renderTimer->isActive())
		m_renderTimer->setInterval(1000*luxGetIntAttribute("film", "displayInterval"));
}

void MainWindow::LuxGuiErrorHandler(int code, int severity, const char *msg)
{
	if (copyLog2Console)
		luxErrorPrint(code, severity, msg);

	qApp->postEvent(MainWindow::instance, new LuxLogEvent(code,severity,QString(msg)));
}

bool MainWindow::canStopRendering()
{
	if (m_guiRenderState == RENDERING) {
		// Give warning that current rendering is not stopped
		const int rejectButton = 1;
		if (customMessageBox(this, QMessageBox::Question, 
				tr("Rendering in progress"),
				tr("Do you want to stop the current render and load the new scene?"),
				CustomButtonsList()
					<< qMakePair(tr("Load new scene"), QMessageBox::AcceptRole)
					<< qMakePair(tr("Cancel"), QMessageBox::RejectRole),				
				rejectButton) == rejectButton) {
			return false;
		}
	}
	return true;
}

// File menu slots
void MainWindow::exitAppSave()
{
	endRenderingSession(false);
	qApp->exit();
}

void MainWindow::exitApp()
{
	// abort rendering
	endRenderingSession(true);
}

void MainWindow::openFile()
{
	if (!canStopRendering())
		return;

	QString fileName = QFileDialog::getOpenFileName(this, tr("Choose a scene- or queue-file file to open"), m_lastOpendir, tr("LuxRender Files (*.lxs *.lxq)"));

	if(!fileName.isNull()) {
		if (fileName.endsWith(".lxs")){
			renderNewScenefile(fileName);
		} else {
			// handle queue files
			openQueueFile(fileName);
		}
	}
}

void MainWindow::openQueueFile(const QString& fileName)
{
	QFile listFile(fileName);
	QString renderQueueEntry;
	if ( listFile.open(QIODevice::ReadOnly) ) {
		LOG(LUX_INFO, LUX_NOERROR) << "Reading queue file '" << qPrintable(fileName) << "'";
		QTextStream lfStream(&listFile);
		QDir::setCurrent(QFileInfo(fileName).absoluteDir().path());
		while(!lfStream.atEnd()) {
			QString name(lfStream.readLine());
			renderQueueEntry = QFileInfo(name).absoluteFilePath();
			if (!renderQueueEntry.isNull()) {
				LOG(LUX_INFO, LUX_NOERROR) << "Adding file '" << qPrintable(name) << "'";
				renderQueueList << renderQueueEntry;
			} else {
				LOG(LUX_WARNING, LUX_NOFILE) << "Skipping file '" << qPrintable(name) << "'";
			}
		};
	} else {
		LOG(LUX_ERROR, LUX_NOFILE) << "Could not open queue file '" << qPrintable(fileName) << "'";
	}
	if (renderQueueList.count()) {
		ui->tabs_main->setCurrentIndex(1);	// jump to queue tab
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setText("Please select the desired haltspp or halttime for this queue");
		msgBox.exec();
		foreach( renderQueueEntry, renderQueueList ) {
			addFileToRenderQueue(renderQueueEntry);
		}
		RenderNextFileInQueue(false);
	}
}	
	
void MainWindow::openRecentFile()
{
	if (!canStopRendering())
		return;

	QAction *action = qobject_cast<QAction *>(sender());

	if (action) {
		renderNewScenefile(action->data().toString());
	}
}

void MainWindow::resumeFLM()
{
	if (!canStopRendering())
		return;

	QString lxsFileName = QFileDialog::getOpenFileName(this, tr("Choose a scene file to open"), m_lastOpendir, tr("LuxRender Files (*.lxs)"));

	if(lxsFileName.isNull())
		return;

	// suggest .flm file with same name if it exists
	QFileInfo info(lxsFileName);
	QFileInfo openDirFile(info.absolutePath() + "/" + info.completeBaseName() + ".flm");
	QString openDirName = openDirFile.exists() ? openDirFile.absoluteFilePath() : info.absolutePath();

	QString flmFileName = QFileDialog::getOpenFileName(this, tr("Choose an FLM file to open"), openDirName, tr("LuxRender FLM files (*.flm)"));

	if(flmFileName.isNull())
		return;

	renderNewScenefile(lxsFileName, flmFileName);
}

void MainWindow::loadFLM(QString flmFileName)
{
	if (!canStopRendering())
		return;

	if (flmFileName.isNull() || flmFileName.isEmpty())
			flmFileName = QFileDialog::getOpenFileName(this, tr("Choose an FLM file to open"), m_lastOpendir, tr("LuxRender FLM files (*.flm)"));

	if(flmFileName.isNull())
		return;

	setCurrentFile(flmFileName); // show flm-name in windowheader

	endRenderingSession();
	ClearRenderingQueue();

	//SetTitle(wxT("LuxRender - ")+fn.GetName());

	indicateActivity ();
	statusMessage->setText("Loading FLM...");

	// Start load thread
	m_loadTimer->start(1000);

	delete m_flmloadThread;
	m_flmloadThread = new FlmLoadThread(this, flmFileName);
	m_flmloadThread->start();
}

void MainWindow::saveFLM()
{
	if( !luxStatistics("sceneIsReady") && !luxStatistics("filmIsReady") )
		return;

	if(m_guiRenderState == WAITING )
		return;

	// add filename suggestion
	QString saveDirName = (m_CurrentFile.isEmpty() || m_CurrentFile == "-") ? m_lastOpendir : m_lastOpendir + "/" + m_CurrentFileBaseName + ".flm";

	QString flmFileName = QFileDialog::getSaveFileName(this, tr("Choose an FLM file to save to"), saveDirName, tr("LuxRender FLM files (*.flm)"));

	if(flmFileName.isNull())
		return;

	// Start save thread

	indicateActivity ();
	statusMessage->setText("Saving FLM...");

	m_saveTimer->start(1000);

	delete m_flmsaveThread;
	m_flmsaveThread = new FlmSaveThread(this, flmFileName);
	m_flmsaveThread->start();
}

// Render menu slots
void MainWindow::resumeRender()
{
	if (m_guiRenderState != RENDERING && m_guiRenderState != TONEMAPPING) {
		//UpdateNetworkTree();

		// Start display update timer
		renderView->reload();
		histogramwidget->Update();

		m_renderTimer->start(1000*luxGetIntAttribute("film", "displayInterval"));
		m_statsTimer->start(1000);
		m_netTimer->start(10000);

		if (m_guiRenderState == STOPPED) {
			// reset flags, keep haltspp
			int haltspp = luxGetIntAttribute("film", "haltSamplesPerPixel");
			luxSetHaltSamplesPerPixel(haltspp, false, false);
		}

		if (m_guiRenderState == PAUSED || m_guiRenderState == STOPPED) // Only re-start if we were previously stopped
			luxStart();

		changeRenderState(RENDERING);
		showRenderresolution();
		showZoomfactor();
	}
}

void MainWindow::pauseRender()
{
	if (m_guiRenderState != PAUSED && m_guiRenderState != TONEMAPPING) {
		// We have to check if network rendering is enabled. In this case,
		// we don't stop timer in order to save the image to the disk, etc.
		if (luxGetIntAttribute("render_farm", "slaveNodeCount") < 1) {
			m_renderTimer->stop();
			m_statsTimer->stop();
		}

		if (m_guiRenderState == RENDERING)
			luxPause();

		changeRenderState(PAUSED);
	}
}

void MainWindow::stopRender()
{
	if ((m_guiRenderState == RENDERING || m_guiRenderState == PAUSED) && m_guiRenderState != TONEMAPPING) {
		m_renderTimer->stop();
		// Leave stats timer running so we know when threads stopped
		if (!m_statsTimer->isActive())
			m_statsTimer->start(1000);

		// Make sure lux core halts
		int haltspp = luxGetIntAttribute("film", "haltSamplesPerPixel");
		luxSetHaltSamplesPerPixel(haltspp, true, true);

		statusMessage->setText(tr("Waiting for render threads to stop."));
		changeRenderState(STOPPING);
	}
}

void MainWindow::endRender()
{
	if ((m_guiRenderState == RENDERING || m_guiRenderState == PAUSED) && m_guiRenderState != TONEMAPPING) {
		m_renderTimer->stop();
		// Leave stats timer running so we know when threads stopped
		if (!m_statsTimer->isActive())
			m_statsTimer->start(1000);

		// Make sure lux core shuts down
		int haltspp = luxGetIntAttribute("film", "haltSamplesPerPixel");
		luxSetHaltSamplesPerPixel(haltspp, true, false);

		statusMessage->setText(tr("Waiting for render threads to stop."));
		changeRenderState(ENDING);
	}
}

void MainWindow::outputTonemapped()
{
	QString filter;
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Tonemapped Image"), m_lastOpendir + "/" + m_CurrentFileBaseName, tr("PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;Windows Bitmap (*.bmp);;TIFF Image (*.tif *.tiff)"), &filter);
	if (fileName.isEmpty())
		return;
	QString suffix = QFileInfo(fileName).suffix().toLower();
	if (filter == "PNG Image (*.png)" && suffix != "png")
		fileName += ".png";
	else if (filter == "JPEG Image (*.jpg *.jpeg)" && suffix != "jpg" && suffix != "jpeg")
		fileName += ".jpg";
	else if (filter == "Windows Bitmap (*.bmp)" && suffix != "bmp")
		fileName += ".bmp";
	else if (filter == "TIFF Image (*.tif *.tiff)" && suffix != "tif" && suffix != "tiff")
		fileName += ".tif";

	if (saveCurrentImageTonemapped(fileName, ui->action_overlayStats->isChecked(), ui->action_useAlpha->isChecked())) {
		statusMessage->setText(tr("Tonemapped image saved"));
		LOG(LUX_INFO, LUX_NOERROR) << "Tonemapped image saved to '" << qPrintable(fileName) << "'";
	} else {
		statusMessage->setText(tr("ERROR: Tonemapped image NOT saved."));
		LOG(LUX_WARNING, LUX_SYSTEM) << "Error while saving tonemapped image to '" << qPrintable(fileName) << "'";
	}
}


void MainWindow::outputHDR()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save High Dynamic Range Image"), m_lastOpendir + "/" + m_CurrentFileBaseName, tr("OpenEXR Image (*.exr)"));
	if (fileName.isEmpty())
		return;

	// Get OpenEXR options
	OpenEXROptionsDialog *options = new OpenEXROptionsDialog(this, openExrHalfFloats, openExrDepthBuffer, openExrCompressionType);
	// options->disableZBufferCheckbox();		// TODO: Check if film has a ZBuffer and disable it here if not
	if(options->exec() == QDialog::Rejected) return;
	openExrHalfFloats = options->useHalfFloats();
	openExrDepthBuffer = options->includeZBuffer();
	openExrCompressionType = options->getCompressionType();
	delete options;

	if (saveCurrentImageHDR(fileName)) {
		statusMessage->setText(tr("HDR image saved"));
		LOG(LUX_INFO, LUX_NOERROR) << "HDR image saved to '" << qPrintable(fileName) << "'";
	} else {
		statusMessage->setText(tr("ERROR: HDR image NOT saved."));
		LOG(LUX_WARNING, LUX_SYSTEM) << "Error while saving HDR image to '" << qPrintable(fileName) << "'";
	}
}

bool MainWindow::saveCurrentImageHDR(const QString &outFile)
{
	// Done inside API for now (set openExr* members to control OpenEXR format options)
	if (ui->action_HDR_tonemapped->isChecked())
		luxSaveEXR(outFile.toAscii().data(), openExrHalfFloats, openExrDepthBuffer, openExrCompressionType, true);
	else
		luxSaveEXR(outFile.toAscii().data(), openExrHalfFloats, openExrDepthBuffer, openExrCompressionType, false);
	return true;
}

void MainWindow::outputBufferGroupsTonemapped()
{
	// Where should these be output
	QString filter;
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Tonemapped Image"), m_lastOpendir + "/" + m_CurrentFileBaseName, tr("PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;Windows Bitmap (*.bmp);;TIFF Image (*.tif *.tiff)"), &filter);
	if (fileName.isEmpty())
		return;
	QString suffix = QFileInfo(fileName).suffix().toLower();
	if (filter == "PNG Image (*.png)" && suffix != "png")
		fileName += ".png";
	else if (filter == "JPEG Image (*.jpg *.jpeg)" && suffix != "jpg" && suffix != "jpeg")
		fileName += ".jpg";
	else if (filter == "Windows Bitmap (*.bmp)" && suffix != "bmp")
		fileName += ".bmp";
	else if (filter == "TIFF Image (*.tif *.tiff)" && suffix != "tif" && suffix != "tiff")
		fileName += ".tif";

	if (fileName.isEmpty())
		return;

	// Show busy cursor
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// Output the light groups
	saveAllLightGroups(fileName, false);

	// Stop showing busy cursor
	QApplication::restoreOverrideCursor();
}

void MainWindow::outputBufferGroupsHDR()
{
	// Where should these be output
	QString fileName = QFileDialog::getSaveFileName(this, tr("Select a destination for the images"), m_lastOpendir, tr("OpenEXR Image (*.exr)"));
	if (fileName.isEmpty())
		return;

	// Get OpenEXR options
	OpenEXROptionsDialog *options = new OpenEXROptionsDialog(this, openExrHalfFloats, openExrDepthBuffer, openExrCompressionType);
	// options->disableZBufferCheckbox();		// TODO: Check if film has a ZBuffer and disable it here if not
	if(options->exec() == QDialog::Rejected) return;
	openExrHalfFloats = options->useHalfFloats();
	openExrDepthBuffer = options->includeZBuffer();
	openExrCompressionType = options->getCompressionType();
	delete options;

	// Show busy cursor
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// Output the light groups
	saveAllLightGroups(fileName, true);

	// Stop showing busy cursor
	QApplication::restoreOverrideCursor();
}

bool MainWindow::saveAllLightGroups(const QString &outFilename, const bool &asHDR)
{
	// Get number of light groups
	int lgCount = (int)luxGetParameterValue(LUX_FILM, LUX_FILM_LG_COUNT);

	// Start by save current light group state and turning off ALL light groups
	vector<float> prevLGState(lgCount);
	for (int i = 0; i < lgCount; ++i)
	{
		prevLGState[i] = luxGetParameterValue(LUX_FILM,
			LUX_FILM_LG_ENABLE, i);
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, 0.f, i);
	}

	// Prepare filename (will hack up when outputting each light group)
	QFileInfo filenamePath(outFilename);

	// Now, turn one light group on at a time, update the film and save to an image
	bool result = true;
	for (int i = 0; i < lgCount; ++i) {
		// Get light group name
		char lgName[256];
		luxGetStringParameterValue(LUX_FILM, LUX_FILM_LG_NAME, lgName,
			256, i);

		// Enable light group (and tonemap if not saving as HDR)
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, 1.f, i);
		if (!asHDR)
			luxUpdateFramebuffer();

		// Output image
		QString outputName = QDir(filenamePath.absolutePath()).absoluteFilePath(filenamePath.completeBaseName() + "-" + lgName);

		if (asHDR) {
			outputName += ".exr";
			luxSaveEXR(qPrintable(outputName),
				openExrHalfFloats, openExrDepthBuffer,
				openExrCompressionType,
				ui->action_HDR_tonemapped->isChecked());
			statusMessage->setText(tr("HDR image saved"));
			LOG(LUX_INFO, LUX_NOERROR) <<
				"Light group HDR image saved to '" <<
				qPrintable(outputName) << "'";
		} else {
			outputName += "." + filenamePath.suffix();
			result = saveCurrentImageTonemapped(outputName,
				ui->action_overlayStats->isChecked(),
				ui->action_useAlpha->isChecked());
			if (result) {
				statusMessage->setText(tr("Tonemapped image saved"));
				LOG(LUX_INFO, LUX_NOERROR) <<
					"Light group tonemapped image saved to '" <<
					qPrintable(outputName) << "'";
			} else {
				statusMessage->setText(tr("ERROR: Tonemapped image NOT saved."));
				LOG(LUX_WARNING, LUX_SYSTEM) <<
					"Error while saving light group tonemapped image to '" <<
					qPrintable(outputName) << "'";
			}
		}

		// Turn group back off
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, 0.f, i);
		if (!result)
			break;
	}

	// Restore previous light group state
	for (int i = 0; i < lgCount; ++i)
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE,
			prevLGState[i], i);

	if (!asHDR)
		luxUpdateFramebuffer();

	// Report success or failure (always success when outputting HDR)
	return result;
}

void MainWindow::batchProcess()
{
	// Are we rendering?
	if (!canStopRendering()) 
		return;

	// Is there already film in the camera?
	if(luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		int ret = QMessageBox::warning(this, tr("Film/Scene Loaded"),
			tr("There is exposed film in the camera (from a previously loaded scene or flm file). "
			"You must discard this film before starting a batch process.\n\n"
			"Save the existing film?"),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

		switch(ret)
		{
			// Call save function instead, then fall-through to abort
			case QMessageBox::Save: saveFLM();

			// Return from function/abort batch processing
			default: // fall thru
			case QMessageBox::Cancel:
				return;
			break;

			// Continue with batch processing
			case QMessageBox::Discard: break;
		}
	}

	// Show the batch processing dialog for user to customize
	BatchProcessDialog *batchDialog = new BatchProcessDialog(m_lastOpendir, this);
	if(batchDialog->exec() == QDialog::Rejected)
		return;

	// Get options from dialog
	bool allLightGroups = batchDialog->individualLightGroups();
	QString inDir = batchDialog->inputDir();
	QString outDir = batchDialog->outputDir();
	bool asHDR = !batchDialog->applyTonemapping();

	// Get OpenEXR options
	if(asHDR)
	{
		OpenEXROptionsDialog *options = new OpenEXROptionsDialog(this, openExrHalfFloats, openExrDepthBuffer, openExrCompressionType);
		// options->disableZBufferCheckbox();		// TODO: Check if film has a ZBuffer and disable it here if not
		if(options->exec() == QDialog::Rejected) return;
		openExrHalfFloats = options->useHalfFloats();
		openExrDepthBuffer = options->includeZBuffer();
		openExrCompressionType = options->getCompressionType();
		delete options;
	}

	// Make sure rendering is ended
	endRenderingSession();
	ClearRenderingQueue();

	QString outExtension = "exr";
	if(!asHDR) {
		switch(batchDialog->format()) {
			default:
			case 0: outExtension = "png"; break;
			case 1: outExtension = "jpg"; break;
			case 2: outExtension = "bmp"; break;
			case 3: outExtension = "tif"; break;
		}
	}

	// Show modal progress dialog
	if(batchProgress != NULL) delete batchProgress;
	batchProgress = new QProgressDialog(tr("Processing ..."), tr("Cancel"), 0, 0, this);
	batchProgress->setSizeGripEnabled(false);
	batchProgress->setModal(true);
	batchProgress->show();

	// Execute in seperate thread
	if(m_batchProcessThread != NULL)
		delete m_batchProcessThread;
	m_batchProcessThread = new BatchProcessThread(this, inDir, outDir, outExtension, allLightGroups, asHDR);
	m_batchProcessThread->start();
}

// Stop rendering session entirely - this is different from stopping it; it's not resumable
void MainWindow::endRenderingSession(bool abort)
{
	statusMessage->setText("");
	// Clean up if this is not the first rendering
	if (m_guiRenderState != WAITING) {
		changeRenderState(ENDING);
		// at least give user some hints of what's going on
		activityMessage->setText("Shutting down...");
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

		m_renderTimer->stop ();
		m_statsTimer->stop ();
		m_netTimer->stop ();
		if (!IsFileQueued())
			// dont clear log if rendering a queue
			clearLog();

		if (m_flmloadThread)
			m_flmloadThread->wait();
		delete m_flmloadThread;
		m_flmloadThread = NULL;
		if (m_flmsaveThread)
			m_flmsaveThread->wait();
		delete m_flmsaveThread;
		m_flmsaveThread = NULL;
		if (m_updateThread)
			m_updateThread->wait();
		delete m_updateThread;
		m_updateThread = NULL;

		// TODO - make this async as it can block for tens of seconds
		if (abort)
			luxAbort();
		else
			luxExit();
		if (m_engineThread)
			m_engineThread->wait();
		delete m_engineThread;
		m_engineThread = NULL;
		m_CurrentFile.clear();
		changeRenderState(WAITING);
		renderView->setLogoMode ();
	}
	LOG( LUX_INFO,LUX_NOERROR)<< "Freeing resources.";
	luxCleanup();
}

void MainWindow::copyLog()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->textEdit_log->toPlainText());
}

void MainWindow::clearLog()
{
	ui->textEdit_log->setPlainText("");
	blinkTrigger(false);
}

void MainWindow::fullScreen()
{
	if (renderView->isFullScreen()) {
		delete renderView; // delete and reinitialize to recenter render
		renderView = new RenderView(ui->frame_render);
		ui->renderLayout->addWidget(renderView, 0, 0, 1, 1);
		connect(renderView, SIGNAL(viewChanged()), this, SLOT(viewportChanged())); // reconnect
		renderView->reload();
		renderView->show ();
		ui->action_normalScreen->setEnabled (false);
		showAlphaChanged(ui->action_showAlphaView->isChecked());
		overlayStatsChanged(ui->action_overlayStatsView->isChecked());
		showUserSamplingMapChanged(ui->action_showUserSamplingMapView->isChecked());
	}
	else {
		renderView->setParent( NULL );
		renderView->move(pos()); // move renderview to same monitor as mainwindow
		renderView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
		renderView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
		renderView->showFullScreen();
		ui->action_normalScreen->setEnabled (true);
	}
}

void MainWindow::normalScreen()
{
	if (renderView->isFullScreen()) {
		delete renderView; // delete and reinitialize to recenter render
		renderView = new RenderView(ui->frame_render);
		ui->renderLayout->addWidget(renderView, 0, 0, 1, 1);
		connect(renderView, SIGNAL(viewChanged()), this, SLOT(viewportChanged())); // reconnect
		renderView->reload();
		renderView->show ();
		ui->action_normalScreen->setEnabled (false);
	}
}

void MainWindow::overlayStatsChanged(bool checked)
{
	renderView->setOverlayStatistics(checked);
	renderView->reload();
}

void MainWindow::showAlphaChanged(bool checked)
{
	renderView->setShowAlpha(checked);
	static const QIcon alphaicon(":/icons/clipboardicon_alpha.png");
	static const QIcon icon(":/icons/clipboardicon.png");
	if(checked){
		ui->button_copyToClipboard->setIcon(alphaicon);
	}else{
		ui->button_copyToClipboard->setIcon(icon);	
	}
	renderView->reload();
}

void MainWindow::showUserSamplingMapChanged(bool checked)
{
	renderView->setShowUserSamplingMap(checked);
	renderView->reload();

	if (checked)
		ui->outputTabs->setCurrentIndex(OUTPUT_TAB_ID_REFINE);
	else
		ui->outputTabs->setCurrentIndex(OUTPUT_TAB_ID_IMAGING);
}

// Help menu slots
void MainWindow::aboutDialog()
{
	AboutDialog *dialog = new AboutDialog();

	dialog->exec();
}

void MainWindow::applyTonemapping(bool withlayercomputation)
{
	if (m_updateThread == NULL && ( luxStatistics("sceneIsReady") || luxStatistics("filmIsReady") ) &&
		(!isMinimized () || m_guiRenderState == FINISHED)) {
		if (!withlayercomputation) {
			LOG(LUX_DEBUG,LUX_NOERROR)<< tr("GUI: Updating framebuffer...").toLatin1().data();
			statusMessage->setText(tr("Tonemapping..."));
		} else {
			LOG(LUX_DEBUG,LUX_NOERROR)<< tr("GUI: Updating framebuffer/Computing Lens Effect Layer(s)...").toLatin1().data();
			statusMessage->setText(tr("Computing Lens Effect Layer(s) & Tonemapping..."));
			indicateActivity();
		}
		m_updateThread = new UpdateThread(this);
		m_updateThread->start();
	}
	else if (m_updateThread != NULL ) // Tonemapping in progress, request second event, hack for "tonemapping lag."
	{
		m_bTonemapPending = true;
	}
}

void MainWindow::userSamplingAddPen() {
	renderView->setUserSamplingPen(true);
}

void MainWindow::userSamplingSubPen() {
	renderView->setUserSamplingPen(false);
}

void MainWindow::userSamplingPenSize(int size) {
	renderView->setUserSamplingPenSize(size);
}

void MainWindow::userSamplingPenStrength(int s) {
	renderView->setUserSamplingPenSprayIntensity(s / 100.f);
}

void MainWindow::userSamplingMapOpacity(int size) {
	renderView->setUserSamplingMapOpacity(size / 100.f);
}

void MainWindow::userSamplingApply() {
	renderView->applyUserSampling();
}

void MainWindow::userSamplingUndo() {
	renderView->undoUserSampling();
}

void MainWindow::userSamplingReset() {
	renderView->resetUserSampling();
}

void MainWindow::EngineThread::run()
{
	// NOTE - lordcrc - initialize rand()
	qsrand(time(NULL));

	// if stdin is input, don't use full path
	if (filename == QString::fromAscii("-"))
		luxParse(qPrintable(filename));
	else {
		QFileInfo fullPath(filename);
		QDir::setCurrent(fullPath.absoluteDir().path());
		luxParse(qPrintable(fullPath.absoluteFilePath()));
	}

	if (luxStatistics("terminated") || mainWindow->m_guiRenderState == ENDING)
		return;

	if(!luxStatistics("sceneIsReady")) {
		qApp->postEvent(mainWindow, new QEvent((QEvent::Type)EVT_LUX_PARSEERROR));
		luxWait();
	} else {
		luxWait();
		qApp->postEvent(mainWindow, new QEvent((QEvent::Type)EVT_LUX_FINISHED));
		LOG(LUX_INFO,LUX_NOERROR)<< tr("Rendering done.").toLatin1().data();
	}
}

void MainWindow::UpdateThread::run()
{
	luxUpdateFramebuffer ();
	luxUpdateLogFromNetwork();
	qApp->postEvent(mainWindow, new QEvent((QEvent::Type)EVT_LUX_TONEMAPPED));
}

void MainWindow::FlmLoadThread::run()
{
	luxLoadFLM(qPrintable(filename));

	if (!luxStatistics("filmIsReady")) {
		qApp->postEvent(mainWindow, new QEvent((QEvent::Type)EVT_LUX_FLMLOADERROR));
	}
}

void MainWindow::FlmSaveThread::run()
{
	luxSaveFLM(qPrintable(filename));

	qApp->postEvent(mainWindow, new QEvent((QEvent::Type)EVT_LUX_SAVEDFLM));
}

void MainWindow::BatchProcessThread::run()
{
	QProgressDialog *batchProgress = mainWindow->batchProgress;
	// Find 'flm' files in the input directory
	QFileInfoList flmFiles(QDir(inDir).entryInfoList(QStringList("*.flm"),
		QDir::Files));

	// Process the 'flm' files
	uint i = 0;
	for(QFileInfoList::Iterator f(flmFiles.begin()); f != flmFiles.end(); ++i, ++f) {
		// Update progress
		qApp->postEvent(mainWindow, new BatchEvent(f->completeBaseName(), i, flmFiles.size()));

		// Load FLM into Lux engine
		luxLoadFLM(qPrintable(f->absoluteFilePath()));
		if(!luxStatistics("filmIsReady"))
			continue;

		// Check for cancel
		if (batchProgress && batchProgress->wasCanceled())
			return;

		// Make output filename
		QString outName = QDir(outDir).absoluteFilePath(f->completeBaseName() + "." + outExtension);

		// Save loaded FLM
		if(allLightGroups)
			mainWindow->saveAllLightGroups(outName, asHDR);
		else {
			luxUpdateFramebuffer();
			if(asHDR)
				mainWindow->saveCurrentImageHDR(outName);
			else
				saveCurrentImageTonemapped(outName);
		}
		LOG(LUX_INFO, LUX_NOERROR) << "Saved '" << qPrintable(f->path()) << "' as '" << qPrintable(outName);

		// Check again for cancel
		if (batchProgress && batchProgress->wasCanceled())
			return;
	}

	// Signal completion
	qApp->postEvent(this, new BatchEvent("", flmFiles.size(), flmFiles.size()));
}

void MainWindow::NetworkAddRemoveSlavesThread::run() {

	for (int i = 0; i < slaves.size(); ++i) {
		switch (action) {
			case AddSlaves:
				luxAddServer(qPrintable(slaves[i]));
				break;
			case RemoveSlaves:
				luxRemoveServer(qPrintable(slaves[i]));
				break;
			default:
				break;
		}
	}

	qApp->postEvent(mainWindow, new NetworkUpdateTreeEvent());
}


void MainWindow::SetRenderThreads(int num)
{
	if(luxStatistics("sceneIsReady")) {
		if(num > m_numThreads) {
			for(; num > m_numThreads; m_numThreads++)
				luxAddThread();
		} else {
			for(; num < m_numThreads; m_numThreads--)
				luxRemoveThread();
		}
	} else {
		m_numThreads = num;
	}

	ui->label_threadCount->setText(QString("Threads:"));
	updateWidgetValue(ui->spinBox_Threads, m_numThreads);
}

#if defined(__APPLE__) // Doubleclick or dragging .lxs, .lxm or .lxq in OSX Finder to LuxRender

void  MainWindow::loadFile(const QString &fileName)
{
	if (fileName.endsWith(".lxs")){
		if (!canStopRendering())
			return;
		renderNewScenefile(fileName);
	} else if (fileName.endsWith(".flm")){
		if (!canStopRendering())
			return;
		if(fileName.isNull())
			return;

		setCurrentFile(fileName); // show flm-name in windowheader

		endRenderingSession();
		ClearRenderingQueue();

		indicateActivity ();
		statusMessage->setText("Loading FLM...");
		// Start load thread
		m_loadTimer->start(1000);

		delete m_flmloadThread;
		m_flmloadThread = new FlmLoadThread(this, fileName);
		m_flmloadThread->start();
	} else if (fileName.endsWith(".lxq")){
		if (!canStopRendering())
			return;
		if(fileName.isNull())
			return;
		openQueueFile(fileName);

	} else {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		QFileInfo fi(fileName);
		QString name = fi.fileName();
		msgBox.setText(name +(" is not a supported filetype. Choose an .lxs, .lxm or .lxq"));
		msgBox.exec();
	}
}
#endif

// Helper class for MainWindow::updateStatistics()
class AttributeFormatter {
public:
	static const int maxStatLength = 10;

	AttributeFormatter(QBoxLayout* l, int& label_count) : layout(l), count(label_count) { }

	void operator()(QString s) {
		QRegExp m("([^%]*)%([^%]*)%([^%]*)");
		for (int pos = 0; (pos = m.indexIn(s, pos)) >= 0; pos += m.matchedLength()) {
			// leading text in first capture subgroup
			if (m.pos(1) >= 0 && m.cap(1).length() > 0) {
				QLabel* label = getNextLabel();
				label->setText(m.cap(1));
				label->setToolTip("");
			}

			// attribute in second capture subgroup
			if (m.pos(2) >= 0) {
				QLabel* label = getNextLabel();
				if (m.cap(2).length() > 0) {
					QString attr(m.cap(2));

					QString statValue = getStringAttribute("renderer_statistics_formatted", qPrintable(attr));
					QString statDesc;

					if (statValue.length() <= maxStatLength)
						statDesc = getAttributeDescription("renderer_statistics_formatted", qPrintable(attr));
					else {
						statValue = getStringAttribute("renderer_statistics_formatted_short", qPrintable(attr));
						statDesc = getAttributeDescription("renderer_statistics_formatted_short", qPrintable(attr));
					}

					label->setText(statValue);
					label->setToolTip(statDesc);
				} else {
					label->setText("%");
					label->setToolTip("");
				}
			}

			// trailing text in third capture subgroup
			if (m.pos(3) >= 0 && m.cap(3).length() > 0) {
				QLabel* label = getNextLabel();
				label->setText(m.cap(3));
				label->setToolTip("");
			}
		}
	}

private:
	QLabel* getNextLabel() {
		const int idx = count++;
		QLayoutItem* item = layout->itemAt(idx);

		// if item is a stretcher then widget() returns null
		QLabel* label = item ? qobject_cast<QLabel*>(item->widget()) : NULL;
		if (!label) {
			// no existing label, create new
			label = new QLabel("");
			layout->insertWidget(idx, label);
			label->setVisible(true);
		}

		return label;
	}


	QBoxLayout* layout;
	int& count;
};

void MainWindow::updateStatistics()
{
	// prevent redraws while updating
	statsBox->setUpdatesEnabled(false);

	luxUpdateStatisticsWindow();

	QString st(getStringAttribute("renderer_statistics_formatted", "_recommended_string_template"));

	int active_label_count = 0;
	AttributeFormatter fmt(statsBoxLayout, active_label_count);
	fmt(st);

	// clear remaining labels
	QLayoutItem* item;
	while ((item = statsBoxLayout->itemAt(active_label_count++)) != NULL) {
		QLabel* label = qobject_cast<QLabel*>(item->widget());
		if (!label)
			continue;
		label->setText("");
		label->setToolTip("");
	}

	// fallback statistics
	if (active_label_count == 2)	// if only the spacer exists
	{
		int pixels = luxGetIntAttribute("film", "xResolution") * luxGetIntAttribute("film", "yResolution");
		double spp = luxGetDoubleAttribute("film", "numberOfResumedSamples") / pixels;

		QLabel* label = new QLabel(QString("%1 %2S/p").arg(luxMagnitudeReduce(spp), 0, 'f', 2).arg(luxMagnitudePrefix(spp)));
		label->setToolTip("Average number of samples per pixel");
		statsBoxLayout->insertWidget(0, label);
	}

	// update progress bar
	double percentComplete = luxGetDoubleAttribute("renderer_statistics", "percentComplete");
	statusProgress->setValue(static_cast<int>(min(percentComplete, 100.0)));

	statsBox->setUpdatesEnabled(true);
}

// show the render-resolution
void MainWindow::showRenderresolution()
{
	if (luxHasObject("film")) {
		int w = luxGetIntAttribute("film", "xResolution");
		int h = luxGetIntAttribute("film", "yResolution");
		int cw = luxGetIntAttribute("film", "xPixelCount");
		int ch = luxGetIntAttribute("film", "yPixelCount");
		ui->resolutioniconLabel->setPixmap(QPixmap(":/icons/resolutionicon.png"));
		if (cw != w || ch != h)
			ui->resinfoLabel->setText(QString(" %1 x %2 (%3 x %4) ").arg(cw).arg(ch).arg(w).arg(h));
		else 
			ui->resinfoLabel->setText(QString(" %1 x %2 ").arg(w).arg(h));
		ui->resinfoLabel->setVisible(true);
	} else {
		ui->resinfoLabel->setVisible(false);
	}
}
// show the zoom-factor in viewport
void MainWindow::showZoomfactor()
{
	ui->zoominfoLabel->setText((QString(" %1").arg(renderView->getZoomFactor()))+ "% ");
}
// show the actual viewportsize
void MainWindow::viewportChanged() {
	showZoomfactor();
}

void MainWindow::renderScenefile(const QString& sceneFilename, const QString& flmFilename)
{
	if (flmFilename != "") {
		// Set the FLM filename
		luxOverrideResumeFLM(qPrintable(QFileInfo(flmFilename).absoluteFilePath()));
	}

	// override server update interval
	// trigger edit finished slot
	updateIntervalChanged();
	LOG(LUX_INFO,LUX_NOERROR) << "Server requests interval: " << luxGetIntAttribute("render_farm", "pollingInterval") << " seconds";

	// Render the scene
	setCurrentFile(sceneFilename);

	changeRenderState(PARSING);

	indicateActivity ();
	statusMessage->setText("Loading scene...");
	if (sceneFilename == "-")
		LOG(LUX_INFO,LUX_NOERROR) << "Loading piped scene...";
	else
		LOG(LUX_INFO,LUX_NOERROR) << "Loading scene file: '" << qPrintable(sceneFilename) << "'...";

	m_loadTimer->start(1000);

	// Start main render thread
	if (m_engineThread) {
		m_engineThread->wait();
		delete m_engineThread;
	}

	m_engineThread = new EngineThread(this, sceneFilename);
	m_engineThread->start();
}

void MainWindow::renderNewScenefile(const QString& sceneFilename, const QString& flmFilename)
{
	endRenderingSession();
	ClearRenderingQueue();
	addFileToRenderQueue(sceneFilename, flmFilename);
	RenderNextFileInQueue();
}

void MainWindow::setCurrentFile(const QString& filename)
{
	m_CurrentFile = filename;
	setWindowModified(false);
	QString showName = "Untitled";

	if (!m_CurrentFile.isEmpty()) {
		QFileInfo info(m_CurrentFile);
		showName = info.fileName();
		if (filename == "-")
			showName = "LuxRender - Piped Scene";
		else {
			showName = "LuxRender - " + showName;

			m_CurrentFileBaseName = info.completeBaseName();

			m_lastOpendir = info.absolutePath();
			if (filename.endsWith(".lxs")) {
				m_recentFiles.removeAll(info);
				m_recentFiles.prepend(info);

				updateRecentFileActions(); // only parseble files .lxs to recentlist
			}
		}
	}

	setWindowTitle(tr("%1[*]").arg(showName));
}

void MainWindow::updateRecentFileActions()
{
	QMutableListIterator<QFileInfo> i(m_recentFiles);
	while (i.hasNext()) {
		i.peekNext().refresh();
		if (!i.next().exists()) {
			i.remove();
		}
	}

	for (int j = 0; j < MaxRecentFiles; ++j) {
		if (j < m_recentFiles.count()) {
			QFontMetrics fm(m_recentFileActions[j]->font());
			QString filename = m_recentFiles[j].absoluteFilePath();

			QString text = tr("&%1 %2").arg(j + 1).arg(QDir::toNativeSeparators(pathElidedText(fm, filename, 250, 0)));

			m_recentFileActions[j]->setText(text);
			m_recentFileActions[j]->setData(filename);
			m_recentFileActions[j]->setVisible(true);
		} else
			m_recentFileActions[j]->setVisible(false);
	}
}

// TODO: replace by QStateMachine
void MainWindow::changeRenderState(LuxGuiRenderState state)
{
	switch (state) {
		case WAITING:
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
			ui->action_saveFLM->setEnabled (false);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (false);
			ui->action_outputTonemapped->setEnabled (false);
			ui->action_outputHDR->setEnabled (false);
			ui->action_outputBufferGroupsTonemapped->setEnabled (false);
			ui->action_outputBufferGroupsHDR->setEnabled (false);
			ui->action_batchProcess->setEnabled (true);
			activityMessage->setText("Idle");
			showRenderresolution();
			statusProgress->setRange(0, 100);
			break;
		case PARSING:
			// Waiting for input file. Most controls disabled.
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled(false);
			ui->action_saveFLM->setEnabled (false);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled(false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled(false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (false);
			ui->action_outputTonemapped->setEnabled (false);
			ui->action_outputHDR->setEnabled (false);
			ui->action_outputBufferGroupsTonemapped->setEnabled (false);
			ui->action_outputBufferGroupsHDR->setEnabled (false);
			ui->action_batchProcess->setEnabled (false);
			//m_viewerToolBar->Disable();
			activityMessage->setText("Parsing scenefile");
			renderView->setLogoMode();
			break;
		case RENDERING:
			// Rendering is in progress.
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
			ui->action_saveFLM->setEnabled (true);
			ui->button_pause->setEnabled (true);
			ui->action_pauseRender->setEnabled (true);
			ui->button_stop->setEnabled (true);
			ui->action_stopRender->setEnabled (true);
			ui->action_endRender->setEnabled (true);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			ui->action_outputHDR->setEnabled (true);
			ui->action_outputBufferGroupsTonemapped->setEnabled (true);
			ui->action_outputBufferGroupsHDR->setEnabled (true);
			ui->action_batchProcess->setEnabled (true);
			activityMessage->setText("Rendering...");
			break;
		case TONEMAPPING:
		case FINISHED:
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
			ui->action_saveFLM->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			ui->action_outputHDR->setEnabled (true);
			ui->action_outputBufferGroupsTonemapped->setEnabled (true);
			ui->action_outputBufferGroupsHDR->setEnabled (true);
			ui->action_batchProcess->setEnabled (true);
			showRenderresolution();
			activityMessage->setText("Render is finished");
			break;
		case STOPPING:
			// Rendering is being stopped.
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
			ui->action_saveFLM->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			ui->action_outputHDR->setEnabled (true);
			ui->action_outputBufferGroupsTonemapped->setEnabled (true);
			ui->action_outputBufferGroupsHDR->setEnabled (true);
			ui->action_batchProcess->setEnabled (false);
			break;
		case STOPPED:
			// Rendering is stopped.
			ui->button_resume->setEnabled (true);
			ui->action_resumeRender->setEnabled (true);
			ui->action_saveFLM->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			ui->action_outputHDR->setEnabled (true);
			ui->action_outputBufferGroupsTonemapped->setEnabled (true);
			ui->action_outputBufferGroupsHDR->setEnabled (true);
			ui->action_batchProcess->setEnabled (true);
			activityMessage->setText("Render is stopped");
			break;
		case ENDING:
			// Rendering is ending.
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
			ui->action_saveFLM->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			ui->action_outputHDR->setEnabled (true);
			ui->action_outputBufferGroupsTonemapped->setEnabled (true);
			ui->action_outputBufferGroupsHDR->setEnabled (true);
			ui->action_batchProcess->setEnabled (false);
			break;
		case ENDED:
			// Rendering has ended.
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
			ui->action_saveFLM->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			ui->action_outputHDR->setEnabled (true);
			ui->action_outputBufferGroupsTonemapped->setEnabled (true);
			ui->action_outputBufferGroupsHDR->setEnabled (true);
			ui->action_batchProcess->setEnabled (true);
			activityMessage->setText("Render is over");
			break;
		case PAUSED:
			// Rendering is paused.
			ui->button_resume->setEnabled (true);
			ui->action_resumeRender->setEnabled (true);
			ui->action_saveFLM->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->action_endRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			ui->action_outputHDR->setEnabled (true);
			ui->action_outputBufferGroupsTonemapped->setEnabled (true);
			ui->action_outputBufferGroupsHDR->setEnabled (true);
			ui->action_batchProcess->setEnabled (true);
			activityMessage->setText("Render is paused");
			break;
	}
	m_guiRenderState = state;
}

// TODO: replace with queued signals
bool MainWindow::event (QEvent *event)
{
	bool retval = FALSE;
	int eventtype = event->type();

	// Check if it's one of "our" events
	if (eventtype == EVT_LUX_TONEMAPPED) {
		// Make sure the update thread has ended so we can start another one later.
		if (m_updateThread)
			m_updateThread->wait();
		delete m_updateThread;
		m_updateThread = NULL;
		statusMessage->setText("");
		// reset progressindicator
		indicateActivity(false);
		renderView->reload();
		histogramwidget->Update();

		if ( m_bTonemapPending ) // hack for "tonemapping lag."
		{
			m_bTonemapPending = false;
			applyTonemapping();
		}

		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_PARSEERROR) {
		m_loadTimer->stop();
		blinkTrigger();
		indicateActivity(false);
		statusMessage->setText("Loading aborted");
		changeRenderState(FINISHED);
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_FLMLOADERROR) {
		blinkTrigger();
		indicateActivity(false);
		statusMessage->setText("Loading aborted");
		if (m_flmloadThread) {
			m_flmloadThread->wait();
			delete m_flmloadThread;
			m_flmloadThread = NULL;
		}
		changeRenderState(WAITING);
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_FINISHED) {
		if (m_guiRenderState == RENDERING) {
			// Ignoring finished events if another file is being opened (state != RENDERING)

			bool renderingNext = false;

			if (IsFileQueued())
				renderingNext = RenderNextFileInQueue(false);

			if (!renderingNext) {
				// Stop timers and update output one last time.
				stopRender();

				changeRenderState(FINISHED);
			}
		}
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_SAVEDFLM) {
		m_saveTimer->stop();
		statusMessage->setText("");
		// reset progressindicator
		indicateActivity(false);
		if (m_flmsaveThread)
			m_flmsaveThread->wait();
		delete m_flmsaveThread;
		m_flmsaveThread = NULL;
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_LOGEVENT) {
		logEvent((LuxLogEvent *)event);
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_BATCHEVENT) {
		if(((BatchEvent*)event)->isFinished())
		{
			batchProgress->setMaximum(((BatchEvent*)event)->getTotal());
			batchProgress->setValue(((BatchEvent*)event)->getNumCompleted());
			loadFLM(((BatchEvent*)event)->getCurrentFile());
		}
		else
		{
			batchProgress->setMaximum(((BatchEvent*)event)->getTotal());
			batchProgress->setValue(((BatchEvent*)event)->getNumCompleted());
			batchProgress->setLabelText(tr("Processing %1 ...").arg(((BatchEvent*)event)->getCurrentFile()));
		}

		retval = TRUE;
	} else if (eventtype == EVT_LUX_NETWORKUPDATETREEEVENT) {
		ui->button_addServer->setEnabled(true);
		ui->button_removeServer->setEnabled(true);

		UpdateNetworkTree();

		retval = TRUE;
	}

	if (retval) {
		// Was our event, stop the event propagation
		event->accept();
	}

	return QMainWindow::event(event);
}

void MainWindow::logEvent(LuxLogEvent *event)
{
	static const QColor debugColour = Qt::black;
	static const QColor infoColour = Qt::green;
	static const QColor warningColour = Qt::darkYellow;
	static const QColor errorColour = Qt::red;
	static const QColor severeColour = Qt::red;

	QTextStream ss(new QString());
	ss << '[' << QDateTime::currentDateTime().toString(tr("yyyy-MM-dd hh:mm:ss")) << ' ';

	// changing cursor does not change the visible cursor
	QTextCursor cursor = ui->textEdit_log->textCursor();
	bool atEnd = cursor.atEnd();

	// Append log message to end of document
	cursor.movePosition(QTextCursor::End);

	QTextCharFormat fmt(cursor.charFormat());

	QColor textColor = Qt::black;

	switch(event->getSeverity()) {
		case LUX_DEBUG:
			ss << tr("Debug: ");
			textColor = debugColour;
			break;
		case LUX_INFO:
		default:
			ss << tr("Info: ");
			textColor = infoColour;
			break;
		case LUX_WARNING:
			ss << tr("Warning: ");
			textColor = warningColour;
			break;
		case LUX_ERROR:
			ss << tr("Error: ");
			textColor = errorColour;
			break;
		case LUX_SEVERE:
			ss << tr("Severe error: ");
			textColor = severeColour;
			break;
	}

	ss << event->getCode() << "] ";
	ss.flush();

	fmt.setForeground(QBrush(textColor));
	cursor.setCharFormat(fmt);
	cursor.insertText(ss.readAll());

	fmt.setForeground(QBrush(Qt::black));
	cursor.setCharFormat(fmt);
	ss << event->getMessage() << endl;
	cursor.insertText(ss.readAll());

	// scroll new message into view if cursor was at end
	if (atEnd)
		ui->textEdit_log->ensureCursorVisible();

	int currentIndex = ui->tabs_main->currentIndex();
	if (currentIndex != getTabIndex(TAB_ID_LOG) && event->getSeverity() > LUX_INFO) {
		blink = true;
		if (event->getSeverity() < LUX_ERROR) {
			static const QIcon icon(":/icons/warningicon.png");
			ShowTabLogIcon(TAB_ID_LOG, icon);
		} else {
			blinkTrigger();
			statusMessage->setText("Check Log Please");
		}
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	exitApp();
}

//user acknowledged error/warning-conditions
void MainWindow::tabChanged(int)
{
	int currentIndex = ui->tabs_main->currentIndex();

	if (currentIndex == getTabIndex(TAB_ID_LOG)) {
		blinkTrigger(false);
		static const QIcon icon(":/icons/logtabicon.png");
		ShowTabLogIcon(TAB_ID_LOG, icon);
		statusMessage->setText("Checking Log acknowledged");
	}
}

void MainWindow::outputTabChanged(int) {
	int currentIndex = ui->outputTabs->currentIndex();

	if (currentIndex == OUTPUT_TAB_ID_REFINE) {
		// Always show the map when the refine tab is selected
		ui->action_showUserSamplingMapView->setChecked(true);
		renderView->setShowUserSamplingMap(true);
		renderView->reload();
	} else {
		// Always hide the map when the refine tab is selected
		ui->action_showUserSamplingMapView->setChecked(false);
		renderView->setShowUserSamplingMap(false);
		renderView->reload();
	}
}

// Icon blinking flipflop
void MainWindow::blinkTrigger(bool active)
{
	if (active) {
		m_blinkTimer->start(1000);
		blink = !blink;
		if (blink) {
			static const QIcon icon(":/icons/erroricon.png");
			ShowTabLogIcon(TAB_ID_LOG, icon);
		}
		else {
			static const QIcon icon(":/icons/logtabicon.png");
			ShowTabLogIcon(TAB_ID_LOG, icon);
		}
	} else {
		m_blinkTimer->stop();
		blink = false;
		static const QIcon icon(":/icons/logtabicon.png");
		ShowTabLogIcon(TAB_ID_LOG, icon);
	}
}

void MainWindow::renderTimeout()
{
	if (m_updateThread == NULL && (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) &&
		(!isMinimized () || m_guiRenderState == FINISHED)) {
		LOG(LUX_DEBUG,LUX_NOERROR)<< tr("GUI: Updating framebuffer...").toLatin1().data();
		statusMessage->setText("Tonemapping...");
		m_updateThread = new UpdateThread(this);
		m_updateThread->start();
	}
}

void MainWindow::statsTimeout()
{
	if(luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		updateStatistics();
		if ((m_guiRenderState == STOPPING || m_guiRenderState == ENDING || m_guiRenderState == FINISHED)) {
			// Render threads stopped, do one last render update
			LOG(LUX_DEBUG,LUX_NOERROR)<< tr("GUI: Updating framebuffer...").toLatin1().data();
			statusMessage->setText(tr("Tonemapping..."));
			qApp->postEvent(this, new NetworkUpdateTreeEvent());
			if (m_updateThread)
				m_updateThread->wait();
			delete m_updateThread;
			m_updateThread = new UpdateThread(this);
			m_updateThread->start();
			m_statsTimer->stop();
			//luxPause();
			if (m_guiRenderState == FINISHED)
				LOG(LUX_INFO,LUX_NOERROR)<< tr("Rendering finished.").toLatin1().data();
			else if (m_guiRenderState == STOPPING) {
				LOG(LUX_INFO,LUX_NOERROR)<< tr("Rendering stopped by user.").toLatin1().data();
				changeRenderState(STOPPED);
			} else { //if (m_guiRenderState == ENDING) {
				LOG(LUX_INFO,LUX_NOERROR)<< tr("Rendering ended by user.").toLatin1().data();
				changeRenderState(ENDED);
			}
		}
	}
}

void MainWindow::loadTimeout()
{
	if(luxStatistics("sceneIsReady") || m_guiRenderState == FINISHED) {
		statusProgress->setValue(0);
		indicateActivity(false);
		statusMessage->setText("");
		m_loadTimer->stop();
		gammawidget->resetFromFilm(true);

		if (luxStatistics("sceneIsReady")) {
			addRemoveSlaves(QVector<QString>::fromList(networkSlaves.keys()), AddSlaves);

			// Scene file loaded
			// Add other render threads if necessary
			int curThreads = 1;
			while(curThreads < m_numThreads) {
				luxAddThread();
				curThreads++;
			}

			updateWidgetValue(ui->spinBox_overrideDisplayInterval, luxGetIntAttribute("film", "displayInterval"));

			// override halt conditions if needed
			if (!m_resetOverrides)
			{
				if (ui->spinBox_overrideHaltSpp->value() > 0)
					luxSetIntAttribute("film", "haltSamplesPerPixel", ui->spinBox_overrideHaltSpp->value());
				if (ui->spinBox_overrideHaltTime->value() > 0)
					luxSetIntAttribute("film", "haltTime", ui->spinBox_overrideHaltTime->value());
				
				bool writeFlm = ui->checkBox_overrideWriteFlm->isChecked();
				luxSetBoolAttribute("film", "writeResumeFlm", writeFlm);
				if (writeFlm)
					luxSetBoolAttribute("film", "restartResumeFlm", false);
				else
					luxSetBoolAttribute("film", "restartResumeFlm", luxGetBoolAttributeDefault("film", "restartResumeFlm"));
			}
			else
			{
				ui->spinBox_overrideHaltSpp->setValue(0);
				ui->spinBox_overrideHaltTime->setValue(0);

				bool lxswriteFlm = luxGetBoolAttribute("film", "writeResumeFlm");
				updateWidgetValue(ui->checkBox_overrideWriteFlm, lxswriteFlm);
			}

			// Start updating the display by faking a resume menu item click.
			ui->action_resumeRender->activate(QAction::Trigger);

			// Enable tonemapping options and reset from values trough API
			resetToneMappingFromFilm(false);
			ResetLightGroupsFromFilm(false);
			// reset values from film before enabling the tabs
			// to prevent overwriting film settings
			ui->outputTabs->setEnabled(true);

			renderView->resetTransform();
		}
	}
	else if ( luxStatistics("filmIsReady") ) {
		statusProgress->setValue(0);
		indicateActivity(false);
		statusMessage->setText("");
		m_loadTimer->stop();

		if(m_flmloadThread) {
			m_flmloadThread->wait();
			delete m_flmloadThread;
			m_flmloadThread = NULL;
		}

		changeRenderState(TONEMAPPING);

		// Enable tonemapping options and reset from values trough API
		resetToneMappingFromFilm(false);
		ResetLightGroupsFromFilm(false);
		// reset values from film before enabling the tabs
		// to prevent overwriting film settings
		ui->outputTabs->setEnabled(true);
		// Update framebuffer
		if (!m_auto_tonemap)
			// if auto tonemap is enabled then
			// resetToneMappingFromFilm already performed tonemap
			luxUpdateFramebuffer();
		renderView->resetTransform();
		renderView->reload();
		// Update stats
		updateStatistics();
	}
}

void MainWindow::saveTimeout()
{
	indicateActivity(false);
	statusMessage->setText("");
}

void MainWindow::netTimeout()
{
	UpdateNetworkTree();
}

void MainWindow::resetToneMapping()
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		resetToneMappingFromFilm( true );
		statusMessage->setText("Reloading Tonemapping from Film");
		return;
	}

	tonemapwidget->resetValues();
	lenseffectswidget->resetValues();
	colorspacewidget->resetValues();
	gammawidget->resetValues();
	noisereductionwidget->resetValues();

	updateTonemapWidgetValues ();

	ui->outputTabs->setEnabled (false);
}

void MainWindow::updateTonemapWidgetValues()
{
	tonemapwidget->updateWidgetValues();
	lenseffectswidget->updateWidgetValues();
	colorspacewidget->updateWidgetValues();
	gammawidget->updateWidgetValues();
	noisereductionwidget->updateWidgetValues();
	advancedinfowidget->updateWidgetValues();

	// Histogram
	histogramwidget->SetEnabled(true);
}

void MainWindow::resetToneMappingFromFilm (bool useDefaults)
{
	tonemapwidget->resetFromFilm(useDefaults);
	lenseffectswidget->resetFromFilm(useDefaults);
	colorspacewidget->resetFromFilm(useDefaults);
	gammawidget->resetFromFilm(useDefaults);
	noisereductionwidget->resetFromFilm(useDefaults);

	updateTonemapWidgetValues ();

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::UpdateLightGroupWidgetValues()
{
	for (QVector<PaneWidget*>::iterator it = m_LightGroupPanes.begin(); it != m_LightGroupPanes.end(); it++) {
		PaneWidget *pane = (*it);
		LightGroupWidget *widget = (LightGroupWidget *)pane->getWidget();
		widget->UpdateWidgetValues();
	}
}

void MainWindow::ResetLightGroups( void )
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		ResetLightGroupsFromFilm( true );
		return;
	}

	// Remove the lightgroups
	for (QVector<PaneWidget*>::iterator it = m_LightGroupPanes.begin(); it != m_LightGroupPanes.end(); it++) {
		PaneWidget *pane = (*it);
		LightGroupWidget *currWidget = (LightGroupWidget *)pane->getWidget();
		ui->lightGroupsLayout->removeWidget(pane);
		delete currWidget;
	}
	ui->lightGroupsLayout->removeItem(spacer);
	m_LightGroupPanes.clear();
}

void MainWindow::ResetLightGroupsFromFilm( bool useDefaults )
{
	// Remove the old lightgroups
	for (QVector<PaneWidget*>::iterator it = m_LightGroupPanes.begin(); it != m_LightGroupPanes.end(); it++) {
		PaneWidget *pane = *it;
		ui->lightGroupsLayout->removeWidget(pane);
		delete pane;
	}

	ui->lightGroupsLayout->removeItem(spacer);

	m_LightGroupPanes.clear();

	// Add the new lightgroups
	int numLightGroups = (int)luxGetParameterValue(LUX_FILM, LUX_FILM_LG_COUNT);
	for (int i = 0; i < numLightGroups; i++) {
		PaneWidget *pane = new PaneWidget(ui->lightGroupsAreaContents);
		pane->SetIndex(i);
		pane->showOnOffButton();
		pane->showSoloButton();
		LightGroupWidget *currWidget = new LightGroupWidget(pane);
		currWidget->SetIndex(i);
		currWidget->ResetValuesFromFilm( useDefaults );
		pane->setTitle(currWidget->GetTitle());
		pane->setIcon(":/icons/lightgroupsicon.png");
		pane->setWidget(currWidget);
		connect(currWidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));
		connect(pane, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));
		connect(pane, SIGNAL(signalLightGroupSolo(int)), this, SLOT(setLightGroupSolo(int)));
		ui->lightGroupsLayout->addWidget(pane);
		pane->expand();
		m_LightGroupPanes.push_back(pane);
	}
	ui->lightGroupsLayout->addItem(spacer);

	// Update
	UpdateLightGroupWidgetValues();
}

void MainWindow::setLightGroupSolo( int index )
{
	int n = 0;

	for (QVector<PaneWidget*>::iterator it = m_LightGroupPanes.begin(); it != m_LightGroupPanes.end(); it++, n++ )
	{
		if ( (*it)->powerON )
		{
			if ( index == -1 )
			{
				((LightGroupWidget *)(*it)->getWidget())->setEnabled( true );
				(*it)->SetSolo( SOLO_OFF );
			}
			else if ( n != index )
			{
				((LightGroupWidget *)(*it)->getWidget())->setEnabled( false );
				(*it)->SetSolo( SOLO_ON );
			}
			else
			{
				((LightGroupWidget *)(*it)->getWidget())->setEnabled( true );
				(*it)->SetSolo( SOLO_ENABLED );
			}
		}
	}
}

void MainWindow::UpdateNetworkTree()
{
	int currentrow = ui->table_servers->currentRow();

	int nServers = luxGetIntAttribute("render_farm", "slaveNodeCount");

	vector<RenderingServerInfo> serverInfoList;

	if (nServers > 0) {
		serverInfoList.resize(nServers);
		nServers = luxGetRenderingServersStatus( &serverInfoList[0], nServers );
		serverInfoList.resize(nServers);
	}

	// temporarily disable sorting while we update the table
	bool sorted = ui->table_servers->isSortingEnabled();
	ui->table_servers->setSortingEnabled(false);

	ui->table_servers->setRowCount(static_cast<int>(serverInfoList.size() + networkSlaves.size()));

	double totalpixels = luxGetIntAttribute("film", "xPixelCount") * luxGetIntAttribute("film", "yPixelCount");

	int n = 0;

	QSet<QString> connectedSlaves;

	for (uint i = 0; i < serverInfoList.size(); ++i) {
		QTableWidgetItem *status = new QTableWidgetItem(tr("Active session"));

		QTableWidgetItem *servername = new QTableWidgetItem(QString::fromUtf8(serverInfoList[i].name));
		QTableWidgetItem *port = new QTableWidgetItem(QString::fromUtf8(serverInfoList[i].port));

		connectedSlaves.insert(QString("%1:%2").arg(servername->text(), port->text()).toLower());

		double spp = serverInfoList[i].numberOfSamplesReceived / totalpixels;
		double sps = serverInfoList[i].calculatedSamplesPerSecond;

		QString spp_string = QString("%1 %2S/p").arg(luxMagnitudeReduce(spp),0,'g',3).arg(luxMagnitudePrefix(spp));
		QString sps_string = QString("%1 %2S/s").arg(luxMagnitudeReduce(sps),0,'g',3).arg(luxMagnitudePrefix(sps));

		QTableWidgetItem *spp_widget = new QTableWidgetItem((totalpixels > 0) ? spp_string : "");
		QTableWidgetItem *sps_widget = new QTableWidgetItem((totalpixels > 0) ? sps_string : "");

		ui->table_servers->setItem(n, 0, servername);
		ui->table_servers->setItem(n, 1, port);
		ui->table_servers->setItem(n, 2, status);
		ui->table_servers->setItem(n, 3, spp_widget);
		ui->table_servers->setItem(n, 4, sps_widget);
		n++;
	}

	// add saved slaves which aren't in the connected list
	QVector<QString> disconnectedSlaves;

	QList<QString> slaves = networkSlaves.keys();

	while (!slaves.empty()) {
		QString s(slaves.takeFirst());
		if (connectedSlaves.contains(s))
			continue;
		disconnectedSlaves.append(s);
	}

	for( int i = 0; i < disconnectedSlaves.size(); i++ ) {
		QTableWidgetItem *status = new QTableWidgetItem(tr("No session"));

		// assume saved slaves contain port number
		int si = disconnectedSlaves[i].lastIndexOf(':');
		QTableWidgetItem *servername = new QTableWidgetItem(disconnectedSlaves[i].left(si));
		QTableWidgetItem *port = new QTableWidgetItem(disconnectedSlaves[i].mid(si+1));

		ui->table_servers->setItem(n, 0, servername);
		ui->table_servers->setItem(n, 1, port);
		ui->table_servers->setItem(n, 2, status);
		ui->table_servers->setItem(n, 3, NULL);
		ui->table_servers->setItem(n, 4, NULL);
		n++;
	}

	ui->table_servers->setRowCount(n);

	// enable sorting again
	ui->table_servers->setSortingEnabled(sorted);
	ui->table_servers->sortItems(0);

	ui->table_servers->blockSignals (true);
	ui->table_servers->selectRow(currentrow);
	ui->table_servers->blockSignals (false);
}

void MainWindow::addRemoveSlaves(QVector<QString> slaves, ChangeSlavesAction action) {

	if (m_networkAddRemoveSlavesThread != NULL) {
		m_networkAddRemoveSlavesThread->wait();
		delete m_networkAddRemoveSlavesThread;
	}

	ui->button_addServer->setEnabled(false);
	ui->button_removeServer->setEnabled(false);

	// update list of slaves
	for (int i = 0; i < slaves.size(); i++) {
		QString slave = slaves[i].toLower();
		int pi = slave.lastIndexOf(':');
		int pi6 = slave.lastIndexOf("::");
		if (pi < 0 || (pi > 0 && pi-1 == pi6)) {
			// append default port
			slave.append(":").append(QString::number(luxGetIntAttribute("render_farm", "defaultTcpPort")));
		}
		switch (action) {
			case AddSlaves:
				networkSlaves.insert(slave, networkSlaves.size());
				break;
			case RemoveSlaves:
				networkSlaves.remove(slave);
				break;
			default:
				LOG(LUX_SEVERE, LUX_SYSTEM) << "Invalid action in addRemoveSlaves: " << action;
		}
	}

	// then update core
	m_networkAddRemoveSlavesThread = new NetworkAddRemoveSlavesThread(this, slaves, action);
	m_networkAddRemoveSlavesThread->start();
}

void MainWindow::setServerUpdateInterval(int interval) {

	if (interval > 0) {
		// only update if valid interval
		luxSetIntAttribute("render_farm", "pollingInterval", interval);
	} else {
		// invalid interval, update combobox with old value
		interval = luxGetIntAttribute("render_farm", "pollingInterval");
	}

	QString s = QString("%0").arg(interval);

	int idx = ui->comboBox_updateInterval->findText(s);

	ui->comboBox_updateInterval->blockSignals(true);
	if (idx < 0) {
		ui->comboBox_updateInterval->setCurrentIndex(0);
		ui->comboBox_updateInterval->lineEdit()->setText(QString("%0").arg(interval));
	} else {
		ui->comboBox_updateInterval->setCurrentIndex(idx);
	}
	ui->comboBox_updateInterval->blockSignals(false);
}

void MainWindow::AddNetworkSlaves(const QVector<QString> &slaves) {
	addRemoveSlaves(slaves, AddSlaves);
}

void MainWindow::RemoveNetworkSlaves(const QVector<QString> &slaves) {
	addRemoveSlaves(slaves, RemoveSlaves);
}

void MainWindow::addServer()
{
	QString server = ui->lineEdit_server->text();

	if (!server.isEmpty()) {
		QVector<QString> slaves;
		slaves.push_back(server);

		AddNetworkSlaves(slaves);

		m_recentServersModel->add(server);
	}
}

void MainWindow::removeServer()
{
	QString server = ui->lineEdit_server->text();

	if (!server.isEmpty()) {
		QVector<QString> slaves;
		slaves.push_back(server);

		RemoveNetworkSlaves(slaves);
	}
}

void MainWindow::resetServer()
{
	QString server = ui->lineEdit_server->text();

	if (server.isEmpty())
		return;

	bool ok;
	QString password = QInputDialog::getText(this, tr("Reset server session"),
		tr("Server password (leave empty for no password):"), 
		QLineEdit::Password, "", &ok);

	if (!ok)
		return;

	luxResetServer(server.toAscii(), password.toAscii());

	qApp->postEvent(this, new NetworkUpdateTreeEvent());
}

void MainWindow::updateIntervalChanged(int value)
{
	if (value < 0)
		return;
	
	setServerUpdateInterval(ui->comboBox_updateInterval->itemText(value).toInt());
}

void MainWindow::updateIntervalChanged()
{
	setServerUpdateInterval(ui->comboBox_updateInterval->lineEdit()->text().toInt());
}

void MainWindow::networknodeSelectionChanged()
{
	int currentrow = ui->table_servers->currentRow();
	QTableWidgetItem *server = ui->table_servers->item(currentrow,0);
	QTableWidgetItem *port = ui->table_servers->item(currentrow,1);

	if (server) {
		ui->table_servers->blockSignals (true);
		ui->lineEdit_server->setText(QString("%1:%2").arg(server->text()).arg(port->text()));
		ui->table_servers->blockSignals (false);
	}
}

/**
 * Method to add files to the render queue. Normally called by the command line processor
 * but it could be possibly engaged by the API for dynamic updates.
 * It addes the files to the data model, the corresponding View will pick it up automatically.
 */

void MainWindow::addQueueHeaders()
{
	renderQueueData.setColumnCount(4);
	renderQueueData.setHeaderData( 0, Qt::Horizontal, QObject::tr("File Name"));
	renderQueueData.setHeaderData( 1, Qt::Horizontal, QObject::tr("Status"));
	renderQueueData.setHeaderData( 2, Qt::Horizontal, QObject::tr("Pass #"));
	renderQueueData.setHeaderData( 3, Qt::Horizontal, QObject::tr("FLM Name"));
}

bool MainWindow::addFileToRenderQueue(const QString& sceneFileName, const QString& flmFilename)
{
	int row = renderQueueData.rowCount();
	// Avoid adding duplicates
	if (IsFileInQueue(sceneFileName))
		return false;    
  
	QStandardItem* fileName = new QStandardItem(sceneFileName);
	QStandardItem* flmName = new QStandardItem(flmFilename);
	QStandardItem* status = new QStandardItem(tr("Waiting"));
	QStandardItem* pass = new QStandardItem("0");
  
	if (sceneFileName == m_CurrentFile)
		status->setText(tr("Rendering"));
	
	ui->table_queue->setColumnHidden(2, false);
	ui->table_queue->setColumnHidden(3, true);
	renderQueueData.setItem(row,0,fileName);
	renderQueueData.setItem(row,1,status);
	renderQueueData.setItem(row,2,pass);
	renderQueueData.setItem(row,3,flmName);

	return true;
}

void MainWindow::addQueueFiles()
{
	QStringList files = QFileDialog::getOpenFileNames(this, tr("Select files to add"), m_lastOpendir, tr("LuxRender Files (*.lxs)"));

	if (files.empty())
		return;

	// scrub filenames, due to a bug in Qt 4.6.2 (at least)
	// it will return paths with \ instead of / which it should
	for (int i = 0; i < files.count(); i++) {
		files[i] = QDir::fromNativeSeparators(files[i]);
	}

	if (m_guiRenderState == RENDERING && !IsFileInQueue(m_CurrentFile)) {
		// add current file to queue, since user created one
		// first ensure it's not in the selected files already
		if (files.indexOf(m_CurrentFile) < 0)
			files.insert(0, m_CurrentFile);

		// update haltspp/time
		overrideHaltSppChanged(ui->spinBox_overrideHaltSpp->value());
		overrideHaltTimeChanged(ui->spinBox_overrideHaltTime->value());
		if (ui->checkBox_overrideWriteFlm->isChecked())
			overrideWriteFlmChanged(true);
	}

	for (int i = 0; i < files.count(); i++) {
		addFileToRenderQueue(files[i]);
	}

	ui->table_queue->resizeColumnsToContents();

	if (m_guiRenderState != RENDERING) {
		LOG(LUX_INFO,LUX_NOERROR) << ">>> " << files.count() << " files added to the Render Queue. Now rendering...";
		RenderNextFileInQueue(false);
	}
}

void MainWindow::removeQueueFiles()
{
 
 	QItemSelectionModel* selectionModel = ui->table_queue->selectionModel();
	QModelIndexList indexes = selectionModel->selectedRows();
	QModelIndex index;
	QList<int> rows;
	int currentFileIndex = -1;
	// Collect the indexes, we don't want to delete from inside the loop...
	foreach(index, indexes) {
		QStandardItem *fname = renderQueueData.item(index.row(), 0);
		if (fname->text() == m_CurrentFile) {
			endRenderingSession();
			changeRenderState(STOPPED);
			currentFileIndex = index.row();
		}
		rows << index.row();
	}
  
	selectionModel->clear();
	// Delete now
	int row;
	foreach(row, rows) {
		renderQueueData.removeRow(row);
		if (row == currentFileIndex) {
			RenderNextFileInQueue(false);
		}
	}
}

void MainWindow::overrideHaltSppChanged(int value)
{
	if (value <= 0)
		value = luxGetIntAttributeDefault("film", "haltSamplesPerPixel");

	if (m_guiRenderState == RENDERING)
		luxSetIntAttribute("film", "haltSamplesPerPixel", value);
}

void MainWindow::overrideHaltTimeChanged(int value)
{
	if (value <= 0)
		value = luxGetIntAttributeDefault("film", "haltTime");

	if (m_guiRenderState == RENDERING)
		luxSetIntAttribute("film", "haltTime", value);
}

void MainWindow::loopQueueChanged(int state)
{
	bool loop = state == Qt::Checked;

	if (loop && !ui->checkBox_overrideWriteFlm->isChecked()) {
		// looping makes little sense without resume films, so just enabled it
		ui->checkBox_overrideWriteFlm->setChecked(true);
	}
}

void MainWindow::overrideWriteFlmChanged(bool checked)
{
	if (!checked) {
		if (QMessageBox::question(this, tr("Override resume file settings"),tr("Are you sure you want to disable the resume film setting override?\n\nIf the scene files do not specify usage of resume films you will be unable to use queue looping.\n\nIt is highly recommended that you do not disable this."), QMessageBox::Yes|QMessageBox::No) == QMessageBox::No) {
			updateWidgetValue(ui->checkBox_overrideWriteFlm, true);
			return;
		}
	}

	if (m_guiRenderState == RENDERING) {
		luxSetBoolAttribute("film", "writeResumeFlm", checked);
		if (checked)
			luxSetBoolAttribute("film", "restartResumeFlm", false);
		else
			luxSetBoolAttribute("film", "restartResumeFlm", luxGetBoolAttributeDefault("film", "restartResumeFlm"));
	}
}

bool MainWindow::IsFileInQueue(const QString &filename)
{  
  for (int i = 0; i < renderQueueData.rowCount(); i++) {
		QStandardItem *fname = renderQueueData.item(i, 0);

		if (fname && (fname->text() == filename))
			return true;
	}

	return false;
}

bool MainWindow::IsFileQueued()
{
	return renderQueueData.rowCount() > 0;
}

bool MainWindow::RenderNextFileInQueue(bool resetOverrides)
{
	m_resetOverrides = resetOverrides;
	int idx = -1;

	// update current file
	
	for (int i = 0; i < renderQueueData.rowCount(); i++) {
		QStandardItem *fname = renderQueueData.item(i, 0);
		if (fname->text() == m_CurrentFile) {
			idx = i;
			break;
		}
	}

	if (idx >= 0) {
		QStandardItem *status = renderQueueData.item(idx, 1);
		status->setText(tr("Completed ") + QDateTime::currentDateTime().toString(Qt::DefaultLocaleShortDate));
		LOG(LUX_INFO,LUX_NOERROR) << "==== Queued file '" << qPrintable(m_CurrentFileBaseName) << "' done ====";
	}
	return RenderNextFileInQueue(++idx);
}

bool MainWindow::RenderNextFileInQueue(int idx)
{
	// render next
  
  if (idx >= renderQueueData.rowCount()) {
		if (ui->checkBox_loopQueue->isChecked()) {
			ui->table_queue->setColumnHidden(2, false);
			ui->table_queue->setColumnHidden(3, true);
			idx = 0;
		}
		else
			return false;
	}

	QString filename = renderQueueData.item(idx, 0)->text();
	QString flmname = renderQueueData.item(idx, 3)->text();
	QStandardItem *status = renderQueueData.item(idx, 1);
	QStandardItem *pass = renderQueueData.item(idx, 2);
  
  
	status->setText(tr("Rendering"));
	pass->setText(QString("%1").arg(pass->text().toInt() + 1));

	ui->table_queue->resizeColumnsToContents();


	endRenderingSession();

	if (ui->checkBox_overrideWriteFlm->isChecked())
		luxOverrideResumeFLM("");

	renderScenefile(filename, flmname);

	return true;
}

void MainWindow::ClearRenderingQueue()
{
	renderQueueData.clear();
	addQueueHeaders(); // restore headers from Data Model for the Render Queue
	ui->table_queue->setColumnHidden(2, true);
	ui->table_queue->setColumnHidden(3, true);
}

///////////////////////////////////////////////////////////////////////////////
// Save and Load panel settings: Light groups, tonemapping, CRF.

void MainWindow::SavePanelSettings()
{
	QString Settings_file = QFileDialog::getSaveFileName(this, tr("Choose a Luxrender panel settings file to save"), m_lastOpendir, tr("Luxrender panel settings (*.ini *.txt)"));

	if(Settings_file.isEmpty()) return;

	QFileInfo info(Settings_file);
	m_lastOpendir = info.absolutePath();

	tonemapwidget->SaveSettings( Settings_file );
	gammawidget->SaveSettings( Settings_file );

	for (QVector<PaneWidget*>::iterator it = m_LightGroupPanes.begin(); it != m_LightGroupPanes.end(); it++)
	{
		((LightGroupWidget *)(*it)->getWidget())->SaveSettings( Settings_file );
	}
}

void MainWindow::LoadPanelSettings()
{
	QString Settings_file = QFileDialog::getOpenFileName(this, tr("Choose a Luxrender panel settings file to open"), m_lastOpendir, tr("Luxrender panel settings (*.ini *.txt)"));

	if(Settings_file.isEmpty()) return;

	QFileInfo info(Settings_file);
	m_lastOpendir = info.absolutePath();

	tonemapwidget->LoadSettings( Settings_file );
	gammawidget->LoadSettings( Settings_file );

	for (QVector<PaneWidget*>::iterator it = m_LightGroupPanes.begin(); it != m_LightGroupPanes.end(); it++)
	{
		((LightGroupWidget *)(*it)->getWidget())->LoadSettings( Settings_file );
	}
}

// View side panel
void MainWindow::ShowSidePanel(bool checked)
{
	ui->outputTabs->setVisible( checked );
}

void MainWindow::setVerbosity(int choice)
{
	ui->comboBox_verbosity->setCurrentIndex(choice);
	switch (choice) {
		case 0:
			//default
			lux::luxLogFilter=LUX_INFO;
			statusMessage->setText(tr("Log level set to Default"));
			break;
		case 1:
			//verbose
			lux::luxLogFilter=LUX_DEBUG;
			statusMessage->setText(tr("Log level set to Verbose"));
			break;
		case 2:
			//quiet
			lux::luxLogFilter=LUX_WARNING;
			statusMessage->setText(tr("Log level set to Quiet"));
			break;
		case 3:
			//very quiet
			lux::luxLogFilter=LUX_ERROR;
			statusMessage->setText(tr("Log level set to Very Quiet"));
			break;
		default:
			break;
	}
}

