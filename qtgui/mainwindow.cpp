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

#include <boost/bind.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <boost/cast.hpp>

#include <sstream>
#include <clocale>

#include <QList>

#include <QDateTime>
#include <QTextStream>

#include "error.h"

#include "mainwindow.hxx"
#include "ui_luxrender.h"
#include "aboutdialog.hxx"

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

LuxLogEvent::LuxLogEvent(int code,int severity,QString message) : QEvent((QEvent::Type)EVT_LUX_LOGEVENT), _message(message), _severity(severity), _code(code)
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

int ValueToLogSliderVal(float value, const float logLowerBound, const float logUpperBound)
{

	if (value <= 0)
		return 0;

	float logvalue = Clamp<float>(log10f(value), logLowerBound, logUpperBound);

	const int val = static_cast<int>((logvalue - logLowerBound) / 
		(logUpperBound - logLowerBound) * FLOAT_SLIDER_RES);
	return val;
}

float LogSliderValToValue(int sliderval, const float logLowerBound, const float logUpperBound)
{

	float logvalue = (float)sliderval * (logUpperBound - logLowerBound) / 
		FLOAT_SLIDER_RES + logLowerBound;

	return powf(10.f, logvalue);
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

MainWindow::MainWindow(QWidget *parent, bool copylog2console) : QMainWindow(parent), ui(new Ui::MainWindow), m_copyLog2Console(copylog2console)
{
	
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
	connect(ui->action_exitApp, SIGNAL(triggered()), this, SLOT(exitApp()));
	
	// Image menu slots
	connect(ui->action_outputTonemapped, SIGNAL(triggered()), this, SLOT(outputTonemapped()));

	// Render menu slots
	connect(ui->action_resumeRender, SIGNAL(triggered()), this, SLOT(resumeRender()));
	connect(ui->button_resume, SIGNAL(clicked()), this, SLOT(resumeRender()));
	connect(ui->action_pauseRender, SIGNAL(triggered()), this, SLOT(pauseRender()));
	connect(ui->button_pause, SIGNAL(clicked()), this, SLOT(pauseRender()));
	connect(ui->action_stopRender, SIGNAL(triggered()), this, SLOT(stopRender()));
	connect(ui->button_stop, SIGNAL(clicked()), this, SLOT(stopRender()));
	
	// View menu slots
	connect(ui->action_copyLog, SIGNAL(triggered()), this, SLOT(copyLog()));
	connect(ui->action_clearLog, SIGNAL(triggered()), this, SLOT(clearLog()));
	connect(ui->action_fullScreen, SIGNAL(triggered()), this, SLOT(fullScreen()));
    connect(ui->action_normalScreen, SIGNAL(triggered()), this, SLOT(normalScreen()));
	
	// Help menu slots
	connect(ui->action_aboutDialog, SIGNAL(triggered()), this, SLOT(aboutDialog()));
	connect(ui->action_documentation, SIGNAL(triggered()), this, SLOT(openDocumentation()));
	connect(ui->action_forums, SIGNAL(triggered()), this, SLOT(openForums()));
	
	connect(ui->checkBox_imagingAuto, SIGNAL(stateChanged(int)), this, SLOT(autoEnabledChanged(int)));

	// Panes
	panes[0] = new PaneWidget(ui->panesAreaContents, "Tone Mapping", ":/icons/tonemapicon.png");
	panes[1] = new PaneWidget(ui->panesAreaContents, "Lens Effects", ":/icons/lenseffectsicon.png", true);
	panes[2] = new PaneWidget(ui->panesAreaContents, "Color Space", ":/icons/colorspaceicon.png");
	panes[3] = new PaneWidget(ui->panesAreaContents, "Gamma", ":/icons/gammaicon.png", true);
	panes[4] = new PaneWidget(ui->panesAreaContents, "HDR Histogram", ":/icons/histogramicon.png");
	panes[5] = new PaneWidget(ui->panesAreaContents, "Noise Reduction", ":/icons/noisereductionicon.png", true);
	
#if defined(__APPLE__)
	ui->outputTabs->setFont(QFont  ("Lucida Grande", 11));
#endif

	// Tonemap page
	tonemapwidget = new ToneMapWidget(panes[0]);
	panes[0]->setWidget(tonemapwidget);
	ui->panesLayout->addWidget(panes[0]);
	panes[0]->expand();
	connect(tonemapwidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Lens effects page
	lenseffectswidget = new LensEffectsWidget(panes[1]);
	panes[1]->setWidget(lenseffectswidget);
	ui->panesLayout->addWidget(panes[1]);
	connect(lenseffectswidget, SIGNAL(forceUpdate()), this, SLOT(forceToneMapUpdate()));
	connect(lenseffectswidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Colorspace
	colorspacewidget = new ColorSpaceWidget(panes[2]);
	panes[2]->setWidget(colorspacewidget);
	ui->panesLayout->addWidget(panes[2]);
	connect(colorspacewidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Gamma
	gammawidget = new GammaWidget(panes[3]);
	panes[3]->setWidget(gammawidget);
	ui->panesLayout->addWidget(panes[3]);
	connect(gammawidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));

	// Histogram
	histogramwidget = new HistogramWidget(panes[4]);
	panes[4]->setWidget(histogramwidget);
	ui->panesLayout->addWidget(panes[4]);
    panes[4]->expand();

	// Noise reduction
	noisereductionwidget = new NoiseReductionWidget(panes[5]);
	panes[5]->setWidget(noisereductionwidget);
	ui->panesLayout->addWidget(panes[5]);
	connect(noisereductionwidget, SIGNAL(valuesChanged()), this, SLOT(toneMapParamsChanged()));
	
	ui->panesLayout->setAlignment(Qt::AlignTop);
	ui->panesLayout->addItem(new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
	
	ui->lightGroupsLayout->setAlignment(Qt::AlignTop);
	spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);

	// Network tab
	connect(ui->button_addServer, SIGNAL(clicked()), this, SLOT(addServer()));
	connect(ui->lineEdit_server, SIGNAL(returnPressed()), this, SLOT(addServer()));
	connect(ui->button_removeServer, SIGNAL(clicked()), this, SLOT(removeServer()));
	connect(ui->spinBox_updateInterval, SIGNAL(valueChanged(int)), this, SLOT(updateIntervalChanged(int)));
	connect(ui->table_servers, SIGNAL(itemSelectionChanged()), this, SLOT(networknodeSelectionChanged()));

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

	// used in ResetToneMapping
	m_auto_tonemap = false;
	resetToneMapping();
	m_auto_tonemap = true;

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
	delete renderView;
	delete tonemapwidget;
	delete lenseffectswidget;
	delete colorspacewidget;
	delete noisereductionwidget;
	delete histogramwidget;

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
	m_recentFiles = settings.value("recentFiles").toStringList();
	m_lastOpendir = settings.value("lastOpenDir","").toString();
	settings.endGroup();

	updateRecentFileActions();
}

void MainWindow::WriteSettings()
{
	QSettings settings("luxrender.net", "LuxRender GUI");

	settings.beginGroup("MainWindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("splittersizes", ui->splitter->saveState());
	settings.setValue("recentFiles", m_recentFiles);
	settings.setValue("lastOpenDir", m_lastOpendir);
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

void MainWindow::LuxGuiErrorHandler(int code, int severity, const char *msg)
{
	if (copyLog2Console)
	    luxErrorPrint(code, severity, msg);
	
	foreach (QWidget *widget, qApp->topLevelWidgets())
		qApp->postEvent(widget, new LuxLogEvent(code,severity,QString(msg)));
}

bool MainWindow::canStopRendering()
{
	if (m_guiRenderState == RENDERING) {
		// Give warning that current rendering is not stopped
		if (QMessageBox::question(this, tr("Current file is still rendering"),tr("Do you want to stop the current render and start a new one?"), QMessageBox::Yes|QMessageBox::No) == QMessageBox::No) {
			return false;
		}
	}
	return true;
}

// File menu slots
void MainWindow::exitApp()
{
	endRenderingSession();
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

	endRenderingSession ();

	renderScenefile (lxsFileName, flmFileName);
}

void MainWindow::loadFLM()
{
	if (!canStopRendering())
		return;

	QString flmFileName = QFileDialog::getOpenFileName(this, tr("Choose an FLM file to open"), m_lastOpendir, tr("LuxRender FLM files (*.flm)"));
	
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

		m_renderTimer->start(1000*luxStatistics("displayInterval"));
		m_statsTimer->start(1000);
		m_netTimer->start(10000);
		
		if (m_guiRenderState == PAUSED || m_guiRenderState == STOPPED) // Only re-start if we were previously stopped
			luxStart();
		
		if (m_guiRenderState == STOPPED)
			luxSetHaltSamplePerPixel(-1, false, false);
		changeRenderState(RENDERING);
		showRenderresolution();
		showZoomfactor();
		showViewportsize();
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

		// Make sure lux core stops
		luxSetHaltSamplePerPixel(1, true, true);
		
		statusMessage->setText(tr("Waiting for render threads to stop."));
		changeRenderState(STOPPING);
	}
}

void MainWindow::outputTonemapped()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Tonemapped Image"), m_lastOpendir, tr("PNG Image (*.png);;JPEG Image (*.jpg);;Windows Bitmap (*.bmp);;TIFF Image (*.tif)"));
	if(fileName.isNull() || fileName.isEmpty()) return;
	if(renderView->outputTonemapped(fileName)) statusMessage->setText(tr("Tonemapped image saved to '%1'").arg(fileName));
	else statusMessage->setText(tr("ERROR: Tonemapped image NOT saved. May be an unsupported image type."));
}

// Stop rendering session entirely - this is different from stopping it; it's not resumable
void MainWindow::endRenderingSession()
{
	statusMessage->setText("");
	// Clean up if this is not the first rendering
	if (m_guiRenderState != WAITING) {
		m_renderTimer->stop ();
		m_statsTimer->stop ();
		m_netTimer->stop ();
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
		luxExit ();
		if (m_engineThread)
			m_engineThread->join();
		delete m_engineThread;
		m_engineThread = NULL;
		LOG( LUX_INFO,LUX_NOERROR)<< "Freeing resources.";
		luxCleanup ();
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
	if (fileName.endsWith("lxs")){
		if (!canStopRendering())
			return;
		endRenderingSession();
		renderScenefile(fileName);
	} else if (fileName.endsWith("flm")){
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
		msgBox.setText("Doubleclick and drag only handles Lux scenefiles. Please choose an .lxs");
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
		ui->resinfoLabel->setText(QString("Resolution: %1 x %2").arg(w).arg(h));
		ui->resinfoLabel->setVisible(true);
	} else {
		ui->resinfoLabel->setVisible(false);
	}
}
// show the zoom-factor in viewport
void MainWindow::showZoomfactor()
{
	ui->zoominfoLabel->setText((QString("Zoom Factor: %1").arg(renderView->getZoomFactor()))+ "%");
}
// show the actual viewportsize
void MainWindow::viewportChanged() {
	showZoomfactor();
	showViewportsize();
}
// actual viewportsize
void MainWindow::showViewportsize() {
	int viewportw = renderView->width(), viewporth = renderView->height();
	ui->viewportinfoLabel->setText(QString("Viewportsize: %1 x %2").arg(viewportw).arg(viewporth));
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
		else
			showName = "LuxRender - " + showName;
        
		m_CurrentFileBaseName = info.completeBaseName();

		m_lastOpendir = info.absolutePath();
		if (filename.endsWith("lxs")) {
			m_recentFiles.removeAll(m_CurrentFile);
			m_recentFiles.prepend(m_CurrentFile);
		
			updateRecentFileActions(); // only parseble files .lxs to recentlist
		}

	}

	setWindowTitle(tr("%1[*]").arg(showName));
}

void MainWindow::updateRecentFileActions()
{
	QMutableStringListIterator i(m_recentFiles);
	while (i.hasNext()) {
		if (!QFile::exists(i.next())) {
			i.remove();
		}
	}

	for (int j = 0; j < MaxRecentFiles; ++j) {
		if (j < m_recentFiles.count()) {
			QString text = tr("&%1 %2").arg(j + 1).arg( QFileInfo(m_recentFiles[j]).fileName());
			m_recentFileActions[j]->setText(text);
			m_recentFileActions[j]->setData(m_recentFiles[j]);
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

	// NOTE - lordcrc - create progress dialog before starting engine thread
	//                  so we don't try to destroy it before it's properly created
	
    indicateActivity ();
	statusMessage->setText("Loading scene...");
    
	m_loadTimer->start(1000);


	// Start main render thread
	if (m_engineThread)
		delete m_engineThread;

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
			ui->button_copyToClipboard->setEnabled (false);
			ui->action_outputTonemapped->setEnabled (false);
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
			ui->button_copyToClipboard->setEnabled (false);
			ui->action_outputTonemapped->setEnabled (false);
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
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
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
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
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
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			break;
		case STOPPED:
			// Rendering is stopped.
			ui->button_resume->setEnabled (true);
			ui->action_resumeRender->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
			activityMessage->setText("Render is stopped");
			break;
		case PAUSED:
			// Rendering is paused.
			ui->button_resume->setEnabled (true);
			ui->action_resumeRender->setEnabled (true);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (true);
			ui->action_stopRender->setEnabled (true);
			ui->button_copyToClipboard->setEnabled (true);
			ui->action_outputTonemapped->setEnabled (true);
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
			// Stop timers and update output one last time.
			stopRender();

			changeRenderState(FINISHED);
		}
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_SAVEDFLM) {
		m_saveTimer->stop();

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

	bool hasSelection = ui->textEdit_log->textCursor().hasSelection();
	int startPos, endPos;
	
	// Remember current selection, if any
	if (hasSelection) {
		startPos = ui->textEdit_log->textCursor().selectionStart();
		endPos = ui->textEdit_log->textCursor().selectionEnd();
	}

	// Append log message to end of document
	ui->textEdit_log->moveCursor(QTextCursor::End);

	switch(event->getSeverity()) {
		case LUX_DEBUG:
			ss << tr("Debug: ");
			ui->textEdit_log->setTextColor(debugColour);
			break;
		case LUX_INFO:
		default:
			ss << tr("Info: ");
			ui->textEdit_log->setTextColor(infoColour);
			break;
		case LUX_WARNING:
			ss << tr("Warning: ");
			ui->textEdit_log->setTextColor(warningColour);
			warning = true;            
			break;
		case LUX_ERROR:
			ss << tr("Error: ");
			ui->textEdit_log->setTextColor(errorColour);
			error = true;
			break;
		case LUX_SEVERE:
			ss << tr("Severe error: ");
			ui->textEdit_log->setTextColor(severeColour);
			break;
	}

	ss << event->getCode() << "] ";
	ss.flush();
	
	ui->textEdit_log->textCursor().insertText(ss.readAll());
	ui->textEdit_log->setTextColor(Qt::black);
	ss << event->getMessage() << endl;
	ui->textEdit_log->textCursor().insertText(ss.readAll());

	// Restore previous selection, if any
	if (hasSelection) {
		ui->textEdit_log->textCursor().setPosition(endPos);
		ui->textEdit_log->textCursor().setPosition(startPos);
	}

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
		if ((m_guiRenderState == STOPPING || m_guiRenderState == FINISHED) && luxStatistics("samplesSec") == 0.0) {
			// Render threads stopped, do one last render update
			LOG(LUX_INFO,LUX_NOERROR)<< tr("GUI: Updating framebuffer...").toLatin1().data();
			statusMessage->setText(tr("Tonemapping..."));
			delete m_updateThread;
			m_updateThread = new boost::thread(boost::bind(&MainWindow::updateThread, this));
			m_statsTimer->stop();
			luxPause();
			if (m_guiRenderState == FINISHED)
				LOG(LUX_INFO,LUX_NOERROR)<< tr("Rendering finished.").toLatin1().data();
			else {
				LOG(LUX_INFO,LUX_NOERROR)<< tr("Rendering stopped by user.").toLatin1().data();
				changeRenderState(STOPPED);
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
			// Scene file loaded
			// Add other render threads if necessary
			int curThreads = 1;
			while(curThreads < m_numThreads) {
				luxAddThread();
				curThreads++;
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
		ui->lightGroupsLayout->addWidget(pane);
		if (i == 0)
			pane->expand();
		m_LightGroupPanes.push_back(pane);
	}
	ui->lightGroupsLayout->addItem(spacer);

	// Update
	UpdateLightGroupWidgetValues();
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

void MainWindow::addServer()
{
	QString server = ui->lineEdit_server->text();

	if (!server.isEmpty()) {
		luxAddServer(server.toStdString().c_str());
		UpdateNetworkTree();
	}
}

void MainWindow::removeServer()
{
	QString server = ui->lineEdit_server->text();

	if (!server.isEmpty()) {
		luxRemoveServer(server.toStdString().c_str());
		UpdateNetworkTree();
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

