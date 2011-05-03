/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

#define BOOST_FILESYSTEM_VERSION 2
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
//#include <boost/filesystem/path.hpp>
//#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <boost/cast.hpp>

#include <sstream>
#include <clocale>

#include <QProgressDialog>

#include <QList>

#include <QDateTime>
#include <QTextStream>

#include <QTextLayout>
#include <QFontMetrics>

#include "error.h"

#include "mainwindow.hxx"
#include "ui_luxrender.h"
#include "aboutdialog.hxx"
#include "batchprocessdialog.hxx"
#include "openexroptionsdialog.hxx"
#include "guiutil.h"

#if defined(WIN32) && !defined(__CYGWIN__)
#include "direct.h"
#define chdir _chdir
#endif

inline int Floor2Int(float val) {
	return static_cast<int>(floorf(val));
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
	connect(ui->action_overlayStatsView, SIGNAL(triggered(bool)), this, SLOT(overlayStatsChanged(bool)));
	
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

#if defined(__APPLE__)
	ui->outputTabs->setFont(QFont  ("Lucida Grande", 11));
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
	connect(ui->spinBox_updateInterval, SIGNAL(valueChanged(int)), this, SLOT(updateIntervalChanged(int)));
	connect(ui->table_servers, SIGNAL(itemSelectionChanged()), this, SLOT(networknodeSelectionChanged()));

	// Queue tab
	connect(ui->button_addQueueFiles, SIGNAL(clicked()), this, SLOT(addQueueFiles()));
	connect(ui->button_removeQueueFiles, SIGNAL(clicked()), this, SLOT(removeQueueFiles()));
	connect(ui->spinBox_overrideHaltSpp, SIGNAL(valueChanged(int)), this, SLOT(overrideHaltSppChanged(int)));
	connect(ui->spinBox_overrideHaltTime, SIGNAL(valueChanged(int)), this, SLOT(overrideHaltTimeChanged(int)));
	connect(ui->checkBox_loopQueue, SIGNAL(stateChanged(int)), this, SLOT(loopQueueChanged(int)));
	connect(ui->checkBox_overrideWriteFlm, SIGNAL(toggled(bool)), this, SLOT(overrideWriteFlmChanged(bool)));
	ui->table_queue->setColumnHidden(2, true);

	// Buttons
	connect(ui->button_imagingApply, SIGNAL(clicked()), this, SLOT(applyTonemapping()));
	connect(ui->button_imagingReset, SIGNAL(clicked()), this, SLOT(resetToneMapping()));
	connect(ui->tabs_main, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

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
	statsMessage = new QLabel();
    
	activityLabel->setMaximumWidth(60);
	activityMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	activityMessage->setMaximumWidth(140);
	statusLabel->setMaximumWidth(60);
	statusMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statusMessage->setMaximumWidth(320);
	statusProgress->setMaximumWidth(100);
	statusProgress->setRange(0, 100);
	statsLabel->setMaximumWidth(70);
	statsMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
	ui->statusbar->addPermanentWidget(activityLabel, 1);
	ui->statusbar->addPermanentWidget(activityMessage, 1);
    
	ui->statusbar->addPermanentWidget(statusLabel, 1);
	ui->statusbar->addPermanentWidget(statusMessage, 1);
	ui->statusbar->addPermanentWidget(statusProgress, 1);
	ui->statusbar->addPermanentWidget(statsLabel, 1);
	ui->statusbar->addPermanentWidget(statsMessage, 1);

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

	updateWidgetValue(ui->spinBox_updateInterval, luxGetNetworkServerUpdateInterval());

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

	WriteSettings();

	delete ui;
	delete statsMessage;
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
	settings.endGroup();
}

void MainWindow::ShowTabLogIcon ( int index, const QIcon & icon )
{
	ui->tabs_main->setTabIcon(index, icon);
}

void MainWindow::toneMapParamsChanged()
{
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::indicateActivity(bool active)
{
	if (active) {
#if !defined(__APPLE__)
		statusProgress->show ();
#endif
		statusProgress->setRange(0, 0);
	}
	else {
#if !defined(__APPLE__)
		statusProgress->hide ();
#endif
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
		if (QMessageBox::question(this, tr("Current file is still rendering"),tr("Do you want to stop the current render and start a new one?"), QMessageBox::Yes|QMessageBox::No) == QMessageBox::No) {
			return false;
		}
	}
	// clear rendering queue
	ClearRenderingQueue();
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
	
	QString fileName = QFileDialog::getOpenFileName(this, tr("Choose a scene file to open"), m_lastOpendir, tr("LuxRender Files (*.lxs)"));
    
	if(!fileName.isNull()) {
		endRenderingSession();
		renderScenefile(fileName);
	}
}

void MainWindow::openRecentFile()
{
	if (!canStopRendering())
		return;
    
	QAction *action = qobject_cast<QAction *>(sender());

	if (action) {
		endRenderingSession();
		renderScenefile(action->data().toString());
	}
}

void MainWindow::resumeFLM()
{
	if (!canStopRendering())
		return;

	QString lxsFileName = QFileDialog::getOpenFileName(this, tr("Choose a scene file to open"), m_lastOpendir, tr("LuxRender Files (*.lxs)"));
	
	if(lxsFileName.isNull())
		return;
	
	setCurrentFile(lxsFileName); // make sure m_lastOpendir stays at lxs-location
	
	// suggest .flm file with same name if it exists
	QFileInfo openDirFile(m_lastOpendir + "/" + m_CurrentFileBaseName + ".flm");
	QString openDirName = openDirFile.exists() ? openDirFile.absoluteFilePath() : m_lastOpendir;

	QString flmFileName = QFileDialog::getOpenFileName(this, tr("Choose an FLM file to open"), openDirName, tr("LuxRender FLM files (*.flm)"));
	
	if(flmFileName.isNull())
		return;

	endRenderingSession();

	renderScenefile (lxsFileName, flmFileName);
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

	//SetTitle(wxT("LuxRender - ")+fn.GetName());

	indicateActivity ();
	statusMessage->setText("Loading FLM...");

	// Start load thread
	m_loadTimer->start(1000);

	delete m_flmloadThread;
	m_flmloadThread = new boost::thread(boost::bind(&MainWindow::flmLoadThread, this, flmFileName));
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
	m_flmsaveThread = new boost::thread(boost::bind(&MainWindow::flmSaveThread, this, flmFileName));
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
		if (luxGetServerCount() < 1) {
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
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Tonemapped Image"), m_lastOpendir + "/" + m_CurrentFileBaseName, tr("PNG Image (*.png);;JPEG Image (*.jpg);;Windows Bitmap (*.bmp);;TIFF Image (*.tif)"));
	if (fileName.isEmpty()) 
		return;

	if (saveCurrentImageTonemapped(fileName, ui->action_overlayStats->isChecked(), ui->action_useAlpha->isChecked())) {
		statusMessage->setText(tr("Tonemapped image saved"));
		LOG(LUX_INFO, LUX_NOERROR) << "Tonemapped image saved to '" << fileName.toStdString() << "'";
	} else {
		statusMessage->setText(tr("ERROR: Tonemapped image NOT saved."));
		LOG(LUX_WARNING, LUX_SYSTEM) << "Error while saving tonemapped image to '" << fileName.toStdString() << "'";
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
		LOG(LUX_INFO, LUX_NOERROR) << "HDR image saved to '" << fileName.toStdString() << "'";
	} else {
		statusMessage->setText(tr("ERROR: HDR image NOT saved."));
		LOG(LUX_WARNING, LUX_SYSTEM) << "Error while saving HDR image to '" << fileName.toStdString() << "'";
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
	QString fileName = QFileDialog::getSaveFileName(this, tr("Select a destination for the images"), m_lastOpendir, tr("PNG Image (*.png);;JPEG Image (*.jpg);;Windows Bitmap (*.bmp);;TIFF Image (*.tif)"));
	if (fileName.isEmpty()) 
		return;
	
	// Show busy cursor
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	
	// Output the light groups
	if (saveAllLightGroups(fileName, false)) {
		statusMessage->setText(tr("Light group tonemapped images saved"));
		LOG(LUX_INFO, LUX_NOERROR) << "Light group tonemapped images saved to '" << fileName.toStdString() << "'";
	} else {
		statusMessage->setText(tr("ERROR: Light group tonemapped images NOT saved"));
		LOG(LUX_WARNING, LUX_SYSTEM) << "Error while saving light group tonemapped images to '" << fileName.toStdString() << "'";
	}
	
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
	if (saveAllLightGroups(fileName, true)) {
		statusMessage->setText(tr("Light group HDR images saved"));
		LOG(LUX_INFO, LUX_NOERROR) << "Light group HDR images saved to '" << fileName.toStdString() << "'";
	} else {
		statusMessage->setText(tr("ERROR: Light group HDR images NOT saved"));
		LOG(LUX_WARNING, LUX_SYSTEM) << "Error while saving light group HDR images to '" << fileName.toStdString() << "'";
	}
	
	// Stop showing busy cursor
	QApplication::restoreOverrideCursor();
}

bool MainWindow::saveAllLightGroups(const QString &outFilename, const bool &asHDR)
{	
	// Get number of light groups
	int lgCount = (int)luxGetParameterValue(LUX_FILM, LUX_FILM_LG_COUNT);
	
	// Start by save current light group state and turning off ALL light groups
	vector<bool> prevLGState(lgCount);
	for(int i=0; i<lgCount; i++)
	{
		prevLGState[i] = (luxGetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, i) != 0.f);
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, 0.f, i);
	}
	
	// Prepare filename (will hack up when outputting each light group)
	boost::filesystem::path filenamePath(outFilename.toStdString());
	
	// Get film resolution (only needed for tonemapped case but better off being outside loop)
	int w = luxGetIntAttribute("film", "xResolution");
	int h = luxGetIntAttribute("film", "yResolution");
	
	// Now, turn one light group on at a time, update the film and save to an image
	bool result = true;
	for(int i=0; i<lgCount; i++) {
		// Get light group name
		char lgName[256];
		luxGetStringParameterValue(LUX_FILM, LUX_FILM_LG_NAME, lgName, 256, i);
		
		// Enable light group (and tonemap if not saving as HDR)
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, 1.f, i);
		if (!asHDR) 
			luxUpdateFramebuffer();
		
		// Output image
		QString outputName = QString("%1/%2-%3").arg(filenamePath.parent_path().string().c_str())
			.arg(filenamePath.stem().c_str()).arg(lgName);

		if (asHDR)
			if (ui->action_HDR_tonemapped->isChecked())
				luxSaveEXR(QString("%1.exr").arg(outputName).toAscii().data(), openExrHalfFloats, openExrDepthBuffer, openExrCompressionType, true);
			else
				luxSaveEXR(QString("%1.exr").arg(outputName).toAscii().data(), openExrHalfFloats, openExrDepthBuffer, openExrCompressionType, false);
		else {
			QImage image = getFramebufferImage();
			if(!image.isNull())
				result = image.save(QString("%1%2").arg(outputName).arg(filenamePath.extension().c_str()));
			else
				result = false;
		}
		
		// Turn group back off
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, 0.f, i);
		if (!result) 
			break;
	}
	
	// Restore previous light group state
	for(int i=0; i<lgCount; i++)
		luxSetParameterValue(LUX_FILM, LUX_FILM_LG_ENABLE, (prevLGState[i]?1.f:0.f), i);
	
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
    if(luxStatistics("sceneIsReady") || luxStatistics("filmIsReady"))
    {
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
			default:
			case QMessageBox::Cancel:
				return;
			break;
				
			// Continue with batch processing
			case QMessageBox::Discard: break;
        }
    }
	
    // Show the batch processing dialog for user to customize
    BatchProcessDialog *batchDialog = new BatchProcessDialog(m_lastOpendir, this);
    if(batchDialog->exec() == QDialog::Rejected) return;
		
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

    QString outExtension = "exr";
    if(!asHDR) {
        switch(batchDialog->format())
        {
            default: case 0: outExtension = "png"; break;
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
    if(m_batchProcessThread != NULL) delete m_batchProcessThread;
    m_batchProcessThread = new boost::thread(boost::bind(&MainWindow::batchProcessThread, this, inDir, outDir, outExtension, allLightGroups, asHDR));
}

// Stop rendering session entirely - this is different from stopping it; it's not resumable
void MainWindow::endRenderingSession(bool abort)
{
	statusMessage->setText("");
	// Clean up if this is not the first rendering
	if (m_guiRenderState != WAITING) {
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
			m_flmloadThread->join();
		delete m_flmloadThread;
		m_flmloadThread = NULL;
		if (m_flmsaveThread)
			m_flmsaveThread->join();
		delete m_flmsaveThread;
		m_flmsaveThread = NULL;
		if (m_updateThread)
			m_updateThread->join();
		delete m_updateThread;
		m_updateThread = NULL;

		// TODO - make this async as it can block for tens of seconds
		if (abort)
			luxAbort();
		else
			luxExit();
		if (m_engineThread)
			m_engineThread->join();
		delete m_engineThread;
		m_engineThread = NULL;
		LOG( LUX_INFO,LUX_NOERROR)<< "Freeing resources.";
		luxCleanup();
		changeRenderState (WAITING);
		renderView->setLogoMode ();
	}
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
			LOG(LUX_INFO,LUX_NOERROR)<< tr("GUI: Updating framebuffer...").toLatin1().data();
			statusMessage->setText(tr("Tonemapping..."));
		} else {
			LOG(LUX_INFO,LUX_NOERROR)<< tr("GUI: Updating framebuffer/Computing Lens Effect Layer(s)...").toLatin1().data();
			statusMessage->setText(tr("Computing Lens Effect Layer(s) & Tonemapping..."));
			indicateActivity();
		}
		m_updateThread = new boost::thread(boost::bind(&MainWindow::updateThread, this));
	}
	else if (m_updateThread != NULL ) // Tonemapping in progress, request second event, hack for "tonemapping lag."
	{
		m_bTonemapPending = true;
	}
}

void MainWindow::engineThread(QString filename)
{
	boost::filesystem::path fullPath(boost::filesystem::initial_path());
	fullPath = boost::filesystem::system_complete(boost::filesystem::path(filename.toStdString().c_str(), boost::filesystem::native));

	chdir(fullPath.branch_path().string().c_str());

	// NOTE - lordcrc - initialize rand()
	qsrand(time(NULL));

	// if stdin is input, don't use full path
	if (filename == QString::fromAscii("-"))
		luxParse(filename.toStdString().c_str());
	else
		luxParse(fullPath.leaf().c_str());

	if (luxStatistics("terminated"))
		return;

	if(!luxStatistics("sceneIsReady")) {
		qApp->postEvent(this, new QEvent((QEvent::Type)EVT_LUX_PARSEERROR));
		luxWait();
	} else {
		luxWait();
		qApp->postEvent(this, new QEvent((QEvent::Type)EVT_LUX_FINISHED));
		LOG(LUX_INFO,LUX_NOERROR)<< tr("Rendering done.").toLatin1().data();
	}
}

void MainWindow::updateThread()
{
	luxUpdateFramebuffer ();
	luxUpdateLogFromNetwork();
	qApp->postEvent(this, new QEvent((QEvent::Type)EVT_LUX_TONEMAPPED));
}

void MainWindow::flmLoadThread(QString filename)
{
	luxLoadFLM(filename.toStdString().c_str());

	if (!luxStatistics("filmIsReady")) {
		qApp->postEvent(this, new QEvent((QEvent::Type)EVT_LUX_FLMLOADERROR));
	}
}

void MainWindow::flmSaveThread(QString filename)
{
	luxSaveFLM(filename.toStdString().c_str());

	qApp->postEvent(this, new QEvent((QEvent::Type)EVT_LUX_SAVEDFLM));
}

void MainWindow::batchProcessThread(QString inDir, QString outDir, QString outExtension, bool allLightGroups, bool asHDR)
{
    // Find 'flm' files in the input directory
    QVector<string> flmFiles, flmStems;
    for(boost::filesystem::directory_iterator itr(inDir.toStdString());
        itr != boost::filesystem::directory_iterator();
        itr++)
    {
        const boost::filesystem::path curPath = itr->path();
        if(curPath.extension() == ".flm")
        {
            flmFiles.push_back(curPath.string());
            flmStems.push_back(curPath.stem());
        }
    }

    // Process the 'flm' files
    for(int i=0; i<flmFiles.size(); i++)
    {
        // Update progress
        qApp->postEvent(this, new BatchEvent(QString(flmStems[i].c_str()), i, flmFiles.size()));

        // Load FLM into Lux engine
        luxLoadFLM(flmFiles[i].c_str());
        if(!luxStatistics("filmIsReady")) continue;

        // Check for cancel
        if (batchProgress && batchProgress->wasCanceled()) return;

        // Make output filename
        QString outName = QString("%1/%2.%3").arg(outDir).arg(flmStems[i].c_str()).arg(outExtension);

        // Save loaded FLM
        if(allLightGroups) saveAllLightGroups(outName, asHDR);
        else
        {
            luxUpdateFramebuffer();
            if(asHDR) saveCurrentImageHDR(outName);
            else saveCurrentImageTonemapped(outName);
        }
		LOG(LUX_INFO, LUX_NOERROR) << "Saved '" << flmFiles[i] << "' as '" << outName.toStdString();

        // Check again for cancel
        if (batchProgress && batchProgress->wasCanceled()) return;
    }

	// Signal completion
	qApp->postEvent(this, new BatchEvent(flmFiles[flmFiles.size()-1].c_str(), flmFiles.size(), flmFiles.size()));
}

void MainWindow::networkAddRemoveSlavesThread(QVector<QString> slaves, ChangeSlavesAction action) {
	for (int i = 0; i < slaves.size(); ++i) {
		switch (action) {
			case AddSlaves:
				luxAddServer(slaves[i].toStdString().c_str());
				break;
			case RemoveSlaves:
				luxRemoveServer(slaves[i].toStdString().c_str());
				break;
			default:
				break;
		}
	}
	qApp->postEvent(this, new NetworkUpdateTreeEvent());
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

#if defined(__APPLE__) // Doubleclick or dragging .lxs in OSX Finder to LuxRender

void  MainWindow::loadFile(const QString &fileName)
{
	if (fileName.endsWith(".lxs")){
		if (!canStopRendering())
			return;
		endRenderingSession();
		renderScenefile(fileName);
	} else if (fileName.endsWith(".flm")){
		if (!canStopRendering())
			return;
		if(fileName.isNull())
			return;
		
		setCurrentFile(fileName); // show flm-name in windowheader
		
		endRenderingSession();
		
		indicateActivity ();
		statusMessage->setText("Loading FLM...");
		// Start load thread
		m_loadTimer->start(1000);
		
		delete m_flmloadThread;
		m_flmloadThread = new boost::thread(boost::bind(&MainWindow::flmLoadThread, this, fileName));
	} else {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		QFileInfo fi(fileName);
		QString name = fi.fileName();
		msgBox.setText(name +(" is not a renderable scenefile. Please choose an .lxs"));
		msgBox.exec();
	}
}
#endif

void MainWindow::updateStatistics()
{
//	m_samplesSec = luxStatistics("samplesSec");
//	int samplesSec = Floor2Int(m_samplesSec);
//	int samplesTotSec = Floor2Int(luxStatistics("samplesTotSec"));
//	int secElapsed = Floor2Int(luxStatistics("secElapsed"));
//	double samplesPx = luxStatistics("samplesPx");
//	int efficiency = Floor2Int(luxStatistics("efficiency"));
//	int EV = luxGetFloatAttribute("film", "EV");
//
//	int secs = (secElapsed) % 60;
//	int mins = (secElapsed / 60) % 60;
//	int hours = (secElapsed / 3600);
//
//	statsMessage->setText(QString("%1:%2:%3 - %4 S/s - %5 TotS/s - %6 S/px - %7% eff - EV = %8").arg(hours,2,10,QChar('0')).arg(mins, 2,10,QChar('0')).arg(secs,2,10,QChar('0')).arg(samplesSec).arg(samplesTotSec).arg(samplesPx, 0, 'f', 2).arg(efficiency).arg(EV));
	statsMessage->setText(QString( luxPrintableStatistics(true) ));
}

// show the render-resolution
void MainWindow::showRenderresolution()
{
	if (luxHasObject("film")) {
		int w = luxGetIntAttribute("film", "xResolution");
		int h = luxGetIntAttribute("film", "yResolution");
		ui->resolutioniconLabel->setPixmap(QPixmap(":/icons/resolutionicon.png"));
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
	// Get the absolute path of the flm file
	boost::filesystem::path fullPath(boost::filesystem::initial_path());
	fullPath = boost::filesystem::system_complete(boost::filesystem::path(flmFilename.toStdString().c_str(), boost::filesystem::native));

	// Set the FLM filename
	luxOverrideResumeFLM(fullPath.string().c_str());

	// Render the scene
	renderScenefile(sceneFilename);
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

void MainWindow::renderScenefile(const QString& filename)
{
	// CF
	setCurrentFile(filename);

	changeRenderState(PARSING);
	
    indicateActivity ();
	statusMessage->setText("Loading scene...");
    
	m_loadTimer->start(1000);

	// Start main render thread
	if (m_engineThread) {
		m_engineThread->join();
		delete m_engineThread;
	}

	m_engineThread = new boost::thread(boost::bind(&MainWindow::engineThread, this, filename));
}

// TODO: replace by QStateMachine
void MainWindow::changeRenderState(LuxGuiRenderState state)
{
	switch (state) {
		case WAITING:
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
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
			m_updateThread->join();
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
			m_flmloadThread->join();
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
				renderingNext = RenderNextFileInQueue();
			
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
			m_flmsaveThread->join();
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

		saveNetworkSlaves();
		UpdateNetworkTree();

		retval = TRUE;
	}

	if (retval)
		// Was our event, stop the event propagation
		event->accept();

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
	bool warning = false;
	bool error = false;

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
			warning = true;            
			break;
		case LUX_ERROR:
			ss << tr("Error: ");
			textColor = errorColour;
			error = true;
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
	if (currentIndex != 1 && event->getSeverity() > LUX_INFO) {
		blink = true;
		if (event->getSeverity() < LUX_ERROR) {
			static const QIcon icon(":/icons/warningicon.png");
			ShowTabLogIcon(1, icon);
		} else {
			blinkTrigger();
			statusMessage->setText("Check Log Please");
		}
	}
}

//user acknowledged error/warning-conditions 
void MainWindow::tabChanged(int)
{ 
	int currentIndex = ui->tabs_main->currentIndex();
	if (currentIndex == 1) {
		blinkTrigger(false);
		static const QIcon icon(":/icons/logtabicon.png");
		ShowTabLogIcon(1, icon);
		statusMessage->setText("Checking Log acknowledged");
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
			ShowTabLogIcon(1, icon);
		}
		else {
			static const QIcon icon(":/icons/logtabicon.png");
			ShowTabLogIcon(1, icon);
		}
	} else {
		m_blinkTimer->stop();
		blink = false;
		static const QIcon icon(":/icons/logtabicon.png");
		ShowTabLogIcon(1, icon);
	}
}

void MainWindow::renderTimeout()
{
	if (m_updateThread == NULL && (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) &&
		(!isMinimized () || m_guiRenderState == FINISHED)) {
		LOG(LUX_INFO,LUX_NOERROR)<< tr("GUI: Updating framebuffer...").toLatin1().data();
		statusMessage->setText("Tonemapping...");
		m_updateThread = new boost::thread(boost::bind(&MainWindow::updateThread, this));
	}
}

void MainWindow::statsTimeout()
{
	if(luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		updateStatistics();
		if ((m_guiRenderState == STOPPING || m_guiRenderState == ENDING || m_guiRenderState == FINISHED) 
			&& luxStatistics("samplesSec") == 0.0) {
			// Render threads stopped, do one last render update
			LOG(LUX_INFO,LUX_NOERROR)<< tr("GUI: Updating framebuffer...").toLatin1().data();
			statusMessage->setText(tr("Tonemapping..."));
			if (m_updateThread)
				m_updateThread->join();
			delete m_updateThread;
			m_updateThread = new boost::thread(boost::bind(&MainWindow::updateThread, this));
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
		indicateActivity(false);
		statusMessage->setText("");
		m_loadTimer->stop();
		gammawidget->resetFromFilm(true);

		if (luxStatistics("sceneIsReady")) {
			addRemoveSlaves(savedNetworkSlaves, AddSlaves);

			// Scene file loaded
			// Add other render threads if necessary
			int curThreads = 1;
			while(curThreads < m_numThreads) {
				luxAddThread();
				curThreads++;
			}

			updateWidgetValue(ui->spinBox_overrideDisplayInterval, luxGetIntAttribute("film", "displayInterval"));

			// override halt conditions if needed
			if (IsFileInQueue(m_CurrentFile)) {
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

			// Start updating the display by faking a resume menu item click.
			ui->action_resumeRender->activate(QAction::Trigger);

			// Enable tonemapping options and reset from values trough API
			ui->outputTabs->setEnabled (true);
			resetToneMappingFromFilm (false);
			ResetLightGroupsFromFilm (false);
			
			renderView->resetTransform();
		}
	}
	else if ( luxStatistics("filmIsReady") ) {
		indicateActivity(false);
		statusMessage->setText("");
		m_loadTimer->stop();

		if(m_flmloadThread) {
			m_flmloadThread->join();
			delete m_flmloadThread;
			m_flmloadThread = NULL;
		}

		changeRenderState(TONEMAPPING);

		// Enable tonemapping options and reset from values trough API
		ui->outputTabs->setEnabled (true);
		resetToneMappingFromFilm (false);
		ResetLightGroupsFromFilm( false );
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
		pane->showOnOffButton();
		LightGroupWidget *currWidget = new LightGroupWidget(pane);
		currWidget->SetIndex(i);
		currWidget->ResetValuesFromFilm( useDefaults );
		pane->setTitle(currWidget->GetTitle());
		pane->setIcon(":/icons/lightgroupsicon.png");
		pane->setWidget(currWidget);
		connect(currWidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));
		connect(currWidget, SIGNAL(signalLightGroupSolo(int)), this, SLOT(setLightGroupSolo(int)));
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
			}
			else if ( n != index )
			{
				((LightGroupWidget *)(*it)->getWidget())->setEnabled( false );
			}
		}
	}
}

void MainWindow::UpdateNetworkTree()
{
	updateWidgetValue(ui->spinBox_updateInterval, luxGetNetworkServerUpdateInterval());

	int currentrow = ui->table_servers->currentRow();

	ui->table_servers->setRowCount(0);
	
	int nServers = luxGetServerCount();

	RenderingServerInfo *pInfoList = new RenderingServerInfo[nServers];
	nServers = luxGetRenderingServersStatus( pInfoList, nServers );

	ui->table_servers->setRowCount(nServers);

	double totalpixels = luxGetIntAttribute("film", "xResolution") * luxGetIntAttribute("film", "yResolution");
	double spp;
	
	for( int n = 0; n < nServers; n++ ) {
		QTableWidgetItem *servername = new QTableWidgetItem(QString::fromUtf8(pInfoList[n].name));
		QTableWidgetItem *port = new QTableWidgetItem(QString::fromUtf8(pInfoList[n].port));
		
		spp = pInfoList[n].numberOfSamplesReceived / totalpixels;
		
		QString s = QString("%1").arg(spp,0,'g',3);
		
		QTableWidgetItem *spp = new QTableWidgetItem(s);
		
		ui->table_servers->setItem(n, 0, servername);
		ui->table_servers->setItem(n, 1, port);
		ui->table_servers->setItem(n, 2, spp);
	}

	ui->table_servers->blockSignals (true);
	ui->table_servers->selectRow(currentrow);
	ui->table_servers->blockSignals (false);
	
	delete[] pInfoList;
}

void MainWindow::saveNetworkSlaves() {
	int nServers = luxGetServerCount();

	RenderingServerInfo *pInfoList = new RenderingServerInfo[nServers];
	nServers = luxGetRenderingServersStatus( pInfoList, nServers );

	savedNetworkSlaves.clear();

	for (int i = 0; i < nServers; ++i) {
		savedNetworkSlaves.push_back(QString("%1:%2").arg(pInfoList[i].name).arg(pInfoList[i].port));
	}

	delete[] pInfoList;
}

void MainWindow::addRemoveSlaves(QVector<QString> slaves, ChangeSlavesAction action) {

	if (m_networkAddRemoveSlavesThread != NULL) {
		m_networkAddRemoveSlavesThread->join();
		delete m_networkAddRemoveSlavesThread;
	}

	ui->button_addServer->setEnabled(false);
	ui->button_removeServer->setEnabled(false);

	m_networkAddRemoveSlavesThread = new boost::thread(boost::bind(&MainWindow::networkAddRemoveSlavesThread, this, slaves, action));
}


void MainWindow::addServer()
{
	QString server = ui->lineEdit_server->text();

	if (!server.isEmpty()) {
		QVector<QString> slaves;
		slaves.push_back(server);

		addRemoveSlaves(slaves, AddSlaves);
	}
}

void MainWindow::removeServer()
{
	QString server = ui->lineEdit_server->text();

	if (!server.isEmpty()) {
		QVector<QString> slaves;
		slaves.push_back(server);

		addRemoveSlaves(slaves, RemoveSlaves);
	}
}

void MainWindow::updateIntervalChanged(int value)
{
	luxSetNetworkServerUpdateInterval(value);
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


	int row = ui->table_queue->rowCount();

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

		// not allowing duplicates
		if (IsFileInQueue(files[i]))
			continue;

		QTableWidgetItem *filename = new QTableWidgetItem(files[i]);
		QTableWidgetItem *status = new QTableWidgetItem("");
		QTableWidgetItem *pass = new QTableWidgetItem("0");

		if (files[i] == m_CurrentFile)
			status->setText(tr("Rendering"));

		ui->table_queue->insertRow(row);

		ui->table_queue->setItem(row, 0, filename);
		ui->table_queue->setItem(row, 1, status);
		ui->table_queue->setItem(row, 2, pass);

		row++;
	}

	ui->table_queue->resizeColumnsToContents();

	if (m_guiRenderState != RENDERING) {
		RenderNextFileInQueue();
	}
}

void MainWindow::removeQueueFiles()
{
	int idx = -1;
	int idxOffset = 0;
	for (int i = ui->table_queue->rowCount()-1; i >= 0; i--) {
		QTableWidgetItem *fname = ui->table_queue->item(i, 0);
		
		if (!fname->isSelected())
			continue;

		// increase offset for rows removed above active file
		if (idx >= 0)
			idxOffset++;

		// stop rendering if current file is active
		if (fname->text() == m_CurrentFile) {
			idx = i;
		}

		ui->table_queue->removeRow(i);
	}

	if (idx >= 0) {
		endRenderingSession();
		changeRenderState(STOPPED);

		RenderNextFileInQueue(idx - idxOffset);
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
	for (int i = 0; i < ui->table_queue->rowCount(); i++) {
		QTableWidgetItem *fname = ui->table_queue->item(i, 0);

		if (fname && (fname->text() == filename))
			return true;
	}

	return false;
}

bool MainWindow::IsFileQueued()
{
	return ui->table_queue->rowCount() > 0;
}

bool MainWindow::RenderNextFileInQueue()
{
	int idx = -1;

	// update current file
	for (int i = 0; i < ui->table_queue->rowCount(); i++) {
		QTableWidgetItem *fname = ui->table_queue->item(i, 0);

		if (fname->text() == m_CurrentFile) {
			idx = i;
			break;
		}
	}

	if (idx >= 0) {
		QTableWidgetItem *status = ui->table_queue->item(idx, 1);
		status->setText(tr("Completed ") + QDateTime::currentDateTime().toString(Qt::DefaultLocaleShortDate));
		LOG(LUX_INFO,LUX_NOERROR) << "==== Queued file '" << m_CurrentFileBaseName.toStdString() << "' done ====";
	}

	ui->table_queue->resizeColumnsToContents();

	idx++;
	return RenderNextFileInQueue(idx);
}

bool MainWindow::RenderNextFileInQueue(int idx)
{
	// render next
	if (idx >= ui->table_queue->rowCount()) {
		if (ui->checkBox_loopQueue->isChecked()) {
			ui->table_queue->setColumnHidden(2, false);
			idx = 0;
		}
		else
			return false;
	}

	QString filename = ui->table_queue->item(idx, 0)->text();
	QTableWidgetItem *status = ui->table_queue->item(idx, 1);
	QTableWidgetItem *pass = ui->table_queue->item(idx, 2);

	status->setText(tr("Rendering"));
	pass->setText(QString("%1").arg(pass->text().toInt() + 1));

	ui->table_queue->resizeColumnsToContents();


	endRenderingSession();

	if (ui->checkBox_overrideWriteFlm->isChecked())
		luxOverrideResumeFLM("");

	renderScenefile(filename);

	return true;
}

void MainWindow::ClearRenderingQueue()
{
	ui->table_queue->clearContents();
	ui->table_queue->setColumnHidden(2, true);
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

