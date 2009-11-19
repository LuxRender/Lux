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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <boost/thread.hpp>

#include <QtGui/QMainWindow>
#include <QtGui/QProgressDialog>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QSlider>
#include <QtGui/QSpinBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QCheckBox>
#include <QTimer>
#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QMatrix>
#include <QPoint>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QVector>
#include <QSettings>

#include "lux.h"
#include "api.h"
#include "error.h"
#include "renderview.hxx"
#include "histogramview.hxx"
#include "lightgroupwidget.hxx"

#define FLOAT_SLIDER_RES 512.f

#define TM_REINHARD_YWA_RANGE 1.0f
#define TM_REINHARD_PRESCALE_RANGE 8.0f
#define TM_REINHARD_POSTSCALE_RANGE 8.0f
#define TM_REINHARD_BURN_RANGE 12.0f

#define TM_LINEAR_EXPOSURE_LOG_MIN -3.f
#define TM_LINEAR_EXPOSURE_LOG_MAX 2.f
#define TM_LINEAR_SENSITIVITY_RANGE 1000.0f
#define TM_LINEAR_FSTOP_RANGE 64.0f
#define TM_LINEAR_GAMMA_RANGE 5.0f

#define TM_CONTRAST_YWA_LOG_MIN -4.f
#define TM_CONTRAST_YWA_LOG_MAX 4.f

#define TORGB_XWHITE_RANGE 1.0f
#define TORGB_YWHITE_RANGE 1.0f
#define TORGB_XRED_RANGE 1.0f
#define TORGB_YRED_RANGE 1.0f
#define TORGB_XGREEN_RANGE 1.0f
#define TORGB_YGREEN_RANGE 1.0f
#define TORGB_XBLUE_RANGE 1.0f
#define TORGB_YBLUE_RANGE 1.0f

#define TORGB_GAMMA_RANGE 5.0f

#define BLOOMRADIUS_RANGE 1.0f
#define BLOOMWEIGHT_RANGE 1.0f

#define VIGNETTING_SCALE_RANGE 1.0f
#define ABERRATION_AMOUNT_RANGE 1.0f
#define ABERRATION_AMOUNT_FACTOR 0.01f

#define GLARE_AMOUNT_RANGE 0.3f
#define GLARE_RADIUS_RANGE 0.2f
#define GLARE_BLADES_MIN 3
#define GLARE_BLADES_MAX 100

#define GREYC_AMPLITUDE_RANGE 200.0f
#define GREYC_SHARPNESS_RANGE 2.0f
#define GREYC_ANISOTROPY_RANGE 1.0f
#define GREYC_ALPHA_RANGE 12.0f
#define GREYC_SIGMA_RANGE 12.0f
#define GREYC_GAUSSPREC_RANGE 12.0f
#define GREYC_DL_RANGE 1.0f
#define GREYC_DA_RANGE 90.0f
#define GREYC_NB_ITER_RANGE 16.0f

#define CHIU_RADIUS_MIN 1
#define CHIU_RADIUS_MAX 9

#define LG_SCALE_LOG_MIN -4.f
#define LG_SCALE_LOG_MAX 4.f
#define LG_TEMPERATURE_MIN 1000.f
#define LG_TEMPERATURE_MAX 10000.f

enum LuxGuiRenderState
{
	WAITING,
	PARSING,
	RENDERING,
	STOPPING,
	STOPPED,
	PAUSED,
	FINISHED,
	TONEMAPPING // tonemapping an FLM file (not really a 'render' state)
};

enum LuxGuiWindowState
{
	SHOWN,
	HIDDEN
};

namespace Ui
{
	class MainWindow;
}

class LuxLogEvent: public QEvent {
public:
	LuxLogEvent(int code,int severity,QString message);
 
	QString getMessage() { return _message; }
	int getSeverity() { return _severity; }
	int getCode() { return _code; }

private:
	QString _message;
	int _severity;
	int _code;
};

class luxTreeData
{
public:

	QString m_SlaveName;
	QString m_SlaveFile;
	QString m_SlavePort;
	QString m_SlaveID;

	unsigned int m_secsSinceLastContact;
	double m_numberOfSamplesReceived;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0, bool opengl = false, bool copylog2console = FALSE);
	~MainWindow();

	void SetRenderThreads(int num);
	void updateStatistics();
	void renderScenefile(QString sceneFilename, QString flmFilename);
	void renderScenefile(QString filename);
	void changeRenderState (LuxGuiRenderState state);
	void endRenderingSession ();
	void updateTonemapWidgetValues ();
	void updateLenseffectsWidgetValues();
	void UpdatedTonemapParam();
	static void updateParam(luxComponent comp, luxComponentParameters param, double value, int index = 0);
	static void updateParam(luxComponent comp, luxComponentParameters param, const char* value, int index = 0);
	static double retrieveParam (bool useDefault, luxComponent comp, luxComponentParameters param, int index = 0);

	void resetToneMappingFromFilm (bool useDefaults);

	static void updateWidgetValue(QSlider *slider, int value);
	static void updateWidgetValue(QDoubleSpinBox *spinbox, double value);
	static void updateWidgetValue(QSpinBox *spinbox, int value);

	void UpdateNetworkTree();

	void ShowDialogBox(const std::string &msg, const std::string &caption = "LuxRender", QMessageBox::Icon icon = QMessageBox::Information);
	void ShowWarningDialogBox(const std::string &msg, const std::string &caption = "LuxRender");
	void ShowErrorDialogBox(const std::string &msg, const std::string &caption = "LuxRender");

private:
	Ui::MainWindow *ui;

	QLabel *statusMessage;
	QLabel *statsMessage;
	
	RenderView *renderView;
	HistogramView *histogramView;
	QString m_CurrentFile;

	QVector<LightGroupWidget*> m_LightGroupWidgets;

	int m_numThreads;
	bool m_copyLog2Console;
	bool m_showParseWarningDialog;
	bool m_showParseErrorDialog;
	bool m_showWarningDialog;
	double m_samplesSec;
	
	LuxGuiRenderState m_guiRenderState;
	LuxGuiWindowState m_guiWindowState;
	
	QProgressDialog *m_progDialog;
	QTimer *m_renderTimer, *m_statsTimer, *m_loadTimer, *m_saveTimer, *m_netTimer;
	
	boost::thread *m_engineThread, *m_updateThread, *m_flmloadThread, *m_flmsaveThread;

	// Tonemapping/ToRGB variables
	bool m_auto_tonemap;
	int m_TM_kernel;
	bool m_opengl;

	double m_TM_reinhard_prescale;
	double m_TM_reinhard_postscale;
	double m_TM_reinhard_burn;

	double m_TM_linear_exposure;
	double m_TM_linear_sensitivity;
	double m_TM_linear_fstop;
	double m_TM_linear_gamma;

	double m_TM_contrast_ywa;

	double m_TORGB_xwhite, m_TORGB_ywhite;
	double m_TORGB_xred, m_TORGB_yred;
	double m_TORGB_xgreen, m_TORGB_ygreen;
	double m_TORGB_xblue, m_TORGB_yblue;

	bool m_Gamma_enabled;
	double m_TORGB_gamma;

	bool m_Lenseffects_enabled;

	double m_bloomradius, m_bloomweight;

	bool m_Vignetting_Enabled;
	double m_Vignetting_Scale;

	bool m_Aberration_enabled;
	double m_Aberration_amount;

	double m_Glare_amount, m_Glare_radius;
	int m_Glare_blades;

	bool m_Noisereduction_enabled;

	bool m_GREYC_enabled, m_GREYC_fast_approx;
	double m_GREYC_amplitude, m_GREYC_sharpness, m_GREYC_anisotropy, m_GREYC_alpha, m_GREYC_sigma, m_GREYC_gauss_prec, m_GREYC_dl, m_GREYC_da;
	double m_GREYC_nb_iter;
	int m_GREYC_interp;

	bool m_Chiu_enabled, m_Chiu_includecenter;
	double m_Chiu_radius;
	
	static void LuxGuiErrorHandler(int code, int severity, const char *msg);
	
	void engineThread(QString filename);
	void updateThread();
	void flmLoadThread(QString filename);
	void flmSaveThread(QString filename);
	
	bool event (QEvent * event);

	void logEvent(LuxLogEvent *event);

	int ValueToLogSliderVal(float value, const float logLowerBound, const float logUpperBound);
	float LogSliderValToValue(int sliderval, const float logLowerBound, const float logUpperBound);

	bool canStopRendering ();

	void UpdateLightGroupWidgetValues();
	void ResetLightGroups(void);
	void ResetLightGroupsFromFilm(bool useDefaults);

	void ReadSettings();
	void WriteSettings();

private slots:

	void exitApp ();
	void openFile ();
	void resumeFLM ();
	void loadFLM ();
	void saveFLM ();
	void resumeRender ();
	void pauseRender ();
	void stopRender ();
	void copyLog ();
	void clearLog ();
	void fullScreen ();
	void aboutDialog ();
	void openDocumentation ();
	void openForums ();
	
	void renderTimeout ();
	void statsTimeout ();
	void loadTimeout ();
	void saveTimeout ();
	void netTimeout ();

	void autoEnabledChanged (int value);

	// Tonemapping slots
	void setTonemapKernel (int choice);
	void applyTonemapping (bool withlayercomputation = false);
	void resetToneMapping ();

	void addThread ();
	void removeThread ();

	void prescaleChanged (int value);
	void prescaleChanged (double value);
	void postscaleChanged (int value);
	void postscaleChanged (double value);
	void burnChanged (int value);
	void burnChanged (double value);

	void sensitivityChanged (int value);
	void sensitivityChanged (double value);
	void exposureChanged (int value);
	void exposureChanged (double value);
	void fstopChanged (int value);
	void fstopChanged (double value);
	void gammaLinearChanged (int value);
	void gammaLinearChanged (double value);

	void ywaChanged (int value);
	void ywaChanged (double value);
	
	// Lens effects slots
	void gaussianAmountChanged (int value);
	void gaussianAmountChanged (double value);
	void gaussianRadiusChanged (int value);
	void gaussianRadiusChanged (double value);
	void computeBloomLayer ();
	void deleteBloomLayer ();
	void vignettingAmountChanged (int value);
	void vignettingAmountChanged (double value);
	void vignettingEnabledChanged (int value);
	void caAmountChanged (int value);
	void caAmountChanged (double value);
	void caEnabledChanged (int value);
	void glareAmountChanged (int value);
	void glareAmountChanged (double value);
	void glareRadiusChanged (int value);
	void glareRadiusChanged (double value);
	void glareBladesChanged (int value);
	void computeGlareLayer ();
	void deleteGlareLayer ();
	
	// Colorspace slots
	void setColorSpacePreset (int choice);
	void setWhitepointPreset (int choice);
	void whitePointXChanged (int value);
	void whitePointXChanged (double value);
	void whitePointYChanged (int value);
	void whitePointYChanged (double value);
	
	void redXChanged (int value);
	void redXChanged (double value);
	void redYChanged (int value);
	void redYChanged (double value);
	void blueXChanged (int value);
	void blueXChanged (double value);
	void blueYChanged (int value);
	void blueYChanged (double value);
	void greenXChanged (int value);
	void greenXChanged (double value);
	void greenYChanged (int value);
	void greenYChanged (double value);

	void gammaChanged (int value);
	void gammaChanged (double value);

	void SetOption(int option);
	void LogChanged(int value);

	void regularizationEnabledChanged(int value);
	void fastApproximationEnabledChanged(int value);
	void chiuEnabledChanged(int value);
	void includeCenterEnabledChanged(int value);
	void iterationsChanged(int value);
	void iterationsChanged(double value);
	void amplitudeChanged(int value);
	void amplitudeChanged(double value);
	void precisionChanged(int value);
	void precisionChanged(double value);
	void alphaChanged(int value);
	void alphaChanged(double value);
	void sigmaChanged(int value);
	void sigmaChanged(double value);
	void sharpnessChanged(int value);
	void sharpnessChanged(double value);
	void anisoChanged(int value);
	void anisoChanged(double value);
	void spatialChanged(int value);
	void spatialChanged(double value);
	void angularChanged(int value);
	void angularChanged(double value);
	void setInterpolType(int value);
	void chiuRadiusChanged(int value);
	void chiuRadiusChanged(double value);
	
	void addServer();
	void removeServer();
	void updateIntervalChanged(int value);
	void networknodeSelectionChanged();
	
	void copyToClipboard ();
};

#endif // MAINWINDOW_H
