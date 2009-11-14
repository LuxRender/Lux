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

#include "mainwindow.hxx"
#include "ui_luxrender.h"
#include "aboutdialog.hxx"

#if defined(WIN32) && !defined(__CYGWIN__)
#include "direct.h"
#define chdir _chdir
#endif

using namespace lux;

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

MainWindow::MainWindow(QWidget *parent, bool opengl, bool copylog2console) : QMainWindow(parent), ui(new Ui::MainWindow), m_copyLog2Console(copylog2console), m_opengl(opengl)
{
	
	luxErrorHandler(&MainWindow::LuxGuiErrorHandler);
	
	ui->setupUi(this);

	// Remove the default page - gets created dynamically later
	ui->toolBox_lightgroups->removeItem(0);
	
	// File menu slots
	connect(ui->action_openFile, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(ui->action_resumeFLM, SIGNAL(triggered()), this, SLOT(resumeFLM()));
	connect(ui->action_loadFLM, SIGNAL(triggered()), this, SLOT(loadFLM()));
	connect(ui->action_saveFLM, SIGNAL(triggered()), this, SLOT(saveFLM()));
	connect(ui->action_exitApp, SIGNAL(triggered()), this, SLOT(exitApp()));
	
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
	
	// Help menu slots
	connect(ui->action_aboutDialog, SIGNAL(triggered()), this, SLOT(aboutDialog()));
	connect(ui->action_documentation, SIGNAL(triggered()), this, SLOT(openDocumentation()));
	connect(ui->action_forums, SIGNAL(triggered()), this, SLOT(openForums()));
	
	// Tonemap page - only show Reinhard frame by default
	
	ui->frame_toneMapLinear->hide ();
	ui->frame_toneMapContrast->hide ();

	connect(ui->checkBox_imagingAuto, SIGNAL(stateChanged(int)), this, SLOT(autoEnabledChanged(int)));

	// Reinhard
	connect(ui->comboBox_kernel, SIGNAL(currentIndexChanged(int)), this, SLOT(setTonemapKernel(int)));
	connect(ui->slider_prescale, SIGNAL(valueChanged(int)), this, SLOT(prescaleChanged(int)));
	connect(ui->spinBox_prescale, SIGNAL(valueChanged(double)), this, SLOT(prescaleChanged(double)));
	connect(ui->slider_postscale, SIGNAL(valueChanged(int)), this, SLOT(postscaleChanged(int)));
	connect(ui->spinBox_postscale, SIGNAL(valueChanged(double)), this, SLOT(postscaleChanged(double)));
	connect(ui->slider_burn, SIGNAL(valueChanged(int)), this, SLOT(burnChanged(int)));
	connect(ui->spinBox_burn, SIGNAL(valueChanged(double)), this, SLOT(burnChanged(double)));

	// Linear
	connect(ui->slider_sensitivity, SIGNAL(valueChanged(int)), this, SLOT(sensitivityChanged(int)));
	connect(ui->spinBox_sensitivity, SIGNAL(valueChanged(double)), this, SLOT(sensitivityChanged(double)));
	connect(ui->slider_exposure, SIGNAL(valueChanged(int)), this, SLOT(exposureChanged(int)));
	connect(ui->spinBox_exposure, SIGNAL(valueChanged(double)), this, SLOT(exposureChanged(double)));
	connect(ui->slider_fstop, SIGNAL(valueChanged(int)), this, SLOT(fstopChanged(int)));
	connect(ui->spinBox_fstop, SIGNAL(valueChanged(double)), this, SLOT(fstopChanged(double)));
	connect(ui->slider_gamma_linear, SIGNAL(valueChanged(int)), this, SLOT(gammaLinearChanged(int)));
	connect(ui->spinBox_gamma_linear, SIGNAL(valueChanged(double)), this, SLOT(gammaLinearChanged(double)));
	
	// Max contrast
	connect(ui->slider_ywa, SIGNAL(valueChanged(int)), this, SLOT(ywaChanged(int)));
	connect(ui->spinBox_ywa, SIGNAL(valueChanged(double)), this, SLOT(ywaChanged(double)));

	// Lens effects page
	
	// Bloom
	connect(ui->slider_gaussianAmount, SIGNAL(valueChanged(int)), this, SLOT(gaussianAmountChanged(int)));
	connect(ui->spinBox_gaussianAmount, SIGNAL(valueChanged(double)), this, SLOT(gaussianAmountChanged(double)));
	connect(ui->slider_gaussianRadius, SIGNAL(valueChanged(int)), this, SLOT(gaussianRadiusChanged(int)));
	connect(ui->spinBox_gaussianRadius, SIGNAL(valueChanged(double)), this, SLOT(gaussianRadiusChanged(double)));
	connect(ui->button_gaussianComputeLayer, SIGNAL(clicked()), this, SLOT(computeBloomLayer()));
	connect(ui->button_gaussianDeleteLayer, SIGNAL(clicked()), this, SLOT(deleteBloomLayer()));
	
	// Vignetting
	connect(ui->slider_vignettingAmount, SIGNAL(valueChanged(int)), this, SLOT(vignettingAmountChanged(int)));
	connect(ui->spinBox_vignettingAmount, SIGNAL(valueChanged(double)), this, SLOT(vignettingAmountChanged(double)));
	connect(ui->checkBox_vignettingEnabled, SIGNAL(stateChanged(int)), this, SLOT(vignettingEnabledChanged(int)));
	
	// Chromatic abberration
	connect(ui->slider_caAmount, SIGNAL(valueChanged(int)), this, SLOT(caAmountChanged(int)));
	connect(ui->spinBox_caAmount, SIGNAL(valueChanged(double)), this, SLOT(caAmountChanged(double)));
	connect(ui->checkBox_caEnabled, SIGNAL(stateChanged(int)), this, SLOT(caEnabledChanged(int)));

	// Glare
	connect(ui->slider_glareAmount, SIGNAL(valueChanged(int)), this, SLOT(glareAmountChanged(int)));
	connect(ui->spinBox_glareAmount, SIGNAL(valueChanged(double)), this, SLOT(glareAmountChanged(double)));
	connect(ui->slider_glareRadius, SIGNAL(valueChanged(int)), this, SLOT(glareRadiusChanged(int)));
	connect(ui->spinBox_glareRadius, SIGNAL(valueChanged(double)), this, SLOT(glareRadiusChanged(double)));
	connect(ui->spinBox_glareBlades, SIGNAL(valueChanged(int)), this, SLOT(glareBladesChanged(int)));
	connect(ui->button_glareComputeLayer, SIGNAL(clicked()), this, SLOT(computeGlareLayer()));
	connect(ui->button_glareDeleteLayer, SIGNAL(clicked()), this, SLOT(deleteGlareLayer()));

	// Colourspace
	connect(ui->comboBox_colorSpacePreset, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorSpacePreset(int)));
	connect(ui->comboBox_whitePointPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(setWhitepointPreset(int)));
	connect(ui->slider_whitePointX, SIGNAL(valueChanged(int)), this, SLOT(whitePointXChanged(int)));
	connect(ui->spinBox_whitePointX, SIGNAL(valueChanged(double)), this, SLOT(whitePointXChanged(double)));
	connect(ui->slider_whitePointY, SIGNAL(valueChanged(int)), this, SLOT(whitePointYChanged(int)));
	connect(ui->spinBox_whitePointY, SIGNAL(valueChanged(double)), this, SLOT(whitePointYChanged(double)));
	
	connect(ui->slider_redX, SIGNAL(valueChanged(int)), this, SLOT(redXChanged(int)));
	connect(ui->spinBox_redX, SIGNAL(valueChanged(double)), this, SLOT(redXChanged(double)));
	connect(ui->slider_redY, SIGNAL(valueChanged(int)), this, SLOT(redYChanged(int)));
	connect(ui->spinBox_redY, SIGNAL(valueChanged(double)), this, SLOT(redYChanged(double)));
	connect(ui->slider_blueX, SIGNAL(valueChanged(int)), this, SLOT(blueXChanged(int)));
	connect(ui->spinBox_blueX, SIGNAL(valueChanged(double)), this, SLOT(blueXChanged(double)));
	connect(ui->slider_blueY, SIGNAL(valueChanged(int)), this, SLOT(blueYChanged(int)));
	connect(ui->spinBox_blueY, SIGNAL(valueChanged(double)), this, SLOT(blueYChanged(double)));
	connect(ui->slider_greenX, SIGNAL(valueChanged(int)), this, SLOT(greenXChanged(int)));
	connect(ui->spinBox_greenX, SIGNAL(valueChanged(double)), this, SLOT(greenXChanged(double)));
	connect(ui->slider_greenY, SIGNAL(valueChanged(int)), this, SLOT(greenYChanged(int)));
	connect(ui->spinBox_greenY, SIGNAL(valueChanged(double)), this, SLOT(greenYChanged(double)));

	connect(ui->slider_gamma, SIGNAL(valueChanged(int)), this, SLOT(gammaChanged(int)));
	connect(ui->spinBox_gamma, SIGNAL(valueChanged(double)), this, SLOT(gammaChanged(double)));

	// Noise reduction
	connect(ui->checkBox_regularizationEnabled, SIGNAL(stateChanged(int)), this, SLOT(regularizationEnabledChanged(int)));
	connect(ui->checkBox_fastApproximation, SIGNAL(stateChanged(int)), this, SLOT(fastApproximationEnabledChanged(int)));
	connect(ui->checkBox_chiuEnabled, SIGNAL(stateChanged(int)), this, SLOT(chiuEnabledChanged(int)));
	connect(ui->checkBox_includeCenter, SIGNAL(stateChanged(int)), this, SLOT(includeCenterEnabledChanged(int)));
	connect(ui->slider_iterations, SIGNAL(valueChanged(int)), this, SLOT(iterationsChanged(int)));
	connect(ui->spinBox_iterations, SIGNAL(valueChanged(double)), this, SLOT(iterationsChanged(double)));
	connect(ui->slider_amplitude, SIGNAL(valueChanged(int)), this, SLOT(amplitudeChanged(int)));
	connect(ui->spinBox_amplitude, SIGNAL(valueChanged(double)), this, SLOT(amplitudeChanged(double)));
	connect(ui->slider_precision, SIGNAL(valueChanged(int)), this, SLOT(precisionChanged(int)));
	connect(ui->spinBox_precision, SIGNAL(valueChanged(double)), this, SLOT(precisionChanged(double)));
	connect(ui->slider_alpha, SIGNAL(valueChanged(int)), this, SLOT(alphaChanged(int)));
	connect(ui->spinBox_alpha, SIGNAL(valueChanged(double)), this, SLOT(alphaChanged(double)));
	connect(ui->slider_sigma, SIGNAL(valueChanged(int)), this, SLOT(sigmaChanged(int)));
	connect(ui->spinBox_sigma, SIGNAL(valueChanged(double)), this, SLOT(sigmaChanged(double)));
	connect(ui->slider_sharpness, SIGNAL(valueChanged(int)), this, SLOT(sharpnessChanged(int)));
	connect(ui->spinBox_sharpness, SIGNAL(valueChanged(double)), this, SLOT(sharpnessChanged(double)));
	connect(ui->slider_aniso, SIGNAL(valueChanged(int)), this, SLOT(anisoChanged(int)));
	connect(ui->spinBox_aniso, SIGNAL(valueChanged(double)), this, SLOT(anisoChanged(double)));
	connect(ui->slider_aniso, SIGNAL(valueChanged(int)), this, SLOT(anisoChanged(int)));
	connect(ui->spinBox_aniso, SIGNAL(valueChanged(double)), this, SLOT(anisoChanged(double)));
	connect(ui->slider_spatial, SIGNAL(valueChanged(int)), this, SLOT(spatialChanged(int)));
	connect(ui->spinBox_spatial, SIGNAL(valueChanged(double)), this, SLOT(spatialChanged(double)));
	connect(ui->slider_angular, SIGNAL(valueChanged(int)), this, SLOT(angularChanged(int)));
	connect(ui->spinBox_angular, SIGNAL(valueChanged(double)), this, SLOT(angularChanged(double)));
	connect(ui->comboBox_interpolType, SIGNAL(currentIndexChanged(int)), this, SLOT(setInterpolType(int)));
	connect(ui->slider_chiuRadius, SIGNAL(valueChanged(int)), this, SLOT(chiuRadiusChanged(int)));
	connect(ui->spinBox_chiuRadius, SIGNAL(valueChanged(double)), this, SLOT(chiuRadiusChanged(double)));
	
	// Network tab
	connect(ui->button_addServer, SIGNAL(clicked()), this, SLOT(addServer()));
	connect(ui->lineEdit_server, SIGNAL(returnPressed()), this, SLOT(addServer()));
	connect(ui->button_removeServer, SIGNAL(clicked()), this, SLOT(removeServer()));
	connect(ui->spinBox_updateInterval, SIGNAL(valueChanged(int)), this, SLOT(updateIntervalChanged(int)));
	connect(ui->table_servers, SIGNAL(itemSelectionChanged()), this, SLOT(networknodeSelectionChanged()));

	// Buttons
	connect(ui->button_imagingApply, SIGNAL(clicked()), this, SLOT(applyTonemapping()));
	connect(ui->button_imagingReset, SIGNAL(clicked()), this, SLOT(resetToneMapping()));

	// Render threads
	connect(ui->button_addThread, SIGNAL(clicked()), this, SLOT(addThread()));
	connect(ui->button_removeThread, SIGNAL(clicked()), this, SLOT(removeThread()));
	
	// Clipboard
	connect(ui->button_copyToClipboard, SIGNAL(clicked()), this, SLOT(copyToClipboard()));

	// Statusbar
	statusMessage = new QLabel();
	statsMessage = new QLabel();
	statusMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	statsMessage->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	ui->statusbar->addPermanentWidget(statusMessage, 1);
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

	// Init render area
	renderView = new RenderView(ui->frame_render, m_opengl);
	ui->renderLayout->addWidget(renderView, 0, 0, 1, 1);
	renderView->show ();

	// Init histogram
	histogramView = new HistogramView(ui->frame_histogram);
	ui->histogramLayout->addWidget(histogramView, 0, 0, 1, 1);
	connect(ui->comboBox_histogramChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(SetOption(int)));
	connect(ui->checkBox_histogramLog, SIGNAL(stateChanged(int)), this, SLOT(LogChanged(int)));
	histogramView->SetEnabled (true);
	histogramView->show ();

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

	// Set splitter sizes
	QList<int> sizes;
	sizes << 500 << 700;
	ui->splitter->setSizes(sizes);

	updateWidgetValue(ui->spinBox_updateInterval, luxGetNetworkServerUpdateInterval());

	m_guiWindowState = SHOWN;
	changeRenderState(WAITING);
}

MainWindow::~MainWindow()
{
	for (QVector<LightGroupWidget*>::iterator it = m_LightGroupWidgets.begin(); it != m_LightGroupWidgets.end(); it++) {
		LightGroupWidget *currWidget = *it;
		ui->toolBox_lightgroups->removeItem(currWidget->GetIndex());
		delete currWidget;
	}
	m_LightGroupWidgets.clear();

	delete ui;
	delete statsMessage;
	delete m_engineThread;
	delete m_updateThread;
	delete m_flmloadThread;
	delete m_flmsaveThread;
	delete m_renderTimer;
	delete renderView;
	delete histogramView;
}

void MainWindow::updateWidgetValue(QSlider *slider, int value)
{
	slider->blockSignals (true);
	slider->setValue (value);
	slider->blockSignals (false);
}

void MainWindow::updateWidgetValue(QDoubleSpinBox *spinbox, double value)
{
	spinbox->blockSignals (true);
	spinbox->setValue (value);
	spinbox->blockSignals (false);
}

void MainWindow::updateWidgetValue(QSpinBox *spinbox, int value)
{
	spinbox->blockSignals (true);
	spinbox->setValue (value);
	spinbox->blockSignals (false);
}

void MainWindow::UpdatedTonemapParam()
{
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::SetOption(int option)
{
	histogramView->SetOption(option);
}

void MainWindow::LogChanged(int value)
{
	histogramView->LogChanged(value);
}

void MainWindow::openDocumentation ()
{
	QDesktopServices::openUrl(QUrl("http://www.luxrender.net/wiki/index.php?title=Main_Page"));
}

void MainWindow::openForums ()
{
	QDesktopServices::openUrl(QUrl("http://www.luxrender.net/forum/"));
}

void MainWindow::addThread ()
{
	if (m_numThreads < 16)
		SetRenderThreads (m_numThreads + 1);
}

void MainWindow::removeThread ()
{
	if (m_numThreads > 1)
		SetRenderThreads (m_numThreads - 1);
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

void MainWindow::prescaleChanged (int value)
{
	prescaleChanged ((double)value / ( FLOAT_SLIDER_RES / TM_REINHARD_PRESCALE_RANGE ));
}

void MainWindow::prescaleChanged (double value)
{
	m_TM_reinhard_prescale = value;
	int sliderval = (int)((FLOAT_SLIDER_RES / TM_REINHARD_PRESCALE_RANGE) * m_TM_reinhard_prescale);

	updateWidgetValue(ui->slider_prescale, sliderval);
	updateWidgetValue(ui->spinBox_prescale, m_TM_reinhard_prescale);

	updateParam (LUX_FILM, LUX_FILM_TM_REINHARD_PRESCALE, m_TM_reinhard_prescale);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::postscaleChanged (int value)
{
	postscaleChanged ((double)value / ( FLOAT_SLIDER_RES / TM_REINHARD_PRESCALE_RANGE ));
}

void MainWindow::postscaleChanged (double value)
{
	m_TM_reinhard_postscale = value;
	int sliderval = (int)((FLOAT_SLIDER_RES / TM_REINHARD_POSTSCALE_RANGE) * m_TM_reinhard_postscale);

	updateWidgetValue(ui->slider_postscale, sliderval);
	updateWidgetValue(ui->spinBox_postscale, m_TM_reinhard_postscale);

	updateParam (LUX_FILM, LUX_FILM_TM_REINHARD_POSTSCALE, m_TM_reinhard_postscale);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::burnChanged (int value)
{
	burnChanged ((double)value / ( FLOAT_SLIDER_RES / TM_REINHARD_BURN_RANGE ) );
}

void MainWindow::burnChanged (double value)
{
	m_TM_reinhard_burn = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TM_REINHARD_BURN_RANGE) * m_TM_reinhard_burn);

	updateWidgetValue(ui->slider_burn, sliderval);
	updateWidgetValue(ui->spinBox_burn, m_TM_reinhard_burn);

	updateParam(LUX_FILM, LUX_FILM_TM_REINHARD_BURN, m_TM_reinhard_burn);
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::sensitivityChanged (int value)
{
	sensitivityChanged ((double)value / ( FLOAT_SLIDER_RES / TM_LINEAR_SENSITIVITY_RANGE ) );
}

void MainWindow::sensitivityChanged (double value)
{
	m_TM_linear_sensitivity = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TM_LINEAR_SENSITIVITY_RANGE) * m_TM_linear_sensitivity);

	updateWidgetValue(ui->slider_sensitivity, sliderval);
	updateWidgetValue(ui->spinBox_sensitivity, m_TM_linear_sensitivity);

	updateParam (LUX_FILM, LUX_FILM_TM_LINEAR_SENSITIVITY, m_TM_linear_sensitivity);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::exposureChanged (int value)
{
	exposureChanged ( LogSliderValToValue(value, TM_LINEAR_EXPOSURE_LOG_MIN, TM_LINEAR_EXPOSURE_LOG_MAX) );
}

void MainWindow::exposureChanged (double value)
{
	m_TM_linear_exposure = value;

	int sliderval = ValueToLogSliderVal (m_TM_linear_exposure, TM_LINEAR_EXPOSURE_LOG_MIN, TM_LINEAR_EXPOSURE_LOG_MAX);

	updateWidgetValue(ui->slider_exposure, sliderval);
	updateWidgetValue(ui->spinBox_exposure, m_TM_linear_exposure);

	updateParam (LUX_FILM, LUX_FILM_TM_LINEAR_EXPOSURE, m_TM_linear_exposure);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::fstopChanged (int value)
{
	fstopChanged ( (double)value / ( FLOAT_SLIDER_RES / TM_LINEAR_FSTOP_RANGE ) );
}

void MainWindow::fstopChanged (double value)
{
	m_TM_linear_fstop = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TM_LINEAR_FSTOP_RANGE) * m_TM_linear_fstop);

	updateWidgetValue(ui->slider_fstop, sliderval);
	updateWidgetValue(ui->spinBox_fstop, m_TM_linear_fstop);

	updateParam (LUX_FILM, LUX_FILM_TM_LINEAR_FSTOP, m_TM_linear_fstop);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::gammaLinearChanged (int value)
{
	gammaLinearChanged ( (double)value / ( FLOAT_SLIDER_RES / TM_LINEAR_GAMMA_RANGE ) );
}

void MainWindow::gammaLinearChanged (double value)
{
	m_TM_linear_gamma = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TM_LINEAR_GAMMA_RANGE) * m_TM_linear_gamma);

	updateWidgetValue(ui->slider_gamma_linear, sliderval);
	updateWidgetValue(ui->spinBox_gamma_linear, m_TM_linear_gamma);

	updateParam (LUX_FILM, LUX_FILM_TM_LINEAR_GAMMA, m_TM_linear_gamma);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::ywaChanged (int value)
{
	ywaChanged ( LogSliderValToValue(value, TM_CONTRAST_YWA_LOG_MIN, TM_CONTRAST_YWA_LOG_MAX) );
}

void MainWindow::ywaChanged (double value)
{
	m_TM_contrast_ywa = value;

	int sliderval = ValueToLogSliderVal (m_TM_contrast_ywa, TM_CONTRAST_YWA_LOG_MIN, TM_CONTRAST_YWA_LOG_MAX);

	updateWidgetValue(ui->slider_ywa, sliderval);
	updateWidgetValue(ui->spinBox_ywa, m_TM_contrast_ywa);

	updateParam (LUX_FILM, LUX_FILM_TM_CONTRAST_YWA, m_TM_contrast_ywa);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::gaussianAmountChanged (int value)
{
	gaussianAmountChanged ( (double)value / ( FLOAT_SLIDER_RES / BLOOMWEIGHT_RANGE ) );
}

void MainWindow::gaussianAmountChanged (double value)
{
	m_bloomweight = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / BLOOMWEIGHT_RANGE) * m_bloomweight);

	updateWidgetValue(ui->slider_gaussianAmount, sliderval);
	updateWidgetValue(ui->spinBox_gaussianAmount, m_bloomweight);

	updateParam (LUX_FILM, LUX_FILM_BLOOMWEIGHT, m_bloomweight);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::gaussianRadiusChanged (int value)
{
	gaussianRadiusChanged ( (double)value / ( FLOAT_SLIDER_RES / BLOOMRADIUS_RANGE ) );
}

void MainWindow::gaussianRadiusChanged (double value)
{
	m_bloomradius = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / BLOOMRADIUS_RANGE) * m_bloomradius);

	updateWidgetValue(ui->slider_gaussianRadius, sliderval);
	updateWidgetValue(ui->spinBox_gaussianRadius, m_bloomradius);

	updateParam (LUX_FILM, LUX_FILM_BLOOMRADIUS, m_bloomradius);
}

void MainWindow::computeBloomLayer()
{
	// Signal film to update bloom layer at next tonemap
	updateParam(LUX_FILM, LUX_FILM_UPDATEBLOOMLAYER, 1.0f);
	ui->button_gaussianDeleteLayer->setEnabled (true);
	ui->slider_gaussianAmount->setEnabled(true);
	ui->spinBox_gaussianAmount->setEnabled(true);
	applyTonemapping (true);
}

void MainWindow::deleteBloomLayer()
{
	// Signal film to delete bloom layer
	updateParam(LUX_FILM, LUX_FILM_DELETEBLOOMLAYER, 1.0f);
	ui->button_gaussianDeleteLayer->setEnabled (false);
	ui->slider_gaussianAmount->setEnabled(false);
	ui->spinBox_gaussianAmount->setEnabled(false);
	applyTonemapping(true);
}

void MainWindow::vignettingAmountChanged(int value)
{
	vignettingAmountChanged ( (double)value / FLOAT_SLIDER_RES );
}

void MainWindow::vignettingAmountChanged(double value)
{
	double pos = value;
	pos -= 0.5f;
	pos *= VIGNETTING_SCALE_RANGE * 2.f;
	m_Vignetting_Scale = pos;
	
	int sliderval; 
	if ( m_Vignetting_Scale >= 0.f )
		sliderval = (int) (FLOAT_SLIDER_RES/2) + (( (FLOAT_SLIDER_RES/2) / VIGNETTING_SCALE_RANGE ) * (m_Vignetting_Scale));
	else
		sliderval = (int)(( FLOAT_SLIDER_RES/2 * VIGNETTING_SCALE_RANGE ) * (1.f - fabsf(m_Vignetting_Scale)));

	updateWidgetValue(ui->slider_vignettingAmount, sliderval);
	updateWidgetValue(ui->spinBox_vignettingAmount, m_Vignetting_Scale);

	updateParam (LUX_FILM, LUX_FILM_VIGNETTING_SCALE, m_Vignetting_Scale);

	if(m_auto_tonemap && m_Vignetting_Enabled)
		applyTonemapping();
}

void MainWindow::vignettingEnabledChanged(int value)
{
	if (value == Qt::Checked)
		m_Vignetting_Enabled = true;
	else
		m_Vignetting_Enabled = false;

	updateParam (LUX_FILM, LUX_FILM_VIGNETTING_ENABLED, m_Vignetting_Enabled);
	
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::caAmountChanged (int value)
{
	caAmountChanged ( (double)value / ( FLOAT_SLIDER_RES / ABERRATION_AMOUNT_RANGE ) );
}

void MainWindow::caAmountChanged (double value)
{
	m_Aberration_amount = value;

	if ( m_Aberration_amount > ABERRATION_AMOUNT_RANGE ) 
		m_Aberration_amount = ABERRATION_AMOUNT_RANGE;
	else if ( m_Aberration_amount < 0.0f ) 
		m_Aberration_amount = 0.0f;

	int sliderval = (int) (( FLOAT_SLIDER_RES / ABERRATION_AMOUNT_RANGE ) * (m_Aberration_amount));

	updateWidgetValue(ui->slider_caAmount, sliderval);
	updateWidgetValue(ui->spinBox_caAmount, m_Aberration_amount);

	updateParam(LUX_FILM, LUX_FILM_ABERRATION_AMOUNT, ABERRATION_AMOUNT_FACTOR * m_Aberration_amount);
	
	if (m_auto_tonemap && m_Aberration_enabled)
		applyTonemapping();
}

void MainWindow::caEnabledChanged(int value)
{
	if (value == Qt::Checked)
		m_Aberration_enabled = true;
	else
		m_Aberration_enabled = false;

	updateParam (LUX_FILM, LUX_FILM_ABERRATION_ENABLED, m_Aberration_enabled);
	
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::glareAmountChanged (int value)
{
	glareAmountChanged ( (double)value / ( FLOAT_SLIDER_RES / GLARE_AMOUNT_RANGE ) );
}

void MainWindow::glareAmountChanged (double value)
{
	m_Glare_amount = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GLARE_AMOUNT_RANGE) * m_Glare_amount);

	updateWidgetValue(ui->slider_glareAmount, sliderval);
	updateWidgetValue(ui->spinBox_glareAmount, m_Glare_amount);

	updateParam (LUX_FILM, LUX_FILM_GLARE_AMOUNT, m_Glare_amount);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::glareRadiusChanged (int value)
{
	glareRadiusChanged ( (double)value / ( FLOAT_SLIDER_RES / GLARE_RADIUS_RANGE ) );
}

void MainWindow::glareRadiusChanged (double value)
{
	m_Glare_radius = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GLARE_RADIUS_RANGE) * m_Glare_radius);

	updateWidgetValue(ui->slider_glareRadius, sliderval);
	updateWidgetValue(ui->spinBox_glareRadius, m_Glare_radius);

	updateParam (LUX_FILM, LUX_FILM_GLARE_RADIUS, m_Glare_radius);
}

void MainWindow::glareBladesChanged(int value)
{
	m_Glare_blades = value;

	if ( m_Glare_blades > GLARE_BLADES_MAX ) 
		m_Glare_blades = GLARE_BLADES_MAX;
	else if ( m_Glare_blades < GLARE_BLADES_MIN ) 
		m_Glare_blades = GLARE_BLADES_MIN;

	updateParam (LUX_FILM, LUX_FILM_GLARE_BLADES, m_Glare_blades);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::computeGlareLayer()
{
	// Signal film to update glare layer at next tonemap
	updateParam(LUX_FILM, LUX_FILM_UPDATEGLARELAYER, 1.0f);
	ui->button_glareDeleteLayer->setEnabled (true);
	ui->slider_glareAmount->setEnabled(true);
	ui->spinBox_glareAmount->setEnabled(true);
	applyTonemapping (true);
}

void MainWindow::deleteGlareLayer()
{
	// Signal film to delete glare layer
	updateParam(LUX_FILM, LUX_FILM_UPDATEGLARELAYER, 1.0f);
	ui->button_glareDeleteLayer->setEnabled (false);
	ui->slider_glareAmount->setEnabled(false);
	ui->spinBox_glareAmount->setEnabled(false);
	applyTonemapping(true);
}

void MainWindow::whitePointXChanged(int value)
{
	whitePointXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE ) );
}

void MainWindow::whitePointXChanged(double value)
{
	m_TORGB_xwhite = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE) * m_TORGB_xwhite);

	updateWidgetValue(ui->slider_whitePointX, sliderval);
	updateWidgetValue(ui->spinBox_whitePointX, m_TORGB_xwhite);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::whitePointYChanged (int value)
{
	whitePointYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE ) );
}

void MainWindow::whitePointYChanged (double value)
{
	m_TORGB_ywhite = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE) * m_TORGB_ywhite);

	updateWidgetValue(ui->slider_whitePointY, sliderval);
	updateWidgetValue(ui->spinBox_whitePointY, m_TORGB_ywhite);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::redYChanged (int value)
{
	redYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YRED_RANGE ) );
}

void MainWindow::redYChanged (double value)
{
	m_TORGB_yred = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YRED_RANGE) * m_TORGB_yred);

	updateWidgetValue(ui->slider_redY, sliderval);
	updateWidgetValue(ui->spinBox_redY, m_TORGB_yred);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::redXChanged (int value)
{
	redXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XRED_RANGE ) );
}

void MainWindow::redXChanged (double value)
{
	m_TORGB_xred = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XRED_RANGE) * m_TORGB_xred);

	updateWidgetValue(ui->slider_redX, sliderval);
	updateWidgetValue(ui->spinBox_redX, m_TORGB_xred);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::blueYChanged (int value)
{
	blueYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE ) );
}

void MainWindow::blueYChanged (double value)
{
	m_TORGB_yblue = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE) * m_TORGB_yblue);

	updateWidgetValue(ui->slider_blueY, sliderval);
	updateWidgetValue(ui->spinBox_blueY, m_TORGB_yblue);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::blueXChanged (int value)
{
	blueXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE ) );
}

void MainWindow::blueXChanged (double value)
{
	m_TORGB_xblue = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE) * m_TORGB_xblue);

	updateWidgetValue(ui->slider_blueX, sliderval);
	updateWidgetValue(ui->spinBox_blueX, m_TORGB_xblue);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::greenYChanged (int value)
{
	greenYChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE ) );
}

void MainWindow::greenYChanged (double value)
{
	m_TORGB_ygreen = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE) * m_TORGB_ygreen);

	updateWidgetValue(ui->slider_greenY, sliderval);
	updateWidgetValue(ui->spinBox_greenY, m_TORGB_ygreen);

	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::greenXChanged (int value)
{
	greenXChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE ) );
}

void MainWindow::greenXChanged (double value)
{
	m_TORGB_xgreen = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE) * m_TORGB_xgreen);

	updateWidgetValue(ui->slider_greenX, sliderval);
	updateWidgetValue(ui->spinBox_greenX, m_TORGB_xgreen);

	updateParam (LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::gammaChanged (int value)
{
	gammaChanged ( (double)value / ( FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE ) );
}

void MainWindow::gammaChanged (double value)
{
	m_TORGB_gamma = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * m_TORGB_gamma);

	updateWidgetValue(ui->slider_gamma, sliderval);
	updateWidgetValue(ui->spinBox_gamma, m_TORGB_gamma);

	updateParam (LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);

	if (m_auto_tonemap)
		applyTonemapping();
}


void MainWindow::regularizationEnabledChanged(int value)
{
	if (value == Qt::Checked)
		m_GREYC_enabled = true;
	else
		m_GREYC_enabled = false;

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_ENABLED, m_GREYC_enabled);
	
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::fastApproximationEnabledChanged(int value)
{
	if (value == Qt::Checked)
		m_GREYC_fast_approx = true;
	else
		m_GREYC_fast_approx = false;

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_FASTAPPROX, m_GREYC_fast_approx);
	
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::chiuEnabledChanged(int value)
{
	if (value == Qt::Checked)
		m_Chiu_enabled = true;
	else
		m_Chiu_enabled = false;

	updateParam (LUX_FILM, LUX_FILM_NOISE_CHIU_ENABLED, m_Chiu_enabled);
	
	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::includeCenterEnabledChanged(int value)
{
	if (value == Qt::Checked)
		m_Chiu_includecenter = true;
	else
		m_Chiu_includecenter = false;

	updateParam (LUX_FILM, LUX_FILM_NOISE_CHIU_INCLUDECENTER, m_Chiu_includecenter);
	
	if (m_auto_tonemap && m_Chiu_enabled)
		applyTonemapping();
}

void MainWindow::iterationsChanged(int value)
{
	iterationsChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_NB_ITER_RANGE ) );
}

void MainWindow::iterationsChanged(double value)
{
	m_GREYC_nb_iter = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_NB_ITER_RANGE) * m_GREYC_nb_iter);

	updateWidgetValue(ui->slider_iterations, sliderval);
	updateWidgetValue(ui->spinBox_iterations, m_GREYC_nb_iter);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_NBITER, m_GREYC_nb_iter);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::amplitudeChanged(int value)
{
	amplitudeChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_AMPLITUDE_RANGE ) );
}

void MainWindow::amplitudeChanged(double value)
{
	m_GREYC_amplitude = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_AMPLITUDE_RANGE) * m_GREYC_amplitude);

	updateWidgetValue(ui->slider_amplitude, sliderval);
	updateWidgetValue(ui->spinBox_amplitude, m_GREYC_amplitude);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_AMPLITUDE, m_GREYC_amplitude);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::precisionChanged(int value)
{
	precisionChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_GAUSSPREC_RANGE ) );
}

void MainWindow::precisionChanged(double value)
{
	m_GREYC_gauss_prec = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_GAUSSPREC_RANGE) * m_GREYC_gauss_prec);

	updateWidgetValue(ui->slider_precision, sliderval);
	updateWidgetValue(ui->spinBox_precision, m_GREYC_gauss_prec);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_GAUSSPREC, m_GREYC_gauss_prec);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::alphaChanged(int value)
{
	precisionChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_ALPHA_RANGE ) );
}

void MainWindow::alphaChanged(double value)
{
	m_GREYC_alpha = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_ALPHA_RANGE) * m_GREYC_alpha);

	updateWidgetValue(ui->slider_alpha, sliderval);
	updateWidgetValue(ui->spinBox_alpha, m_GREYC_alpha);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_ALPHA, m_GREYC_alpha);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::sigmaChanged(int value)
{
	sigmaChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_SIGMA_RANGE ) );
}

void MainWindow::sigmaChanged(double value)
{
	m_GREYC_sigma = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_SIGMA_RANGE) * m_GREYC_sigma);

	updateWidgetValue(ui->slider_sigma, sliderval);
	updateWidgetValue(ui->spinBox_sigma, m_GREYC_sigma);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_SIGMA, m_GREYC_sigma);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::sharpnessChanged(int value)
{
	sharpnessChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_SHARPNESS_RANGE ) );
}

void MainWindow::sharpnessChanged(double value)
{
	m_GREYC_sharpness = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_SHARPNESS_RANGE) * m_GREYC_sharpness);

	updateWidgetValue(ui->slider_sharpness, sliderval);
	updateWidgetValue(ui->spinBox_sharpness, m_GREYC_sharpness);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_SHARPNESS, m_GREYC_sharpness);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::anisoChanged(int value)
{
	anisoChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_ANISOTROPY_RANGE ) );
}

void MainWindow::anisoChanged(double value)
{
	m_GREYC_anisotropy = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_ANISOTROPY_RANGE) * m_GREYC_anisotropy);

	updateWidgetValue(ui->slider_aniso, sliderval);
	updateWidgetValue(ui->spinBox_aniso, m_GREYC_anisotropy);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_ANISOTROPY, m_GREYC_anisotropy);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::spatialChanged(int value)
{
	spatialChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_DL_RANGE ) );
}

void MainWindow::spatialChanged(double value)
{
	m_GREYC_dl = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_DL_RANGE) * m_GREYC_dl);

	updateWidgetValue(ui->slider_spatial, sliderval);
	updateWidgetValue(ui->spinBox_spatial, m_GREYC_dl);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_DL, m_GREYC_dl);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::angularChanged(int value)
{
	angularChanged ( (double)value / ( FLOAT_SLIDER_RES / GREYC_DA_RANGE ) );
}

void MainWindow::angularChanged(double value)
{
	m_GREYC_da = value;

	int sliderval = (int)((FLOAT_SLIDER_RES / GREYC_DA_RANGE) * m_GREYC_da);

	updateWidgetValue(ui->slider_angular, sliderval);
	updateWidgetValue(ui->spinBox_angular, m_GREYC_da);

	updateParam (LUX_FILM, LUX_FILM_NOISE_GREYC_DA, m_GREYC_da);

	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::setInterpolType(int value)
{
	m_GREYC_interp = value;
	updateParam(LUX_FILM, LUX_FILM_NOISE_GREYC_INTERP, m_GREYC_interp);
	if (m_auto_tonemap && m_GREYC_enabled)
		applyTonemapping();
}

void MainWindow::chiuRadiusChanged(int value)
{
	chiuRadiusChanged ( (double)value / (FLOAT_SLIDER_RES / (CHIU_RADIUS_MAX - CHIU_RADIUS_MIN)) + CHIU_RADIUS_MIN );
}

void MainWindow::chiuRadiusChanged(double value)
{
	m_Chiu_radius = value;

	if ( m_Chiu_radius > CHIU_RADIUS_MAX )
		m_Chiu_radius = CHIU_RADIUS_MAX;
	else if ( m_Chiu_radius < CHIU_RADIUS_MIN )
		m_Chiu_radius = CHIU_RADIUS_MIN;

	int sliderval = (int)(( FLOAT_SLIDER_RES / (CHIU_RADIUS_MAX - CHIU_RADIUS_MIN) ) * (m_Chiu_radius - CHIU_RADIUS_MIN) );

	updateWidgetValue(ui->slider_chiuRadius, sliderval);
	updateWidgetValue(ui->spinBox_chiuRadius, m_Chiu_radius);

	updateParam (LUX_FILM, LUX_FILM_NOISE_CHIU_RADIUS, m_Chiu_radius);

	if (m_auto_tonemap && m_Chiu_enabled)
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

	QString fileName = QFileDialog::getOpenFileName(this, tr("Choose a scene file to open"), "", tr("LuxRender Files (*.lxs)"));

	if(!fileName.isNull()) {
		endRenderingSession();
		renderScenefile(fileName);
	}
}

void MainWindow::resumeFLM()
{
	if (!canStopRendering())
		return;

	QString lxsFileName = QFileDialog::getOpenFileName(this, tr("Choose a scene file to open"), "", tr("LuxRender Files (*.lxs)"));

	if(lxsFileName.isNull())
		return;
	
	QString flmFileName = QFileDialog::getOpenFileName(this, tr("Choose an FLM file to open"), "", tr("LuxRender FLM files (*.flm)"));

	if(flmFileName.isNull())
		return;
	
	endRenderingSession ();
	renderScenefile (lxsFileName, flmFileName);
}

void MainWindow::loadFLM()
{
	if (!canStopRendering())
		return;

	QString flmFileName = QFileDialog::getOpenFileName(this, tr("Choose an FLM file to open"), "", tr("LuxRender FLM files (*.flm)"));

	if(flmFileName.isNull())
		return;
	
	endRenderingSession();

	//SetTitle(wxT("LuxRender - ")+fn.GetName());

	m_progDialog = new QProgressDialog(tr("Loading FLM..."),QString(),0,0,NULL);
	m_progDialog->setWindowModality(Qt::WindowModal);
	m_progDialog->show();

	// Start load thread
	m_loadTimer->start(1000);

	delete m_flmloadThread;
	m_flmloadThread = new boost::thread(boost::bind(&MainWindow::flmLoadThread, this, flmFileName));
}

void MainWindow::saveFLM()
{
	if( !luxStatistics("sceneIsReady") && !luxStatistics("filmIsReady") )
		return;

	if( m_guiRenderState == WAITING )
		return;

	QString flmFileName = QFileDialog::getSaveFileName(this, tr("Choose an FLM file to save to"), "", tr("LuxRender FLM files (*.flm)"));

	if(flmFileName.isNull())
		return;

	// Start save thread
	m_progDialog = new QProgressDialog(tr("Saving FLM..."),QString(),0,0,NULL);
	m_progDialog->setWindowModality(Qt::WindowModal);
	m_progDialog->show();
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
		histogramView->Update();

		m_renderTimer->start(1000*luxStatistics("displayInterval"));
		m_statsTimer->start(1000);
		m_netTimer->start(10000);
		
		if (m_guiRenderState == PAUSED || m_guiRenderState == STOPPED) // Only re-start if we were previously stopped
			luxStart();
		
		if (m_guiRenderState == STOPPED)
			luxSetHaltSamplePerPixel(-1, false, false);
		
		changeRenderState(RENDERING);
	}
}

void MainWindow::pauseRender()
{
	if (m_guiRenderState != PAUSED && m_guiRenderState != TONEMAPPING) {
		// Stop display update timer
		m_renderTimer->stop();
		m_statsTimer->stop();
		
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

// Stop rendering session entirely - this is different from stopping it; it's not resumable
void MainWindow::endRenderingSession()
{
	statusMessage->setText("");
	// Clean up if this is not the first rendering
	if (m_guiRenderState != WAITING) {
		m_renderTimer->stop ();
		m_statsTimer->stop ();
		m_netTimer->stop ();

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
		luxError (LUX_NOERROR, LUX_INFO, "Freeing resources.");
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
}

void MainWindow::fullScreen()
{
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
    (m_guiWindowState == SHOWN || m_guiRenderState == FINISHED)) {
		if (!withlayercomputation) {
			luxError(LUX_NOERROR, LUX_INFO, tr("GUI: Updating framebuffer...").toLatin1().data());
			statusMessage->setText(tr("Tonemapping..."));
		} else {
			luxError(LUX_NOERROR, LUX_INFO, tr("GUI: Updating framebuffer/Computing Lens Effect Layer(s)...").toLatin1().data());
			statusMessage->setText(tr("Computing Lens Effect Layer(s) & Tonemapping..."));
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
		ParseFile(filename.toStdString().c_str());
	else
		ParseFile(fullPath.leaf().c_str());

	if (luxStatistics("terminated"))
		return;

	if(!luxStatistics("sceneIsReady")) {
		qApp->postEvent(this, new QEvent((QEvent::Type)EVT_LUX_PARSEERROR));
		luxWait();
	} else {
		luxWait();
		qApp->postEvent(this, new QEvent((QEvent::Type)EVT_LUX_FINISHED));
		luxError(LUX_NOERROR, LUX_INFO, tr("Rendering done.").toLatin1().data());
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

    ui->label_threadCount->setText(QString("Threads: %1").arg(m_numThreads));
}

void MainWindow::updateStatistics()
{
	m_samplesSec = luxStatistics("samplesSec");
	int samplesSec = Floor2Int(m_samplesSec);
	int samplesTotSec = Floor2Int(luxStatistics("samplesTotSec"));
	int secElapsed = Floor2Int(luxStatistics("secElapsed"));
	double samplesPx = luxStatistics("samplesPx");
	int efficiency = Floor2Int(luxStatistics("efficiency"));
	int EV = luxStatistics("filmEV");

	int secs = (secElapsed) % 60;
	int mins = (secElapsed / 60) % 60;
	int hours = (secElapsed / 3600);

	statsMessage->setText(QString("%1:%2:%3 - %4 S/s - %5 TotS/s - %6 S/px - %7% eff - EV = %8").arg(hours,2,10,QChar('0')).arg(mins, 2,10,QChar('0')).arg(secs,2,10,QChar('0')).arg(samplesSec).arg(samplesTotSec).arg(samplesPx,0,'g',3).arg(efficiency).arg(EV));
}

void MainWindow::renderScenefile(QString sceneFilename, QString flmFilename)
{
	// Get the absolute path of the flm file
	boost::filesystem::path fullPath(boost::filesystem::initial_path());
	fullPath = boost::filesystem::system_complete(boost::filesystem::path(flmFilename.toStdString().c_str(), boost::filesystem::native));

	// Set the FLM filename
	luxOverrideResumeFLM(fullPath.string().c_str());

	// Render the scene
	renderScenefile(sceneFilename);
}

void MainWindow::renderScenefile(QString filename)
{
	// CF
	m_CurrentFile = filename;

	changeRenderState(PARSING);

	// NOTE - lordcrc - create progress dialog before starting engine thread
	//                  so we don't try to destroy it before it's properly created
	
	m_progDialog = new QProgressDialog(tr("Loading scene..."),QString(),0,0,NULL);
	m_progDialog->setWindowModality(Qt::WindowModal);
	m_progDialog->show();

	m_loadTimer->start(1000);

	m_showParseWarningDialog = true;
	m_showParseErrorDialog = true;

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
			// Waiting for input file. Most controls disabled.
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled(false);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled(false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled(false);
			ui->button_copyToClipboard->setEnabled (false);
			//m_viewerToolBar->Disable();
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
			break;
		case TONEMAPPING:
		case FINISHED:
		case STOPPING:
			// Rendering is being stopped.
			ui->button_resume->setEnabled (false);
			ui->action_resumeRender->setEnabled (false);
			ui->button_pause->setEnabled (false);
			ui->action_pauseRender->setEnabled (false);
			ui->button_stop->setEnabled (false);
			ui->action_stopRender->setEnabled (false);
			ui->button_copyToClipboard->setEnabled (true);
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
		renderView->reload();
		histogramView->Update();
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_PARSEERROR) {
		m_progDialog->cancel();
		delete m_progDialog;
		m_progDialog = NULL;
		m_loadTimer->stop();

		//wxMessageBox(wxT("Scene file parse error.\nSee log for details."), wxT("Error"), wxOK | wxICON_ERROR, this);
		changeRenderState(FINISHED);
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_FLMLOADERROR) {
		//wxMessageBox(wxT("FLM load error.\nSee log for details."), wxT("Error"), wxOK | wxICON_ERROR, this);
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
			//wxMessageBox(wxT("Rendering is finished."), wxT("LuxRender"), wxOK | wxICON_INFORMATION, this);
			changeRenderState(FINISHED);
			// Stop timers and update output one last time.
			m_renderTimer->stop();
			//wxTimerEvent rendUpdEvent(ID_RENDERUPDATE, GetId());
			//wxTimerEvent statUpdEvent(ID_STATSUPDATE, GetId());
			//GetEventHandler()->AddPendingEvent(statUpdEvent);
		}
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_SAVEDFLM) {
		m_progDialog->cancel();
		delete m_progDialog;
		m_progDialog = NULL;
		m_saveTimer->stop();

		if (m_flmsaveThread)
			m_flmsaveThread->join();
		delete m_flmsaveThread;
		m_flmsaveThread = NULL;
		retval = TRUE;
	}
	else if (eventtype == EVT_LUX_LOGEVENT)
		logEvent((LuxLogEvent *)event);

	if (retval)
		// Was our event, stop the event propagation
		event->accept();

	return QMainWindow::event(event);
}

void MainWindow::logEvent(LuxLogEvent *event)
{
	QPalette p = ui->textEdit_log->palette();

	static const QColor debugColour = Qt::black;
	static const QColor infoColour = Qt::green;
	static const QColor warningColour = Qt::darkYellow;
	static const QColor errorColour = Qt::red;
	static const QColor severeColour = Qt::red;
	
	QTextStream ss(new QString());
	ss << '[' << QDateTime::currentDateTime().toString(tr("yyyy-MM-dd hh:mm:ss")) << ' ';
	bool warning = false;
	bool error = false;

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
	ui->textEdit_log->ensureCursorVisible();
}

void MainWindow::renderTimeout()
{
	if (m_updateThread == NULL && (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) &&
		(m_guiWindowState == SHOWN || m_guiRenderState == FINISHED)) {
		luxError(LUX_NOERROR, LUX_INFO, tr("GUI: Updating framebuffer...").toLatin1().data());
		statusMessage->setText("Tonemapping...");
		m_updateThread = new boost::thread(boost::bind(&MainWindow::updateThread, this));
	}
}

void MainWindow::statsTimeout()
{
	if(luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		updateStatistics();
		if(m_guiRenderState == STOPPING && m_samplesSec == 0.0) {
			// Render threads stopped, do one last render update
			luxError(LUX_NOERROR, LUX_INFO, tr("GUI: Updating framebuffer...").toLatin1().data());
			statusMessage->setText(tr("Tonemapping..."));
			delete m_updateThread;
			m_updateThread = new boost::thread(boost::bind(&MainWindow::updateThread, this));
			m_statsTimer->stop();
			luxPause();
			luxError(LUX_NOERROR, LUX_INFO, tr("Rendering stopped by user.").toLatin1().data());
			changeRenderState(STOPPED);
		}
	}
}

void MainWindow::loadTimeout()
{
	m_progDialog->setValue(m_progDialog->value()+1);
	if(luxStatistics("sceneIsReady") || m_guiRenderState == FINISHED) {
		m_progDialog->cancel();
		delete m_progDialog;
		m_progDialog = NULL;
		m_loadTimer->stop();

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
		}
	}
	else if ( luxStatistics("filmIsReady") ) {
		m_progDialog->cancel();
		delete m_progDialog;
		m_progDialog = NULL;
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
		renderView->reload();
		// Update stats
		updateStatistics();
	}
}

void MainWindow::saveTimeout()
{
	m_progDialog->setValue(m_progDialog->value()+1);
}

void MainWindow::netTimeout()
{
	UpdateNetworkTree();
}

void MainWindow::setTonemapKernel(int choice)
{
	ui->comboBox_kernel->setCurrentIndex(choice);
	updateParam(LUX_FILM, LUX_FILM_TM_TONEMAPKERNEL, (double)choice);
	m_TM_kernel = choice;
	switch (choice) {
		case 0:
			// Reinhard
			ui->frame_toneMapReinhard->show();
			ui->frame_toneMapLinear->hide();
			ui->frame_toneMapContrast->hide();
			break;
		case 1:
			// Linear
			ui->frame_toneMapReinhard->hide();
			ui->frame_toneMapLinear->show();
			ui->frame_toneMapContrast->hide();
			break;
		case 2:
			// Contrast
			ui->frame_toneMapReinhard->hide();
			ui->frame_toneMapLinear->hide();
			ui->frame_toneMapContrast->show();
			break;
		case 3:
			// MaxWhite
			ui->frame_toneMapReinhard->hide();
			ui->frame_toneMapLinear->hide();
			ui->frame_toneMapContrast->hide();
			break;
		default:
			break;
	}
	//m_Tonemap->GetSizer()->Layout();
	//m_Tonemap->GetSizer()->FitInside(m_Tonemap);
	//Refresh();
	if (m_auto_tonemap)
		applyTonemapping ();
}

int MainWindow::ValueToLogSliderVal(float value, const float logLowerBound, const float logUpperBound)
{

	if (value <= 0)
		return 0;

	float logvalue = Clamp<float>(log10f(value), logLowerBound, logUpperBound);

	const int val = static_cast<int>((logvalue - logLowerBound) / 
		(logUpperBound - logLowerBound) * FLOAT_SLIDER_RES);
	return val;
}

float MainWindow::LogSliderValToValue(int sliderval, const float logLowerBound, const float logUpperBound)
{

	float logvalue = (float)sliderval * (logUpperBound - logLowerBound) / 
		FLOAT_SLIDER_RES + logLowerBound;

	return powf(10.f, logvalue);
}

void MainWindow::updateParam(luxComponent comp, luxComponentParameters param, double value, int index)
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		// Update lux's film
		luxSetParameterValue(comp, param, value, index);
	}
}

void MainWindow::updateParam(luxComponent comp, luxComponentParameters param, const char* value, int index)
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		// Update lux's film
		luxSetStringParameterValue(comp, param, value, index);
	}
}

double MainWindow::retrieveParam(bool useDefault, luxComponent comp, luxComponentParameters param, int index)
{
	if (useDefault)
		return luxGetDefaultParameterValue(comp, param, index);	
	else
		return luxGetParameterValue(comp, param, index);
}

void MainWindow::setColorSpacePreset(int choice)
{
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_colorSpacePreset->setCurrentIndex(choice);
	ui->comboBox_colorSpacePreset->blockSignals(false);
	
	ui->comboBox_colorSpacePreset->blockSignals(true);
	ui->comboBox_whitePointPreset->setCurrentIndex(0);
	ui->comboBox_colorSpacePreset->blockSignals(false);

	switch (choice) {
		case 0: {
				// sRGB - HDTV (ITU-R BT.709-5)
				m_TORGB_xwhite = 0.314275f; m_TORGB_ywhite = 0.329411f;
				m_TORGB_xred = 0.63f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.31f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 1: {
				// ROMM RGB
				m_TORGB_xwhite = 0.346f; m_TORGB_ywhite = 0.359f;
				m_TORGB_xred = 0.7347f; m_TORGB_yred = 0.2653f;
				m_TORGB_xgreen = 0.1596f; m_TORGB_ygreen = 0.8404f;
				m_TORGB_xblue = 0.0366f; m_TORGB_yblue = 0.0001f; }
			break;
		case 2: {
				// Adobe RGB 98
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.64f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.21f; m_TORGB_ygreen = 0.71f;
				m_TORGB_xblue = 0.15f; m_TORGB_yblue = 0.06f; }
			break;
		case 3: {
				// Apple RGB
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.625f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.28f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 4: {
				// NTSC (FCC 1953, ITU-R BT.470-2 System M)
				m_TORGB_xwhite = 0.310f; m_TORGB_ywhite = 0.316f;
				m_TORGB_xred = 0.67f; m_TORGB_yred = 0.33f;
				m_TORGB_xgreen = 0.21f; m_TORGB_ygreen = 0.71f;
				m_TORGB_xblue = 0.14f; m_TORGB_yblue = 0.08f; }
			break;
		case 5: {
				// NTSC (1979) (SMPTE C, SMPTE-RP 145)
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.63f; m_TORGB_yred = 0.34f;
				m_TORGB_xgreen = 0.31f; m_TORGB_ygreen = 0.595f;
				m_TORGB_xblue = 0.155f; m_TORGB_yblue = 0.07f; }
			break;
		case 6: {
				// PAL/SECAM (EBU 3213, ITU-R BT.470-6)
				m_TORGB_xwhite = 0.313f; m_TORGB_ywhite = 0.329f;
				m_TORGB_xred = 0.64f; m_TORGB_yred = 0.33f;
				m_TORGB_xgreen = 0.29f; m_TORGB_ygreen = 0.60f;
				m_TORGB_xblue = 0.15f; m_TORGB_yblue = 0.06f; }
			break;
		case 7: {
				// CIE (1931) E
				m_TORGB_xwhite = 0.333f; m_TORGB_ywhite = 0.333f;
				m_TORGB_xred = 0.7347f; m_TORGB_yred = 0.2653f;
				m_TORGB_xgreen = 0.2738f; m_TORGB_ygreen = 0.7174f;
				m_TORGB_xblue = 0.1666f; m_TORGB_yblue = 0.0089f; }
			break;
		default:
			break;
	}

	// Update values in film trough API
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);

	updateTonemapWidgetValues ();

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::setWhitepointPreset(int choice)
{
	ui->comboBox_whitePointPreset->blockSignals(true);
	ui->comboBox_whitePointPreset->setCurrentIndex(choice);
	ui->comboBox_whitePointPreset->blockSignals(false);

	// first choice is "none"
	if (choice < 1)
		return;

	// default to E
	float x = 1;
	float y = 1;
	float z = 1;

	switch (choice - 1) {
		case 0: 
			{
				// A
				x = 1.09850;
				y = 1.00000;
				z = 0.35585;
			}
			break;
		case 1: 
			{
				// B
				x = 0.99072;
				y = 1.00000;
				z = 0.85223;
			}
			break;
		case 2: 
			{
				// C
				x = 0.98074;
				y = 1.00000;
				z = 1.18232;
			}
			break;
		case 3: 
			{
				// D50
				x = 0.96422;
				y = 1.00000;
				z = 0.82521;
			}
			break;
		case 4: 
			{
				// D55
				x = 0.95682;
				y = 1.00000;
				z = 0.92149;
			}
			break;
		case 5: 
			{
				// D65
				x = 0.95047;
				y = 1.00000;
				z = 1.08883;
			}
			break;
		case 6: 
			{
				// D75
				x = 0.94972;
				y = 1.00000;
				z = 1.22638;
			}
			break;
		case 7: 
			{
				// E
				x = 1.00000;
				y = 1.00000;
				z = 1.00000;
			}
			break;
		case 8: 
			{
				// F2
				x = 0.99186;
				y = 1.00000;
				z = 0.67393;
			}
			break;
		case 9: 
			{
				// F7
				x = 0.95041;
				y = 1.00000;
				z = 1.08747;
			}
			break;
		case 10: 
			{
				// F11
				x = 1.00962;
				y = 1.00000;
				z = 0.64350;
			}
			break;
		default:
			break;
	}

	m_TORGB_xwhite = x / (x+y+z);
	m_TORGB_ywhite = y / (x+y+z);

	// Update values in film trough API
	updateParam (LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	updateParam (LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);

	updateTonemapWidgetValues ();

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::resetToneMapping()
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		resetToneMappingFromFilm( true );
		return;
	}

	m_TM_kernel = 0; // *

	m_TM_reinhard_prescale = 1.0;
	m_TM_reinhard_postscale = 1.0;
	m_TM_reinhard_burn = 6.0;

	m_TM_linear_exposure = 1.0f;
	m_TM_linear_sensitivity = 50.0f;
	m_TM_linear_fstop = 2.8;
	m_TM_linear_gamma = 1.0;

	m_TM_contrast_ywa = 0.1;

	m_bloomradius = 0.07f;
	m_bloomweight = 0.25f;

	m_Vignetting_Enabled = false;
	m_Vignetting_Scale = 0.4;
	
	m_Aberration_enabled = false;
	m_Aberration_amount = 0.5;

	m_Glare_amount = 0.03f;
	m_Glare_radius = 0.03f;
	m_Glare_blades = 3;

	m_TORGB_xwhite = 0.314275f;
	m_TORGB_ywhite = 0.329411f;
	m_TORGB_xred = 0.63f;
	m_TORGB_yred = 0.34f;
	m_TORGB_xgreen = 0.31f;
	m_TORGB_ygreen = 0.595f;
	m_TORGB_xblue = 0.155f;
	m_TORGB_yblue = 0.07f;
	m_TORGB_gamma = 2.2f;

	m_GREYC_enabled = false;
	m_GREYC_fast_approx = true;
	m_GREYC_amplitude = 40.0;
	m_GREYC_sharpness = 0.8;
	m_GREYC_anisotropy = 0.2;
	m_GREYC_alpha = 0.8;
	m_GREYC_sigma = 1.1;
	m_GREYC_gauss_prec = 2.0;
	m_GREYC_dl = 0.8;
	m_GREYC_da = 30.0;
	m_GREYC_nb_iter = 1;
	m_GREYC_interp = 0;

	m_Chiu_enabled = false;
	m_Chiu_includecenter = false;
	m_Chiu_radius = 3;

	updateTonemapWidgetValues ();
	updateLenseffectsWidgetValues ();
	ui->outputTabs->setEnabled (false);
//	m_outputNotebook->Enable( false );
//	Refresh();
}

void MainWindow::updateTonemapWidgetValues()
{
	// Tonemap kernel selection
	setTonemapKernel (m_TM_kernel);

	// Reinhard widgets
	
	updateWidgetValue(ui->slider_prescale, (int)((FLOAT_SLIDER_RES / TM_REINHARD_PRESCALE_RANGE) * m_TM_reinhard_prescale) );
	updateWidgetValue(ui->spinBox_prescale, m_TM_reinhard_prescale);

	updateWidgetValue(ui->slider_postscale, (int)((FLOAT_SLIDER_RES / TM_REINHARD_POSTSCALE_RANGE) * (m_TM_reinhard_postscale)));
	updateWidgetValue(ui->spinBox_postscale, m_TM_reinhard_postscale);

	updateWidgetValue(ui->slider_burn, (int)((FLOAT_SLIDER_RES / TM_REINHARD_BURN_RANGE) * m_TM_reinhard_burn));
	updateWidgetValue(ui->spinBox_burn, m_TM_reinhard_burn);

	// Linear widgets
	updateWidgetValue(ui->slider_exposure, ValueToLogSliderVal(m_TM_linear_exposure, TM_LINEAR_EXPOSURE_LOG_MIN, TM_LINEAR_EXPOSURE_LOG_MAX) );
	updateWidgetValue(ui->spinBox_exposure, m_TM_linear_exposure);

	updateWidgetValue(ui->slider_sensitivity, (int)((FLOAT_SLIDER_RES / TM_LINEAR_SENSITIVITY_RANGE) * m_TM_linear_sensitivity) );
	updateWidgetValue(ui->spinBox_sensitivity, m_TM_linear_sensitivity);

	updateWidgetValue(ui->slider_fstop, (int)((FLOAT_SLIDER_RES / TM_LINEAR_FSTOP_RANGE) * m_TM_linear_fstop));
	updateWidgetValue(ui->spinBox_fstop, m_TM_linear_fstop);

	updateWidgetValue(ui->slider_gamma, (int)((FLOAT_SLIDER_RES / TM_LINEAR_GAMMA_RANGE) * m_TM_linear_gamma));
	updateWidgetValue(ui->spinBox_gamma, m_TM_linear_gamma);

	// Contrast widgets
	updateWidgetValue(ui->slider_ywa, ValueToLogSliderVal(m_TM_contrast_ywa, TM_CONTRAST_YWA_LOG_MIN, TM_CONTRAST_YWA_LOG_MAX) );
	updateWidgetValue(ui->spinBox_ywa, m_TM_contrast_ywa);

	// Colorspace widgets
	updateWidgetValue(ui->slider_whitePointX, (int)((FLOAT_SLIDER_RES / TORGB_XWHITE_RANGE) * m_TORGB_xwhite));
	updateWidgetValue(ui->spinBox_whitePointX, m_TORGB_xwhite);
	updateWidgetValue(ui->slider_whitePointY, (int)((FLOAT_SLIDER_RES / TORGB_YWHITE_RANGE) * m_TORGB_ywhite));
	updateWidgetValue(ui->spinBox_whitePointY, m_TORGB_ywhite);

	updateWidgetValue(ui->slider_redX, (int)((FLOAT_SLIDER_RES / TORGB_XRED_RANGE) * m_TORGB_xred));
	updateWidgetValue(ui->spinBox_redX, m_TORGB_xred);
	updateWidgetValue(ui->slider_redY, (int)((FLOAT_SLIDER_RES / TORGB_YRED_RANGE) * m_TORGB_yred));
	updateWidgetValue(ui->spinBox_redY, m_TORGB_yred);

	updateWidgetValue(ui->slider_greenX, (int)((FLOAT_SLIDER_RES / TORGB_XGREEN_RANGE) * m_TORGB_xgreen));
	updateWidgetValue(ui->spinBox_greenX, m_TORGB_xgreen);
	updateWidgetValue(ui->slider_greenY, (int)((FLOAT_SLIDER_RES / TORGB_YGREEN_RANGE) * m_TORGB_ygreen));
	updateWidgetValue(ui->spinBox_greenY, m_TORGB_ygreen);

	updateWidgetValue(ui->slider_blueX, (int)((FLOAT_SLIDER_RES / TORGB_XBLUE_RANGE) * m_TORGB_xblue));
	updateWidgetValue(ui->spinBox_blueX, m_TORGB_xblue);
	updateWidgetValue(ui->slider_blueY, (int)((FLOAT_SLIDER_RES / TORGB_YBLUE_RANGE) * m_TORGB_yblue));
	updateWidgetValue(ui->spinBox_blueY, m_TORGB_yblue);
   
	// Gamma widgets
	updateWidgetValue(ui->slider_gamma, (int)((FLOAT_SLIDER_RES / TORGB_GAMMA_RANGE) * m_TORGB_gamma));
	updateWidgetValue(ui->spinBox_gamma, m_TORGB_gamma);
	
	// Bloom widgets
	updateWidgetValue(ui->slider_gaussianRadius, (int)((FLOAT_SLIDER_RES / BLOOMRADIUS_RANGE) * m_bloomradius));
	updateWidgetValue(ui->spinBox_gaussianRadius, m_bloomradius);

	updateWidgetValue(ui->slider_gaussianAmount, (int)((FLOAT_SLIDER_RES / BLOOMWEIGHT_RANGE) * m_bloomweight));
	updateWidgetValue(ui->spinBox_gaussianAmount, m_bloomweight);

	// GREYC widgets
	updateWidgetValue(ui->slider_iterations, (int) m_GREYC_nb_iter );
	updateWidgetValue(ui->spinBox_iterations, m_GREYC_nb_iter);

	updateWidgetValue(ui->slider_amplitude, (int)((FLOAT_SLIDER_RES / GREYC_AMPLITUDE_RANGE) * m_GREYC_amplitude));
	updateWidgetValue(ui->spinBox_amplitude, m_GREYC_amplitude);

	updateWidgetValue(ui->slider_precision, (int)((FLOAT_SLIDER_RES / GREYC_GAUSSPREC_RANGE) * m_GREYC_gauss_prec));
	updateWidgetValue(ui->spinBox_precision, m_GREYC_gauss_prec);

	updateWidgetValue(ui->slider_alpha, (int)((FLOAT_SLIDER_RES / GREYC_ALPHA_RANGE) * m_GREYC_alpha));
	updateWidgetValue(ui->spinBox_alpha, m_GREYC_alpha);

	updateWidgetValue(ui->slider_sigma, (int)((FLOAT_SLIDER_RES / GREYC_SIGMA_RANGE) * m_GREYC_sigma));
	updateWidgetValue(ui->spinBox_sigma, m_GREYC_sigma);

	updateWidgetValue(ui->slider_sharpness, (int)((FLOAT_SLIDER_RES / GREYC_SHARPNESS_RANGE) * m_GREYC_sharpness));
	updateWidgetValue(ui->spinBox_sharpness, m_GREYC_sharpness);

	updateWidgetValue(ui->slider_aniso, (int)((FLOAT_SLIDER_RES / GREYC_ANISOTROPY_RANGE) * m_GREYC_anisotropy));
	updateWidgetValue(ui->spinBox_aniso, m_GREYC_anisotropy);

	updateWidgetValue(ui->slider_spatial, (int)((FLOAT_SLIDER_RES / GREYC_DL_RANGE) * m_GREYC_dl));
	updateWidgetValue(ui->spinBox_spatial, m_GREYC_dl);

	updateWidgetValue(ui->slider_angular, (int)((FLOAT_SLIDER_RES / GREYC_DA_RANGE) * m_GREYC_da));
	updateWidgetValue(ui->spinBox_angular, m_GREYC_da);

	ui->comboBox_interpolType->setCurrentIndex(m_GREYC_interp);

	ui->checkBox_regularizationEnabled->setChecked(m_GREYC_enabled);
	ui->checkBox_fastApproximation->setChecked(m_GREYC_fast_approx);
	ui->checkBox_chiuEnabled->setChecked(m_Chiu_enabled);
	ui->checkBox_includeCenter->setChecked(m_Chiu_includecenter);

	updateWidgetValue(ui->slider_chiuRadius,  (int)((FLOAT_SLIDER_RES / (CHIU_RADIUS_MAX-CHIU_RADIUS_MIN)) * (m_Chiu_radius-CHIU_RADIUS_MIN)) );
	updateWidgetValue(ui->spinBox_chiuRadius, m_Chiu_radius);

	// Histogram
	histogramView->SetEnabled(true);
}

void MainWindow::updateLenseffectsWidgetValues()
{
	int sliderval;
	
	updateWidgetValue (ui->slider_gaussianAmount, (int)((FLOAT_SLIDER_RES / BLOOMWEIGHT_RANGE) * m_bloomweight));
	updateWidgetValue (ui->spinBox_gaussianAmount, m_bloomweight);

	updateWidgetValue (ui->slider_gaussianRadius, (int)((FLOAT_SLIDER_RES / BLOOMRADIUS_RANGE) * m_bloomweight));
	updateWidgetValue (ui->spinBox_gaussianRadius, m_bloomradius);

	if (m_Vignetting_Scale >= 0.f)
		sliderval = (int) (FLOAT_SLIDER_RES/2) + (( (FLOAT_SLIDER_RES/2) / VIGNETTING_SCALE_RANGE ) * (m_Vignetting_Scale));
	else
		sliderval = (int)(( FLOAT_SLIDER_RES/2 * VIGNETTING_SCALE_RANGE ) * (1.f - fabsf(m_Vignetting_Scale)));
	
	updateWidgetValue (ui->slider_vignettingAmount, sliderval);
	updateWidgetValue (ui->spinBox_vignettingAmount, m_Vignetting_Scale);

	updateWidgetValue(ui->slider_caAmount, (int) (( FLOAT_SLIDER_RES / ABERRATION_AMOUNT_RANGE ) * (m_Aberration_amount)));
	updateWidgetValue(ui->spinBox_caAmount, m_Aberration_amount);
	
	updateWidgetValue(ui->slider_glareAmount, (int)((FLOAT_SLIDER_RES / GLARE_AMOUNT_RANGE) * m_Glare_amount));
	updateWidgetValue(ui->spinBox_glareAmount, m_Glare_amount);
	
	updateWidgetValue(ui->slider_glareRadius, (int)((FLOAT_SLIDER_RES / GLARE_RADIUS_RANGE) * m_Glare_radius));
	updateWidgetValue(ui->spinBox_glareRadius, m_Glare_radius);
	
	updateWidgetValue(ui->spinBox_glareBlades, m_Glare_blades);
}

void MainWindow::resetToneMappingFromFilm (bool useDefaults)
{
	double t;

	m_TM_kernel = (int)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_TONEMAPKERNEL);

	m_TM_reinhard_prescale = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_REINHARD_PRESCALE);
	m_TM_reinhard_postscale = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_REINHARD_POSTSCALE);
	m_TM_reinhard_burn = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_REINHARD_BURN);

	m_TM_linear_exposure = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_LINEAR_EXPOSURE);
	m_TM_linear_sensitivity = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_LINEAR_SENSITIVITY);
	m_TM_linear_fstop = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_LINEAR_FSTOP);
	m_TM_linear_gamma = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_LINEAR_GAMMA);

	m_TM_contrast_ywa = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TM_CONTRAST_YWA);

	m_TORGB_xwhite = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_WHITE);
	m_TORGB_ywhite = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_WHITE);
	m_TORGB_xred = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_RED);
	m_TORGB_yred = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_RED);
	m_TORGB_xgreen = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_GREEN);
	m_TORGB_ygreen = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_GREEN);
	m_TORGB_xblue = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_X_BLUE);
	m_TORGB_yblue = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_Y_BLUE);
	m_TORGB_gamma =  retrieveParam( useDefaults, LUX_FILM, LUX_FILM_TORGB_GAMMA);

	m_bloomradius = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_BLOOMRADIUS);
	m_bloomweight = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_BLOOMWEIGHT);

	t = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_VIGNETTING_ENABLED);
	m_Vignetting_Enabled = t != 0.0;
	m_Vignetting_Scale = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_VIGNETTING_SCALE);

	t = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_ABERRATION_ENABLED);
	m_Aberration_enabled = t != 0.0;
	m_Aberration_amount = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_ABERRATION_AMOUNT);


	m_Glare_amount = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_GLARE_AMOUNT);
	m_Glare_radius = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_GLARE_RADIUS);
	m_Glare_blades = (int)retrieveParam( useDefaults, LUX_FILM, LUX_FILM_GLARE_BLADES);

	t = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_ENABLED);
	
	if (t != 0.0)
		m_GREYC_enabled = true;
	else
		m_GREYC_enabled = false;
	
	t = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_FASTAPPROX);
	
	if(t != 0.0)
		m_GREYC_fast_approx = true;
	else
		m_GREYC_fast_approx = false;

	m_GREYC_amplitude = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_AMPLITUDE);
	m_GREYC_sharpness = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_SHARPNESS);
	m_GREYC_anisotropy = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_ANISOTROPY);
	m_GREYC_alpha = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_ALPHA);
	m_GREYC_sigma = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_SIGMA);
	m_GREYC_gauss_prec = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_GAUSSPREC);
	m_GREYC_dl = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_DL);
	m_GREYC_da = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_DA);
	m_GREYC_nb_iter = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_NBITER);
	m_GREYC_interp = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_GREYC_INTERP);

	t = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_CHIU_ENABLED);
	m_Chiu_enabled = t != 0.0;
	t = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_CHIU_INCLUDECENTER);
	m_Chiu_includecenter = t != 0.0;
	m_Chiu_radius = retrieveParam( useDefaults, LUX_FILM, LUX_FILM_NOISE_CHIU_RADIUS);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_PRESCALE, m_TM_reinhard_prescale);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_POSTSCALE, m_TM_reinhard_postscale);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_REINHARD_BURN, m_TM_reinhard_burn);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_EXPOSURE, m_TM_linear_exposure);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_SENSITIVITY, m_TM_linear_sensitivity);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_FSTOP, m_TM_linear_fstop);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_LINEAR_GAMMA, m_TM_linear_gamma);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TM_CONTRAST_YWA, m_TM_contrast_ywa);

	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_WHITE, m_TORGB_xwhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_WHITE, m_TORGB_ywhite);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_RED, m_TORGB_xred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_RED, m_TORGB_yred);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_GREEN, m_TORGB_xgreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_GREEN, m_TORGB_ygreen);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_X_BLUE, m_TORGB_xblue);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_Y_BLUE, m_TORGB_yblue);
	luxSetParameterValue(LUX_FILM, LUX_FILM_TORGB_GAMMA, m_TORGB_gamma);

	luxSetParameterValue(LUX_FILM, LUX_FILM_BLOOMRADIUS, m_bloomradius);
	luxSetParameterValue(LUX_FILM, LUX_FILM_BLOOMWEIGHT, m_bloomweight);

	luxSetParameterValue(LUX_FILM, LUX_FILM_VIGNETTING_ENABLED, m_Vignetting_Enabled);
	luxSetParameterValue(LUX_FILM, LUX_FILM_VIGNETTING_SCALE, m_Vignetting_Scale);

	luxSetParameterValue(LUX_FILM, LUX_FILM_ABERRATION_ENABLED, m_Aberration_enabled);
	luxSetParameterValue(LUX_FILM, LUX_FILM_ABERRATION_AMOUNT, m_Aberration_amount);

	luxSetParameterValue(LUX_FILM, LUX_FILM_GLARE_AMOUNT, m_Glare_amount);
	luxSetParameterValue(LUX_FILM, LUX_FILM_GLARE_RADIUS, m_Glare_radius);
	luxSetParameterValue(LUX_FILM, LUX_FILM_GLARE_BLADES, m_Glare_blades);

	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_ENABLED, m_GREYC_enabled);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_FASTAPPROX, m_GREYC_fast_approx);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_AMPLITUDE, m_GREYC_amplitude);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_SHARPNESS, m_GREYC_sharpness);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_ANISOTROPY, m_GREYC_anisotropy);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_ALPHA, m_GREYC_alpha);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_SIGMA, m_GREYC_sigma);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_GAUSSPREC, m_GREYC_gauss_prec);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_DL, m_GREYC_dl);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_DA, m_GREYC_da);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_NBITER, m_GREYC_nb_iter);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_GREYC_INTERP, m_GREYC_interp);

	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_CHIU_ENABLED, m_Chiu_enabled);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_CHIU_INCLUDECENTER, m_Chiu_includecenter);
	luxSetParameterValue(LUX_FILM, LUX_FILM_NOISE_CHIU_RADIUS, m_Chiu_radius);
	
	updateTonemapWidgetValues ();
	updateLenseffectsWidgetValues ();

	if (m_auto_tonemap)
		applyTonemapping();
}

void MainWindow::UpdateLightGroupWidgetValues()
{
	for (QVector<LightGroupWidget*>::iterator it = m_LightGroupWidgets.begin(); it != m_LightGroupWidgets.end(); it++) {
		(*it)->UpdateWidgetValues();
	}
}

void MainWindow::ResetLightGroups( void )
{
	if (luxStatistics("sceneIsReady") || luxStatistics("filmIsReady")) {
		ResetLightGroupsFromFilm( true );
		return;
	}

	// Remove the lightgroups
	for (QVector<LightGroupWidget*>::iterator it = m_LightGroupWidgets.begin(); it != m_LightGroupWidgets.end(); it++) {
		LightGroupWidget *currWidget = *it;
		ui->toolBox_lightgroups->removeItem(currWidget->GetIndex());
		delete currWidget;
	}

	m_LightGroupWidgets.clear();
}

void MainWindow::ResetLightGroupsFromFilm( bool useDefaults )
{
	// Remove the old lightgroups
	for (QVector<LightGroupWidget*>::iterator it = m_LightGroupWidgets.begin(); it != m_LightGroupWidgets.end(); it++) {
		LightGroupWidget *currWidget = *it;
		ui->toolBox_lightgroups->removeItem(currWidget->GetIndex());
		delete currWidget;
	}

	m_LightGroupWidgets.clear();

	// Add the new lightgroups
	int numLightGroups = (int)luxGetParameterValue(LUX_FILM, LUX_FILM_LG_COUNT);
	for (int i = 0; i < numLightGroups; i++) {
		LightGroupWidget *currWidget = new LightGroupWidget(this, this);
		currWidget->SetIndex(i);
		currWidget->ResetValuesFromFilm( useDefaults );
		ui->toolBox_lightgroups->addItem(currWidget, currWidget->GetTitle());
		m_LightGroupWidgets.push_back(currWidget);
	}

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

	double totalpixels = luxStatistics("filmXres") * luxStatistics("filmYres");
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

